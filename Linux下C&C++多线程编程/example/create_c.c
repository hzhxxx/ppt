

//todo
//gcc -std=gnu11 create_c.c -o create_c -lpthread;./create_c
//[root@slave02 thread]# gcc -Wall -std=gnu11 -g create_c.c -o create_c -lpthread;./create_c

/*
注意看顺序,index:2,array:[2],thread id:140479835748096
注意看顺序,index:3,array:[3],thread id:140479827355392
注意看顺序,index:1,array:[1],thread id:140479844140800
注意看顺序,index:0,array:[0],thread id:140479852533504
注意看顺序,index:4,array:[4],thread id:140479818962688
thread id 140479852533504 exit code :0
thread id 140479844140800 EINVAL
thread id 140479835748096 exit code :2
thread id 140479827355392 EINVAL
thread id 140479818962688 exit code :4
*/

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

void* f_c(void * i);

int main(int argc,char * argv[])
{
    #define NUM 5
    pthread_t ts[NUM] = {0};
    for(int16_t i = 0; i < NUM;++i)
    {
        int16_t * index = (int16_t *)malloc(sizeof(int16_t));
        *index = i; 
        pthread_create(&ts[i],0,f_c,index);
    }

    for(int16_t i = 0;i < NUM;++i)
    {
        //marks  the  thread  identified by thread as detached
        if( i % 2 != 0)
        {
           pthread_detach(ts[i]);
        }
    }

    for(int16_t i = 0;i < NUM;++i)
    {
        void *exitcode = 0;
        int value = pthread_join(ts[i],&exitcode);
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
    snprintf(buff,sizeof(buff),"注意看顺序,index:%d,array:[%d],thread id:%lu\n",
             local,*(int16_t*)i,pthread_self());
    printf(buff);
    int32_t * exitcode = 0;
    if(*(int16_t *)i % 2 == 0)
    {
       exitcode = (int32_t *)malloc(sizeof(int32_t));
       *exitcode = local;
       sleep(1);
    }
    else
    {
        sleep(10);    
    }
    free(i);
    return (void *)exitcode;
}



