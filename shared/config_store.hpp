
/*
 * $Copyright$
 * Copyright (c) 2016 All Rights Reserved, Sean Harre
 *
 * Sean Harre is the copyright holder of all code below.
 * Do not re-use without permission.
 */

#ifndef config_store_hpp
#define config_store_hpp

#include "target.h"
#include "singleton.hpp"
#include <assert.h>
#include <string>
#include <map>
#include <mutex>

using namespace std;

#ifdef _WIN32
#pragma warning( disable : 4715 )		// not all control paths in getBoolKey() return a value
#endif

//==========================
//
// VOLATILE STORE
//
//==========================

class config {
public:
    
    //
    // mutex around any access to m_map
    string getVal(string s) {
        std::lock_guard<std::mutex> lock( m_access_mutex );
        return m_map[s];
    }
    void setKeyVal(string s, string v) {
        std::lock_guard<std::mutex> lock( m_access_mutex );
        m_map[s] = v;
    }
    bool delKey(string s) {
        std::lock_guard<std::mutex> lock( m_access_mutex );
        return m_map.erase(s);
    }
    // ----
    //
    
    bool getBoolKey(string s) {
        string v = getVal(s);
        if (v == "false")
            return false;
        else if (v == "true")
            return true;
        else
            assert(0);
    }
    
    int getIntKey(string s) {
        return atoi(getVal(s).c_str());
    }
    void setIntKey(string s, int v) {
        char str[80];
        snprintf(str, 80, "%d",v);
        setKeyVal(s, string(str));
    }
    
    // debug
    void dump_config() {
        std::lock_guard<std::mutex> lock( m_access_mutex );
        for (map<string,string>::iterator it = m_map.begin(); it != m_map.end(); ++it)
            printf("config [%s]=[%s]\n",it->first.c_str(),it->second.c_str());
    }
private:
    map<string,string> m_map;
    std::mutex m_access_mutex;
};

typedef singleton<config> Config;


#endif /* config_store_hpp */
