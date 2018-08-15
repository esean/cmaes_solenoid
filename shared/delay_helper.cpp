
/*
 * $Copyright$
 * Copyright (c) 2016 All Rights Reserved, Sean Harre
 *
 * Sean Harre is the copyright holder of all code below.
 * Do not re-use without permission.
 */

#include "delay_helper.hpp"

#ifdef _WIN32
 #include <windows.h>
#else
 #include "unistd.h"
#endif

void delay_ms(unsigned int millisec) {
#ifdef _WIN32
	::Sleep(millisec);
#else
	unsigned int dly = millisec * 1000;
	::usleep(dly);
#endif
}