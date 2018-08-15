#!/usr/bin/env python
import threading
import serial
import time
import sys

# NOTE: this runs in a thread, so the Rx data receiver function (in cb_fcn)
# should use a thread-safe way, such as a Queue(), to transfer the data between
# the thread CB and it's instance

class UART_thread(threading.Thread):
    ''' Creates a thread to listen for Rx and passes up to caller with callback.
        '''

    RX_TIMEOUT_SEC = 0.001
    TX_TIMEOUT_SEC = 1.0
    SERIAL_PORT = '/dev/ttyAMA0'

    def __init__(self,baudrate,cb_fcn, isBinaryData=False,debug=False):
        self.cb = cb_fcn
        self.debug = debug
        self.bin_data = isBinaryData
        self.ser = serial.Serial(
                port = self.SERIAL_PORT,
                baudrate = baudrate,
                parity = serial.PARITY_NONE,
                stopbits = serial.STOPBITS_ONE,
                bytesize = serial.EIGHTBITS,
                timeout = self.RX_TIMEOUT_SEC,
                write_timeout = self.TX_TIMEOUT_SEC
                )
        if not self.ser:
            raise "ERROR: [UART] serial not initialized"
        self.ser.flushInput()
        self.ser.flushOutput()
        self.keep_running = True
        threading.Thread.__init__ ( self )

    def write(self,msg):
        if not self.ser.isOpen():
            raise Exception("ERROR: [UART] trying to write but serial port is not open")
        #if self.debug: print "# DEBUG: [UART] UART_thread::write(%s)" % msg.strip()
        try:
            self.ser.write(msg)
            self.ser.flush()
        except Exception as e:
            # kill RX thread
            self.keep_running = False
            raise Exception("UART failure:%s" % e)

    def run(self):
        #if self.debug: print "# DEBUG: [UART] UART_thread::run() STARTING..."
        while self.keep_running:
            try:
                resp = self.ser.readline()
                if resp:
                    # if rx is binary, just send it up
                    if self.bin_data:
                        if self.cb: self.cb(resp)
                    else:               
                        # if not binary, parse into lines
                        for ln in resp.splitlines():
                            ln = ln.strip()
                            # send Rx data up w/ CB
                            if ln and self.cb: self.cb(ln)
            # if we have a problem reading, close down port and exit thread
            except:
                e = sys.exc_info()[0] 
                print "ERROR: [UART] UART_thread::run() returned:%s" % e
                self.close_port() 
                raise
        #if self.debug: print "# DEBUG: [UART] UART_thread::run() EXITING..."

    def close_port(self):
        self.keep_running = False
        self.ser.close()
        while self.ser.is_open:
            time.sleep(0.1)
