
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_PROCESS_H_INCLUDED_
#define _NGX_PROCESS_H_INCLUDED_


#include <ngx_setaffinity.h>
#include <ngx_setproctitle.h>


typedef pid_t       ngx_pid_t;

#define NGX_INVALID_PID  -1

typedef void (*ngx_spawn_proc_pt) (ngx_cycle_t *cycle, void *data);
//主要是给master进程管理全局信息使用
typedef struct {
	//进程ID
    ngx_pid_t           pid;
	//由waitpid系统调用获取到的进程状态
    int                 status;
	//由socketpair系统调用生产出的用于进程间通信的socket句柄，这一对socket句柄可以相互通信，目前用于master父进程和子进程间的通信
    ngx_socket_t        channel[2];

	//子进程的循环执行方法，当父进程调用ngx_spawn_process生成子进程时使用
    ngx_spawn_proc_pt   proc;
	//上面的ngx_spawn_proc_pt方法中第二个参数需要传递1个指针，他是可选择的，例如，worker子进程就不需要，而cache manage进程就需要ngx_cache_manager_ctx上下文成员，这时，data一般与ngx_spawn_proc_pt方法中的第二个参数是等价的
    void               *data;
	//进程名称，操作系统中显示的进程名称与name相同
    char               *name;

	//标志位,为1表示在重新生成子进程
    unsigned            respawn:1;
	//标志位,为1表示正在生成子进程
    unsigned            just_spawn:1;
	//标志位,为1表示在进行父子进程分离
    unsigned            detached:1;
	//标志位,为1表示进程正在退出
    unsigned            exiting:1;
	//标志位,为1表示进程已经退出
    unsigned            exited:1;
} ngx_process_t;


typedef struct {
    char         *path;
    char         *name;
    char *const  *argv;
    char *const  *envp;
} ngx_exec_ctx_t;

//最多子进程个数,为何是1024个?
#define NGX_MAX_PROCESSES         1024

//NGX_PROCESS_JUST_RESPAWN标识最终会在ngx_spawn_process()创建worker进程时，将ngx_processes[s].just_spawn = 1，以此作为区别旧的worker进程的标记。
//cache loader会用到，当第一次启动的时候，使用NGX_PROCESS_NORESPAWN，就是启动一个进程执行ngx_cache_manager_process_cycle.但需要注意和上面的DETACHED的区别，因为在nginx里，一般父子进程都有很多管道通讯，只有DETACHED的模式下没有pipe通讯，这个NORESPAWN是保留了和父进程的管道通讯的
#define NGX_PROCESS_NORESPAWN     -1
//用于cache manager
#define NGX_PROCESS_JUST_SPAWN    -2
//这个是最常规的操作，fork worker进程的时候设置这个标志，当worker进程因为意外退出的时候，master进程会执行再生(respawn)操作。
#define NGX_PROCESS_RESPAWN       -3
/*
 * just是刚刚的意思，刚刚spawn出来的，用于更新配置的时候，因为更新配置执行如下的步骤
 * 1.master加载新配置文件
 * 2.fork新的worker进程
 * 3.给使用旧配置文件的worker进程发QUIT信号
第二步fork进程的时候腰加上NGX_PROCESS_JUST_RESPAWN这个标志，用于给第三步区分哪些是旧进程，哪些是新欢。
*/
#define NGX_PROCESS_JUST_RESPAWN  -4
//这是说fork出来的进程和父进程没有管理的关系，比如nginx的master升级（老版本有bug），新的master从旧的mastr fork出来，就需要这样的标志，fork出来后和父进程没啥关系
#define NGX_PROCESS_DETACHED      -5


#define ngx_getpid   getpid

#ifndef ngx_log_pid
#define ngx_log_pid  ngx_pid
#endif


ngx_pid_t ngx_spawn_process(ngx_cycle_t *cycle,
    ngx_spawn_proc_pt proc, void *data, char *name, ngx_int_t respawn);
ngx_pid_t ngx_execute(ngx_cycle_t *cycle, ngx_exec_ctx_t *ctx);
ngx_int_t ngx_init_signals(ngx_log_t *log);
void ngx_debug_point(void);


#if (NGX_HAVE_SCHED_YIELD)
#define ngx_sched_yield()  sched_yield()
#else
#define ngx_sched_yield()  usleep(1)
#endif


extern int            ngx_argc;
extern char         **ngx_argv;
extern char         **ngx_os_argv;

extern ngx_pid_t      ngx_pid;
extern ngx_socket_t   ngx_channel;
//当前操作的进程在ngx_processes数组中的下标
extern ngx_int_t      ngx_process_slot;
//ngx_processes数组中有意义的ngx_process_t元素中最大的下标
extern ngx_int_t      ngx_last_process;
//存储所有子进程的数组
extern ngx_process_t  ngx_processes[NGX_MAX_PROCESSES];


#endif /* _NGX_PROCESS_H_INCLUDED_ */
