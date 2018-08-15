#include "rpi_uart.hpp"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>  // memset

const int rpi_uart::BAUD_115200;

rpi_uart::rpi_uart() : m_dev_filename(""), m_dev_baud(0), is_open(false)  {}
rpi_uart::rpi_uart(std::string dev_filename, int baud) : m_dev_filename(dev_filename), m_dev_baud(0), is_open(false) {
    switch(baud) {
        case 115200:    m_dev_baud = rpi_uart::BAUD_115200;
                        break;
        default:        printf("ERROR:rpi_uart() invalid baud %d\n",baud);
                        break;
    }
}
rpi_uart::~rpi_uart() {
    close(fd);
}

int rpi_uart::open() {
    if (m_dev_baud == 0) return -1;
    // Open the Port. We want read/write, no "controlling tty" status, and open it no matter what state DCD is in
    fd = ::open(m_dev_filename.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd <= 0) {
        perror("open_port: Unable to open /dev/ttyAMA0 - ");
        return(-1);
    }
    struct termios tc;
    tcgetattr(fd, &tc);

    /* input flags */
    tc.c_iflag &= ~ IGNBRK; /* enable ignoring break */
    tc.c_iflag &= ~(IGNPAR | PARMRK); /* disable parity checks */
    tc.c_iflag &= ~ INPCK; /* disable parity checking */
    tc.c_iflag &= ~ ISTRIP; /* disable stripping 8th bit */
    tc.c_iflag &= ~(INLCR | ICRNL); /* disable translating NL <-> CR */
    tc.c_iflag &= ~ IGNCR; /* disable ignoring CR */
    tc.c_iflag &= ~(IXON | IXOFF); /* disable XON/XOFF flow control */
    /* output flags */
    tc.c_oflag &= ~ OPOST; /* disable output processing */
    tc.c_oflag &= ~(ONLCR | OCRNL); /* disable translating NL <-> CR */
    /* not for FreeBSD */
    tc.c_oflag &= ~ OFILL; /* disable fill characters */
    /* control flags */
    tc.c_cflag |= CLOCAL; /* prevent changing ownership */
    tc.c_cflag |= CREAD; /* enable reciever */
    tc.c_cflag &= ~ PARENB; /* disable parity */
    tc.c_cflag &= ~ CSTOPB; /* disable 2 stop bits */
    tc.c_cflag &= ~ CSIZE; /* remove size flag... */
    tc.c_cflag |= CS8; /* ...enable 8 bit characters */
    tc.c_cflag |= HUPCL; /* enable lower control lines on close - hang up */
//   if (fRtsCts)
//        tc.c_cflag |= CRTSCTS; /* enable hardware CTS/RTS flow control */
//    else
        tc.c_cflag &= ~ CRTSCTS; /* disable hardware CTS/RTS flow control */
    /* local flags */
    tc.c_lflag &= ~ ISIG; /* disable generating signals */
    tc.c_lflag &= ~ ICANON; /* disable canonical mode - line by line */
    tc.c_lflag &= ~ ECHO; /* disable echoing characters */
    tc.c_lflag &= ~ ECHONL; /* ??? */
    tc.c_lflag &= ~ NOFLSH; /* disable flushing on SIGINT */
    tc.c_lflag &= ~ IEXTEN; /* disable input processing */

    // clear control characters
    memset(tc.c_cc,0,sizeof(tc.c_cc));

    // set io rate
    cfsetispeed(&tc, m_dev_baud);
    cfsetospeed(&tc, m_dev_baud);
    tcsetattr(fd, TCSANOW, &tc);

    /* enable input & output transmission */
    tcflow(fd, TCOON | TCION);

    // let settle after opening, so if there is any data it's there to be flushed
    usleep(10000);
    if (flush_data_buffers()) return -1;
    is_open = true;
    return 0;
}

int rpi_uart::flush_data_buffers() {
    if (tcflush(fd,TCIOFLUSH)) {
        fprintf(stderr,"ERROR: rpi_uart::open() tcflush() returned error\n");
        return -1;
    } 
    return 0;
}

uint8_t rpi_uart::read_byte() {
    char c;
    int n = read(fd, (void*)&c, 1);
    if (n<0) {
        //fprintf(stderr,"ERROR: rpi_uart::read_byte() read() returned error:%d\n",n);
        return n;
    }
    return static_cast<uint8_t>(c);
}

int rpi_uart::write_buf(uint8_t *buf, uint8_t buf_len) {
    if (!is_open) return -1;
    return write(fd,(const void*)buf,buf_len);
}

// RETURNS: number Rx characters, or 0 or less on error
int rpi_uart::receive_buf(uint8_t *buf, uint8_t buf_max_len) {
    if (!is_open) return -1;
	int n = 0;              // bytes received in last rx
	int i = 0;	            // index thru buf[]
	int rx_len = 0;         // 2nd byte in good pkt is RX len
	int offset_chars = 2;   // don't store first 2 bytes in return buf[]
	int wait_response_header_status = 0;
	do
	{
	    char c = 0;
	    n = read(fd, (void*)&c, 1);
	    if (i==0 && c>0 && c!=0xbb) {	// 1st byte response s/b 0xbb
		    //printf("ERROR: RX did not RX 0xbc:0x%X\n",c);
		    rx_len=1;	// trick to get error code
		    wait_response_header_status=(int)c;
	    }
	    else if (i==1) {
		    if (wait_response_header_status>0) {
			    //printf("))))FAILCODE[0x%X]=%d\n",wait_response_header_status,c);
                if (c == 0x01)      // write success is only 0x1, all others failures
                    return 0;
			    //fprintf(stderr,"FAILCODE[0x%X]=%d\n",wait_response_header_status,c);
                //flush_data_buffers();
                return -1;
		    }
		    rx_len=int(c);	// 2nd byte is RX length
	    }
	    if (n > 0) {
	      if (i>=offset_chars) 	// only  store 'data' section
		      buf[i-offset_chars] = c;
	      i++;
	    }
	}
	while (i<rx_len+offset_chars && i<buf_max_len-1);
	buf[i-offset_chars] = '\0';

	if (n < 0) {
        perror("Read failed - ");
        return -1;
    }
	else if (i == 0) {
        printf("No data on port\n");
        return 0;
    }
    return i-offset_chars;
}

