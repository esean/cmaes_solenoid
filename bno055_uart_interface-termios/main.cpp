#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>
#include <cstdlib>
#include "bno055_uart.hpp"

//#define TEST_WRITE_REG
//#define TEST_READ_REG
#define TEST_READ_LINACC

#define WANT_UPDATE_HZ  (1.0/3.0) // update fps to console every 3sec, setting 2.0 gives 2x/sec

void usage(int argc, char ** argv) {
	printf("\nUSAGE: %s [start register] [num registers to read]\n",argv[0]);
}

int main(int argc, char ** argv) { 
    bool whileOne = true; //false;  // set true to run cmd over-over and measure fps
    double rate_oneOver = 1.0/WANT_UPDATE_HZ;
    double fps = 50.0;	// just guess to get going
    int n;
    int loop_cntr = 0;
    int bad_cntr = 0;

	if (argc <3) {
		usage(argc,argv);
		return -1;
	}
	int start_reg = atoi(argv[1]);
	int num_reg = atoi(argv[2]);
	int slp_tm = 0;
	if (argc>3) slp_tm = atoi(argv[3]);
	
	bno055_uart m_dev("/dev/ttyAMA0",115200);

    if (m_dev.open()) {
        printf("FAILED OPEN\n");
        return -1;
    } else if (m_dev.start()) {
		printf("FAILED START\n");
		return -1;
	}

    struct timeval t1, t2;
    gettimeofday(&t1, NULL);

    double max_mag = 0.0;

    do {

#ifdef TEST_WRITE_REG
        // WRITE REGISTER
        n = m_dev.write_register(start_reg,num_reg);	// odd names, but that's (register,value)
        if (n<0) {
            ++bad_cntr;
            continue;
        }
        uint8_t val;
        n = m_dev.read_register(start_reg,&val);
        printf("REGISTER[%d] = %d\n",start_reg,val);

#endif
#ifdef TEST_READ_REG
        // READ REGISTER
        uint8_t buf[255];
        n = m_dev.read_register_buf(buf,start_reg,num_reg);
        if (n<0) { 
            ++bad_cntr;
            continue;
        }
        #if 1
        printf("%d:",n);
        for (int ix=0; ix<n; ++ix) printf("0x%X ",buf[ix]);
        printf("\n");
        #endif
#endif	
#ifdef TEST_READ_LINACC
        double x,y,z;
        n = m_dev.get_linear_acceleration(&x,&y,&z);
        if (n<0) { 
            ++bad_cntr;
            continue;
        }
        // save time, don't need to sqrt()
        double magnitude_sq = x*x + y*y + z*z;
        if (magnitude_sq > max_mag) {
            max_mag = magnitude_sq;
            printf("LINACC:%f\n",magnitude_sq);
        }
        max_mag *= 0.9999;
#endif

        //compute fps
        ++loop_cntr;
        if (loop_cntr % int(rate_oneOver * fps) == 0) {
            gettimeofday(&t2, NULL);
            double elapsedTime;
            elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;
            elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;
            //convert to seconds
            double delta = elapsedTime /1000;
            fps = loop_cntr/delta;
            printf("(%d (%d bad pkts) in %f sec = %0.2f fps)\n",loop_cntr,bad_cntr,delta,fps);
        }
        if (slp_tm>0) usleep(slp_tm);

  } while (whileOne);

  return 0;
}

