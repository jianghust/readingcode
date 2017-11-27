
	/* $Id: fpm_conf.h,v 1.12.2.2 2008/12/13 03:46:49 anight Exp $ */
	/* (c) 2007,2008 Andrei Nigmatulin */

#ifndef FPM_CONF_H
#define FPM_CONF_H 1

#include <stdint.h>
#include "php.h"

#define PM2STR(a) (a == PM_STYLE_STATIC ? "static" : (a == PM_STYLE_DYNAMIC ? "dynamic" : "ondemand"))

#define FPM_CONF_MAX_PONG_LENGTH 64

struct key_value_s;

struct key_value_s {
	struct key_value_s *next;
	char *key;
	char *value;
};

/*
 * Please keep the same order as in fpm_conf.c and in php-fpm.conf.in
 */
struct fpm_global_config_s {
	char *pid_file;//pid的文件路径
	char *error_log;//error日志路径
#ifdef HAVE_SYSLOG_H
	char *syslog_ident;//打印系统日志时候的前缀，用来和其他日志做区分
	int syslog_facility;//打印系统日志时候的日志类型
#endif
	int log_level;//日志等级
	int emergency_restart_threshold;//表示在emergency_restart_interval所设值内出现SIGSEGV(内存段错误)或者SIGBUS(总线错误)错误的php-cgi进程数如果超过 emergency_restart_threshold个，php-fpm就会优雅重启。这两个选项一般保持默认值
	int emergency_restart_interval;//同上解释
	int process_control_timeout;//设置子进程接受主进程复用信号的超时时间
	int process_max;//控制子进程最大数的全局变量, 后边的设置子进程数量的指令受到这个值的限制, 0表示无限制
	int process_priority;//master进程的优先级, -19~20, 子进程会继承该值,只有在以root帐号运行的情况下才会生效
	int daemonize;//设置 FPM 在后台运行。设置“no”将 FPM 保持在前台运行用于调试。默认值：yes。
	int rlimit_files;//设置文件打开描述符的 rlimit 限制。默认值：系统定义值。
	int rlimit_core;//设置核心 rlimit 最大限制值。可用值：'unlimited'，0 或者正整数。默认值：系统定义值。
	char *events_mechanism;//事件通知机制, 注释掉则自动选择, 目前最流行的是epoll, 将准备就绪的进程号(大概这样理解吧)放到一个文件里, 每次只用扫描这个文件就知道谁准备好了
#ifdef HAVE_SYSTEMD
	int systemd_watchdog;//是否开启监控
	int systemd_interval;//systemd监控心跳时间
#endif
};

extern struct fpm_global_config_s fpm_global_config;

/*
 * Please keep the same order as in fpm_conf.c and in php-fpm.conf.in
 */
struct fpm_worker_pool_config_s {
	char *name;
	char *prefix;
	char *user;//运行时的用户
	char *group;//运行时的用户组
	char *listen_address;//监听地址
	int listen_backlog;//网络编程listen时候的参数
	/* Using chown */
	char *listen_owner;//权限用户
	char *listen_group;//权限组
	char *listen_mode;//权限模式
	char *listen_allowed_clients;//允许连接的client ip,默认为所有都允许
	int process_priority;//进程优先级
	int pm;//设置进程管理器如何管理子进程
	int pm_max_children;//最大子进程数
	int pm_start_servers;//启动的子进程数
	int pm_min_spare_servers;//设置空闲服务进程的最低数目
	int pm_max_spare_servers;//设置空闲服务进程的最大数目
	int pm_process_idle_timeout;//进程闲置超时时间,ondemand模式下使用,如果进程闲置超过此时间点,则直接kill掉此子进程
	int pm_max_requests; //每个进程处理多少个请求之后自动终止，可以有效防止内存溢出，如果为0则不会自动终止，默认为0
	char *pm_status_path;//FPM 状态页面的网址。如果没有设置，则无法访问状态页面
	char *ping_path;//FPM 监控页面的 ping 网址
	char *ping_response;//用于定义 ping 请求的返回响应
	char *access_log;//log访问地址
	char *access_format;//日志类型
	char *slowlog;//慢请求的记录日志
	int request_slowlog_timeout;//慢请求的时间,处理超过此时间表示慢请求
	int request_terminate_timeout;//单个请求的终止时间
	int rlimit_files;//设置文件打开描述符的 rlimit 限制。默认值：系统定义值。
	int rlimit_core;//设置核心 rlimit 最大限制值。可用值：'unlimited'，0 或者正整数。默认值：系统定义值。
	char *chroot;//启动时的 Chroot 目录。所定义的目录需要是绝对路径。如果没有设置，则 chroot 不被使用
	char *chdir;//设置启动目录，启动时会自动 Chdir 到该目录。所定义的目录需要是绝对路径。默认值：当前目录，或者根目录
	int catch_workers_output;//重定向运行过程中的stdout和stderr到主要的错误日志文件中. 如果没有设置, stdout 和 stderr 将会根据FastCGI的规则被重定向到 /dev/null . 默认值: 空.
	int clear_env;
	char *security_limit_extensions;//脚本后缀，默认.php
	struct key_value_s *env;//定义php环境变量
	struct key_value_s *php_admin_values;
	struct key_value_s *php_values;
#ifdef HAVE_APPARMOR
	char *apparmor_hat;
#endif
#ifdef HAVE_FPM_ACL
	/* Using Posix ACL */
	char *listen_acl_users;
	char *listen_acl_groups;
#endif
};

struct ini_value_parser_s {
	char *name;
	char *(*parser)(zval *, void **, intptr_t);
	intptr_t offset;
};

enum {
	PM_STYLE_STATIC = 1,
	PM_STYLE_DYNAMIC = 2,
	PM_STYLE_ONDEMAND = 3
};

int fpm_conf_init_main(int test_conf, int force_daemon);
int fpm_worker_pool_config_free(struct fpm_worker_pool_config_s *wpc);
int fpm_conf_write_pid();
int fpm_conf_unlink_pid();

#endif

