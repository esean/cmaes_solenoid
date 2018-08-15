#include "bno_thread.hpp"
#include <unistd.h> // usleep
#include <iostream>	// std::cout

#define UART_DEV    "/dev/ttyAMA0"	// HACKs!!
#define UART_BAUD   115200			// HACKs!!

#define BNO055_UPDATE_HZ    100
#define BNO055_MEAS_SLP_US    int((1.0/(double)BNO055_UPDATE_HZ)*1000000.0/2.0)    // sleep slightly less

// define to enable
//#define CFG_DUMP_RAW_SQ_MAG

//bno_thread::bno_thread() {}
bno_thread::bno_thread() : //std::string dev_filename, int baud) :
	is_open(false), is_running(false),
	m_dev(UART_DEV,UART_BAUD),
	//m_dev(dev_filename,baud),
	m_x(0.0), m_y(0.0), m_z(0.0),fps(0.0)
{
    // BNO955
    if (m_dev.open()) {
        printf("FAILED OPEN\n");
        throw;
    } else if (m_dev.start()) {
        printf("FAILED START\n");
        throw;
    } else {
        is_open = true;
        m_reader_thread = std::thread(&bno_thread::reader_thread, this);
        usleep(1000);   // let it start sampling, so ready when returned here
    }
}
bno_thread::~bno_thread() {
	//std::cout << "~bno_thread: waiting join..." << std::endl;
	is_running = false;
    m_reader_thread.join();
    //std::cout << "~bno_thread: done" << std::endl;
}

void bno_thread::start() {}
void bno_thread::reader_thread() {
	//std::cout << "thread START" << std::endl;
	is_running = true;
	try {
#ifdef CFG_DUMP_RAW_SQ_MAG
		std::cout << "#SQMAG:# sec,sq_mag" << std::endl;
#endif
		int loop_cnt = 0;
		start_time = std::chrono::high_resolution_clock::now();
		while (is_running) {
			{	// mutex lock
				std::lock_guard<std::mutex> lck(m_mutex);
			    (void)m_dev.get_linear_acceleration(&m_x,&m_y,&m_z);
#ifdef CFG_DUMP_RAW_SQ_MAG
				std::chrono::high_resolution_clock::time_point now_tm2 = std::chrono::high_resolution_clock::now();
				double tm_delta2 = (std::chrono::duration_cast<std::chrono::microseconds>(now_tm2-start_time).count() / 1e+6);
				double sq_mag2 = m_x*m_x + m_y*m_y + m_z*m_z;
				std::cout << "#SQMAG:" << tm_delta2 << "," << sq_mag2 << std::endl;
#endif
			}
			++loop_cnt;
			std::chrono::high_resolution_clock::time_point now_tm = std::chrono::high_resolution_clock::now();
			double tm_delta_sec = (std::chrono::duration_cast<std::chrono::milliseconds>(now_tm-start_time).count() / 1000.0);
			if (tm_delta_sec>0.0) fps = (double)loop_cnt/(double)tm_delta_sec;
		    usleep(BNO055_MEAS_SLP_US);
		}
	}
	catch(...) {
		std::cerr << "ERROR: thread Exception" << std::endl;
	}
	//std::cout << "thread exiting..." << std::endl;
}

// RETURNS: -1 on failure
double bno_thread::get_linacc_sq_magnitude() {
    if (!is_open) return -1;
    // save time, don't need to sqrt()
    std::lock_guard<std::mutex> lck(m_mutex);
    double magnitude_sq = m_x*m_x + m_y*m_y + m_z*m_z;
    return magnitude_sq;
}

void bno_thread::get_linear_acceleration(double *x, double *y, double *z) {
    if (!is_open) {
        *x = -1.0;
        *y = -1.0;
        *z = -1.0;
        return;
    }
    std::lock_guard<std::mutex> lck(m_mutex);
    *x = m_x;
    *y = m_y;
    *z = m_z;
}

