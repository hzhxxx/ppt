

#include <cstdint>
#include <iostream>
#include <deque>
#include <thread>
#include <mutex>
#include <chrono>
#include <condition_variable>

using namespace std;

//todo
/**********************************************************
Description: 多线程阻塞消息队列
Others: 采用一个锁，两个条件变量
Function List:
1. 多线程
2. 阻塞式,支持秒级超时机制

[root@slave02 thread]# g++ -Wall -std=gnu++11 -g blocking_queue_cpp.cpp -o blocking_queue_cpp -lpthread;./blocking_queue_cpp
now=1524476424
push_queue thread 140507280336640 start
push_queue thread 140507297122048 start
pull_queue thread 140507288729344 start
pull_queue thread 140507271943936 start
push_queue thread 140507297122048 exit,failover=145,i=6000000
push_queue thread 140507280336640 exit,failover=132,i=6000000
pull_queue thread 140507271943936 exit,failover=1,i=5900353
pull_queue thread 140507288729344 exit,failover=1,i=6099647
now:1524476434,time consuming:10

**********************************************************/

template<typename T> class BlockingQueue
{
public:
    explicit BlockingQueue(int32_t max_size):capacity_(max_size > 0 && max_size <= INT32_MAX ? max_size : INT32_MAX)
    	{
    		//
 	}
 	
 	//增加和取出队列元素
	bool push(const T& value,int32_t seconds)
	{
	    std::unique_lock<std::mutex> unique(mutex_);	    
	    while (queue_.size() == this->capacity_)
	    {
	    	std::cv_status s = cond_push_.wait_for(unique,std::chrono::seconds(seconds));
	    	if(s == std::cv_status::timeout)
	    	{	    		
	    		return false;
	    	}
	    	else if(s == std::cv_status::no_timeout)
	    	{
	    		continue;
	    	}	    	
	    }	    
	    queue_.push_back(value);	    
	    cond_pull_.notify_all();
	    return true;
	}
		
	bool pull(T & value,int32_t seconds)
	{	    
	    std::unique_lock<std::mutex> unique(mutex_);
	    	    	    
	    /*int32_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
             auto tp = std::chrono::system_clock::from_time_t(now + seconds);
	    if(!cond_pull_.wait_until(unique,tp, [this](){ return !queue_.empty(); } ))
	    {
	    	return false;
	    }*/
	    	    
	    while(queue_.empty())
	    {	    	
	    	if(std::cv_status::timeout == cond_pull_.wait_for(unique,std::chrono::seconds(seconds)))
	    	{	    	
	    		return false;
	    	}	    	
	    }
	    value = queue_.front();
	    queue_.pop_front();
	    cond_push_.notify_all();	    
	    return true;
	}
	
	int32_t get_size()
	{		
		std::lock_guard<std::mutex> guard(mutex_);
		return queue_.size();
	}
	
	int32_t get_capacity()
	{
		return capacity_;
	}
 private:
 	//一把锁,两个条件变量的经典实现
  	mutable std::mutex mutex_;
  	std::condition_variable cond_push_,cond_pull_;
  	//最大容量
    	int32_t capacity_;
  	std::deque<T> queue_;
 };

void *push_queue(void *data)
{
    printf("push_queue thread %lu start\n",pthread_self());
    BlockingQueue<int32_t> *queue = (BlockingQueue<int32_t> *)data;
    int total = 6000000;
    static __thread int i = 0;
    static __thread int failover = 0;
    while(total > i)
    {
        int data = i;
        if(!queue->push(data,i % 2 == 0 ? 0 : 1))
        {
            //插入失败,自己清理现场
            failover++;            
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
    BlockingQueue<int32_t> *queue = (BlockingQueue<int32_t> *)data;
    static __thread int failover = 0;
    static __thread int i = 0;    
    while(1)
    {        
        int32_t data = 0;
        if(!queue->pull(data,1))
        {
        	    failover++;
             if(queue->get_size() <= 0)
             {
                     break;
             }             
        }
        else
        {
            ++i;
        }        
    }
    printf("pull_queue thread %lu exit,failover=%d,i=%d\n",pthread_self(),failover,i);
    return 0;
}


void simple_queue_test(void)
{
    BlockingQueue<int32_t> queue(600000);   
    
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
}


int main(int argc,char *argv[])
{
    simple_queue_test(); 
    	
    return 0;
}