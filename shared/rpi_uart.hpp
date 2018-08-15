#include <string>
#include <inttypes.h>
#include <termios.h>

#ifndef rpi_uart_hpp
#define rpi_uart_hpp

class rpi_uart {

public:
    rpi_uart(std::string dev_filename, int baud);
    rpi_uart();
    ~rpi_uart();

    int open();
    int write_buf(uint8_t *buf, uint8_t buf_len);
    int receive_buf(uint8_t *buf, uint8_t buf_max_len);

    uint8_t read_byte();
    int flush_data_buffers();

private:
    static const int BAUD_115200 = B115200;
    int fd;
    std::string m_dev_filename;
    int m_dev_baud;
    bool is_open;
};

#endif

