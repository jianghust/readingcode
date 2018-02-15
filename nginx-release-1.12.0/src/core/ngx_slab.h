
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_SLAB_H_INCLUDED_
#define _NGX_SLAB_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef struct ngx_slab_page_s  ngx_slab_page_t;

struct ngx_slab_page_s {
	//多用途
    uintptr_t         slab;
	//指向双向链表中的下一页
    ngx_slab_page_t  *next;
	//多用途，同时用于指向双向链表的上一页
    uintptr_t         prev;
};


typedef struct {
    ngx_uint_t        total;
    ngx_uint_t        used;

    ngx_uint_t        reqs;
    ngx_uint_t        fails;
} ngx_slab_stat_t;


typedef struct {
	//mutex的锁
    ngx_shmtx_sh_t    lock;

	//内存缓存obj最小的大小，一般是1个byte,最小分配的空间是8byte
    size_t            min_size;
	//slab pool以shift来比较和计算所需分配的obj大小、每个缓存页能够容纳obj个数以及所分配的页在缓存空间的位置
    size_t            min_shift;

	//slab page空间的开头   初始指向pages * sizeof(ngx_slab_page_t)首地址
    ngx_slab_page_t  *pages;
	//也就是指向实际的数据页pages*ngx_pagesize，指向最后一个pages页
    ngx_slab_page_t  *last;
	//管理free的页面   是一个链表头,用于连接空闲页面
    ngx_slab_page_t   free;

    ngx_slab_stat_t  *stats;
    ngx_uint_t        pfree;

	//实际缓存obj的空间的开头   这个是对地址空间进行ngx_pagesize对齐后的起始地址
    u_char           *start;
    u_char           *end;

    ngx_shmtx_t       mutex;

    u_char           *log_ctx;
    u_char            zero;

    unsigned          log_nomem:1;

    void             *data;
    void             *addr;
} ngx_slab_pool_t;


//初始化共享内存
void ngx_slab_init(ngx_slab_pool_t *pool);
//加锁保护的内存分配方法
void *ngx_slab_alloc(ngx_slab_pool_t *pool, size_t size);
//不加锁保护的内存分配方法
void *ngx_slab_alloc_locked(ngx_slab_pool_t *pool, size_t size);
void *ngx_slab_calloc(ngx_slab_pool_t *pool, size_t size);
void *ngx_slab_calloc_locked(ngx_slab_pool_t *pool, size_t size);
//加锁的内存释放
void ngx_slab_free(ngx_slab_pool_t *pool, void *p);
//不加锁的内存释放
void ngx_slab_free_locked(ngx_slab_pool_t *pool, void *p);


#endif /* _NGX_SLAB_H_INCLUDED_ */
