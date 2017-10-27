
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>


ngx_uint_t  ngx_pagesize;
ngx_uint_t  ngx_pagesize_shift;
ngx_uint_t  ngx_cacheline_size;

/**
 * 封装malloc，增加分配失败判断及调试日志
 */
void *
ngx_alloc(size_t size, ngx_log_t *log)
{
    void  *p;

    p = malloc(size);
    if (p == NULL) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno,
                      "malloc(%uz) failed", size);
    }
	/* 在编译时指定debug模式是否开启，如果不开启则此句仅是括号中的逗号表达式 */
    ngx_log_debug2(NGX_LOG_DEBUG_ALLOC, log, 0, "malloc: %p:%uz", p, size);

    return p;
}

/**
 * 封装ngx_alloc，如果分配成功，初始化为0
 **/
void *
ngx_calloc(size_t size, ngx_log_t *log)
{
    void  *p;

    p = ngx_alloc(size, log);

    if (p) {
        ngx_memzero(p, size);
    }

    return p;
}


#if (NGX_HAVE_POSIX_MEMALIGN)
/*
 *  Linux系统下调用posix_memalign进行内存分配和内存对齐
 *
 */
void *
ngx_memalign(size_t alignment, size_t size, ngx_log_t *log)
{
    void  *p;
    int    err;
    /*
     * 背景：
     *      1）POSIX 1003.1d
     *      2）POSIX 标明了通过malloc( ), calloc( ), 和 realloc( ) 返回的地址对于
     *      任何的C类型来说都是对齐的
     * 功能：由posix_memalign分配的内存空间，需要由free释放。
     * 参数：
     *      p           分配好的内存空间的首地址
     *      alignment   对齐边界，Linux中，32位系统是8字节，64位系统是16字节
     *      size        指定分配size字节大小的内存
     *
     * 要求：
     *      1）要求alignment是2的幂，并且是p指针大小的倍数
     *      2）要求size是alignment的倍数
     * 返回：
     *      0       成功
     *      EINVAL  参数不满足要求
     *      ENOMEM  内存分配失败
     * 注意：
     *      1）该函数不影响errno，只能通过返回值判断
     *
     */
	//预对齐内存的分配
    err = posix_memalign(&p, alignment, size);

    if (err) {
        ngx_log_error(NGX_LOG_EMERG, log, err,
                      "posix_memalign(%uz, %uz) failed", alignment, size);
        p = NULL;
    }

    ngx_log_debug3(NGX_LOG_DEBUG_ALLOC, log, 0,
                   "posix_memalign: %p:%uz @%uz", p, size, alignment);

    return p;
}

#elif (NGX_HAVE_MEMALIGN)
/*
 * Solaris系统下调用memalign进行内存分配和内存对齐
 *
 */
void *
ngx_memalign(size_t alignment, size_t size, ngx_log_t *log)
{
    void  *p;
	//将分配一个由size指定大小，地址是alignment的倍数的内存块
    p = memalign(alignment, size);
    if (p == NULL) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno,
                      "memalign(%uz, %uz) failed", alignment, size);
    }

    ngx_log_debug3(NGX_LOG_DEBUG_ALLOC, log, 0,
                   "memalign: %p:%uz @%uz", p, size, alignment);

    return p;
}

#endif
