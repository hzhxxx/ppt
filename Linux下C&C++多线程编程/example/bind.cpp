
//todo
/*

[root@Slave02 thread]# g++ bind.cpp  -o bind -g -Wall -std=gnu++11 -lpthread;./bind
class void Test::print(const std::string& str,int32_t i) 对比测试 std::mem_fn 和 std::bind,并调用类重载函数,1
class void Test::print(const std::string& str,int32_t i) 对比测试 std::mem_fn 和 std::bind,并调用类重载函数,1
class void Test::print(const std::string& str,int32_t i) 对比测试 std::mem_fn 和 std::bind,并调用类重载函数,1

class void Test::print() no parameter,调用无参数函数
class void Test::print() no parameter,调用无参数函数
class void Test::print() no parameter,调用无参数函数

class void Test::print() no parameter,调用无参数函数
class void Test::print() no parameter,调用无参数函数
class void Test::print() no parameter,调用无参数函数

*/

#ifndef _BIND_INCLUDE_H
#define _BIND_INCLUDE_H

#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <functional>
#endif

using namespace std;
using namespace placeholders;

class Test
{
  public:
    void print(const std::string& str,int32_t i)
    {
    	std::stringstream ss;
        ss << "class void Test::print(const std::string& str,int32_t i) " << str << "," << i << "\n";
        std::cout << ss.str();
    }
    
    void print()
    {
        std::stringstream ss;
        ss << "class void Test::print() no parameter,调用无参数函数" << "\n";
        std::cout << ss.str();
    }
};

int32_t main(int32_t argc, char *argv[])
{    
    Test t1,t2,t3;
    std::vector<Test *> vv;
    vv.push_back(&t1);
    vv.push_back(&t2);
    vv.push_back(&t3);
    
    std::string s = "对比测试 std::mem_fn 和 std::bind,并调用类重载函数";
    int32_t i = 0;
    /*******************************************************
    1. i 输出一直是1,此处证明只构造了一次函数对象(bind),后续多次通过调用 bind.operator()(Test *)函数
    2. 对于重载的成员函数,需要强制指定调用重载的那个版本
    */
    std::for_each(vv.begin(),vv.end(), std::bind( (void (Test::*)(const std::string &, int32_t)) &Test::print,_1,s,++i) );
    std::cout<<endl;
    std::for_each(vv.begin(),vv.end(), std::bind( (void (Test::*)()) &Test::print,_1));
    std::cout<<endl;
    /* 
    3. 对于无参数构造调用的函数,可以使用 std::mem_fn,减少占位符
    */
    std::for_each(vv.begin(),vv.end(), std::mem_fn( (void (Test::*)()) &Test::print));
        
    return 0;
}

