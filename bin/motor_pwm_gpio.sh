#!/bin/bash
#
# USAGE: $0 [freq] [duty cycle] [on time(sec)]
#

FREQ=${1:-20}
DUTY=${2:-50}
ON_TIME=${3:-1}

# ./motor_pwm_gpio.sh 195 100 1
echo "USAGE: $0 [freq(200/4095)] [duty cycle(100/200)] [on time(sec(1))] - TODO: use params..."

die() { echo "ERROR:$0:$@ !"; exit 1; }

# 50hz = 19.2e6 / 1920 / 200
# duty cycle = 50/200 = 1/4

gpio mode 1 pwm
gpio pwm-ms
gpio pwmc 4095 # pwm clock
gpio pwmr $1  # pwm range
#gpio pwmr 65  # pwm range
gpio pwm 1 $2	# write duty
sleep $ON_TIME
gpio pwm 1 0
