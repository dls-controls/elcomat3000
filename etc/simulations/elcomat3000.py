#!/dls_sw/tools/bin/python2.4

from pkg_resources import require
require('dls_serial_sim')
from dls_serial_sim import serial_device
import re, os, time

class elcomat3000(serial_device):
    Terminator = "\n"
    
    def __init__(self):
        '''Constructor.  Remember to call the base class constructor.'''
        serial_device.__init__(self,
            protocolBranches = [])
        print "Initialising Elcomat3000 simulator, V1.0"
        print "Power is %s" % self.power
        self.transmitType1 = False
        self.transmitType3 = False
        self.transmitType5 = False
        self.transmitType6 = False
        self.measuredX = 4.5
        self.measuredY = 2.3
        self.day = 12
        self.month = 1
        self.year = 2001
        self.focalLength = 300
        return

    def output(self, text):
        print 'Tx: %s' % repr(text)
        self.outq.put(text)

    def outputType1(self):
        text = "1 103 %f %f\r" % (self.measuredX, self.measuredY)
        self.output(text)
        
    def outputType2(self):
        text = "2 103 %f %f\r" % (self.measuredX, self.measuredY)
        self.output(text)
        
    def outputType3(self):
        text = "3 003 %f %f\r" % (self.measuredX, self.measuredY)
        self.output(text)
        
    def outputType4(self):
        text = "4 003 %f %f\r" % (self.measuredX, self.measuredY)
        self.output(text)

    def outputType8(self):
        text = "8 423 %d %d %d %d\r" % (self.day, self.month, self.year, self.focalLength)
        self.output(text)

    def outputTables(self):
        # TODO: Implement table output
        pass

    def reply(self, command):
        '''This function must be defined. It is called by the serial_sim system
        whenever an asyn command is send down the line. Must return a string
        with a response to the command or None.'''
        print "Rx: %s" % repr(command)
        result = None

        if self.isPowerOn():
            if command == 's':
                self.transmitType1 = False
                self.transmitType3 = False
                self.transmitType5 = False
                self.transmitType6 = False
            elif command == 'r':
                self.transmitType1 = False
                self.transmitType3 = False
                self.outputType2()
            elif command == 'R':
                self.transmitType1 = True
            elif command == 'a':
                self.transmitType1 = False
                self.transmitType3 = False
                self.outputType4()
            elif command == 'A':
                self.transmitType3 = True
            elif command == 't':
                self.outputTables()
            elif command == 'd':
                self.outputType8()
            else:
                print "Unknown command %s" % repr(command)
        return result
        
    def initialise(self):
        '''Called by the framework when the power is switched on.'''
        pass

    # The functions below are the backdoor RPC API

    def setInfo(self, day, month, year, focalLength):
        self.day = day
        self.month = month
        self.year = year
        self.focalLength = focalLenght

    def setMeasurement(self, measuredX, measuredY):
        self.measuredX = measuredX
        self.measuredY = measuredY


if __name__ == "__main__":
    # little test function that runs only when you run this file
    dev = elcomat3000()
    dev.start_ip(9015)
    dev.start_rpc(9016)
    # cheesy wait to stop the program exiting immediately
    while True:
        time.sleep(1)

