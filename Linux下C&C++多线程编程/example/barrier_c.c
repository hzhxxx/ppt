
//todo
/*
[root@slave02 thread]# gcc -Wall -std=gnu11 -g barrier_c.c -o barrier_c -lpthread;./barrier_c
注意看顺序,index:0,array:[0],thread id:140007195739904
注意看顺序,index:1,array:[1],thread id:140007187347200
注意看顺序,index:2,array:[2],thread id:140007178954496
thread id:140007178954496 index 2，wait后第一个返回
thread id:140007195739904 index 0，wait后返回
thread id:140007187347200 index 1，wait后返回
所有线程工作已完成...
thread id 140007195739904 exit code :0
thread id 140007187347200 exit code :1
thread id 140007178954496 exit code :2
*/

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

void* f_c(void * i);

#define PTHREAD_BARRIER_SIZE 4
pthread_barrier_t barrier;

int main(int argc,char * argv[])
{
    //PTHREAD_BARRIER_SIZE,这个参数必须和实际等待的线程数目匹配
    pthread_barrier_init(&barrier, NULL, PTHREAD_BARRIER_SIZE);
    #define NUM 3
    pthread_t ts[NUM] = {0};
    for(int16_t i = 0; i < NUM;++i)
    {
        int16_t * index = (int16_t *)malloc(sizeof(int16_t));
        *index = i; 
        pthread_create(&ts[i],0,f_c,index);
    }
    
    //主线程模拟干点其他事情再等待一起出发
    for(int32_t i = 0;i< 5;++i)
    {
    	sleep(1);
    }
    printf("main thread loop over\n");
        
    /* 主线程干完事情后,等待其他线程工作完成后，一起再继续执行下面流程 */
    int32_t result = pthread_barrier_wait(&barrier);
    if (result == PTHREAD_BARRIER_SERIAL_THREAD)
    {
    	printf("main thread id:%lu wait后第一个返回\n",pthread_self());
    }
    else if (result == 0)
    {    	
         printf("main thread id:%lu wait后返回\n",pthread_self());
    }
    pthread_barrier_destroy(&barrier);
    printf("所有线程工作已完成...\n"); 
    
    for(int16_t i = 0;i < NUM;++i)
    {
        void *exitcode = 0;
        int32_t value = pthread_join(ts[i],&exitcode);
        if(value == 0)
        {
             printf("thread id %lu exit code :%d\n",ts[i],*(int32_t*)exitcode);
             free(exitcode);
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

    //主线程退出,变成僵尸进程
    pthread_exit(0);
     
    return 0;
}

void * f_c(void * i)
{
    char buff[256] = {0};
    static int32_t index = 0;
    int32_t local = __sync_fetch_and_add(&index,1);
    snprintf(buff,sizeof(buff),"注意看顺序,index:%d,array:[%d],thread id:%lu\n",local,*(int16_t*)i,pthread_self());
    printf(buff);
    
    /* 等待屏障 */    
    int32_t result = pthread_barrier_wait(&barrier);
    if (result == PTHREAD_BARRIER_SERIAL_THREAD)
    {
    	printf("thread id:%lu index %i，wait后第一个返回\n",pthread_self(),*(int16_t*)i);
    }
    else if (result == 0)
    {    	
         printf("thread id:%lu index %i，wait后返回\n",pthread_self(),*(int16_t*)i);
    }
    free(i);
        
    int32_t * exitcode = 0;    
    exitcode = (int32_t *)malloc(sizeof(int32_t));
    *exitcode = local;    
    
    return (void *)exitcode;
}



