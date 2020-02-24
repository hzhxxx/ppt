

//todo
//g++ -std=gnu++11 -g create_cpp.cpp -o create_cpp -lpthread;./create_cpp
//[root@slave02 thread]# g++ -Wall -std=gnu++11 create_cpp.cpp -o create_cpp -lpthread;./create_cpp
//注意看顺序,index:0,array:[1],thread id:140592559949568
//注意看顺序,index:3,array:[3],thread id:140592543164160
//注意看顺序,index:1,array:[0],thread id:140592568342272
//注意看顺序,index:4,array:[4],thread id:140592534771456
//注意看顺序,index:2,array:[2],thread id:140592551556864

#include <thread>
#include <algorithm>  
#include <functional>
#include <iostream>
#include <sstream>
#include <atomic>
#include <chrono>
using namespace std;


void f_cpp(int16_t i);

int main(int argc,char * argv[])
{
    const int16_t NUM = 5;
    std::thread ts[NUM];
    for(int16_t i = 0; i < NUM;++i)
    {
        ts[i] = std::thread(f_cpp,i);
        //ts[i] = std::thread(std::bind(f_cpp,i));
    }
    //join waits for the thread specified by thread to terminate
    std::for_each(begin(ts),end(ts),std::mem_fn(&std::thread::join));
    return 0;
}

void f_cpp(int16_t i)
{
    std::stringstream ss;
    static std::atomic<int32_t> index(0);
    ss<<"注意看顺序,index:"<<index++<<",array:["<<i<<"],thread id:"<<std::this_thread::get_id()<<endl;
    std::cout<<ss.str();
    std::this_thread::sleep_for(std::chrono::seconds(10));
    return;
}




