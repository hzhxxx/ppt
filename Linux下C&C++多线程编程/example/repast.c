

//todo
/*
[root@Slave02 thread]# gcc -Wall -std=gnu11 -g3 repast.c -o repast -lpthread;./repast 
philosopher A fetches chopstick 5
philosopher B fetches chopstick 1
philosopher D fetches chopstick 3
philosopher C fetches chopstick 2
philosopher C fetches chopstick 3
philosopher C is eating.
philosopher E fetches chopstick 4
philosopher E fetches chopstick 5
philosopher E is eating.
philosopher B fetches chopstick 1
philosopher C release chopstick 2
philosopher C release chopstick 3
philosopher D fetches chopstick 3
philosopher D fetches chopstick 4
philosopher D is eating.
philosopher E release chopstick 4
philosopher E release chopstick 5
philosopher A fetches chopstick 5
philosopher A fetches chopstick 1
philosopher A is eating.
philosopher C fetches chopstick 2

*/

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>

//哲学家个数
#define N 5
//休眠间隔
#define SECONDS 3
//筷子数
pthread_mutex_t chopstick[N];

//工作(吃食物+思考)
void *work_eat_think(void *arg)
{
    char philosopher = *(char *)arg;    
    //左右筷子的编号
    int32_t left = 0, right = 0;
    switch (philosopher)
    {
    case 'A':
    {
        left = 5;
        right = 1;
        break;
    }
    case 'B':
    {
        left = 1;
        right = 2;
        break;
    }
    case 'C':
    {
        left = 2;
        right = 3;
        break;
    }
    case 'D':
    {
        left = 3;
        right = 4;
        break;
    }
    case 'E':
    {
        left = 4;
        right = 5;
        break;
    }
    }

    while (1)
    {
        //模拟思考
        sleep(SECONDS * 2);
        
        //拿起左手的筷子
        pthread_mutex_lock(&chopstick[left]);
        printf("philosopher %c fetches chopstick %d\n", philosopher, left);
        
        //试图拿起右手的筷子
        if (pthread_mutex_trylock(&chopstick[right]) == EBUSY)
        {       
            //如果右边筷子被拿走放下左手的筷子
            pthread_mutex_unlock(&chopstick[left]);
            continue;
        }

        //拿起右手的筷子，如果想观察死锁，把上一句if注释掉，再把这一句的注释去掉
        //pthread_mutex_lock(&chopstick[right]);
        printf("philosopher %c fetches chopstick %d\n", philosopher, right);
        printf("philosopher %c is eating.\n", philosopher);
        
        //模拟吃饭过程
        sleep(SECONDS * 3);
        
        //放下左手的筷子
        pthread_mutex_unlock(&chopstick[left]); 
        printf("philosopher %c release chopstick %d\n", philosopher, left);
        
        //放下右边筷子
        pthread_mutex_unlock(&chopstick[right]); 
        printf("philosopher %c release chopstick %d\n", philosopher, right);        
    }
}

int32_t main(int argc, char *argv[])
{
    pthread_t ts[N]; 
    //哲学家个数
    char philosopher[N] = {'A', 'B', 'C', 'D', 'E'};
    for (int32_t i = 0; i < N; i++)
    {
        pthread_mutex_init(&chopstick[i], 0);
        pthread_create(&ts[i], 0, work_eat_think, &philosopher[i]);
    }

    for (int32_t i = 0; i < N; i++)
    {
        pthread_join(ts[i], 0);
    }
    
    for (int32_t i = 0; i < N; i++)
    {
    	pthread_mutex_destroy(&chopstick[i]);
    }

    return 0;
}