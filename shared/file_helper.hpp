
/*
 * $Copyright$
 * Copyright (c) 2016 All Rights Reserved, Sean Harre
 *
 * Sean Harre is the copyright holder of all code below.
 * Do not re-use without permission.
 */

#ifndef file_helper_hpp
#define file_helper_hpp

#include "target.h"
#include <cstddef>

namespace file_helper {
    
    bool file_helper_get_file_contents(const char* location, char* contents, size_t* written = nullptr);
    bool file_helper_store_contents_to_file(const char* location, const char* contents, const size_t size);
    size_t file_helper_strip_ending_cr_lf(char* str, size_t size);
    
}

#endif /* file_helper_hpp */
