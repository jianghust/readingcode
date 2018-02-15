
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_LIST_H_INCLUDED_
#define _NGX_LIST_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef struct ngx_list_part_s  ngx_list_part_t;

struct ngx_list_part_s {
	//指向数组的起始地址
    void             *elts;
	//表示数组中已经使用了多少个元素
    ngx_uint_t        nelts;
	//下一个链表元素ngx_last_part_t的地址
    ngx_list_part_t  *next;
};


typedef struct {
	//链表的首个元素数组
    ngx_list_part_t  *last;
	//链表的首个元素数组
    ngx_list_part_t   part;
	//每个数组元素的占用空间的大小
    size_t            size;
	//链表的数组元素一旦分配后是不可更改的。nalloc表示每个ngx_list_part_t数组的容量，即最多可存储多少个数据。
    ngx_uint_t        nalloc;
	//链表中管理内存分配的内存池对象。用户要存放的数据占用的内存都是由pool分配的
    ngx_pool_t       *pool;
} ngx_list_t;


//创建新的链表,调用至少会创建一个数组，其中包含n个大小size字节的连续内存块，也就是ngx_list_t的part成员
ngx_list_t *ngx_list_create(ngx_pool_t *pool, ngx_uint_t n, size_t size);

//初始化链表
static ngx_inline ngx_int_t
ngx_list_init(ngx_list_t *list, ngx_pool_t *pool, ngx_uint_t n, size_t size)
{
    list->part.elts = ngx_palloc(pool, n * size);
    if (list->part.elts == NULL) {
        return NGX_ERROR;
    }

    list->part.nelts = 0;
    list->part.next = NULL;
    list->last = &list->part;
    list->size = size;
    list->nalloc = n;
    list->pool = pool;

    return NGX_OK;
}


/*
 *
 *  the iteration through the list:
 *
 *  part = &list.part;
 *  data = part->elts;
 *
 *  for (i = 0 ;; i++) {
 *
 *      if (i >= part->nelts) {
 *          if (part->next == NULL) {
 *              break;
 *          }
 *
 *          part = part->next;
 *          data = part->elts;
 *          i = 0;
 *      }
 *
 *      ...  data[i] ...
 *
 *  }
 */


//添加新的链表元素
void *ngx_list_push(ngx_list_t *list);


#endif /* _NGX_LIST_H_INCLUDED_ */
