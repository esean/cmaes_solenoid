#include "rpi_uart.hpp"
#include <string>
#include <inttypes.h>

#ifndef bno055_uart_hpp
#define bno055_uart_hpp

class bno055_uart {

public:
	bno055_uart();
    bno055_uart(std::string dev_filename, int baud);
    ~bno055_uart();

    int open();
	int start();
    
    // for reading many values at once
	int read_register_buf(uint8_t *buf, int register_number, int num_to_read = 1);
	
	// for reading single value, different methods
	int read_register(int register_number, uint8_t *value);
	uint8_t read_value_from_register(int register_number);

	// write register value
	int write_register(int register_number, int value);
	
    int get_linear_acceleration(double *x, double *y, double *z);
    int get_xyz_at_register_block(int starting_register, double *x, double *y, double *z);

	int awake();
	int sleep();
	int hw_reset();
    int init_port();
	
private:
    bool is_open;
    rpi_uart m_dev_uart;
};

#endif

