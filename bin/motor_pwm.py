#!/usr/bin/env python
#
# USAGE:
#   $0 {[OFF_TIME] [FREQ_SCALE] [MAX_FREQ]}
#
import sys
import time
import RPi.GPIO as GPIO

print "USAGE: %s {[OFF_TIME] [FREQ_SCALE] [MAX_FREQ]}" % sys.argv[0]

OFF_TIME = 0.03
FREQ_SCALE = 2.0
MAX_FREQ = 10

num_args = len(sys.argv)
if num_args >= 2:
    OFF_TIME=float(sys.argv[1])
if num_args >= 3:
    FREQ_SCALE=float(sys.argv[2])
if num_args >= 4:
    MAX_FREQ=int(sys.argv[3])

GPIO.setmode(GPIO.BOARD)
GPIO.setup(12, GPIO.OUT)
p = GPIO.PWM(12, 10)  # channel=12 frequency=50Hz
p.start(0)
try:
    while 1:
        for dc in range(0, MAX_FREQ, 1):
            p.ChangeDutyCycle(dc)
            p.ChangeFrequency((dc+FREQ_SCALE)*FREQ_SCALE)
            time.sleep(OFF_TIME)
        for dc in range(MAX_FREQ, -1, -1):
            p.ChangeDutyCycle(dc)
            p.ChangeFrequency((dc+FREQ_SCALE)*FREQ_SCALE)
            time.sleep(OFF_TIME)
except KeyboardInterrupt:
    pass
p.stop()
GPIO.cleanup()
