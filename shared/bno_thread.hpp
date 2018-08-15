#include <string>
#include <thread>
#include <mutex>
#include <chrono>
#include "bno055_uart.hpp"
#include "singleton.hpp"

#ifndef bno_thread_hpp
#define bno_thread_hpp

class bno_thread {

public:
    bno_thread();
    bno_thread(std::string dev_filename, int baud);
    ~bno_thread();
    
    void start();    // does nothing, just activates singleton

    // returns -1 on failure
    double get_linacc_sq_magnitude();

    // returns -1 (in x,y,z) on failure
    void get_linear_acceleration(double *x, double *y, double *z);
	
	double get_fps() { return this->fps; }
	
private:
    bno055_uart m_dev;
    bool is_open;
    std::thread m_reader_thread;
    double m_x,m_y,m_z;
    std::mutex m_mutex;
    bool is_running;
    // fps
    std::chrono::high_resolution_clock::time_point start_time;
	double fps;
	
	void reader_thread();
};  
    
typedef singleton<bno_thread> BNO_thread;

#endif 
