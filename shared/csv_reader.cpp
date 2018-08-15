
/*
 * $Copyright$
 * Copyright (c) 2016 All Rights Reserved, Sean Harre
 *
 * Sean Harre is the copyright holder of all code below.
 * Do not re-use without permission.
 */

#include "csv_reader.hpp"
#include <iostream>
#include <sstream>
#include <cstring>  // strlen
#include <cstdio>
#include <sys/stat.h>
#include <iostream>		// included twice??

//--------------------------------------------------------------
csv_reader::csv_reader(string str) : m_csv_cfg_filename(str) {
    parse_csv_file();
}
csv_reader::~csv_reader() {}

//--------------------------------------------------------------
bool csv_reader::is_header_line(string str) {
    
    // if first char in line is '#', this is header line
    return (str.at(0) == '#');
}

//--------------------------------------------------------------
// from: http://www.cplusplus.com/forum/general/23392/
bool csv_reader::does_file_exist()
{
    //The variable that holds the file information
    struct stat fileAtt; //the type stat and function stat have exactly the same names, so to refer the type, we put struct before it to indicate it is an structure.
    
    //Use the stat function to get the information
    if (stat(m_csv_cfg_filename.c_str(), &fileAtt) != 0) //start will be 0 when it succeeds
        return false; // throw errno; //So on non-zero, throw an exception
    
    //S_ISREG is a macro to check if the filepath referers to a file. If you don't know what a macro is, it's ok, you can use S_ISREG as any other function, it 'returns' a bool.
#ifdef _WIN32	
#define S_ISREG(m)      (((m) & S_IFMT) == S_IFREG)
#endif	
	return S_ISREG(fileAtt.st_mode);
}

//--------------------------------------------------------------
bool csv_reader::parse_csv_file() {
    
    FILE* file = fopen(m_csv_cfg_filename.c_str(),"r");
    if (!file) {
        fprintf(stderr, "WARNING: csv_reader::parse_csv_file(%s) could not open input cfg file\n",m_csv_cfg_filename.c_str());
        return false;
    }
    
    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        string str = string(line);
        
        // make sure works with csv files with \r\n (from windows),
        // and skip over header rows
        if (!str.empty() && str[str.size() - 1] == '\r')
            str.erase(str.size() - 1);
        if (is_header_line(str)) continue;
        
        string key, val;
        stringstream ss(str);
        if (!getline(ss, key, ',') ||
            !getline(ss, val, ',')) {
            fprintf(stderr, "ERROR: csv_reader::parse_csv_file(%s) had error parsing csv line:%s\n",m_csv_cfg_filename.c_str(),line);
            return false;
        }
        (void)strip_string_ending_cr_lf(&val, strlen(val.c_str()));
        m_map[key] = val;
    }
    
    if (fclose(file) > 0) return false;
    return true;
}

//--------------------------------------------------------------
bool csv_reader::write_out_hash_to_file(unordered_map<string, string> new_map) {
    
    FILE* file = fopen(m_csv_cfg_filename.c_str(),"w");
    if (!file) {
        fprintf(stderr, "ERROR: csv_reader::write_out_hash_to_file(%s) could not open input cfg file!\n",m_csv_cfg_filename.c_str());
        return false;
    }
    
    char buf[1024];
    snprintf(buf, 1024, "# key,value\n");
    if (fwrite(buf, sizeof(char), strlen(buf), file) == 0) {
        fclose(file);
        return false;
    }
    for (unordered_map<string,string>::iterator it = new_map.begin(); it != new_map.end(); ++it) {
        snprintf(buf, 1024, "%s,%s\n",it->first.c_str(),it->second.c_str());
        if (fwrite(buf, sizeof(char), strlen(buf), file) == 0) {
            fclose(file);
            return false;
        }
    }
    
    if (fclose(file) > 0) return false;
    return true;
}

//--------------------------------------------------------------
size_t csv_reader::strip_string_ending_cr_lf(string* str, size_t size) {
    
    // remove any LN or CR, adjusting return size value
    while (size >= 1) {
        if ((str->at(size-1) == '\r') || (str->at(size-1) == '\n')) {
            str->erase(size-1);
            --size;
        } else {
            break;
        }
    }
    return size;
}

//--------------------------------------------------------------
bool csv_reader::print_csv_file_contents() {
    
    FILE* file = fopen(m_csv_cfg_filename.c_str(),"r");
    if (!file) {
        fprintf(stderr, "WARNING: csv_reader::parse_csv_file(%s) could not open input cfg file\n",m_csv_cfg_filename.c_str());
        return false;
    }
    
    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        string val = string(line);
        (void)strip_string_ending_cr_lf(&val, strlen(val.c_str()));
        printf("%s\n",val.c_str());
    }
    
    if (fclose(file) > 0) return false;
    return true;
}

//--------------------------------------------------------------
bool csv_reader::remove_csv_file() {
    
    return (!std::remove(m_csv_cfg_filename.c_str()));
}
