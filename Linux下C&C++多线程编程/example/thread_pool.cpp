

//todo
/*
[root@Slave02 thread]# g++ thread_pool.cpp  -o thread_pool -g -Wall -std=gnu++11 -lpthread;./thread_pool
Hello ThreadPool
class Test2 Hello world,0
Current thread id: 140287557768960, global function test_task str: Hello world
class Test Hello world,0
第一批 4 个任务完成,wait_limit_task exit
class Test Hello world,1
class Test Hello world,0
class Test Hello world,2
第二批 3 个任务完成,wait_limit_task exit
class Test2 Hello world,2
class Test2 Hello world,3
class Test2 Hello world,4
class Test2 Hello world,5
class Test2 Hello world,0
class Test2 Hello world,1
第三批 6 个任务完成,wait_limit_task exit
##############END#################
*/


#ifndef _THREAD_POOL_INCLUDE_H
#define _THREAD_POOL_INCLUDE_H

#include <cassert>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <vector>
#include <queue>
#include <string>
#include <algorithm>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <type_traits>

using namespace std;

class CThread_Pool
{
private:
    using work_thread_ptr_t = std::shared_ptr<std::thread>;
    using task_t = std::function<void ()>;	
public:
    ~CThread_Pool()
    {
        stop();
    }

    //初始化指定线程池内线程数
    void init_thread_num(uint32_t num)
    {
        is_stop_threadpool_.store(false);
        limit_task_num_.store(0);
        if (num <= 0 || num > MAX_THREAD_SIZE)
        {
            std::string str = "Number of threads in the range of 1 to " + std::to_string(MAX_THREAD_SIZE);
            throw std::invalid_argument(str);
        }

        for (uint32_t i = 0; i < num; ++i)
        {
            work_thread_ptr_t t = std::make_shared<std::thread>(std::bind(&CThread_Pool::run_task, this));
            thread_vec_.emplace_back(t);
        }
    }
    
    //等待指定执行的任务结束
    void wait_limit_task()
    {
        // 线程池循环取任务
        std::unique_lock<std::mutex> locker(limit_task_);
        while (!is_stop_threadpool_.load() && limit_task_num_.load() > 0)
        {
            limit_task_finish_.wait(locker);
        }
    }

    // 支持全局函数、静态函数、以及lambda表达式
    template <typename Function, typename... Args> 
    void add_task(const Function &func, const Args... args)
    {
        if (!is_stop_threadpool_.load())
        {
            task_t task = nullptr;
            // 用lambda表达式来保存函数地址和参数
            //task = [&func, args...] { return func(args...); };
            //使用 std::bind绑定
            task = std::bind(func, args...);
            add_task_impl(task);            
        }
    }

    //支持 非 const 对象
    template <typename Function, typename... Args> 
    typename std::enable_if<std::is_class<Function>::value>::type add_task(Function &func, Args... args)
    {
        if (!is_stop_threadpool_.load())
        {
            //task_t task = [&func, args...] { return func(args...); };
            task_t task = std::bind(func, args...);
            add_task_impl(task);
            //std::cout << "void add_task(Function &func, Args... args)" << endl;
        }        
    }

    // 支持类成员函数
    template <typename Function, typename Self, typename... Args>
    void add_task(const Function &func, Self *self, Args... args)
    {
        assert(self);
        
        if (!is_stop_threadpool_.load())
        {
            //task_t task = [&func, &self, args...] { return (*self.*func)(args...); };
            task_t task = std::bind(func,self,args...);
            add_task_impl(task);
            //std::cout << "void add_task(const Function &func, Self *self, Args... args)" << endl;
        }
    }

    void stop()
    {
        // 保证terminate_all函数只被调用一次
        std::call_once(call_flag_, [this] { terminate_all(); });
    }

  private:
    void add_task_impl(const task_t &task)
    {
        {
            // 任务队列满了将等待线程池消费任务队列
            std::unique_lock<std::mutex> locker(task_queue_mutex_);
            while (!is_stop_threadpool_.load() && task_queue_.size() == MAX_TASK_QUQUE_SIZE)
            {
                task_put_.wait(locker);
            }
            task_queue_.emplace(std::move(task));
            limit_task_num_++;
        }

        // 向任务队列插入了一个任务并提示线程池可以来取任务了
        task_get_.notify_one();
    }

    void terminate_all()
    {
        is_stop_threadpool_.store(true);
        task_get_.notify_all();
        for (auto &iter : thread_vec_)
        {
            if (iter != nullptr)
            {
                if (iter->joinable())
                {
                    iter->join();
                }
            }
        }

        thread_vec_.clear();
        clean_task_queue();
    }

    void run_task()
    {
        // 线程池循环取任务
        while (!is_stop_threadpool_.load())
        {            
            task_t task = nullptr;
            {
                // 任务队列为空将等待,局部变量locker,确保做任务的时候已经解锁
                std::unique_lock<std::mutex> locker(task_queue_mutex_);
                while (task_queue_.empty() && !is_stop_threadpool_.load())
                {
                    task_get_.wait(locker);
                }

                if(!task_queue_.empty())
                {
                	task = std::move(task_queue_.front());
                	task_queue_.pop();                	
                }
            }

            if (task != nullptr)
            {
                try
                {
                	task();
                }
                catch(const std::exception &e)
                {
                	//忽略一般异常,避免线程退出
                }
                // 执行任务，并通知同步服务层可以向队列放任务了
                task_put_.notify_one();
            }
            
            //指定执行任务数后结束
            --limit_task_num_ ;
            if(limit_task_num_.load() == 0)
            {
            	limit_task_finish_.notify_one();
            }
        }
    }

    void clean_task_queue()
    {
        std::lock_guard<std::mutex> locker(task_queue_mutex_);
        while (!task_queue_.empty())
        {
            task_queue_.pop();
        }
    }
private:
    //线程池线程数组
    std::vector<work_thread_ptr_t> thread_vec_;

    //任务队列可入、可出信号和队列锁
    std::queue<task_t> task_queue_;
    std::condition_variable task_put_;
    std::condition_variable task_get_;
    std::mutex task_queue_mutex_;

    //确保函数只执行一次
    std::once_flag call_flag_;
    
    //线程池退出标记
    std::atomic<bool> is_stop_threadpool_;
    static const uint32_t MAX_TASK_QUQUE_SIZE;
    static const uint32_t MAX_THREAD_SIZE;

    //指定运行任务数目
    std::atomic<uint32_t> limit_task_num_;
    std::mutex limit_task_;
    std::condition_variable limit_task_finish_;
};

const uint32_t CThread_Pool::MAX_TASK_QUQUE_SIZE = 100000;
const uint32_t CThread_Pool::MAX_THREAD_SIZE = 256;

#endif

//下面为测试代码
//使用 GCC 5.4
///usr/local/gcc-5.4/bin/g++ thread_pool.cpp -g -o thread_pool -std=gnu++11 -lpthread /usr/local/gcc-5.4/lib64/libstdc++.a

#include <sstream>
#include <iostream>
#include <string.h>
#include <chrono>

void test_task(const std::string &str)
{
    std::stringstream ss;
    ss << "Current thread id: " << std::this_thread::get_id() << ", global function test_task str: " << str << "\n";
    std::cout << ss.str();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

class Test
{
  public:
    void print(const std::string &str, int32_t i)
    {
        std::stringstream ss;
        ss << "class Test " << str << "," << i << "\n";
        std::cout << ss.str();
        //std::cout << "Test: " << str << ", i: " << i << std::endl;
        //std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }
};

class Test2
{
  public:
    void operator()(const std::string &str, int32_t i)
    {
        std::stringstream ss;
        ss << "class Test2 " << str << "," << i << "\n";
        std::cout << ss.str();
        //std::cout << "Test2: " << str << ", i: " << i << std::endl;
        //std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }
};

using namespace std::placeholders;
	
	
template<typename _MemberPointer> class _Mem_fn_1;
template<typename _Tp, typename _Class> _Mem_fn_1<_Tp _Class::*> mem_fn_1(_Tp _Class::*) noexcept;	

int32_t main(int32_t argc, char *argv[])
{    
    CThread_Pool pool;
    // 预先启动指定个数线程
    pool.init_thread_num(4);
    
    Test t;
    Test2 t2;
    uint32_t task_num = 0;
    std::string str = "Hello world";
    for (int32_t i = 0; i < 1; ++i)
    {
        // 支持lambda表达式
        pool.add_task([] {
            std::stringstream ss;
            ss << "Hello ThreadPool"<< "\n";
            std::cout << ss.str();
            //std::cout << "Hello ThreadPool"<<endl;
            //std::this_thread::sleep_for(std::chrono::milliseconds(150));
        });
        task_num++;

        // 支持全局函数
        pool.add_task(test_task, str);
        task_num++;

        // 支持函数对象
        pool.add_task(t2, str, i);
        task_num++;

        // 支持类成员函数
        auto f = std::bind(&Test::print, &t, _1, _2);
        pool.add_task(f, str, i);
        task_num++;
        //pool.add_task(&Test::print, &t, str, i);
    }
    pool.wait_limit_task();
    //std::this_thread::sleep_for(std::chrono::milliseconds(150));
    std::cout << "第一批 "<<task_num<<" 个任务完成,wait_limit_task exit" << endl;

    task_num = 3;
    for (uint32_t i = 0; i < task_num; ++i)
    {
        // 支持类成员函数
        pool.add_task(&Test::print, &t, str, i);
    }    
    pool.wait_limit_task();
    std::cout << "第二批 "<<task_num<<" 个任务完成,wait_limit_task exit" << endl;

    task_num = 6;
    for (uint32_t i = 0; i < task_num; ++i)
    {
        // 支持函数对象
        pool.add_task(t2, str, i);
    }
    pool.wait_limit_task();
    std::cout << "第三批 "<<task_num<<" 个任务完成,wait_limit_task exit" << endl;
    
    //停止线程池
    pool.stop();
    std::cout << "##############END#################" << std::endl;
    return 0;
}

