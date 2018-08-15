
/*
 * $Copyright$
 * Copyright (c) 2016 All Rights Reserved, Sean Harre
 *
 * Sean Harre is the copyright holder of all code below.
 * Do not re-use without permission.
 */

#ifndef singleton_hpp
#define singleton_hpp

#include <stddef.h>
#include <assert.h>

using namespace std;

//----------------------------
// Generic singleton template
//
// NOTE: following #def controls if the
// singleton uses dynamic memory 'new'.
//
//#define SINGLETON_USE_DYNAMIC_MEMORY
//

template<class T>
class singleton {
public:
    static T* Instance() {
#ifdef SINGLETON_USE_DYNAMIC_MEMORY
#ifdef _WIN32
 #pragma message ("SINGLETON: using new")
#else
 #warning "SINGLETON: using new"
#endif
        if (m_inst == NULL) m_inst = new T();
        assert(m_inst != NULL);
        return m_inst;
#else 
//#ifdef _WIN32
// #pragma message ("SINGLETON: not using new")
//#else
// #warning "SINGLETON: not using new"
//#endif
        static T inst;
        return &inst;
#endif
    }
private:
#ifdef SINGLETON_USE_DYNAMIC_MEMORY
    static T* m_inst;
#endif
    singleton();
#ifdef SINGLETON_USE_DYNAMIC_MEMORY
    ~singleton() {
        delete m_inst;
    }
#else
    ~singleton();
#endif
    singleton(singleton const&);
    singleton& operator=(singleton const&);
};
#ifdef SINGLETON_USE_DYNAMIC_MEMORY
template<class T> T* singleton<T>::m_inst = NULL;
#endif
//----------------------------


#endif /* singleton_hpp */
