SHARED="../shared/"
g++ -o bno055_test main.cpp ${SHARED}rpi_uart.cpp ${SHARED}bno055_uart.cpp ${SHARED}delay_helper.cpp -I. -I${SHARED} -std=c++11
