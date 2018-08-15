
/*
 * $Copyright$
 * Copyright (c) 2016 All Rights Reserved, Sean Harre
 *
 * Sean Harre is the copyright holder of all code below.
 * Do not re-use without permission.
 */

#include "file_helper.hpp"
#include "stdio.h"

//--------------------------------------------------------------
size_t file_helper::file_helper_strip_ending_cr_lf(char* str, size_t size) {
    
    // remove any LN or CR, adjusting return size value
    while (size >= 1) {
        if ((str[size-1] == '\r') || (str[size-1] == '\n')) {
            str[size-1] = 0;
            --size;
        } else {
            break;
        }
    }
    return size;
}

//--------------------------------------------------------------
bool file_helper::file_helper_get_file_contents(const char* location, char* contents, size_t* written) {
    FILE* hFile;
    size_t lSize;
    
    if (!(hFile = fopen(location,"r"))) return false;
    
    // obtain file size:
    fseek (hFile , 0 , SEEK_END);
    lSize = ftell (hFile);
    rewind (hFile);
    
    if (fread(contents, sizeof(char), lSize, hFile) != lSize) return false;
    if (fclose(hFile) > 0) return false;
    
    // terminate string
    contents[lSize] = 0;
    
    // remove any LN or CR, adjusting return size value
    size_t size = file_helper_strip_ending_cr_lf(contents,lSize);
    if (written) *written = size;
    
    return true;
}

//--------------------------------------------------------------
bool file_helper::file_helper_store_contents_to_file(const char* location, const char* contents, const size_t size) {
    FILE* hFile;
    
    if (!(hFile = fopen(location,"w"))) return false;
    if (fwrite(contents, sizeof(char), size, hFile) == 0) {
        fclose(hFile);
        return false;
    }
    if (fclose(hFile) > 0) return false;
    
    return true;
}


