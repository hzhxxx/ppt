
//todo
/*
[root@slave02 thread]# gcc -Wall -std=gnu11 -g affinity_process.c -o affinity_process;./affinity_process
cpus: 24
this process 124775 of running processor: 0
this process 124775 of running processor: 1
this process 124775 of running processor: 2
this process 124775 of running processor: 3
*/

//top Ӧ���ܿ�������һֱ�� 0,1,2,3 �� CPU ��ĳ�������У�û�б��л���ȥ
//������趨 CPU �׺�,Ӧ�ûᱻ�л������� CPU
//������0, 1,2,3 ���Ǹ��ϣ������ȫ�ɲ���ϵͳ(OS)�ڲ�����
// Ҳ���������ˣ���һ����ȫ��Ч������ OS �ᰴ�����õ����ŷ�ʽ����
    

/*
  NAME
        sched_setaffinity, sched_getaffinity - set and get a process's CPU affinity mask
 
 SYNOPSIS
        #define _GNU_SOURCE              See feature_test_macros(7) 
        #include <sched.h>
 
        int sched_setaffinity(pid_t pid, size_t cpusetsize, cpu_set_t *mask);
        �ú������ý���Ϊpid���������,����������mask���趨��CPU��.���pid��ֵΪ0,
         *���ʾָ�����ǵ�ǰ����,ʹ��ǰ����������mask���趨����ЩCPU��.
         *�ڶ�������cpusetsize��mask��ָ�������ĳ���.ͨ���趨Ϊsizeof(cpu_set_t).
         *�����ǰpid��ָ���Ľ��̴�ʱû��������mask��ָ��������һ��CPU��,
         *���ָ���Ľ��̻������CPU��Ǩ�Ƶ�mask��ָ����һ��CPU������.
 
        int sched_getaffinity(pid_t pid, size_t cpusetsize, cpu_set_t *mask);
        �ú������pid��ָʾ�Ľ��̵�CPUλ����,���������뷵�ص�mask��ָ��Ľṹ��.
         *�����ָ��pid��ǰ������������ЩCPU��.
         *ͬ��,���pid��ֵΪ0.Ҳ��ʾ���ǵ�ǰ����
 
     RETURN VALUE
        On success, sched_setaffinity() and sched_getaffinity() return 0.  On error, -1 is returned, and errno is set appropriately.

 void CPU_ZERO (cpu_set_t *set)
 //������ CPU �� set ���г�ʼ������������Ϊ�ռ���
 void CPU_SET (int cpu, cpu_set_t *set)
 //����꽫 ָ���� cpu ���� CPU �� set ��
 void CPU_CLR (int cpu, cpu_set_t *set)
 //����꽫 ָ���� cpu �� CPU �� set ��ɾ����
 int CPU_ISSET (int cpu, const cpu_set_t *set)
 //��� cpu �� CPU �� set ��һԱ�������ͷ���һ������ֵ��true��������ͷ����㣨false����
 
 */

//������һ����������ӣ�����ǰ���̰󶨵� 0��1��2��3�� cpu��

#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
  
 /* sysconf( _SC_NPROCESSORS_CONF ) �鿴cpu�ĸ�������ӡ��%ld������
  * sysconf( _SC_NPROCESSORS_ONLN ) �鿴��ʹ�õ�cpu��������ӡ��%ld���� 
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

    CPU_ZERO(&mask);    /* ��ʼ��set������set��Ϊ��*/
    CPU_SET(0, &mask);  /* ���ν�0��1��2��3��cpu���뵽���ϣ�ǰ������Ļ����Ƕ�˴�����*/
    CPU_SET(1, &mask);
    CPU_SET(2, &mask);
    CPU_SET(3, &mask);
    
    /*����cpu �׺��ԣ�affinity��*/
    if (sched_setaffinity(0, sizeof(mask), &mask) == -1) {
        printf("Set CPU affinity failue, ERROR:%s\n", strerror(errno));
       return -1; 
    }   
    usleep(1000); /* �õ�ǰ���������㹻ʱ����Ч*/

    /*�鿴��ǰ���̵�cpu �׺���*/
    CPU_ZERO(&get);
    if (sched_getaffinity(0, sizeof(get), &get) == -1) {
        printf("get CPU affinity failue, ERROR:%s\n", strerror(errno));
        return -1; 
    }   
    
    /*�鿴�����ڵ�ǰ���̵�cpu*/
    for(i = 0; i < cpus; i++) {

        if (CPU_ISSET(i, &get)) { /*�鿴cpu i �Ƿ���get ���ϵ���*/
            printf("this process %d of running processor: %d\n", getpid(), i); 
        }    
    }
    
    //�����̵� CPU �׺���	
    f_c(0);    
       
    return 0;
}
