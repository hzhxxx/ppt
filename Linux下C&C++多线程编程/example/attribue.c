

//todo
/*
[root@slave02 thread]# gcc -Wall -std=gnu11 -g attribue.c -o attribue -lpthread;./attribue
_POSIX_THREAD_ATTR_STACKADDR 200809
_POSIX_THREAD_ATTR_STACKSIZE 200809
pthread_attr_setstacksize stacksize 16777216
pthread_attr_setstacksize stacksize 16777216
thread id 139728008083200 EINVAL
pthread_attr_getdetachstate ok,thread 139728008083200 detach state is: PTHREAD_CREATE_DETACHED,buff:
pthread_attr_getstack ok,thread 139728008083200,栈地址: 0x7f14f5497000,end :0x7f14f6498000 stacksize :16781312
pthread_attr_getstacksize ok,thread 139728008083200 stack size:16781312
pthread_attr_getguardsize ok,thread 139728008083200 guard size :8192
pthread_attr_getscope ok,thread 139728008083200 scope:PTHREAD_SCOPE_SYSTEM
pthread_attr_getinheritsched ok,thread 139728008083200 inheritsched :PTHREAD_INHERIT_SCHED
pthread_attr_getschedpolicy ok,thread 139728008083200 schedpolicy :SCHED_OTHER
pthread_attr_getschedparam ok,thread 139728008083200 schedparam :0,sched_get_priority_max:99,sched_get_priority_min:1
pthread_attr_getdetachstate ok,thread 139727991297792 detach state is: PTHREAD_CREATE_JOINABLE,buff:
pthread_attr_getstack ok,thread 139727991297792,栈地址: 0x7f14f4496000,end :0x7f14f5496000 stacksize :16777216
pthread_attr_getstacksize ok,thread 139727991297792 stack size:16777216
pthread_attr_getguardsize ok,thread 139727991297792 guard size :0
pthread_attr_getscope ok,thread 139727991297792 scope:PTHREAD_SCOPE_SYSTEM
pthread_attr_getinheritsched ok,thread 139727991297792 inheritsched :PTHREAD_INHERIT_SCHED
pthread_attr_getschedpolicy ok,thread 139727991297792 schedpolicy :SCHED_OTHER
pthread_attr_getschedparam ok,thread 139727991297792 schedparam :0,sched_get_priority_max:99,sched_get_priority_min:1
thread 139727991297792 exit
*/

#define _GNU_SOURCE
#include <stdint.h>
#include <stdlib.h>
#include <features.h>
#include <unistd.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <pthread.h>

void* f_c(void * i);

int main(int argc,char * argv[])
{
    assert(_POSIX_C_SOURCE >= 200112L);
    printf("_POSIX_THREAD_ATTR_STACKADDR %ld\n",_POSIX_THREAD_ATTR_STACKADDR);
    printf("_POSIX_THREAD_ATTR_STACKSIZE %ld\n",_POSIX_THREAD_ATTR_STACKSIZE);	
    
    #define NUM 2
    pthread_t ts[NUM] = {0};        
    for(int16_t i = 0; i < NUM;++i)
    {
    	pthread_attr_t attr;
    	int32_t ret_v = 0;
    	if(0 != (ret_v = pthread_attr_init(&attr)))
    	{
    		printf("pthread_attr_init fail %d \n",ret_v);
    		exit(EXIT_FAILURE);
    	}
    	
    	//使用 ulimit -a 查看
    	//stack size              (kbytes, -s) 8192,默认8M
    	size_t stacksize = 8192 * 1024;
    	stacksize *= 2;
    	
    	if( i % 2 == 0)
    	{
    		//分离部分创建的线程
    		ret_v = pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
    		if(ret_v != 0)
    		{
    			printf("pthread_attr_setdetachstate fail %d\n",ret_v);
    		}
    		pthread_attr_setguardsize(&attr, 8192);
    		pthread_attr_setscope(&attr,PTHREAD_SCOPE_PROCESS);
    	}
    	else
    	{    		
    		//把线程栈放大两倍,确保栈内可以分配更大内存    		
    		void * stackaddr = 0;    		
    		if(0 == (ret_v = posix_memalign(&stackaddr, sysconf(_SC_PAGESIZE),stacksize)))
    		{
    			ret_v = pthread_attr_setstack(&attr,stackaddr,stacksize);
    			if(ret_v != 0)
    			{
    				printf("pthread_attr_setstack fail %d\n",ret_v);
    			}
    		}
    	}    	
    	printf("pthread_attr_setstacksize stacksize %lu\n", stacksize);  
    	pthread_attr_setstacksize(&attr,stacksize);    	
    	pthread_create(&ts[i],&attr,f_c,0);
    	ret_v = pthread_attr_destroy(&attr);
    }

    for(int16_t i = 0;i < NUM;++i)
    {        
        int value = pthread_join(ts[i],0);
        if(value == 0)
        {             
             printf("thread %lu exit\n",ts[i]);             
        } 
        else if(value == EDEADLK)
        {
            printf("thread id %lu EDEADLK\n",ts[i]);
        }else if(value == EINVAL)
        {
            printf("thread id %lu EINVAL\n",ts[i]);
        }else if(value == ESRCH)
        {
            printf("thread id %lu ESRCH\n",ts[i]);
        }
        else
        {
            printf("thread id %lu not found errno\n",ts[i]);
        }  
    }
    
    //sleep(3);    
    
    pthread_exit(0);

    return 0;
}

void * f_c(void * i)
{
    pthread_t tid = pthread_self();
    
    //不放大会导致崩溃,栈溢出
    char buff[8192 * 1024] = {0};   
    
    int32_t ret_v = 0;
    pthread_attr_t attr;
    pthread_getattr_np(tid,&attr);
    int32_t detachstate = PTHREAD_CREATE_DETACHED;
    ret_v= pthread_attr_getdetachstate(&attr, &detachstate);
    if(ret_v == 0)
    {
    	printf("pthread_attr_getdetachstate ok,thread %lu detach state is: %s,buff:%s\n",tid,
    	detachstate == PTHREAD_CREATE_JOINABLE ? "PTHREAD_CREATE_JOINABLE" : "PTHREAD_CREATE_DETACHED",buff);
    }
    else
    {
    	printf("thread %lu pthread_attr_getdetachstate fail %d\n",tid,ret_v);    	
    }
    	
    void *stackaddr = 0;
    size_t stacksize = 0;    
    ret_v = pthread_attr_getstack(&attr,&stackaddr,&stacksize);    
    if(ret_v == 0)
    {
    	printf("pthread_attr_getstack ok,thread %lu,栈地址: %p,end :%p stacksize :%lu\n",tid,stackaddr,stackaddr + stacksize,stacksize);
    }
    else
    {
    	printf("thread %lu pthread_attr_getstack fail %d\n",tid,ret_v);
    }
    
    //the use of `pthread_attr_getstackaddr' is deprecated, use `pthread_attr_getstack'    
    /*ret_v = pthread_attr_getstackaddr(&attr, &stackaddr);
    if(0 == ret_v)
    {
       printf("pthread_attr_getstackaddr ok,thread %lu stackaddr :%p\n",tid,stackaddr);
    }
    else
    {
    	printf("thread %lu pthread_attr_getstackaddr fail %d\n",tid,ret_v);
    }*/    
    
    ret_v = pthread_attr_getstacksize(&attr,&stacksize);
    if(ret_v == 0)
    {
    	printf("pthread_attr_getstacksize ok,thread %lu stack size:%lu\n",tid,stacksize);
    }
    else
    {
    	printf("thread %lu pthread_attr_getstacksize fail %d\n",tid,ret_v);
    }
    
    //默认 4K,如果修改了栈地址,系统默认自行管理线程栈末尾的警戒缓冲区大小,默认设置为0
    size_t guard_size = 4096;
    ret_v = pthread_attr_getguardsize(&attr,&guard_size);
    if(ret_v == 0)
    {
    	printf("pthread_attr_getguardsize ok,thread %lu guard size :%lu\n",tid,guard_size);
    }
    else
    {
    	printf("thread %lu pthread_attr_getguardsize fail %d\n",tid,ret_v);
    }
    
    //作用域控制线程是否在进程内或在系统级上竞争资源，可能的值是
    //PTHREAD_SCOPE_PROCESS ,进程内竞争资源
    //PTHREAD_SCOPE_SYSTEM ,系统级竞争资源,默认    
    int32_t scope = PTHREAD_SCOPE_SYSTEM; 
    ret_v = pthread_attr_getscope(&attr,&scope);
    if(ret_v == 0)
    {
    	printf("pthread_attr_getscope ok,thread %lu scope:%s\n",tid,
    	scope == PTHREAD_SCOPE_PROCESS ? "PTHREAD_SCOPE_PROCESS" : "PTHREAD_SCOPE_SYSTEM");
    }
    else
    {
    	printf("thread %lu pthread_attr_getscope fail %d\n",tid,ret_v);
    }    
    
    //分别用来设置和得到线程的继承性
    //线程没有默认的继承值设置，所以如果关心线程的调度策略和参数,只能手动设置
    //PTHREAD_INHERIT_SCHED ,新的线程继承创建线程的策略和参数
    //PTHREAD_EXPLICIT_SCHED ,新的线程继承策略和参数来自于schedpolicy和schedparam属性中显式设置的调度信息
    int32_t inheritsched = PTHREAD_INHERIT_SCHED; 
    ret_v = pthread_attr_getinheritsched(&attr,&inheritsched);
    if(ret_v == 0)
    {
    	printf("pthread_attr_getinheritsched ok,thread %lu inheritsched :%s\n",tid,
    	inheritsched == PTHREAD_INHERIT_SCHED ? "PTHREAD_INHERIT_SCHED" : "PTHREAD_EXPLICIT_SCHED");
    }
    else
    {
    	printf("thread %lu pthread_attr_getinheritsched fail %d\n",tid,ret_v);
    }
    
    //设置和得到线程的调度策略
    //所谓调度策略也就是我们之前在OS中所学过的那些调度算法：
    //SCHED_FIFO    ：先进先出
    //SCHED_RR       ：轮转法
    //SCHED_OTHER    ：其他方法,默认调度方法                   
    //SCHED_OTHER是不支持优先级使用的,而SCHED_FIFO和SCHED_RR支持优先级的使用,他们分别为1和99,数值越大优先级越高.
    int32_t schedpolicy = PTHREAD_INHERIT_SCHED; 
    ret_v = pthread_attr_getschedpolicy(&attr,&schedpolicy);
    if(ret_v == 0)
    {
    	printf("pthread_attr_getschedpolicy ok,thread %lu schedpolicy :%s\n",tid,
    	schedpolicy == SCHED_OTHER ? "SCHED_OTHER" : "SCHED_FIFO");
    }
    else
    {
    	printf("thread %lu pthread_attr_getschedpolicy fail %d\n",tid,ret_v);
    }
    
    //设置和得到线程的调度参数
    //struct sched_param
    // {
             int sched_priority;    //!> 参数的本质就是优先级
    // };
    //注意：大的权值对应高的优先级!
    //系统支持的最大和最小的优先级值可以用函数：
    // sched_get_priority_max 和 sched_get_priority_min 得到！
    //Linux allows the static priority value range 1 to 99 for SCHED_FIFO and SCHED_RR and the priority 0 for SCHED_OTHER and
    //SCHED_BATCH.  Scheduling priority ranges for the various policies are not alterable
    struct sched_param param = {0};
    ret_v = pthread_attr_getschedparam(&attr,&param);
    if(ret_v == 0)
    {
    	printf("pthread_attr_getschedparam ok,thread %lu schedparam :%d,sched_get_priority_max:%d,sched_get_priority_min:%d\n",
    	tid,param.sched_priority,sched_get_priority_max(SCHED_FIFO),sched_get_priority_min(SCHED_FIFO));
    }
    else
    {
    	printf("thread %lu pthread_attr_getschedparam fail %d\n",tid,ret_v);
    }        
    	
    pthread_attr_destroy(&attr);
    
    return 0;
}


