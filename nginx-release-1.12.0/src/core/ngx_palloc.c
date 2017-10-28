
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>


static ngx_inline void *ngx_palloc_small(ngx_pool_t *pool, size_t size,
    ngx_uint_t align);
static void *ngx_palloc_block(ngx_pool_t *pool, size_t size);
static void *ngx_palloc_large(ngx_pool_t *pool, size_t size);

/**
 * 初始化内存池
 */
ngx_pool_t *
ngx_create_pool(size_t size, ngx_log_t *log)
{
    ngx_pool_t  *p;
	//内存对齐
    p = ngx_memalign(NGX_POOL_ALIGNMENT, size, log);
    if (p == NULL) {
        return NULL;
    }

    p->d.last = (u_char *) p + sizeof(ngx_pool_t);
    p->d.end = (u_char *) p + size;
    p->d.next = NULL;
    p->d.failed = 0;

    size = size - sizeof(ngx_pool_t);
	//最大不超过4095
    p/lloc->max = (size < NGX_MAX_ALLOC_FROM_POOL) ? size : NGX_MAX_ALLOC_FROM_POOL;

    p->current = p;
    p->chain = NULL;
    p->large = NULL;
    p->cleanup = NULL;
    p->log = log;

    return p;
}

/**
 * 内存池销毁
 * 该函数将遍历内存池链表，所有释放内存，如果注册了clenup(也是一个链表结构)，亦将遍历该cleanup链表结构依次调用clenup的handler清理。同时，还将遍历large链表，释放大块内存。
 */
void
ngx_destroy_pool(ngx_pool_t *pool)
{
    ngx_pool_t          *p, *n;
    ngx_pool_large_t    *l;
    ngx_pool_cleanup_t  *c;
	//cleanup指向析构函数，用于执行相关的内存池销毁之前的清理工作，如文件的关闭等，  
	//清理函数是一个handler的函数指针挂载。因此，在这部分，对内存池中的析构函数遍历调用。  
    for (c = pool->cleanup; c; c = c->next) {
        if (c->handler) {
            ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, pool->log, 0,
                           "run cleanup: %p", c);
            c->handler(c->data);
        }
    }

#if (NGX_DEBUG)

    /*
     * we could allocate the pool->log from this pool
     * so we cannot use this log while free()ing the pool
     */
	//只有debug模式才会执行这个片段的代码，主要是log记录，用以跟踪函数销毁时日志记录。
    for (l = pool->large; l; l = l->next) {
        ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, pool->log, 0, "free: %p", l->alloc);
    }

    for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
        ngx_log_debug2(NGX_LOG_DEBUG_ALLOC, pool->log, 0,
                       "free: %p, unused: %uz", p, p->d.end - p->d.last);

        if (n == NULL) {
            break;
        }
    }

#endif

	//释放分配large
    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
			//这一部分用于清理大块内存，ngx_free实际上就是标准的free函数，  
			//即大内存块就是通过malloc和free操作进行管理的。  
            ngx_free(l->alloc);
        }
    }
	//释放普通内存池
    for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
        ngx_free(p);

        if (n == NULL) {
            break;
        }
    }
}

/**
 * 重置内存池
 * 该函数将释放所有large内存，并且将d->last指针重新指向ngx_pool_t结构之后数据区的开始位置，同刚创建后的位置相同。
 */
void
ngx_reset_pool(ngx_pool_t *pool)
{
    ngx_pool_t        *p;
    ngx_pool_large_t  *l;

	//删除大块内存
    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            ngx_free(l->alloc);
        }
    }

	//重置每个内存块的大小
    for (p = pool; p; p = p->d.next) {
        p->d.last = (u_char *) p + sizeof(ngx_pool_t);
        p->d.failed = 0;
    }

    pool->current = pool;
    pool->chain = NULL;
    pool->large = NULL;
}

/**
 * 对齐分配内存
 *
 */
void *
ngx_palloc(ngx_pool_t *pool, size_t size)
{
#if !(NGX_DEBUG_PALLOC)
    //判断待分配内存与max值,小于max值，则从采用小内存对齐方式分配
    if (size <= pool->max) {
        return ngx_palloc_small(pool, size, 1);
    }
#endif
	//如果大于max值，则执行大块内存分配的函数ngx_palloc_large，在large链表里分配内存
    return ngx_palloc_large(pool, size);
}

/**
 * 非对齐分配内存
 *
 */
void *
ngx_pnalloc(ngx_pool_t *pool, size_t size)
{
#if !(NGX_DEBUG_PALLOC)
    if (size <= pool->max) {
        return ngx_palloc_small(pool, size, 0);
    }
#endif

    return ngx_palloc_large(pool, size);
}


static ngx_inline void *
ngx_palloc_small(ngx_pool_t *pool, size_t size, ngx_uint_t align)
{
    u_char      *m;
    ngx_pool_t  *p;

	//从当前节点开始遍历
    p = pool->current;

    do {
		//找到待分配的起始地址
        m = p->d.last;
		
        if (align) {
			//以last开始，计算以NGX_ALIGNMENT对齐的偏移位置指针,根据需求进行地址对齐
            m = ngx_align_ptr(m, NGX_ALIGNMENT);
        }
		//计算end值减去这个偏移指针位置的大小是否满足索要分配的size大小
		//如果满足，则移动last指针位置，并返回所分配到的内存地址的起始地址
        if ((size_t) (p->d.end - m) >= size) {
			//移动到新的位置
            p->d.last = m + size;

            return m;
        }
		//如果不满足，则查找下一个链
        p = p->d.next;

    } while (p);

	//如果遍历完整个内存池链表均未找到合适大小的内存块供分配，则执行ngx_palloc_block()来分配
	//ngx_palloc_block()函数为该内存池再分配一个block，该block的大小为链表中前面每一个block大小的值。  
    //一个内存池是由多个block链接起来的。分配成功后，将该block链入该poll链的最后，  
    //同时，为所要分配的size大小的内存进行分配，并返回分配内存的起始地址。
    return ngx_palloc_block(pool, size);
}


static void *
ngx_palloc_block(ngx_pool_t *pool, size_t size)
{
    u_char      *m;
    size_t       psize;
    ngx_pool_t  *p, *new;

	//计算pool的大小，即需要分配的block的大小  
    psize = (size_t) (pool->d.end - (u_char *) pool);

	//执行按NGX_POOL_ALIGNMENT对齐方式的内存分配，假设能够分配成功，则继续执行后续代码片段。
    m = ngx_memalign(NGX_POOL_ALIGNMENT, psize, pool->log);
    if (m == NULL) {
        return NULL;
    }

    new = (ngx_pool_t *) m;

	//执行该block相关的初始化。
    new->d.end = m + psize;
    new->d.next = NULL;
    new->d.failed = 0;

	//让m指向该块内存ngx_pool_data_t结构体之后数据区起始位置 
    m += sizeof(ngx_pool_data_t);
    m = ngx_align_ptr(m, NGX_ALIGNMENT);
    new->d.last = m + size;

    for (p = pool->current; p->d.next; p = p->d.next) {
        if (p->d.failed++ > 4) {
			////失败4次以上移动current指针 
            pool->current = p->d.next;
        }
    }

	//将分配的block链入内存池 
    p->d.next = new;

    return m;
}

/**
* 大内存申请
*/
static void *
ngx_palloc_large(ngx_pool_t *pool, size_t size)
{
    void              *p;
    ngx_uint_t         n;
    ngx_pool_large_t  *large;

	//分配指定大小的内存空间
    p = ngx_alloc(size, pool->log);
    if (p == NULL) {
        return NULL;
    }

    n = 0;
	//找到未使用的大块内存结构体
    for (large = pool->large; large; large = large->next) {
        if (large->alloc == NULL) {
			//插到后面
            large->alloc = p;
            return p;
        }
		//查找三次，如果还未找到，则重新申请大块内存结构体空间
        if (n++ > 3) {//为何是3？
            break;
        }
		//关于“3”这个值，大于3采用头插法，小于3采用尾插法，这样的好处就是你最近插入的点，查找的效率快很多
    }
	// 未找到可用的, 重新申请 大块内存结构体的 空间,并且往前插入
    large = ngx_palloc_small(pool, sizeof(ngx_pool_large_t), 1);
    if (large == NULL) {
        ngx_free(p);
        return NULL;
    }

    large->alloc = p;
	//将新申请内存节点插入到前面
    large->next = pool->large;
	//将large改成最新的申请的large
    pool->large = large;

    return p;
}

/**
 * 以指定大小字节对齐, 本质上属于大块内存的分配
 */
void *
ngx_pmemalign(ngx_pool_t *pool, size_t size, size_t alignment)
{
    void              *p;
    ngx_pool_large_t  *large;
	//创建大小为size的内存，并以alignment 字节对齐
    p = ngx_memalign(alignment, size, pool->log);
    if (p == NULL) {
        return NULL;
    }
	//分配大块内存结构体 用于记录前面分配的内存块信息
    large = ngx_palloc_small(pool, sizeof(ngx_pool_large_t), 1);
    if (large == NULL) {
        ngx_free(p);
        return NULL;
    }

    large->alloc = p;
	//添加到内存池 大块内存链表中
    large->next = pool->large;
    pool->large = large;

    return p;
}


ngx_int_t
ngx_pfree(ngx_pool_t *pool, void *p)
{
    ngx_pool_large_t  *l;

    for (l = pool->large; l; l = l->next) {
        if (p == l->alloc) {
            ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, pool->log, 0,
                           "free: %p", l->alloc);
            ngx_free(l->alloc);
            l->alloc = NULL;

            return NGX_OK;
        }
    }

    return NGX_DECLINED;
}


void *
ngx_pcalloc(ngx_pool_t *pool, size_t size)
{
    void *p;

    p = ngx_palloc(pool, size);
    if (p) {
        ngx_memzero(p, size);
    }

    return p;
}

/**
 * 注册cleanup函数,为以后清除做准备
 */
ngx_pool_cleanup_t *
ngx_pool_cleanup_add(ngx_pool_t *p, size_t size)
{
    ngx_pool_cleanup_t  *c;
	//分配ngx_pool_cleanup_t空间
    c = ngx_palloc(p, sizeof(ngx_pool_cleanup_t));
    if (c == NULL) {
        return NULL;
    }
    if (size) {
		//分配参数空间，两次分配空间都是在POOL池中
        c->data = ngx_palloc(p, size);
        if (c->data == NULL) {
            return NULL;
        }

    } else {
        c->data = NULL;
    }

    c->handler = NULL;
    c->next = p->cleanup;

    p->cleanup = c;

    ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, p->log, 0, "add cleanup: %p", c);

    return c;
}


void
ngx_pool_run_cleanup_file(ngx_pool_t *p, ngx_fd_t fd)
{
    ngx_pool_cleanup_t       *c;
    ngx_pool_cleanup_file_t  *cf;

    for (c = p->cleanup; c; c = c->next) {
        if (c->handler == ngx_pool_cleanup_file) {

            cf = c->data;

            if (cf->fd == fd) {
                c->handler(cf);
                c->handler = NULL;
                return;
            }
        }
    }
}

/**
 * 关闭文件
 *
 */
void
ngx_pool_cleanup_file(void *data)
{
    ngx_pool_cleanup_file_t  *c = data;

    ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, c->log, 0, "file cleanup: fd:%d",
                   c->fd);

    if (ngx_close_file(c->fd) == NGX_FILE_ERROR) {
        ngx_log_error(NGX_LOG_ALERT, c->log, ngx_errno,
                      ngx_close_file_n " \"%s\" failed", c->name);
    }
}

/**
 * 删除文件，主要是日志文件
 *
 */
void
ngx_pool_delete_file(void *data)
{
    ngx_pool_cleanup_file_t  *c = data;

    ngx_err_t  err;

    ngx_log_debug2(NGX_LOG_DEBUG_ALLOC, c->log, 0, "file cleanup: fd:%d %s",
                   c->fd, c->name);

    if (ngx_delete_file(c->name) == NGX_FILE_ERROR) {
        err = ngx_errno;

        if (err != NGX_ENOENT) {
            ngx_log_error(NGX_LOG_CRIT, c->log, err,
                          ngx_delete_file_n " \"%s\" failed", c->name);
        }
    }

    if (ngx_close_file(c->fd) == NGX_FILE_ERROR) {
        ngx_log_error(NGX_LOG_ALERT, c->log, ngx_errno,
                      ngx_close_file_n " \"%s\" failed", c->name);
    }
}


#if 0

static void *
ngx_get_cached_block(size_t size)
{
    void                     *p;
    ngx_cached_block_slot_t  *slot;

    if (ngx_cycle->cache == NULL) {
        return NULL;
    }

    slot = &ngx_cycle->cache[(size + ngx_pagesize - 1) / ngx_pagesize];

    slot->tries++;

    if (slot->number) {
        p = slot->block;
        slot->block = slot->block->next;
        slot->number--;
        return p;
    }

    return NULL;
}

#endif
