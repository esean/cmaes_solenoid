/*
 * $Copyright$
 * Copyright (c) 2018 All Rights Reserved, Sean Harre
 *
 * Sean Harre is the copyright holder of all code below.
 * Do not re-use without permission.
 */
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>
#include <cstdlib>
#include <iostream>
#include "canakit_common.hpp"
#include <chrono>
#include <algorithm>
#include <string>
#include <signal.h>
#include <sys/stat.h>
#include <sstream>

#define DEBUG
#define VERBOSE
#define CFG_USE_HW_PWM

#ifdef CFG_USE_HW_PWM
#define MAX_RANGE		4095
#define MIN_RANGE		2.0
#define MAX_DUTY		4095
#define MIN_DUTY		2.0 
#else
#define MAX_RANGE		0.3
#define MIN_RANGE		0.001
#define MAX_DUTY		0.3
#define MIN_DUTY		0.001
#endif

//---------------------------------------------------------
void usage(const int argc, const char ** argv) {
    printf("\nUSAGE: %s [pwm_time(%lf)] [pwm_duty(%lf)] [measurement_time_sec]\n",argv[0],MAX_RANGE/2.0,MAX_DUTY/2.0);
}

//---------------------------------------------------------
void set_PWM_state(const bool is_on) {
    #ifdef VERBOSE
    printf("[PWM] set_state[%d]\n",is_on);
    #endif
    std::ostringstream ss;
    // TODO: set output low when turning off
#ifdef CFG_USE_HW_PWM
    if (is_on)
        ss << "gpio mode 1 pwm; gpio pwm-ms; gpio pwmc 4095";
    else
        ss << "gpio pwm 1 0";
#else
    if (is_on)
        ss << "gpio -g mode 18 out";
    else
        ss << "gpio -g mode 18 in";
#endif
    (void)popen_cmd(ss.str().c_str());
}
void set_PWM_range_duty(const double pwm_range, const double pwm_duty, const int gpio_state=0) {
    #ifdef VERBOSE
    printf("[PWM] set_range_duty[%lf,%lf]\n",pwm_range,pwm_duty);
    #endif
    std::ostringstream ss;
#ifdef CFG_USE_HW_PWM
    ss << "gpio pwmr " << (int)pwm_range << "; gpio pwm 1 " << (int)pwm_duty;
#else
    ss << "gpio -g write 18 " << gpio_state;
#endif
    (void)popen_cmd(ss.str().c_str());
}

//---------------------------------------------------------
//---------------------------------------------------------
//---------------------------------------------------------
// signal trap put IO back low
void sig_handler(const int s) {
    set_PWM_state(false);
    exit(1);
}
int main(const int argc, const char ** argv) { 
    if (argc <2) {
        usage(argc,argv);
        return -1;
    }
    double pwm_range = atof(argv[1]);
    double pwm_duty = atof(argv[2]);
    double meas_time = atof(argv[3]);
    if ((pwm_range<0) || (pwm_range>MAX_RANGE) || (pwm_duty<0) || (pwm_duty>MAX_DUTY) || (meas_time<0)) {
        printf("ERROR: params incorrect\n");
        usage(argc,argv);
        return -1;
    }
    
    signal(SIGINT,sig_handler);
    
    // enable PWM
    set_PWM_state(true);
    set_PWM_range_duty(pwm_range,pwm_duty);

    usleep(meas_time*1e+6);

    set_PWM_state(false);
    return 0;
}

