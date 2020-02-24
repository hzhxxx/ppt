

//todo
/*

[root@Slave02 thread]# gcc -Wall -std=gnu11 -g3 signal.c -o signal -lpthread;./signal&
[4] 27030
[root@Slave02 thread]# kill -USR1 27030
thread 140232974169856 Signal handling thread got signal 10
[root@Slave02 thread]# kill -QUIT 27030
thread 140232974169856 Signal handling thread got signal 3
[root@Slave02 thread]# kill -TERM 27030
[root@Slave02 thread]# kill -TERM 27030
-bash: kill: (27030) - 没有那个进程
[4]+  已终止               ./signal

*/

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>

#define handle_error_en(en, msg)  do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

void sighandler(int sig)
{
	if(sig == SIGUSR1)
	{
		printf("thread %ld Signal handling thread got signal,do with signal %d\n", pthread_self(),sig);
	}
	else
	{
		printf("thread %ld Signal handling thread got signal %d\n", pthread_self(),sig);
	}
}

static void *sig_thread(void *arg)
{
    sigset_t *set = arg;
    int32_t s = 0, sig = 0;
    signal(SIGUSR1, sighandler);
    
    for (;;)
    {    	
    	s = sigwait(set, &sig);
        if (s != 0)
        {
        	handle_error_en(s, "sigwait");
        }
        printf("thread %ld Signal handling thread got signal %d\n", pthread_self(),sig);
    }
}

int main(int argc,char * argv[])
{
    #define NUM 5
    pthread_t ts[NUM] = {0};
     
     sigset_t set;    

     /* Block SIGQUIT and SIGUSR1; other threads created by main()  will inherit a copy of the signal mask. */
     sigemptyset(&set);
     sigaddset(&set, SIGQUIT);
     sigaddset(&set, SIGUSR1);
     int32_t s = pthread_sigmask(SIG_BLOCK, &set, NULL);
     if (s != 0)
     {
         handle_error_en(s, "pthread_sigmask");
     }
     
     signal(SIGUSR1, sighandler);
         
    for(int16_t i = 0; i < NUM;++i)
    {       
        pthread_create(&ts[i], 0, &sig_thread, (void *) &set);
    }

    for(int16_t i = 0;i < NUM;++i)
    {        
        int value = pthread_join(ts[i],0);
        if(value == 0)
        {
             printf("thread id %lu exit\n",ts[i]);        
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
    
    pause();
     
    return 0;
}
