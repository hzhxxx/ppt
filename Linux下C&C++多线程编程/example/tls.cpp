

//todo
/*

[root@Slave02 thread]# g++ tls.cpp  -o tls -g -Wall -std=gnu++11 -lpthread;./tls 
thread :140643771881216,C++11 thread_local i = 3,arg = 3
thread :140643670161152,C++11 thread_local i = 4,arg = 4
thread :140643797059328,C++11 thread_local i = 0,arg = 0
thread :140643788666624,C++11 thread_local i = 1,arg = 1
thread :140643780273920,C++11 thread_local i = 2,arg = 2
thread 140643670161152,glibc name Thread_Name_0,GCC __thread i = 0,arg = 0
thread 140643771881216,glibc name Thread_Name_1,GCC __thread i = 1,arg = 1
thread 140643780273920,glibc name Thread_Name_2,GCC __thread i = 2,arg = 2
thread 140643788666624,glibc name Thread_Name_3,GCC __thread i = 3,arg = 3
thread 140643797059328,glibc name Thread_Name_4,GCC __thread i = 4,arg = 4

*/


#include <cstdio>
#include <thread>
#include <algorithm>
#include <functional>
#include <iostream>
#include <sstream>
#include <atomic>
#include <chrono>
using namespace std;

void f_cpp_tls(int16_t arg);
void* f_key_c(void *arg);

int main(int argc,char * argv[])
{
    const int16_t NUM = 5;
    std::thread ts[NUM];
    for(int16_t i = 0; i < NUM;++i)
    {
        ts[i] = std::thread(f_cpp_tls,i);        
    }
    //join waits for the thread specified by thread to terminate
    std::for_each(begin(ts),end(ts),std::mem_fn(&std::thread::join));
    	
    for(int16_t i = 0; i < NUM;++i)
    {
        int16_t * p = (int16_t*)malloc(sizeof(int16_t));
        *p = i;
        ts[i] = std::thread(f_key_c,(void *)p);
    }
    //join waits for the thread specified by thread to terminate
    std::for_each(begin(ts),end(ts),std::mem_fn(&std::thread::join));    	
    
    return 0;
}

void f_cpp_tls(int16_t arg)
{
	static thread_local int32_t i = 0;
	for(int16_t k = 0;k < arg;++k)
	{
		i++;
	}
	std::stringstream ss;	
	ss<<"thread "<<std::this_thread::get_id()<<",C++11 thread_local i = "<<i<<",arg = "<<arg<<endl;
	std::cout<<ss.str();	
}

static pthread_key_t key;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;
static __thread int32_t g_i = 0;

static void  create_key()
{	
	(void) pthread_key_create(&key, NULL);
}

static void delete_key()
{	
	(void) pthread_key_delete(key);
}

void* f_key_c(void *arg)
{
    int16_t index = *(int16_t*)arg;
    free(arg); 
    
    void *ptr = 0;
    (void) pthread_once(&key_once, create_key);
    if ((ptr = pthread_getspecific(key)) == NULL) 
    {
       	    const int32_t NAME_LEN = 64;
       	    ptr = malloc(NAME_LEN);       	    
       	    snprintf((char *)ptr,NAME_LEN,"Thread_Name_%d",index);
       	    (void) pthread_setspecific(key, ptr);
    }
    
    for(int16_t k = 0;k < index;++k)
    {
    	g_i++;
    }
    
    printf("thread %ld,glibc name %s,GCC __thread g_i = %d,arg = %d\n",pthread_self(),(const char *)pthread_getspecific(key),g_i,index);   
    
    free(ptr);
    (void) pthread_once(&key_once, delete_key);
        
    return 0;
}




