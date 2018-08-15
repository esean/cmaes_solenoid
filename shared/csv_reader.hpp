
/*
 * $Copyright$
 * Copyright (c) 2016 All Rights Reserved, Sean Harre
 *
 * Sean Harre is the copyright holder of all code below.
 * Do not re-use without permission.
 */

// TODO:
//  - add template: map<U,T>

#ifndef csv_reader_hpp
#define csv_reader_hpp

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS		// ignore fopen() security warnings  
#endif

#include <string>
#include <unordered_map>
#include "stdio.h"
#include <mutex>

using namespace std;

//==========================
//
// PERSISTENT STORE
//
//==========================

//
// Intent:
//
// Pass CSV config file to instantiate, where format is
//
//      this_val,3
//      that_val,another
//      # any comment lines are ignored
//      another,3_to_3
//
// Then calling code can query like,
//
//      csv_reader csv_cfg("config.csv");
//      unordered_map<string, string> my_cfg = csv_cfg.get_hash();
//      cout << "File location is " << my_cfg["file_location"] << endl;
//      csv_cfg.add_key_value_pair["version"] = "v3.4";
//      csv_cfg.remove_key["beta"];
//
// where 'config.csv' contains many lines, one containing
//
//      file_location,foo/bar/file.txt
//

class csv_reader {
public:
    csv_reader(string csv_cfg_filename);
    ~csv_reader();
    
    // get current csv config in a map[]
    unordered_map<string, string> get_hash() { return m_map; }
    
    void add_key_value_pair(string key, string val) {
        std::lock_guard<std::mutex> lock( m_access_mutex );
        m_map[key] = val;
        write_out_hash_to_file(m_map);
    }
    void remove_key(string key) {
        std::lock_guard<std::mutex> lock( m_access_mutex );
        if (m_map.count(key) == 0) {
            fprintf(stderr,"WARNING: csv_reader::remove_key(%s) key does not exist\n",key.c_str());
            return;
        }
        m_map.erase(key);
        write_out_hash_to_file(m_map);
    }
    
    bool does_key_exist(string key) {
        std::lock_guard<std::mutex> lock( m_access_mutex );
        return (m_map.count(key) > 0);
    }
    string get_key_value(string key) {
        std::lock_guard<std::mutex> lock( m_access_mutex );
        return m_map[key];
    }
    
    bool print_csv_file_contents();
    bool remove_csv_file();
    bool does_file_exist();
    
private:
    bool parse_csv_file();
    bool is_header_line(string str);
    bool write_out_hash_to_file(unordered_map<string, string> new_map);
    size_t strip_string_ending_cr_lf(string* str, size_t size);
    
    string m_csv_cfg_filename;
    unordered_map<string, string> m_map;
    std::mutex m_access_mutex;
    
};

#endif /* csv_reader_hpp */
