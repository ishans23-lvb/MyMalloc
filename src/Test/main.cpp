//
//  main.cpp
//  MyMalloc
//
//  Created by Ishan Shah on 12/14/19.
//  Copyright © 2019 Ishan Shah. All rights reserved.
//

#include <iostream>
#include <vector>
#include <chrono>
#include <stdio.h>
#include <sys/_types/_off_t.h>
#include "../MyMalloc/MyMalloc.h"
#include "../MyMalloc/efl.h"

#define LS "\n\n\n\n\n\n"

using namespace std;

class T
{
private:
    std::chrono::time_point<std::chrono::steady_clock> s, e;
    
public:
    T(void)
    :s(std::chrono::high_resolution_clock::now())
    {}
 
    ~T(void)
    {
        e=std::chrono::high_resolution_clock::now();
        std::chrono::duration<long double> d=e-s;
        std::cout << d.count() << "s" << std::endl;
    }
};

bool test1(void);
bool test2(void*(*test_malloc)(size_t)=my_malloc,void(*test_free)(void*)=my_free,int allocs=1000,size_t max_size=80,size_t min_size=1,bool print=false);
bool test3(int oops,size_t max_size=80,size_t min_size=1,bool print=false);

int main(int argc, const char * argv[])
{
    cout << typeid(off_t).name() << endl;
    cout << typeid(long long).name() << endl;
    cout << sizeof(off_t) << endl;
    return 0;
}

bool test1(void)
{
    my_malloc(10);
    print_pages();
    efl_print();
    puts(LS);
    my_malloc(10);
    print_pages();
    
    return true;
}
bool test2(void*(*test_malloc)(size_t),void(*test_free)(void*),int allocs,size_t max_size,size_t min_size,bool print)
{
    assert(max_size>=min_size);
    vector<pair<void*,size_t>> v;
    srand(0);
    T O;
    for(int i=0;i<allocs;i++)
    {
        size_t size=rand()%(max_size-min_size)+min_size;
        v.push_back({test_malloc(size),size});
        if(print)
            cout << i << ". Alloc: (" << v.back().first << ", " << v.back().second << ")" << endl;
    }
    if(print)
        cout << endl << endl;
    for(vector<pair<void*,size_t>>::iterator it=v.begin();it!=v.end();it++)
    {
        test_free(it->first);
        if(print)
            cout << "Free: (" << it->first << ", " << it->second << ")" << endl;
        it->first=NULL;it->second=0;
    }
    
    cout << "Time: ";
    return true;
}

bool test3(int oops,size_t max_size,size_t min_size,bool print)
{
    assert(max_size>=min_size);
    vector<pair<void*,size_t>> v;
    
    for(int i=0;i<oops;i++)
    {
        bool alloc= v.size()>0 ? rand()%2 : true;
        if(alloc)
        {
            size_t size=rand()%(max_size-min_size)+min_size;
            v.push_back({my_malloc(size),size});
            memset(v.back().first,'~',v.back().second);
        }
        else
        {
            size_t j=rand()%v.size();
            
            my_free(v[j].first);
            v[j].first=NULL;
            v[j].second=0;
        }
    }
    
    for(vector<pair<void*,size_t>>::iterator it=v.begin();it!=v.end();it++)
    {
        if(it->first)
        {
            my_free(it->first);
        }
    }
    
    return true;
}
