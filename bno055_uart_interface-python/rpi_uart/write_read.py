#!/usr/bin/env python
import time
import sys
import binascii
from Queue import Queue
sys.path.append('../mod_common')
import uart # thread

DEBUG = False #True
BAUD = 115200

# global
uart_msg_queue = Queue()

def clearQ(Q):
    while not Q.empty():
        Q.get()

def data_cb(msg):
    global uart_msg_queue
    uart_msg_queue.put(msg)
    #if DEBUG: print "# DEBUG: data_cb(" + binascii.hexlify(msg) + ")"

# PASS: uart_obj = class allows .write() out UART
# PASS: txBuf =  bytearray of stuff to send out
# PASS: rx_len = expected data bytes (excluding response and length bytes)
# RETURNS: list of '[response header],[data length],[rx data]'
def write_to_uart_get_response(uart_obj,txBuf,rx_len):
    global uart_msg_queue
    len_hdrs = 2
#    if DEBUG: print "# DEBUG: txBuf =",binascii.hexlify(txBuf)," RXlen =",rx_len
    clearQ(uart_msg_queue)
    uart_obj.write(txBuf)
    # now wait for response
    resp_data = []
    while len(resp_data)-len_hdrs < rx_len:
        while uart_msg_queue.empty(): pass
        resp = uart_msg_queue.get()
        #if DEBUG: print "# DEBUG: ok, reg[%d] = %s" % (regnum,binascii.hexlify(resp))
        for dx in resp:
            resp_data.append(ord(dx))
        #if DEBUG: print "# DEBUG: ok, reg[%d] = %s (%d)" % (regnum,resp_data,len(resp_data)-2)
    return resp_data

# RETURNS: [true/false on success/failure],list of [rx data]
def read_register(regnum, rx_len=1):
    txBuf = bytearray()
    txBuf.append('\xaa')	# start byte
    txBuf.append('\x01')	# read
    txBuf.append(regnum)	# register
    txBuf.append(rx_len)	# length
    resp = write_to_uart_get_response(obj,txBuf,rx_len)
    if resp[0] != 0xBB:
        print "FAILURE: ACK_RESP returned after reading REG[%d] from BNO:%s" % (regnum,binascii.hexlify(resp))
        return False,[None]
    if rx_len != resp[1]:
        print "FAILURE: response length is not correct, got %d expected %d" % (resp[1],rx_len)
        return False,[None]
    return True,resp[2:] # now strip off the first 2 bytes (success & rx length)

# RETURNS: [true/false on success/failure],[failure code] (where failure code 0x1 = success)
def write_register(regnum,data_len,data):
    txBuf = bytearray()
    txBuf.append('\xaa')	# start byte
    txBuf.append('\x00')	# write
    txBuf.append(regnum)	# register
    txBuf.append(data_len)	# length of data
    for dx in data:
        txBuf.append(dx)	# data
    resp = write_to_uart_get_response(obj,txBuf,0)
    if resp[0] != 0xEE or resp[1] != 0x01:
        print "FAILURE: ACK_RESP returned after writing REG[%d] from BNO:%s" % (regnum,binascii.hexlify(resp))
    return resp[1]==0x1,resp[1]

def get_user_input():
    line = sys.stdin.readline()
    return line.strip()


# open comm to BNO055 via UART
#---------------------
if DEBUG: print "# starting uart..."
# setting 'True' since data coming back from BNO is binary
obj = uart.UART_thread(BAUD,data_cb,True,DEBUG)
obj.start() # start thread

# set BNO op mode



# TEST: timing
print "# TESTING: constant read 44 bytes from BNO055, fps below"
start_tm = time.time()
meas_cnt = 0
try:
    while True:
        ret,data = read_register(8,44)
        if not ret: print "READ FAILED"
        meas_cnt += 1
        if (meas_cnt % 50) == 0:
            print "%f" % (meas_cnt/(time.time()-start_tm))
    #    print "REG[%d] = %s" % (8,str(data))
except KeyboardInterrupt:
    obj.close_port()
    sys.exit(0)


while True:
    print "\nBNO055 register: Enter (r)ead or (w)rite --> "
    try: mode = get_user_input()
    except KeyboardInterrupt: break
    if not mode: break
    if mode != 'r' and mode != 'R' and mode != 'w' and mode != 'W':
        print "ERROR: unknown command:",mode
        continue

    print " --> register number? "
    regnum = int(get_user_input())

    if mode == 'r' or mode == 'R': 
        print " --> number bytes to read? "
        rx_len = int(get_user_input())
        ret,data = read_register(regnum,rx_len)
        if not ret: print "READ FAILED"
        print "REG[%d] = %s" % (regnum,str(data))

    elif mode == 'w' or mode == 'W':
        print " --> data? "
        reg_data = get_user_input()
        tx_data = []
        for dx in reg_data:
            tx_data.append(int(dx))
        ret,ack_code = write_register(regnum,len(tx_data),tx_data)
        if not ret: print "WRITE FAILED:",ack_code

if DEBUG: print "# closing uart..."
obj.close_port()

