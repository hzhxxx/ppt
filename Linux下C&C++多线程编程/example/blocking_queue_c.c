
#define _GNU_SOURCE
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/queue.h>

//todo
/**********************************************************
Description: 多线程阻塞消息队列
Others: 采用一个锁，两个条件变量
Function List:
1. 多线程
2. 阻塞式,支持秒级超时机制

[root@slave02 thread]# gcc -Wall -std=gnu11 -g blocking_queue_c.c -o blocking_queue_c -lpthread;./blocking_queue_c 
now=1524466544
poll_queue thread 140181240424192 start
poll_queue thread 140181223638784 start
push_queue thread 140181232031488 start
push_queue thread 140181248816896 start
push_queue thread 140181248816896 exit,failover=0,i=6000000
push_queue thread 140181232031488 exit,failover=0,i=6000000
poll_queue thread 140181223638784 exit,failover=1,i=5708750
poll_queue thread 140181240424192 exit,failover=1,i=6291252
now:1524466557,time consuming:13

**********************************************************/

typedef void (*ENTRY_FREE_CB)(void *);
typedef bool (*pfnQUEUE_ITERATOR_CALLBACK)(void *nodedata, void *in_pContext);

#define PULL_QUEUE_WAIT 1
#define PUSH_QUEUE_WAIT 2
#define WAIT_QUEUE_SUCCESS 1
#define WAIT_QUEUE_TIMEOUT 2
#define WAIT_QUEUE_ERROR 3

typedef struct queue_entry_t
{
    void *data_;
    SIMPLEQ_ENTRY(queue_entry_t) queue_entries_t;
} QUEUE_ENTRY_T;
SIMPLEQ_HEAD(queue_head_t,queue_entry_t);

typedef struct blocking_queue_t
{
    //最大容量
    int capacity_;
    //当前元素数
    int size_;
    pthread_mutexattr_t mutexattr_;
    //一把锁,两个条件变量的经典实现
    pthread_mutex_t mutex_;
    pthread_cond_t cond_push_;
    pthread_cond_t cond_pull_;
    //队列头
    struct queue_head_t head_;
} BLOCKING_QUEUE_T;

void initiate_queue(BLOCKING_QUEUE_T *queue,int capacity);
void destroy_queue(BLOCKING_QUEUE_T *queue);

//增加和取出队列元素
bool push_queue_tail(BLOCKING_QUEUE_T *queue,void *data,int seconds);
void *pull_queue_head(BLOCKING_QUEUE_T *queue,int seconds);

//获取队列元素个数
int get_queue_count(BLOCKING_QUEUE_T *queue);
int get_queue_capacity(BLOCKING_QUEUE_T *queue);
int get_queue_remainsize(BLOCKING_QUEUE_T *queue);

//遍历队列
bool iterator_queue(BLOCKING_QUEUE_T *queue, pfnQUEUE_ITERATOR_CALLBACK in_pIteratorFunc, void *in_pContext);

bool lock_queue(BLOCKING_QUEUE_T *queue)
{
    assert(queue);
    if(0 != pthread_mutex_lock(&queue->mutex_))
    {
        int32_t err = errno;
        if(err == EBUSY || err == EDEADLK || err == EAGAIN || err == EPERM || err == EINVAL)
        {
            return false;
        }
        return false;
    }
    return true;
}

void unlock_queue(BLOCKING_QUEUE_T *queue)
{
    assert(queue);
    pthread_mutex_unlock(&queue->mutex_);
}

int32_t wait_queue(BLOCKING_QUEUE_T *queue,int32_t action, int32_t seconds)
{
    int32_t result = 0;
    pthread_cond_t *cond = (action == PULL_QUEUE_WAIT)?
                           (&queue->cond_pull_):(&queue->cond_push_);

    if(seconds <= 0)
    {
        result = pthread_cond_wait(cond,&queue->mutex_);
    }
    else
    {
        struct timeval now = { 0 };
        struct timespec outtime = { 0 };

        // FIXME: cannot support `milliseconds'
        // long t = now.tv_usec * 1000 + milliseconds % 1000 * 1000 * 1000;
        // outtime.tv_nsec = t % 1000000000;
        // outtime.tv_sec = now.tv_sec + milliseconds / 1000 + t/1000000000;
        gettimeofday(&now,0);
        outtime.tv_sec = now.tv_sec + seconds;
        outtime.tv_nsec = now.tv_usec * 1000;
        result = pthread_cond_timedwait(cond,&queue->mutex_,&outtime);
    }

    if(ETIMEDOUT == result)
    {
        result = WAIT_QUEUE_TIMEOUT;
    }
    else if(0 == result)
    {
        result = WAIT_QUEUE_SUCCESS;
    }
    else
    {
        result = WAIT_QUEUE_ERROR;
    }
    return result;
}

void initiate_queue(BLOCKING_QUEUE_T *queue,int32_t capacity)
{
    assert(queue);

    SIMPLEQ_INIT(&queue->head_);
    queue->capacity_ = (capacity > 0 && capacity < INT32_MAX)?capacity:10000;
    queue->size_ = 0;
    pthread_mutexattr_init(&queue->mutexattr_);
    pthread_mutexattr_settype(&queue->mutexattr_,PTHREAD_MUTEX_ERRORCHECK_NP);
    pthread_mutex_init(&queue->mutex_,&queue->mutexattr_);
    pthread_cond_init(&queue->cond_pull_,0);
    pthread_cond_init(&queue->cond_push_,0);
}

void destroy_queue(BLOCKING_QUEUE_T *queue)
{
    assert(queue);
    pthread_cond_destroy(&queue->cond_push_);
    pthread_cond_destroy(&queue->cond_pull_);
    pthread_mutex_destroy(&queue->mutex_);
    pthread_mutexattr_destroy(&queue->mutexattr_);
}

bool push_queue_tail(BLOCKING_QUEUE_T *queue,void *data,int32_t seconds)
{
    assert(queue && data);

    if(!lock_queue(queue)) return false;
    while(queue->capacity_ <= queue->size_)
    {
        //队列满,自动解锁,等待取出唤醒
        if(WAIT_QUEUE_TIMEOUT == wait_queue(queue,PUSH_QUEUE_WAIT,seconds))
        {
            pthread_mutex_unlock(&queue->mutex_);
            return false;
        }
    }

    struct queue_entry_t *node =
        (struct queue_entry_t *)malloc(sizeof(struct queue_entry_t));
    node->data_ = data;
    SIMPLEQ_INSERT_TAIL(&queue->head_,node,queue_entries_t);
    ++queue->size_;
    pthread_mutex_unlock(&queue->mutex_);
    pthread_cond_broadcast(&queue->cond_pull_);

    return true;
}

void *pull_queue_head(BLOCKING_QUEUE_T *queue,int32_t seconds)
{
    assert(queue);

    void *data = 0;
    if(!lock_queue(queue)) return data;
    while(queue->size_ <= 0)
    {
        //队列空,自动解锁,等待插入唤醒
        if(WAIT_QUEUE_SUCCESS != wait_queue(queue,PULL_QUEUE_WAIT,seconds))
        {
            pthread_mutex_unlock(&queue->mutex_);
            return data;
        }
    }

    struct queue_entry_t *node = queue->head_.sqh_first;
    SIMPLEQ_REMOVE_HEAD(&queue->head_,queue_entries_t);
    data = node->data_;
    --queue->size_;
    free(node);
    pthread_mutex_unlock(&queue->mutex_);
    pthread_cond_broadcast(&queue->cond_push_);

    return data;
}

int32_t get_queue_count(BLOCKING_QUEUE_T *queue)
{
    int32_t size = -1;
    if(!lock_queue(queue)) return size;
    size = queue->size_;
    unlock_queue(queue);
    return size;
}

int32_t get_queue_capacity(BLOCKING_QUEUE_T *queue)
{
    int32_t capacity = -1;
    if(!lock_queue(queue)) return capacity;
    capacity = queue->capacity_;
    unlock_queue(queue);
    return capacity;
}

int32_t get_queue_remainsize(BLOCKING_QUEUE_T *queue)
{
    int32_t remainsize = -1;
    if(!lock_queue(queue)) return remainsize;
    remainsize = queue->capacity_ - queue->size_;
    unlock_queue(queue);
    return remainsize;
}

struct queue_entry_t *get_queue_next(struct queue_entry_t *np)
{
    assert(np);
    return (struct queue_entry_t *)SIMPLEQ_NEXT(np,queue_entries_t);
}

struct queue_entry_t *get_queue_first(BLOCKING_QUEUE_T *queue)
{
    assert(queue);
    return SIMPLEQ_FIRST(&queue->head_);
}

bool iterator_queue(BLOCKING_QUEUE_T *queue, pfnQUEUE_ITERATOR_CALLBACK in_pIteratorFunc, void *in_pContext)
{
    if(get_queue_count(queue) > 0)
    {
        if(!lock_queue(queue)) return false;
        struct queue_entry_t *queuenode = get_queue_first(queue);
        while(queuenode)
        {
            if(!in_pIteratorFunc(queuenode->data_, in_pContext))
            {
                unlock_queue(queue);
                return true;
            }
            queuenode = get_queue_next(queuenode);
        }
        unlock_queue(queue);
    }
    return false;
}

void *push_queue(void *data)
{
    printf("push_queue thread %lu start\n",pthread_self());
    BLOCKING_QUEUE_T *queue = (BLOCKING_QUEUE_T *)data;
    int total = 6000000;
    static __thread int i = 0;
    static __thread int failover = 0;
    while(total > i)
    {
        int *p_data = 0;
        p_data = (int *)malloc(sizeof(int));
        *p_data = i;
        if(!push_queue_tail(queue,p_data,i % 2==0?0:1))
        {
            //插入失败,自己清理现场
            failover++;
            free(p_data);
        }
        else
        {
            //__sync_fetch_and_add(&i,1);
            i++;
        }
    }
    printf("push_queue thread %lu exit,failover=%d,i=%d\n",pthread_self(),failover,i);
    return 0;
}

void *pull_queue(void *data)
{
    printf("pull_queue thread %lu start\n",pthread_self());
    BLOCKING_QUEUE_T *queue = (BLOCKING_QUEUE_T *)data;
    static __thread int failover = 0;
    static __thread int i = 0;    
    while(1)
    {
        int *p_data = 0;
        p_data = (int *)pull_queue_head(queue,1);
        if(p_data == 0)
        {
            failover++;
            if(get_queue_count(queue) <= 0)
            {
                break;
            }
        }
        else
        {
            ++i;
        }
        free(p_data);
    }
    printf("pull_queue thread %lu exit,failover=%d,i=%d\n",pthread_self(),failover,i);
    return 0;
}

void simple_queue_test(void)
{
    BLOCKING_QUEUE_T queue;
    initiate_queue(&queue,600000);
    int *p_data = 0;
    p_data = (int *)malloc(sizeof(int));
    *p_data = 1;
    if(!push_queue_tail(&queue,p_data,2))
    {
        //插入失败,自己清理现场
        free(p_data);
    }

    p_data = (int *)malloc(sizeof(int));
    *p_data = 10;
    if(!push_queue_tail(&queue,p_data,2))
    {
        //插入失败,自己清理现场
        free(p_data);
    }   

    int t1 = time(0);
    printf("now=%d\n",t1);
    pthread_t pid_1,pid_2,pid_3,pid_4;
    pthread_create(&pid_1,0,push_queue,(void *)&queue);
    pthread_create(&pid_3,0,pull_queue,(void *)&queue);
    pthread_create(&pid_2,0,push_queue,(void *)&queue);
    pthread_create(&pid_4,0,pull_queue,(void *)&queue);

    pthread_join(pid_1,0);
    pthread_join(pid_2,0);
    pthread_join(pid_3,0);
    pthread_join(pid_4,0);

    printf("now:%ld,time consuming:%ld\n",time(0),time(0) - t1);

    destroy_queue(&queue);
}

int main(int argc,char *argv[])
{
    simple_queue_test();
    return 0;
}