
//todo
/*
[root@slave02 thread]# gcc -Wall -std=gnu11 -g affinity_process.c -o affinity_process;./affinity_process
cpus: 24
this process 124775 of running processor: 0
this process 124775 of running processor: 1
this process 124775 of running processor: 2
this process 124775 of running processor: 3
*/

//top 应该能看到进程一直在 0,1,2,3 号 CPU 上某个上运行，没有被切换出去
//如果不设定 CPU 亲和,应该会被切换到其他 CPU
//具体在0, 1,2,3 号那个上，这个完全由操作系统(OS)内部调度
// 也就是设置了，不一定完全有效，但是 OS 会按你设置的最优方式调度
    

/*
  NAME
        sched_setaffinity, sched_getaffinity - set and get a process's CPU affinity mask
 
 SYNOPSIS
        #define _GNU_SOURCE              See feature_test_macros(7) 
        #include <sched.h>
 
        int sched_setaffinity(pid_t pid, size_t cpusetsize, cpu_set_t *mask);
        该函数设置进程为pid的这个进程,让它运行在mask所设定的CPU上.如果pid的值为0,
         *则表示指定的是当前进程,使当前进程运行在mask所设定的那些CPU上.
         *第二个参数cpusetsize是mask所指定的数的长度.通常设定为sizeof(cpu_set_t).
         *如果当前pid所指定的进程此时没有运行在mask所指定的任意一个CPU上,
         *则该指定的进程会从其它CPU上迁移到mask的指定的一个CPU上运行.
 
        int sched_getaffinity(pid_t pid, size_t cpusetsize, cpu_set_t *mask);
        该函数获得pid所指示的进程的CPU位掩码,并将该掩码返回到mask所指向的结构中.
         *即获得指定pid当前可以运行在哪些CPU上.
         *同样,如果pid的值为0.也表示的是当前进程
 
     RETURN VALUE
        On success, sched_setaffinity() and sched_getaffinity() return 0.  On error, -1 is returned, and errno is set appropriately.

 void CPU_ZERO (cpu_set_t *set)
 //这个宏对 CPU 集 set 进行初始化，将其设置为空集。
 void CPU_SET (int cpu, cpu_set_t *set)
 //这个宏将 指定的 cpu 加入 CPU 集 set 中
 void CPU_CLR (int cpu, cpu_set_t *set)
 //这个宏将 指定的 cpu 从 CPU 集 set 中删除。
 int CPU_ISSET (int cpu, const cpu_set_t *set)
 //如果 cpu 是 CPU 集 set 的一员，这个宏就返回一个非零值（true），否则就返回零（false）。
 
 */

//下面下一个具体的例子：将当前进程绑定到 0、1、2、3号 cpu上

#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
  
 /* sysconf( _SC_NPROCESSORS_CONF ) 查看cpu的个数；打印用%ld长整。
  * sysconf( _SC_NPROCESSORS_ONLN ) 查看在使用的cpu个数；打印用%ld长整 
 */


void * f_c(void *argc)
{
    for(int64_t k = 0;k< 1000000000000;++k)
    {
    	int32_t v = 0;
    	v+=k;
    	if(v >= INT64_MAX)
    	{
    		v = 0;
    	}
    }
    return 0;
}
  
int main(int argc, char **argv)
{
    int32_t cpus = 0;
    int32_t  i = 0;
    cpu_set_t mask;
    cpu_set_t get;

    cpus = sysconf(_SC_NPROCESSORS_CONF);
    printf("cpus: %d\n", cpus);

    CPU_ZERO(&mask);    /* 初始化set集，将set置为空*/
    CPU_SET(0, &mask);  /* 依次将0、1、2、3号cpu加入到集合，前提是你的机器是多核处理器*/
    CPU_SET(1, &mask);
    CPU_SET(2, &mask);
    CPU_SET(3, &mask);
    
    /*设置cpu 亲和性（affinity）*/
    if (sched_setaffinity(0, sizeof(mask), &mask) == -1) {
        printf("Set CPU affinity failue, ERROR:%s\n", strerror(errno));
       return -1; 
    }   
    usleep(1000); /* 让当前的设置有足够时间生效*/

    /*查看当前进程的cpu 亲和性*/
    CPU_ZERO(&get);
    if (sched_getaffinity(0, sizeof(get), &get) == -1) {
        printf("get CPU affinity failue, ERROR:%s\n", strerror(errno));
        return -1; 
    }   
    
    /*查看运行在当前进程的cpu*/
    for(i = 0; i < cpus; i++) {

        if (CPU_ISSET(i, &get)) { /*查看cpu i 是否在get 集合当中*/
            printf("this process %d of running processor: %d\n", getpid(), i); 
        }    
    }
    
    //看进程的 CPU 亲和性	
    f_c(0);    
       
    return 0;
}
