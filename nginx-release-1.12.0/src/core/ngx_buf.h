
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_BUF_H_INCLUDED_
#define _NGX_BUF_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef void *            ngx_buf_tag_t;

typedef struct ngx_buf_s  ngx_buf_t;

//处理大数据的关键数据结构，既用于内存数据也应用于磁盘数据,本质上它提供的仅仅是一些指针成员和标志位
struct ngx_buf_s {
	//处理内存告诉使用者，本次处理应该从pos位置开始处理数据,这样设置是由于同一个ngx_buf_t可能被多次反复处理。pos的含义是由使用它的模块定义的
    u_char          *pos;
	//处理内存表示该buf的有效数据到last为止,pos和last之间的内存是nginx希望处理的内容
    u_char          *last;
	//处理文件时应该从该位置开始,处理文件时，file_pos和file_last的含义与处理内存时的pos和last相同
    off_t            file_pos;
	//处理文件时应该到此结束
    off_t            file_last;

	//buf的起始位置
    u_char          *start;         /* start of buffer */
	//buf的结束位置
    u_char          *end;           /* end of buffer */
	//表示当前缓冲区的类型，那个模块使用，tag指向该模块的ngx_module_t变量地址
    ngx_buf_tag_t    tag;
	//引用的文件
    ngx_file_t      *file;
	//该缓冲区的影子缓冲区，也就是多个ngx_buf_t可能指向的是同一块内存，shadow指向的是多个指针，不怎么使用
    ngx_buf_t       *shadow;


    /* the buf's content could be changed */
	//为1时temporary表示是整形变量，后面的1表示只占据1位，数据在内存中可以修改
    unsigned         temporary:1;

    /*
     * the buf's content is in a memory cache or in a read only memory
     * and must not be changed
     */
	//为1时表示数据不可修改
    unsigned         memory:1;

    /* the buf's content is mmap()ed and must not be changed */
	//为1时表示内存使用mmap系统调用映射过来的，不可修改
    unsigned         mmap:1;

	//为1时表示，可以回收
    unsigned         recycled:1;
	//为1时标书这段缓冲区处理的是文件而不是内存
    unsigned         in_file:1;
	//为1时表示需要进行flush操作
    unsigned         flush:1;
	//为1表示是否使用同步方式，谨慎使用
    unsigned         sync:1;
	//是否是最后一块缓冲区，因为ngx_buf_t可以用ngx_chain_t串联起来
    unsigned         last_buf:1;
	//是否是ngx_chain_t中的最后一块
    unsigned         last_in_chain:1;

    unsigned         last_shadow:1;
	//是否是临时文件
    unsigned         temp_file:1;

    /* STUB */ int   num;
};


//和ngx_buf_t配合使用的链表数据结构
struct ngx_chain_s {
    ngx_buf_t    *buf;
    ngx_chain_t  *next;
};


typedef struct {
    ngx_int_t    num;
    size_t       size;
} ngx_bufs_t;


typedef struct ngx_output_chain_ctx_s  ngx_output_chain_ctx_t;

typedef ngx_int_t (*ngx_output_chain_filter_pt)(void *ctx, ngx_chain_t *in);

typedef void (*ngx_output_chain_aio_pt)(ngx_output_chain_ctx_t *ctx,
    ngx_file_t *file);

struct ngx_output_chain_ctx_s {
    ngx_buf_t                   *buf;
    ngx_chain_t                 *in;
    ngx_chain_t                 *free;
    ngx_chain_t                 *busy;

    unsigned                     sendfile:1;
    unsigned                     directio:1;
    unsigned                     unaligned:1;
    unsigned                     need_in_memory:1;
    unsigned                     need_in_temp:1;
    unsigned                     aio:1;

#if (NGX_HAVE_FILE_AIO || NGX_COMPAT)
    ngx_output_chain_aio_pt      aio_handler;
#if (NGX_HAVE_AIO_SENDFILE || NGX_COMPAT)
    ssize_t                    (*aio_preload)(ngx_buf_t *file);
#endif
#endif

#if (NGX_THREADS || NGX_COMPAT)
    ngx_int_t                  (*thread_handler)(ngx_thread_task_t *task,
                                                 ngx_file_t *file);
    ngx_thread_task_t           *thread_task;
#endif

    off_t                        alignment;

    ngx_pool_t                  *pool;
    ngx_int_t                    allocated;
    ngx_bufs_t                   bufs;
    ngx_buf_tag_t                tag;

    ngx_output_chain_filter_pt   output_filter;
    void                        *filter_ctx;
};


typedef struct {
    ngx_chain_t                 *out;
    ngx_chain_t                **last;
    ngx_connection_t            *connection;
    ngx_pool_t                  *pool;
    off_t                        limit;
} ngx_chain_writer_ctx_t;


#define NGX_CHAIN_ERROR     (ngx_chain_t *) NGX_ERROR


#define ngx_buf_in_memory(b)        (b->temporary || b->memory || b->mmap)
#define ngx_buf_in_memory_only(b)   (ngx_buf_in_memory(b) && !b->in_file)

#define ngx_buf_special(b)                                                   \
    ((b->flush || b->last_buf || b->sync)                                    \
     && !ngx_buf_in_memory(b) && !b->in_file)

#define ngx_buf_sync_only(b)                                                 \
    (b->sync                                                                 \
     && !ngx_buf_in_memory(b) && !b->in_file && !b->flush && !b->last_buf)

#define ngx_buf_size(b)                                                      \
    (ngx_buf_in_memory(b) ? (off_t) (b->last - b->pos):                      \
                            (b->file_last - b->file_pos))

ngx_buf_t *ngx_create_temp_buf(ngx_pool_t *pool, size_t size);
ngx_chain_t *ngx_create_chain_of_bufs(ngx_pool_t *pool, ngx_bufs_t *bufs);


#define ngx_alloc_buf(pool)  ngx_palloc(pool, sizeof(ngx_buf_t))
#define ngx_calloc_buf(pool) ngx_pcalloc(pool, sizeof(ngx_buf_t))

ngx_chain_t *ngx_alloc_chain_link(ngx_pool_t *pool);
#define ngx_free_chain(pool, cl)                                             \
    cl->next = pool->chain;                                                  \
    pool->chain = cl



ngx_int_t ngx_output_chain(ngx_output_chain_ctx_t *ctx, ngx_chain_t *in);
ngx_int_t ngx_chain_writer(void *ctx, ngx_chain_t *in);

ngx_int_t ngx_chain_add_copy(ngx_pool_t *pool, ngx_chain_t **chain,
    ngx_chain_t *in);
ngx_chain_t *ngx_chain_get_free_buf(ngx_pool_t *p, ngx_chain_t **free);
void ngx_chain_update_chains(ngx_pool_t *p, ngx_chain_t **free,
    ngx_chain_t **busy, ngx_chain_t **out, ngx_buf_tag_t tag);

off_t ngx_chain_coalesce_file(ngx_chain_t **in, off_t limit);

ngx_chain_t *ngx_chain_update_sent(ngx_chain_t *in, off_t sent);

#endif /* _NGX_BUF_H_INCLUDED_ */
