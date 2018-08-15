#include "bno055_uart.hpp"
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include "bno055.h"
#include "delay_helper.hpp"

//#define DEBUG

#define DEF_OPERATION_MODE  OPERATION_MODE_NDOF

bno055_uart::bno055_uart() : is_open(false)  {}
bno055_uart::bno055_uart(std::string dev_filename, int baud) : is_open(false), m_dev_uart(dev_filename,baud) {}
bno055_uart::~bno055_uart() {
    sleep();
}

int bno055_uart::open() {
    is_open = m_dev_uart.open() >= 0;
    if (is_open) return 0;
    return -1;
}

// below functions return 0 on success, -1 on failure

int16_t mk_16(uint8_t lsb, uint8_t msb) {
    int16_t tmp = (static_cast<int16_t>(lsb)&0xFF) | ((static_cast<int16_t>(msb)<<8)&0xFF00);
    return tmp;
}

int bno055_uart::get_linear_acceleration(double *x, double *y, double *z) {
	if (!is_open) return -1;
    if (!x || !y || !z) {
        fprintf(stderr,"ERROR: ASSERT: get_linear_acceleration()\n");
        exit(1);
    }
    #ifdef DEBUG
    printf(" [get regblk] ");
    #endif
    int n = get_xyz_at_register_block(BNO055_LINACC_X_LSB,x,y,z);
    *x /= 100.0f;
    *y /= 100.0f;
    *z /= 100.0f;
    #ifdef DEBUG
    printf(" [linacc=%f,%f,%f:%d]\n",*x,*y,*z,n);
    #endif
    if (n<0) return n;
    return 0;
}

// give it starting address of acceleration data (0x8), or linear acceleration(0x28), etc
// and it will read 6 registers values and return those x,y,z values as double
int bno055_uart::get_xyz_at_register_block(int starting_register, double *x, double *y, double *z) {
	if (!is_open) return -1;
    if (!x || !y || !z) {
        fprintf(stderr,"ERROR: ASSERT: get_xyz_at_register_block()\n");
        exit(1);
    }
    uint8_t buf[255];
    #ifdef DEBUG
    printf(" [read 6regs] ");
    #endif
    int n = read_register_buf(buf,starting_register,6);
    if (n<0) return n;
    if (n<6) return -1;
    *x = static_cast<double>(mk_16(buf[0],buf[1]));
    *y = static_cast<double>(mk_16(buf[2],buf[3]));
    *z = static_cast<double>(mk_16(buf[4],buf[5]));
    return 0;
}

int bno055_uart::read_register_buf(uint8_t *buf, int register_number, int num_to_read) {
	if (!is_open) return -1;
	  int n;
	  // Write to the port
	  buf[0]=0xaa;
	  buf[1]=1;	// read
	  buf[2]=register_number;
	  buf[3]=num_to_read;
	  #ifdef DEBUG
	  printf(" [tx] ");
	  #endif
	  n = m_dev_uart.write_buf(buf,4);
	  if (n < 0) {
		  #ifdef DEBUG
		  printf(" ![err flush]! ");
		  #endif
        (void)m_dev_uart.flush_data_buffers();
	    perror("Write failed - ");
	    return -1;
	  }
	  #ifdef DEBUG
	  printf(" [rx] ");
	  #endif
      return m_dev_uart.receive_buf(buf,255);
}

int bno055_uart::read_register(int register_number, uint8_t *value) {
	if (!is_open) return -1;
	if (!value) {
		fprintf(stderr,"ERROR: ASSERT read_register null value\n");
		exit(-1);
	}
	uint8_t buf[255];
	int n = read_register_buf(buf,register_number,1);
	if (n<0) return n;
	*value = buf[0];
	//printf("#DEBUG: bno055::read_register(0x%X) = 0x%X\n",register_number,*value);
	return 0;
}

uint8_t bno055_uart::read_value_from_register(int register_number) {
	uint8_t val;
	int n = read_register(register_number,&val);
	if (n<0) return n;
	return val;
}

int bno055_uart::write_register(int register_number, int value) {
	if (!is_open) return -1;
	  int n;
	  //printf("#DEBUG: bno055::write_register(0x%X,0x%X)\n",register_number,value);
	  uint8_t buf[255];
	  // Write to the port
	  buf[0]=0xaa;
	  buf[1]=0;	// write
	  buf[2]=register_number;
	  buf[3]=1;
	  buf[4]=value;
	  n = m_dev_uart.write_buf(buf,5);
	  if (n < 0) {
        (void)m_dev_uart.flush_data_buffers();
	    perror("Write failed - ");
	    return -1;
	  }
      return m_dev_uart.receive_buf(buf,255);
}

int bno055_uart::awake() {
	if (!is_open) return -1;
	return write_register(BNO055_OPR_MODE, DEF_OPERATION_MODE);
}
int bno055_uart::sleep() {
	if (!is_open) return -1;
	return write_register(BNO055_OPR_MODE, OPERATION_MODE_CONFIG);
}

int bno055_uart::init_port() {
    if (m_dev_uart.flush_data_buffers())
        fprintf(stderr,"WARNING: bno055_uart::init_port() flush_data_buffers() returned error\n");
    int cnt = 100;
    while (cnt-- > 0)
        (void)m_dev_uart.read_byte();
    if (m_dev_uart.flush_data_buffers())
        fprintf(stderr,"WARNING: bno055_uart::init_port() flush_data_buffers() returned error\n");
    return 0;
}

int bno055_uart::start() {
	if (!is_open) return -1;

    int n = init_port();
    if (n<0) {
        fprintf(stderr,"ERROR: bno055_uart::start() init_port() returned failure\n");
        return n;
    }

	n = write_register(BNO055_PAGE_ID,PAGE_0);
	if (n<0) {
        fprintf(stderr,"ERROR: bno055_uart::start() write_register(PAGE_0) returned failure\n");
        return n;
    }
	
    uint8_t chip_id, acc_id, mag_id, gyr_id;
    if (read_register(BNO055_REG_CHIP_ID,&chip_id)
        || read_register(BNO055_REG_ACC_ID,&acc_id)
        || read_register(BNO055_REG_MAG_ID,&mag_id)
        || read_register(BNO055_REG_GYR_ID,&gyr_id))
    {
		fprintf(stderr,"ERROR: bno055_uart::start read IDs failed (0x%X,0x%X,0x%X,0x%X)\n",
            chip_id,acc_id,mag_id,gyr_id);
        return -1;
    }
    if ( (BNO055_REG_CHIP_ID_VAL != chip_id)
        || (BNO055_REG_ACC_ID_VAL != acc_id)
        || (BNO055_REG_MAG_ID_VAL != mag_id)
        || (BNO055_REG_GYR_ID_VAL != gyr_id))
    {
		fprintf(stderr,"ERROR: bno055_uart::start ID compare failed (0x%X,0x%X,0x%X,0x%X)\n",
            chip_id,acc_id,mag_id,gyr_id);
        return -1;
    }
    if (hw_reset()) {
		fprintf(stderr,"ERROR: bno055_uart::start hw_reset() failed\n");
		return -1;
	}
    if (awake()) {
		fprintf(stderr,"ERROR: bno055_uart::start awake() failed\n");
		return -1;
	}
    // this was 10ms, but I'm seeing some bad LINACC data coming out at start, so
    // give it longer. The noise is gone before 100 samples in, so ~1sec
    //delay_ms(10);
    delay_ms(1000);
	return 0;
}

int bno055_uart::hw_reset() {

    int n =0;

    // Reset IC, need to cast to void since it returns 0 (since it reset...)
    // since the chip is being reset, no response data is issued back, so ignore
    (void)write_register(BNO055_SYS_TRIGGER,BNO055_SYS_TRIG_RST);

    /* Wait for sensor to quiesce (see section 1.2) */
    delay_ms(650 + 10);
    while (read_value_from_register(BNO055_REG_CHIP_ID) != BNO055_REG_CHIP_ID_VAL)
        delay_ms(10);

    /* Set to normal power mode */
	n = write_register(BNO055_PWR_MODE, POWER_MODE_NORMAL);
	if (n<0) return n;
    delay_ms(10);

    /* Set page to 0 */
	n = write_register(BNO055_PAGE_ID,PAGE_0);
	if (n<0) return n;
	
    /* Configure axis mapping (see section 3.4) */
    //bno055_set_axis_remap(BNO055_AXISMAP_CAP_MODE);

	n = write_register(BNO055_SYS_TRIGGER, 0x0);
    delay_ms(10);
	if (n<0) return n;

    return 0;
}

