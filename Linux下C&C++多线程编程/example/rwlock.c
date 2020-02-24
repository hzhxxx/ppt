
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
[root@slave02 thread]# gcc -Wall -std=gnu11 -g rwlock.c -o rwlock -lpthread;./rwlock 
now=1524541155
push_queue thread 140595101673216 start
pull_queue thread 140595093280512 start
push_queue thread 140595084887808 start
pull_queue thread 140595076495104 start
push_queue thread 140595101673216 exit,failover=0,i=6000000
push_queue thread 140595084887808 exit,failover=0,i=6000000
pull_queue thread 140595076495104 exit,failover=5,i=5703643
pull_queue thread 140595093280512 exit,failover=8,i=6296357
now:1524541177,time consuming:22

**********************************************************/

typedef void (*ENTRY_FREE_CB)(void *);
typedef bool (*pfnQUEUE_ITERATOR_CALLBACK)(void *nodedata, void *in_pContext);

#define READ_QUEUE_WAIT 1
#define WRITE_QUEUE_WAIT 2
#define WAIT_QUEUE_SUCCESS 1
#define WAIT_QUEUE_TIMEOUT 2
#define WAIT_QUEUE_ERROR 3

typedef struct queue_entry_t
{
    void *data_;
    SIMPLEQ_ENTRY(queue_entry_t) queue_entries_t;
} QUEUE_ENTRY_T;
SIMPLEQ_HEAD(queue_head_t,queue_entry_t);

typedef struct rwlock_queue_t
{
    //最大容量
    int capacity_;
    //当前元素数
    int size_;
    //读写锁
    pthread_rwlock_t rwlock_;    
    //队列头
    struct queue_head_t head_;
} RWLOCK_QUEUE_T;

void initiate_queue(RWLOCK_QUEUE_T *queue,int capacity);
void destroy_queue(RWLOCK_QUEUE_T *queue);

//增加和取出队列元素
bool push_queue_tail(RWLOCK_QUEUE_T *queue,void *data,int seconds);
void *pull_queue_head(RWLOCK_QUEUE_T *queue,int seconds);

//获取队列元素个数
int get_queue_count(RWLOCK_QUEUE_T *queue);
int get_queue_capacity(RWLOCK_QUEUE_T *queue);
int get_queue_remainsize(RWLOCK_QUEUE_T *queue);

//遍历队列
bool iterator_queue(RWLOCK_QUEUE_T *queue, pfnQUEUE_ITERATOR_CALLBACK in_pIteratorFunc, void *in_pContext);

void unlock_queue(RWLOCK_QUEUE_T *queue)
{
    assert(queue);
    pthread_rwlock_unlock(&queue->rwlock_);
}

void initiate_queue(RWLOCK_QUEUE_T *queue,int32_t capacity)
{
    assert(queue);

    SIMPLEQ_INIT(&queue->head_);
    queue->capacity_ = (capacity > 0 && capacity < INT32_MAX)?capacity:10000;
    queue->size_ = 0;
    pthread_rwlock_init(&queue->rwlock_,0);    
}

void destroy_queue(RWLOCK_QUEUE_T *queue)
{
    assert(queue);    
    pthread_rwlock_destroy(&queue->rwlock_);    
}

int32_t wait_queue(RWLOCK_QUEUE_T *queue,int32_t action, int32_t seconds)
{
    int32_t result = 0;
    if(seconds <= 0)
    {        
        if(READ_QUEUE_WAIT == action)
        {
            result = pthread_rwlock_rdlock(&queue->rwlock_);
        }
        else
        {
            result = pthread_rwlock_wrlock(&queue->rwlock_);	
        }       
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
        if(READ_QUEUE_WAIT == action)
        {
        	result = pthread_rwlock_timedrdlock(&queue->rwlock_,&outtime);
        }
        else
        {
        	result = pthread_rwlock_timedwrlock(&queue->rwlock_,&outtime);
        }
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

bool push_queue_tail(RWLOCK_QUEUE_T *queue,void *data,int32_t seconds)
{
    assert(queue && data);

    if(WAIT_QUEUE_SUCCESS != wait_queue(queue,WRITE_QUEUE_WAIT,seconds)) return false;
    while(queue->capacity_ <= queue->size_)
    {
        unlock_queue(queue);
        return false;
    }

    struct queue_entry_t *node =
        (struct queue_entry_t *)malloc(sizeof(struct queue_entry_t));
    node->data_ = data;
    SIMPLEQ_INSERT_TAIL(&queue->head_,node,queue_entries_t);
    ++queue->size_;
    unlock_queue(queue);   
    
    return true;
}

void *pull_queue_head(RWLOCK_QUEUE_T *queue,int32_t seconds)
{
    assert(queue);
    
    if(WAIT_QUEUE_SUCCESS != wait_queue(queue,WRITE_QUEUE_WAIT,seconds)) return 0;
    while(queue->size_ <= 0)
    {
        unlock_queue(queue);
        return false;
    }

    struct queue_entry_t *node = queue->head_.sqh_first;
    SIMPLEQ_REMOVE_HEAD(&queue->head_,queue_entries_t);
    void *data = node->data_;
    --queue->size_;
    free(node);  
    
    unlock_queue(queue);
    return data;
}

int32_t get_queue_count(RWLOCK_QUEUE_T *queue)
{
    int32_t size = -1;
    if(WAIT_QUEUE_SUCCESS != wait_queue(queue,READ_QUEUE_WAIT,0)) return size;
    size = queue->size_;
    unlock_queue(queue);
    return size;
}

int32_t get_queue_capacity(RWLOCK_QUEUE_T *queue)
{
    return queue->capacity_;
}

int32_t get_queue_remainsize(RWLOCK_QUEUE_T *queue)
{
    int32_t remainsize = -1;
    if(WAIT_QUEUE_SUCCESS != wait_queue(queue,READ_QUEUE_WAIT,0)) return remainsize;
    remainsize = queue->capacity_ - queue->size_;
    unlock_queue(queue);
    return remainsize;
}

struct queue_entry_t *get_queue_next(struct queue_entry_t *np)
{
    assert(np);
    return (struct queue_entry_t *)SIMPLEQ_NEXT(np,queue_entries_t);
}

struct queue_entry_t *get_queue_first(RWLOCK_QUEUE_T *queue)
{
    assert(queue);
    return SIMPLEQ_FIRST(&queue->head_);
}

bool iterator_queue(RWLOCK_QUEUE_T *queue, pfnQUEUE_ITERATOR_CALLBACK in_pIteratorFunc, void *in_pContext)
{
    if(get_queue_count(queue) > 0)
    {
        if(WAIT_QUEUE_SUCCESS != wait_queue(queue,READ_QUEUE_WAIT,0)) return false;
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
    RWLOCK_QUEUE_T *queue = (RWLOCK_QUEUE_T *)data;
    int total = 6000000;
    static __thread int i = 0;
    static __thread int failover = 0;
    int32_t retry = 0;
    while(total > i && retry <= 3)
    {
        int *p_data = 0;
        p_data = (int *)malloc(sizeof(int));
        *p_data = i;
        if(!push_queue_tail(queue,(void *)p_data,i % 2 == 0 ? 0 : 1))
        {
            //插入失败,自己清理现场
            failover++;
            free(p_data);
            
            if(get_queue_remainsize(queue) <= 0)
            {
                sleep(1);
                retry++;                
            }
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
    RWLOCK_QUEUE_T *queue = (RWLOCK_QUEUE_T *)data;
    static __thread int failover = 0;
    static __thread int i = 0;
    int32_t retry = 0;
    while(retry <= 3)
    {
        int *p_data = 0;
        p_data = (int *)pull_queue_head(queue,1);
        if(p_data == 0)
        {
            failover++;
            if(get_queue_count(queue) <= 0)
            {
                sleep(1);
                retry++;
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
    RWLOCK_QUEUE_T queue;
    initiate_queue(&queue,6000000);

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