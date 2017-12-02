
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_CONF_FILE_H_INCLUDED_
#define _NGX_CONF_FILE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


/*
 *        AAAA  number of arguments
 *      FF      command flags
 *    TT        command type, i.e. HTTP "location" or "server" command
 */

//配置项不允许带参数
#define NGX_CONF_NOARGS      0x00000001
//配置项可以带1个参数
#define NGX_CONF_TAKE1       0x00000002
//配置项可以带2个参数
#define NGX_CONF_TAKE2       0x00000004
//配置项可以带3个参数
#define NGX_CONF_TAKE3       0x00000008
//配置项可以带4个参数
#define NGX_CONF_TAKE4       0x00000010
//配置项可以带5个参数
#define NGX_CONF_TAKE5       0x00000020
//配置项可以带6个参数
#define NGX_CONF_TAKE6       0x00000040
//配置项可以带7个参数
#define NGX_CONF_TAKE7       0x00000080
//配置最多可以携带的参数
#define NGX_CONF_MAX_ARGS    8
//配置项可以带1或2个参数
#define NGX_CONF_TAKE12      (NGX_CONF_TAKE1|NGX_CONF_TAKE2)
//配置项可以带1或3个参数
#define NGX_CONF_TAKE13      (NGX_CONF_TAKE1|NGX_CONF_TAKE3)
//配置项可以带2或3个参数
#define NGX_CONF_TAKE23      (NGX_CONF_TAKE2|NGX_CONF_TAKE3)
//配置项可以带1-3个参数
#define NGX_CONF_TAKE123     (NGX_CONF_TAKE1|NGX_CONF_TAKE2|NGX_CONF_TAKE3)
//配置项可以带1-4个参数
#define NGX_CONF_TAKE1234    (NGX_CONF_TAKE1|NGX_CONF_TAKE2|NGX_CONF_TAKE3   \
                              |NGX_CONF_TAKE4)

#define NGX_CONF_ARGS_NUMBER 0x000000ff
//配置项定义了一种新的{}块，如：http、server等配置项。
#define NGX_CONF_BLOCK       0x00000100
//配置项只能带一个参数，并且参数必需是on或者off。
#define NGX_CONF_FLAG        0x00000200
//不验证配置项携带的参数个数。
#define NGX_CONF_ANY         0x00000400
//配置项携带的参数必需超过一个。
#define NGX_CONF_1MORE       0x00000800
//配置项携带的参数必需超过二个。
#define NGX_CONF_2MORE       0x00001000

//使用全局配置，主要包括以下命令//ngx_core_commands  ngx_openssl_commands  ngx_google_perftools_commands   ngx_regex_commands  ngx_thread_pool_commands
#define NGX_DIRECT_CONF      0x00010000

/*
总结，一般一级配置(http{}外的配置项)一般属性包括NGX_MAIN_CONF|NGX_DIRECT_CONF。http events等这一行的配置属性,全局配置项worker_priority等也属于这个行列
包括NGX_MAIN_CONF不包括NGX_DIRECT_CONF
配置项可以出现在全局配置中，即不属于任何{}配置块。
*/
#define NGX_MAIN_CONF        0x01000000
/*
配置类型模块是唯一一种只有1个模块的模块类型。配置模块的类型叫做NGX_CONF_MODULE，它仅有的模块叫做ngx_conf_module，这是Nginx最
底层的模块，它指导着所有模块以配置项为核心来提供功能。因此，它是其他所有模块的基础。
*/
#define NGX_ANY_CONF         0x1F000000



#define NGX_CONF_UNSET       -1
#define NGX_CONF_UNSET_UINT  (ngx_uint_t) -1
#define NGX_CONF_UNSET_PTR   (void *) -1
#define NGX_CONF_UNSET_SIZE  (size_t) -1
#define NGX_CONF_UNSET_MSEC  (ngx_msec_t) -1


#define NGX_CONF_OK          NULL
#define NGX_CONF_ERROR       (void *) -1

#define NGX_CONF_BLOCK_START 1
#define NGX_CONF_BLOCK_DONE  2
#define NGX_CONF_FILE_DONE   3

#define NGX_CORE_MODULE      0x45524F43  /* "CORE" */
#define NGX_CONF_MODULE      0x464E4F43  /* "CONF" */


#define NGX_MAX_CONF_ERRSTR  1024


struct ngx_command_s {
	//配置项名称，如"gzip"
    ngx_str_t             name;
	/*配置项类型，type将指定配置项可以出现的位置。例如，出现在server{}或location{}中，以及它可以携带的参数个数
    type决定这个配置项可以在哪些块（如http、server、location、if、upstream块等）中出现，以及可以携带的参数类型和个数等。注意，type可以同时取多个值，各值之间用|符号连接，例如，type可以取值为NGX_TTP_MAIN_CONF | NGX_HTTP_SRV_CONFI | NGX_HTTP_LOC_CONF | NGX_CONF_TAKE。
    */
    ngx_uint_t            type;
    /*
	 * 出现了name中指定的配置项后，将会调用set方法处理配置项的参数
     * cf里面存储的是从配置文件里面解析出的内容，conf是最终用来存储解析内容的内存空间，cmd为存到空间的那个地方(使用偏移量来衡量)
     * 在ngx_conf_parse解析完参数后，在ngx_conf_handler中执行
	 * */
    char               *(*set)(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
	//配置项所处内存的相对偏移位置，仅在type中没有设置NGX_DIRECT_CONF和NGX_MAIN_CONF中时才会生效，对于HTTP模块，conf是必须要设置的
    ngx_uint_t            conf;
	//当前配置项在整个存储配置项结构体中的偏移位置
    ngx_uint_t            offset;
	//多用途，由用户来定义，如果使用Nginx预设的配置项解析方法，就需要根据这些预设方法来决定post的使用方式
    void                 *post;
};

//ngx_null_command只是一个空的ngx_command_t，表示模块的命令数组解析完毕
#define ngx_null_command  { ngx_null_string, 0, NULL, 0, 0, NULL }


struct ngx_open_file_s {
    ngx_fd_t              fd;
    ngx_str_t             name;

    void                (*flush)(ngx_open_file_t *file, ngx_log_t *log);
    void                 *data;
};


typedef struct {
    ngx_file_t            file;
    ngx_buf_t            *buffer;
    ngx_buf_t            *dump;
    ngx_uint_t            line;
} ngx_conf_file_t;


typedef struct {
    ngx_str_t             name;
    ngx_buf_t            *buffer;
} ngx_conf_dump_t;


typedef char *(*ngx_conf_handler_pt)(ngx_conf_t *cf,
    ngx_command_t *dummy, void *conf);


struct ngx_conf_s {
    char                 *name;
    ngx_array_t          *args;

    ngx_cycle_t          *cycle;
    ngx_pool_t           *pool;
    ngx_pool_t           *temp_pool;
    ngx_conf_file_t      *conf_file;
    ngx_log_t            *log;

    void                 *ctx;
    ngx_uint_t            module_type;
    ngx_uint_t            cmd_type;

    ngx_conf_handler_pt   handler;
    char                 *handler_conf;
};


typedef char *(*ngx_conf_post_handler_pt) (ngx_conf_t *cf,
    void *data, void *conf);

typedef struct {
    ngx_conf_post_handler_pt  post_handler;
} ngx_conf_post_t;


typedef struct {
    ngx_conf_post_handler_pt  post_handler;
    char                     *old_name;
    char                     *new_name;
} ngx_conf_deprecated_t;


typedef struct {
    ngx_conf_post_handler_pt  post_handler;
    ngx_int_t                 low;
    ngx_int_t                 high;
} ngx_conf_num_bounds_t;


typedef struct {
    ngx_str_t                 name;
    ngx_uint_t                value;
} ngx_conf_enum_t;


#define NGX_CONF_BITMASK_SET  1

typedef struct {
    ngx_str_t                 name;
    ngx_uint_t                mask;
} ngx_conf_bitmask_t;



char * ngx_conf_deprecated(ngx_conf_t *cf, void *post, void *data);
char *ngx_conf_check_num_bounds(ngx_conf_t *cf, void *post, void *data);


#define ngx_get_conf(conf_ctx, module)  conf_ctx[module.index]



#define ngx_conf_init_value(conf, default)                                   \
    if (conf == NGX_CONF_UNSET) {                                            \
        conf = default;                                                      \
    }

#define ngx_conf_init_ptr_value(conf, default)                               \
    if (conf == NGX_CONF_UNSET_PTR) {                                        \
        conf = default;                                                      \
    }

#define ngx_conf_init_uint_value(conf, default)                              \
    if (conf == NGX_CONF_UNSET_UINT) {                                       \
        conf = default;                                                      \
    }

#define ngx_conf_init_size_value(conf, default)                              \
    if (conf == NGX_CONF_UNSET_SIZE) {                                       \
        conf = default;                                                      \
    }

#define ngx_conf_init_msec_value(conf, default)                              \
    if (conf == NGX_CONF_UNSET_MSEC) {                                       \
        conf = default;                                                      \
    }

#define ngx_conf_merge_value(conf, prev, default)                            \
    if (conf == NGX_CONF_UNSET) {                                            \
        conf = (prev == NGX_CONF_UNSET) ? default : prev;                    \
    }

#define ngx_conf_merge_ptr_value(conf, prev, default)                        \
    if (conf == NGX_CONF_UNSET_PTR) {                                        \
        conf = (prev == NGX_CONF_UNSET_PTR) ? default : prev;                \
    }

#define ngx_conf_merge_uint_value(conf, prev, default)                       \
    if (conf == NGX_CONF_UNSET_UINT) {                                       \
        conf = (prev == NGX_CONF_UNSET_UINT) ? default : prev;               \
    }

#define ngx_conf_merge_msec_value(conf, prev, default)                       \
    if (conf == NGX_CONF_UNSET_MSEC) {                                       \
        conf = (prev == NGX_CONF_UNSET_MSEC) ? default : prev;               \
    }

#define ngx_conf_merge_sec_value(conf, prev, default)                        \
    if (conf == NGX_CONF_UNSET) {                                            \
        conf = (prev == NGX_CONF_UNSET) ? default : prev;                    \
    }

#define ngx_conf_merge_size_value(conf, prev, default)                       \
    if (conf == NGX_CONF_UNSET_SIZE) {                                       \
        conf = (prev == NGX_CONF_UNSET_SIZE) ? default : prev;               \
    }

#define ngx_conf_merge_off_value(conf, prev, default)                        \
    if (conf == NGX_CONF_UNSET) {                                            \
        conf = (prev == NGX_CONF_UNSET) ? default : prev;                    \
    }

#define ngx_conf_merge_str_value(conf, prev, default)                        \
    if (conf.data == NULL) {                                                 \
        if (prev.data) {                                                     \
            conf.len = prev.len;                                             \
            conf.data = prev.data;                                           \
        } else {                                                             \
            conf.len = sizeof(default) - 1;                                  \
            conf.data = (u_char *) default;                                  \
        }                                                                    \
    }

#define ngx_conf_merge_bufs_value(conf, prev, default_num, default_size)     \
    if (conf.num == 0) {                                                     \
        if (prev.num) {                                                      \
            conf.num = prev.num;                                             \
            conf.size = prev.size;                                           \
        } else {                                                             \
            conf.num = default_num;                                          \
            conf.size = default_size;                                        \
        }                                                                    \
    }

#define ngx_conf_merge_bitmask_value(conf, prev, default)                    \
    if (conf == 0) {                                                         \
        conf = (prev == 0) ? default : prev;                                 \
    }


char *ngx_conf_param(ngx_conf_t *cf);
char *ngx_conf_parse(ngx_conf_t *cf, ngx_str_t *filename);
char *ngx_conf_include(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);


ngx_int_t ngx_conf_full_name(ngx_cycle_t *cycle, ngx_str_t *name,
    ngx_uint_t conf_prefix);
ngx_open_file_t *ngx_conf_open_file(ngx_cycle_t *cycle, ngx_str_t *name);
void ngx_cdecl ngx_conf_log_error(ngx_uint_t level, ngx_conf_t *cf,
    ngx_err_t err, const char *fmt, ...);


char *ngx_conf_set_flag_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_str_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_str_array_slot(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
char *ngx_conf_set_keyval_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_num_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_size_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_off_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_msec_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_sec_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_bufs_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_enum_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_bitmask_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);


#endif /* _NGX_CONF_FILE_H_INCLUDED_ */
