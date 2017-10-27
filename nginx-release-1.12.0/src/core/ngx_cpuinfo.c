
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>

// 如果 CPU 架构是 i386 或 amd64，并且编译器是 GNU Compiler 或 Intel Compiler，则定义 cngx_puid 函数
#if (( __i386__ || __amd64__ ) && ( __GNUC__ || __INTEL_COMPILER ))


static ngx_inline void ngx_cpuid(uint32_t i, uint32_t *buf);

// i386 架构的 CPU，调用 CPU 指令 cpuid，获取 CPU 相关信息
#if ( __i386__ )

static ngx_inline void
ngx_cpuid(uint32_t i, uint32_t *buf)
{

    /*
     * we could not use %ebx as output parameter if gcc builds PIC,
     * and we could not save %ebx on stack, because %esp is used,
     * when the -fomit-frame-pointer optimization is specified.
     */

    __asm__ (

    "    mov    %%ebx, %%esi;  "

    "    cpuid;                "
    "    mov    %%eax, (%1);   "
    "    mov    %%ebx, 4(%1);  "
    "    mov    %%edx, 8(%1);  "
    "    mov    %%ecx, 12(%1); "

    "    mov    %%esi, %%ebx;  "

    : : "a" (i), "D" (buf) : "ecx", "edx", "esi", "memory" );
}

// amd64 架构的 CPU，调用 CPU 指令 cpuid，获取 CPU 相关信息
#else /* __amd64__ */


static ngx_inline void
ngx_cpuid(uint32_t i, uint32_t *buf)
{
    uint32_t  eax, ebx, ecx, edx;
	// 内联汇编
    __asm__ (

        "cpuid"

    : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx) : "a" (i) );
	 // 返回值在四个通用寄存器中，赋给 buf，又外部调用处使用
    buf[0] = eax;
    buf[1] = ebx;
    buf[2] = edx;
    buf[3] = ecx;
}


#endif


/* auto detect the L2 cache line size of modern and widespread CPUs */

void
ngx_cpuinfo(void)
{
	 // 存储厂商识别串，即 Vendor ID
    u_char    *vendor;
	// vbuf 作为 EAX=0 时获取到的数据的 buffer
	// cpu 作为 EAX=1 时获取到的 CPU 说明
	// model 为后面根据 CPU 说明中的 Extended Model 和 Model 计算出来的值
    uint32_t   vbuf[5], cpu[4], model;

    vbuf[0] = 0;
    vbuf[1] = 0;
    vbuf[2] = 0;
    vbuf[3] = 0;
    vbuf[4] = 0;
    // cpuid 第0号功能（EAX=0），获取最大功能号和厂商识别串
	// vbuf[0] 存储最大功能号
	// vbuf[1], vbuf[2], vbuf[3] 存储厂商识别号
    ngx_cpuid(0, vbuf);

    vendor = (u_char *) &vbuf[1];

    if (vbuf[0] == 0) {
        return;
    }

    // cpuid 第1号功能（EAX=1），获取 CPU 说明
    // 3:0 - Stepping
    // 7:4 - Model
    // 11:8 - Family
    // 13:12 - Processor Type
    // 19:16 - Extended Model
    // 27:20 - Extended Family
    ngx_cpuid(1, cpu);

	// 如果厂商识别号为 Intel 的
    if (ngx_strcmp(vendor, "GenuineIntel") == 0) {
	 	// 根据 Intel CPU 的家族号来 switch
        switch ((cpu[0] & 0xf00) >> 8) {

        /* Pentium */
        case 5:
            ngx_cacheline_size = 32;
            break;

        /* Pentium Pro, II, III */
        case 6:
			// 根据 Extended Model 和 Model，来确定该情况下的 cacheline
            // 比如 Extended Model 为 0x1，Model 为 0xd，则 model 变量值为 0x1d0，大于 0xd0，满足 if
            // 比如 Extended Model 为 0x0，Model 为 0xd，则 model 变量值为 0x0d0，等于 0xd0，满足 if
            // 比如 Extended Model 为 0x0，Model 为 0xc，则 model 变量值为 0x0c0，小于 0xd0，不满足 if
            ngx_cacheline_size = 32;

            model = ((cpu[0] & 0xf0000) >> 8) | (cpu[0] & 0xf0);

            if (model >= 0xd0) {
                /* Intel Core, Core 2, Atom */
                ngx_cacheline_size = 64;
            }

            break;

        /*
         * Pentium 4, although its cache line size is 64 bytes,
         * it prefetches up to two cache lines during memory read
         */
		// cacheline 也是 64 位，只不过在读内存预取数据时会取两倍 cacheline 长度
        case 15:
            ngx_cacheline_size = 128;
            break;
        }

	// 如果厂商识别号为 AMD 的
    } else if (ngx_strcmp(vendor, "AuthenticAMD") == 0) {
        ngx_cacheline_size = 64;
    }
}

#else


void
ngx_cpuinfo(void)
{
}


#endif
