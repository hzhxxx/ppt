
//todo
/*
[root@slave02 thread]# gcc -Wall -std=gnu++11 -g barrier_cpp.cpp -o barrier_cpp -lpthread -lstdc++;./barrier_cpp
main thread loop over
所有线程工作已完成...
thread id: 140318919038720,index: 1 wait后返回
thread id: 140318927431424,index: 0 wait后返回
thread id: 140318910646016,index: 2 wait后返回
thread: 140318927431424,exitcode: 1
thread: 140318919038720,exitcode: 2
thread: 140318910646016,exitcode: 3
*/

#include <condition_variable>
#include <mutex>
#include <thread>
#include <future>
#include <chrono>
#include <iostream>
#include <sstream>
#include <algorithm>  
#include <functional>
using namespace std;

class Barrier
{
public:
    Barrier(int32_t barrier_size):barrier_size_(barrier_size)
    {
    	//
    }
    
    void wait()
    {
        std::unique_lock <std::mutex> unique(mutex_);         
         if(--barrier_size_ <=0 )
         {
             cond_.notify_all();
         }
         
         while(barrier_size_ > 0)
         {
         	cond_.wait(unique);
         }
    }

private:
    int32_t barrier_size_ = 0;
    std::mutex mutex_;
    std::condition_variable cond_;
};

Barrier barrier(4);

int32_t  f_cpp(int32_t index)
{    
    //模拟在干其他事情
    for(int32_t i = 0;i< 3;++i)
    {    	
    	std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    /* 等待屏障 */
    barrier.wait();
    
    std::stringstream ss;
    ss<<"thread id: "<<std::this_thread::get_id()<<",index: "<<index<<" wait后返回"<<endl;
    std::cout<<ss.str();
    return ++index;
}

int main(int argc,char * argv[])
{    
    const int32_t NUM = 3;
    
    std::packaged_task<int32_t (int32_t)> task[NUM];    
    std::thread ts[NUM];
    std::future<int32_t> future_result[NUM]; 
    std::thread::id  tid[NUM];
    for(int16_t i = 0; i < NUM;++i)
    {       
       task[i] = std::packaged_task<int32_t (int32_t)>(f_cpp);
       future_result[i] = task[i].get_future();
       ts[i] = std::thread(std::move(task[i]), i);
       tid[i] = ts[i].get_id();
    }
    
    //主线程模拟干点其他事情再等待一起出发
    for(int32_t i = 0;i< 3;++i)
    {
    	std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cout<<("main thread loop over\n");
        
    /* 主线程干完事情后,等待其他线程工作完成后，一起再继续执行下面流程 */
    barrier.wait();
        
    std::cout<<("所有线程工作已完成...\n");     
    
    //阻塞获取线程的退出码
    //std::for_each(begin(ts),end(ts),std::mem_fn(&std::thread::join));
    for(int16_t i = 0;i < NUM;++i)
    {
        ts[i].join();
        std::cout<<"thread: "<<tid[i]<<",exitcode: "<<future_result[i].get()<<endl;
    }
         
    return 0;
}



