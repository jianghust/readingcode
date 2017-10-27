
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_ALLOC_H_INCLUDED_
#define _NGX_ALLOC_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>

//分配内存，不进行初始化
void *ngx_alloc(size_t size, ngx_log_t *log);
//分配内存，并自动初始化内存空间为0
void *ngx_calloc(size_t size, ngx_log_t *log);

//释放内存 宏命名 free 为 ngx_free，Nginx 的习惯
#define ngx_free          free


/*
 * Linux has memalign() or posix_memalign()
 * Solaris has memalign()
 * FreeBSD 7.0 has posix_memalign(), besides, early version's malloc()
 * aligns allocations bigger than page size at the page boundary
 */

#if (NGX_HAVE_POSIX_MEMALIGN || NGX_HAVE_MEMALIGN)
//内存对齐分配
void *ngx_memalign(size_t alignment, size_t size, ngx_log_t *log);

#else

#define ngx_memalign(alignment, size, log)  ngx_alloc(size, log)

#endif

/**
 * 声明三个可以被外部使用的变量
 * 为了方便后面内存池等需要内存分配的地方使用而不用每次extern
 */
// 页大小
extern ngx_uint_t  ngx_pagesize;
//页大小对应的移位数
extern ngx_uint_t  ngx_pagesize_shift;
//缓存大小
extern ngx_uint_t  ngx_cacheline_size;


#endif /* _NGX_ALLOC_H_INCLUDED_ */
