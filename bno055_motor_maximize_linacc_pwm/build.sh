SHARED="../shared/"
CC_ARGS="-lcmaes -I /usr/local/include/libcmaes/ -I /usr/include/eigen3/ -I. -I${SHARED} -std=c++11"
#CC="g++"
CC="clang++"

run_cmd() {
    nm="$1"; shift
    cmd="$@"
    echo "# $nm..."
    echo "#   $ $cmd"
    time $cmd
}

cmd="$CC -o set_pwm set_pwm.cpp ${SHARED}canakit_common.cpp ${SHARED}rpi_uart.cpp ${SHARED}bno055_uart.cpp ${SHARED}delay_helper.cpp $CC_ARGS -O3"
run_cmd "building set_pwm.cpp" $cmd

#cmd="$CC -o bno055_motor_maximize_linacc_pwm main.cpp ${SHARED}canakit_common.cpp ${SHARED}rpi_uart.cpp ${SHARED}bno055_uart.cpp ${SHARED}delay_helper.cpp $CC_ARGS"
#run_cmd "building regular" $cmd

cmd="$CC -o bno055_motor_maximize_linacc_pwm_optimized main.cpp ${SHARED}bno_thread.cpp ${SHARED}canakit_common.cpp ${SHARED}rpi_uart.cpp ${SHARED}bno055_uart.cpp ${SHARED}delay_helper.cpp $CC_ARGS -lpthread -O3"
run_cmd "building optimized" $cmd

