
//todo
/*
[root@slave02 thread]# gcc -Wall -std=gnu11 -g affinity_thread.c -o affinity_thread -lpthread;./affinity_thread 
this system has 24 processor(s)
this thread 140509414381312 is running in processor 5
this thread 140509422774016 is running in processor 4
this thread 140509405988608 is running in processor 6
this thread 140509397595904 is running in processor 7
*/

//top Ӧ���ܿ�������һֱ�� 4,5,6,7 �� CPU �����У�û�б��л���ȥ
//������趨 CPU �׺�,Ӧ�ûᱻ�л������� CPU
    

/*

NAME
       pthread_setaffinity_np, pthread_getaffinity_np - set/get CPU affinity of a thread

SYNOPSIS
       #define _GNU_SOURCE             // See feature_test_macros(7) 
       #include <pthread.h>

       int pthread_setaffinity_np(pthread_t thread, size_t cpusetsize,
                                  const cpu_set_t *cpuset);
       int pthread_getaffinity_np(pthread_t thread, size_t cpusetsize,
                                  cpu_set_t *cpuset);

       Compile and link with -pthread.
 */

//�����Ķ������ư󶨽��̵Ķ��壬�ο��󶨽��̵Ľ���,��������һ��������
// pid_t,һ����pthread_t,�ֱ������̺��߳�

//����ͬ���Ǹ���������ӣ�����ǰ�̰߳󶨵�0��1��2��3�� cpu��

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <sched.h>

typedef struct sched_cpu
{
	//CPU ����
	int32_t index_;
	//CPU �ܺ���
	int32_t total_;
} sched_cpu_t;

void *f_c(void *arg)
{
    sched_cpu_t *cpu = (sched_cpu_t *)arg;   
    
    //��Ϊ�ҵĲ��Ի����� 24��,�����ҳ��Դ� 4�� CPU ��ʼ    
    /*��4��5��6��7��ӵ�������*/
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu->index_ + 4, &mask);
    
    /* ����cpu �׺���(affinity)*/
    if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0) {
        fprintf(stderr, "set thread affinity failed\n");
    }   
    
    /* �鿴cpu �׺���(affinity)*/
    cpu_set_t get;
    CPU_ZERO(&get);
    if (pthread_getaffinity_np(pthread_self(), sizeof(get), &get) < 0) {
        fprintf(stderr, "get thread affinity failed\n");
    }   

    /* �鿴��ǰ�߳������е�����cpu*/
    for (int32_t i = 0; i < cpu->total_; i++) {
        if (CPU_ISSET(i, &get)) {
            printf("this thread %lu is running in processor %d\n", pthread_self(), i); 
            break;
        }   
    }
    
    for(int64_t k = 0;k< 1000000000000;++k)
    {
    	int32_t v = 0;
    	v+=k;
    	if(v >= INT64_MAX)
    	{
    		v = 0;
    	}
    }
       
    pthread_exit(0);
}
 
int main(int argc, char *argv[])
{
    #define NUM  4
    pthread_t ts[NUM] = {0};
    sched_cpu_t cpu[NUM] = {{0}};
    
    cpu[0].total_ = sysconf(_SC_NPROCESSORS_CONF);
    printf("this system has %d processor(s)\n", cpu[0].total_);    
	
    
    for(int16_t i = 0; i < NUM;++i)
    {
        cpu[i].total_ = cpu[0].total_;
        cpu[i].index_ = i;
        pthread_create(&ts[i],0,f_c,&cpu[i]);
    }

    for(int16_t i = 0;i < NUM;++i)
    {        
        pthread_join(ts[i],0);        
    }
        
    return 0;
}