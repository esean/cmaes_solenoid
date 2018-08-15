/*
 * $Copyright$
 * Copyright (c) 2018 All Rights Reserved, Sean Harre
 *
 * Sean Harre is the copyright holder of all code below.
 * Do not re-use without permission.
 */

// TODO:
//  - scale inputs so between 0 and 1.0
// 	- need some simple check to see if BNO exists, .open() hangs occasionally
// 	- alternate for popen_cmd() w/ pipe? faster? #include <wiringPi.h> and write direct
// 	- add progress callbacks, can also early terminate if search fval stays under some threshold

// out there:
// add fsr adc reading
// 	compare fps single vs continuous modes, should be faster continuous
// add handle resistance sensor so as user squeezes more, it reduces the goal from max (no hold) to min
// 	can cmaes allow a variable goal, being adjusted from min to max during run for instance? an in-between goal?
// 	probably need to go back to libcmaes ask-tell model (can do bounds?) to respond/adjust goal in real-time?

#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>
#include <cstdlib>
//#include "bno055_uart.hpp"
#include "bno_thread.hpp"
#include "cmaes.h"
#include <iostream>
#include "canakit_common.hpp"
#include <chrono>
#include <algorithm>
#include <string>
#include <signal.h>
#include <sys/stat.h>
#include <sstream>

using namespace libcmaes;

// global HACKs!! (will go away after refactor to cma class)
//bno055_uart Gm_dev("/dev/ttyAMA0",115200);
bool GbMaximizeFitness = true;
double GMAX_TIME = 0.1;				// sec, one pwm cycle should not exceed, set from cmd-line

#define DEBUG
//#define VERBOSE
#define CFG_USE_HW_PWM

#define MAX_EVALS_3d			50
#define MAX_EVALS_2d			(MAX_EVALS_3d * 2)

#define NUM_CMAES_DIM_3D		3   // problem dimensions: pwm_range,pwm_duty_cycle,alpha
#define NUM_CMAES_DIM_2D		2   // problem dimensions: pwm_range,pwm_duty_cycle

#define BNO055_UPDATE_HZ	100
#define BNO055_MEAS_SLP_US	int((1.0/(double)BNO055_UPDATE_HZ)*1000000.0/2.0)	// sleep slightly less

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

#define MAX_SIGMA		(MAX_RANGE/2.0)
#define MIN_SIGMA		0.001 //(MIN_RANGE/2.0)

// prototypes
double do_2dcmaes(std::vector<double>& x0, const int Ndim, const double sigma, const int max_fevals, const bool bMaximizeFitness);
double do_3dcmaes(std::vector<double>& x0, const int Ndim, const double sigma, const int max_fevals, const bool bMaximizeFitness);

//---------------------------------------------------------
void usage(const int argc, const char ** argv) {
    printf("\nUSAGE: %s {[pwm_time_sec(%lf)] [0|1]}\n",argv[0],GMAX_TIME);
    printf("  - 2nd param 0/1 is true/false - maximize_fitness(1) or minimize(0)\n\n");
}

//---------------------------------------------------------
// RETURNS: -1 on failure (-1*-1*-1 = -1)
double get_linacc_mag() { //bno055_uart *m_dev) {
    // get xyz LINACC measurement
    double x,y,z;
    //if (m_dev->get_linear_acceleration(&x,&y,&z) < 0) return -1.0;
    BNO_thread::Instance()->get_linear_acceleration(&x,&y,&z);
    usleep(BNO055_MEAS_SLP_US);
    // save time, don't need to sqrt()
    double magnitude_sq = x*x + y*y + z*z;
    return magnitude_sq;
}

// Called by CMA callback to record avg linacc seen over some 'ms' millisec
//double record_avg_mag_for_ms(bno055_uart *m_dev, double ms) {
double record_avg_mag_for_ms(double ms) {
    int meas_cnt = 0;
    double sum = 0.0;
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    do {
        // check if done, do at top so can't get stuck in loop
        std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() >= ms) break;
        // returns -1 on error
        double magnitude_sq = get_linacc_mag(); //m_dev);
        if (magnitude_sq>=0) {
            ++meas_cnt;
            sum += magnitude_sq;
        }
    } while (1);
    double avg_mag = sum/(double)meas_cnt;
    #ifdef VERBOSE
    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
    std::cout << " avg_mag " << avg_mag \
    << ", req_ms " << ms << ", meas_cnt " << meas_cnt \
    << ", RealDelta " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() \
    << std::endl;
    #endif
    return avg_mag;
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
// CMA-ES 3d callback:
// called for each ND population 'alpha' element i - x1,x2: x[0]x[1] and alpha: x[2]
FitFunc fsphere3d = [](const double *x, const int N)
{
    #ifdef VERBOSE
    printf("[CMAES3d] start 3d cb[%d]\n",N);
    #endif
    int i = 0;  // to replace forloop indexing
    
    // for each x1,x2,alpha, run CMA-ES(x1,x2) w/ alpha
    // problem dimensions: pwm_range,pwm_duty_cycle
    std::vector<double> x0_2d(NUM_CMAES_DIM_2D,x[i]);
    x0_2d[i] = x[i];
    x0_2d[i+1] = x[i+1];
    
    // run do_2dcmaes(x[0],x[1]) with alpha=x[2] and tol=(low)
    double ret = do_2dcmaes(x0_2d, N-1, x[i+2], MAX_EVALS_3d, GbMaximizeFitness);	// take -1 since creating 2D from 3D
    #ifdef DEBUG
    printf("# CMAES3d,%d,%lf,%lf,%lf,%lf\n",N,x[i],x[i+1],x[i+2],ret);
    #endif
    return ret;
};

//---------------------------------------------------------
// CMA-ES callback:
// called for each ND population element it's testing, ie, one of current covariance
// data points, where problem dimensions: pwm_range,pwm_duty_cycle 
FitFunc fsphere = [](const double *x, const int N)
{
    #ifdef VERBOSE
    printf("[CMAES2d] start cb[%d]\n",N);
    #endif
    int i = 0;  // to replace forloop indexing
#ifdef CFG_USE_HW_PWM
    set_PWM_range_duty(x[i],x[i+1]);
    //double ret = record_avg_mag_for_ms(&Gm_dev,GMAX_TIME*1000.0);
    double ret = record_avg_mag_for_ms(GMAX_TIME*1000.0);
    #ifdef DEBUG
    printf("# CMAES2d,%d,%lf,%lf,%lf,%lf\n",N,x[i],x[i+1],GMAX_TIME,ret);
    #endif
#else
    set_PWM_range_duty(x[i],x[i+1],1);
    //double ret = record_avg_mag_for_ms(&Gm_dev,x[i]*1000.0);
    double ret = record_avg_mag_for_ms(x[i]*1000.0);
    set_PWM_range_duty(x[i],x[i+1],0);
    //double ret2 = record_avg_mag_for_ms(&Gm_dev,x[i+1]*1000.0);
    double ret2 = record_avg_mag_for_ms(x[i+1]*1000.0);
    //ret = (ret+ret2)/2.0;
    ret = (ret2>0.0) ? ret/ret2 : 0.0;
    #ifdef DEBUG
    printf("# CMAES2d,%d,%lf,%lf,%lf\n",N,x[i],x[i+1],ret);
    #endif
#endif
    return ret;
};

//---------------------------------------------------------
// dummy genotype / phenotype transform functions.
TransFunc genof = [](const double *ext, double *in, const int &dim)
{
    for (int i=0;i<dim;i++) {
        in[i] = 2.0*ext[i];
        #ifdef VERBOSE
        std::cout << "#::::::: ++ ::::: TODO genof ext " << ext[i] << " in=" << in[i] << std::endl;
        #endif
    }
};
TransFunc phenof = [](const double *in, double *ext, const int &dim)
{
    // todo: does not consider dim
    // problem dimensions: pwm_range,pwm_duty_cycle, alpha
    double pwm_range = in[0];
    double pwm_duty = in[1];
    double alpha = -1.0;
    if (dim >2) alpha = in[2];
    if (pwm_range > MAX_RANGE)
        pwm_range = MAX_RANGE;
    else if (pwm_range < MIN_RANGE)
        pwm_range = MIN_RANGE;
    if (pwm_duty > MAX_DUTY)
        pwm_duty = MAX_DUTY;
    else if (pwm_duty < MIN_DUTY)
        pwm_duty = MIN_DUTY;
    ext[0] = pwm_range;
    ext[1] = pwm_duty;
    if (dim>2) {
		if (alpha > MAX_SIGMA)
			alpha = MAX_SIGMA;
		else if (alpha < MIN_SIGMA)
			alpha = MIN_SIGMA;
		ext[2] = alpha;
	}
    #ifdef DEBUG
    std::cout << "# phenof," << ext[0] << "," << ext[1];
    if (dim>2) {
		std::cout << "," << ext[2];
	}
	std::cout << std::endl;
    #endif
};

//---------------------------------------------------------
// Runs 2D CMA-ES with [x0,x1] and sigma, optimization results returned in x0 and f-value returned
// max_fevals: if lt.0, default, else specs the max number evaluations before terminating
double do_2dcmaes(std::vector<double>& x0, const int Ndim, const double sigma, const int max_fevals, const bool bMaximizeFitness) {
	// x1,x2: x[0]x[1] and alpha: x[2]
	GenoPheno<> gp(genof,phenof);
	CMAParameters<> cmaparams(x0,sigma,-1,0,gp);	// -1 = libcmaes chooses population size
	printf("2dCMA[N=%d]:max_fevals=%d sigma=%lf ",Ndim,max_fevals,sigma);
	for (int x=0; x<Ndim; ++x)
		printf("%d:%lf, ",x,x0[x]);
	printf("\n");
	cmaparams.set_maximize(bMaximizeFitness);
	if (max_fevals>0)
		cmaparams.set_max_fevals(max_fevals);
	cmaparams.set_algo(aCMAES);
	CMASolutions cmasols = cmaes<>(fsphere,cmaparams);	// CMAES optimization performed in this line
	// Converting solution back into the parameter space
	Eigen::VectorXd bestparameters = gp.pheno(cmasols.get_best_seen_candidate().get_x_dvec());
	// adjust x0[] to use best found candidates
	for (int i=0; i< Ndim; ++i) x0[i] = bestparameters[i];
	std::cout << "2d:Eigen::VectorXd bestparameters: " << bestparameters << std::endl;
	std::cout << "2d:status: " << cmasols.run_status() << "best solution: " << cmasols << std::endl;
	//std::cout << "2d:optimization took " << cmasols.elapsed_time() / 1000.0 << " seconds\n" << std::endl;
	std::cout << "2d:best_candidate::get_fvalue: " << cmasols.best_candidate().get_fvalue() << std::endl;
	//std::cout << "2d:get_best_seen_candidate::get_fvalue: " << cmasols.get_best_seen_candidate().get_fvalue() << std::endl;
	double ret = -1.0 * cmasols.best_candidate().get_fvalue();
	return ret;
}
// Runs 3D CMA-ES with [x0,x1,sigma], optimization results returned in x0 and f-value returned
// max_fevals: if lt.0, default, else specs the max number evaluations before terminating
double do_3dcmaes(std::vector<double>& x0, const int Ndim, const double sigma, const int max_fevals, const bool bMaximizeFitness) {
	GenoPheno<> gp(genof,phenof);
	CMAParameters<> cmaparams(x0,sigma,-1,0,gp);	// -1 = libcmaes chooses population size
	printf("3dCMA[N=%d]:max_fevals=%d sigma=%lf ",Ndim,max_fevals,sigma);
	for (int x=0; x<Ndim; ++x)
		printf("%d:%lf, ",x,x0[x]);
	printf("\n");
	cmaparams.set_maximize(bMaximizeFitness);
	if (max_fevals>0)
		cmaparams.set_max_fevals(max_fevals);
    cmaparams.set_algo(aCMAES);
	CMASolutions cmasols = cmaes<>(fsphere3d,cmaparams);	// CMAES optimization performed in this line
	// Converting solution back into the parameter space
	Eigen::VectorXd bestparameters = gp.pheno(cmasols.get_best_seen_candidate().get_x_dvec());
	// adjust x0[] to use best found candidates
	for (int i=0; i< Ndim; ++i) x0[i] = bestparameters[i];
	std::cout << "3d:Eigen::VectorXd bestparameters: " << bestparameters << std::endl;
	std::cout << "3d:status: " << cmasols.run_status() << "best solution: " << cmasols << std::endl;
	//std::cout << "3d:optimization took " << cmasols.elapsed_time() / 1000.0 << " seconds\n" << std::endl;
	std::cout << "2d:best_candidate::get_fvalue: " << cmasols.best_candidate().get_fvalue() << std::endl;
	//std::cout << "2d:get_best_seen_candidate::get_fvalue: " << cmasols.get_best_seen_candidate().get_fvalue() << std::endl;
	double ret = -1.0 * cmasols.best_candidate().get_fvalue();
	return ret;
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
    if (argc <3) {
        usage(argc,argv);
        //return -1;
    }
    bool bRunForever = false;
    if (argc>1) GMAX_TIME = atof(argv[1]);
    if (argc>2) GbMaximizeFitness = atoi(argv[2]);
#if 0
    double pwm_range = ((MAX_RANGE-MIN_RANGE)/2.0)+MIN_RANGE;
    double duty = ((MAX_DUTY-MIN_DUTY)/2.0)+MIN_DUTY;
    double sigma = ((MAX_SIGMA-MIN_SIGMA)/2.0)+MIN_SIGMA;
#else
    double pwm_range = 200.0;
    double duty = 50.0;
    double sigma = 75.0;
#endif
    
    signal(SIGINT,sig_handler);
    
    // BNO955
#if 1
	BNO_thread::Instance()->start();
#else
    if (Gm_dev.open()) {
        printf("FAILED OPEN\n");
        return -1;
    } else if (Gm_dev.start()) {
        printf("FAILED START\n");
        return -1;
    }
#endif
    
    // enable PWM
    set_PWM_state(true);
    
	do {
		double ret;
		
		// CMA-ES: quick full breath search
	    // problem dimensions: pwm_range,pwm_duty_cycle,alpha
	    std::vector<double> x0_3d(NUM_CMAES_DIM_3D,pwm_range);
	    x0_3d[0] = pwm_range;
	    x0_3d[1] = duty;
	    x0_3d[2] = sigma;
#if 0
	    // this is 3d CMA-ES: we search over x1,x2,alpha with low tolerance to find general area
	    // of higher force, then use that optimization result to further hone in on global max/min
	    // TODO: increase num_fevals as we gain confidence
		ret = do_3dcmaes(x0_3d, NUM_CMAES_DIM_3D, x0_3d[2], MAX_EVALS_3d, GbMaximizeFitness);
#endif
	
	    // CMA-ES: using previous optimization info, run more narrow search
	    // problem dimensions: pwm_range,pwm_duty_cycle
	    std::vector<double> x0_2d(NUM_CMAES_DIM_2D,pwm_range);
	    x0_2d[0] = x0_3d[0];
	    x0_2d[1] = x0_3d[1];
        // x0_3d[2] is optimiz.result 'alpha'
	    ret = do_2dcmaes(x0_2d, NUM_CMAES_DIM_2D, x0_3d[2], -1, GbMaximizeFitness);	// -1 was MAX_EVALS_2d
    
	} while (bRunForever);
	
    set_PWM_state(false);
    return 0;
}

