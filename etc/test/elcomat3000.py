#!/dls_sw/prod/tools/RHEL5/bin/python2.6

from pkg_resources import require
require('dls_autotestframework')
require('dls_simulationlib')
from dls_autotestframework import *
import dls_simulationlib.simsocket
import datetime
import numpy
from sim_backdoor import *

################################################
# Test suite for the basic chip register access of the Excalibur project
    
class ChipRegistersTestSuite(TestSuite):
    
    def createTests(self):
        # Define the targets for this test suite
        Target("simulation", self, [
            BuildEntity('excalibur', directory="."),
            IocEntity('ioc', directory='.', bootCmd='bin/linux-x86/stexcaliburFemIoc.boot'),
            #EpicsDbEntity('db', directory='.', fileName="db/excaliburFemIoc.db"),
            GuiEntity('gui', directory='.', runCmd='./bin/linux-x86/stExcaliburFemIoc-gui.sh')])

        # The tests
        CaseDacSense(self)
        CaseDacExternal(self)
        CaseThreshold0Dac(self)
        CaseThreshold1Dac(self)
        CaseThreshold2Dac(self)
        CaseThreshold3Dac(self)
        CaseThreshold4Dac(self)
        CaseThreshold5Dac(self)
        CaseThreshold6Dac(self)
        CaseThreshold7Dac(self)
        CasePreampDac(self)
        CaseIkrumDac(self)
        CaseShaperDac(self)
        CaseDiscDac(self)
        CaseDisclsDac(self)
        CaseThresholdnDac(self)
        CaseDacpixelDac(self)
        CaseDelayDac(self)
        CaseTpbufferinDac(self)
        CaseTpbufferoutDac(self)
        CaseRpzDac(self)
        CaseGndDac(self)
        CaseTprefDac(self)
        CaseFbkDac(self)
        CaseCasDac(self)
        CaseTprefaDac(self)
        CaseTprefbDac(self)
        CasePixelMask(self)
        CasePixelTest(self)
        CasePixelGainMode(self)
        CasePixelThresholdA(self)
        CasePixelThresholdB(self)
        CaseColourMode(self)
        CaseCounterDepth(self)
        
################################################
# Intermediate test case class that provides some utility functions
# for this suite

class ExcaliburCase(TestCase):
    pixelsPerChipX = 256
    pixelsPerChipY = 256
    chipsPerStripeX = 8
    chipsPerStripeY = 1

    sim = SimBackdoor("localhost")
        
    def testRegister(self, p, chip, register, start, end, mpxRegister):
        '''Test a chip register PV.'''
        pv = "%s:1:MPXIII:%s:%s" % (p, chip, register);
        incr = 1
        if (end-start) > 16:
            incr = int((end-start)/16)
        for v in range(start, end+1, incr):
            self.putPv(pv, v)
            self.verifyPv(pv, v)
            simv = self.sim.GetRegister(mpxRegister, chip)
            self.verify(simv, v)

    dacNames = ["Threshd 0", "Threshd 1", "Threshd 2", "Threshd 3",
                "Threshd 4", "Threshd 5", "Threshd 6", "Threshd 7",
                "Preamp", "Ikrum", "Shaper", "Disc", "Disc_LS", "ThreshdN",
                "DAC_pixel", "Delay", "TP_BuffIn", "TP_BuffOut", "RPZ",
                "GND", "TP_REF", "FBK", "Cas", "TP_REFA", "TP_REFB"]
    def testDacSelectionRegister(self, p, chip, register, mpxRegister):
        '''Tests a chip DAC selection register.'''
        pv = "%s:1:MPXIII:%s:%s" % (p, chip, register);
        pvname = "%s:NAME" % pv
        for v in range(25):
            self.putPv(pv, v)
            self.verifyPv(pv, v)
            self.verifyPv(pvname, self.dacNames[v])
            simv = self.sim.GetRegister(mpxRegister, chip)
            self.verify(simv, v)
        
    def testWaveformRegister(self, p, chip, register, start, end, mpxRegister):
        '''Test a chip register waveform PV.'''
        pv = "%s:1:MPXIII:%s:%s" % (p, chip, register);
        incr = 1
        if (end-start) > 32:
            incr = int((end-start)/16)
        for v in range(start, end+1, incr):
            image = []
            for i in range(self.pixelsPerChipX*self.pixelsPerChipY):
                image.append(v)
            self.putPv(pv, image)
            rb = self.getPv(pv)
            if (rb != image).any():
                self.fail("%s[%s] != %s" % (pv, rb, image))
            simv = self.sim.GetPixelMap(mpxRegister, chip)
            if simv != image:
                self.fail("%s[%s] != %s" % (pv, simv, image))

    def testGlobalRegister(self, p, register, start, end, mpxRegister):
        '''Test a chip register PV.'''
        pv = "%s:1:MPXIII:%s" % (p, register);
        incr = 1
        if (end-start) > 16:
            incr = int((end-start)/16)
        for v in range(start, end+1, incr):
            self.putPv(pv, v)
            self.verifyPv(pv, v)
            for chip in range(self.chipsPerStripeX*self.chipsPerStripeY):
                simv = self.sim.GetRegister(mpxRegister, chip+1)
                self.verify(simv, v)
            
################################################
# Test cases
    
class CaseDacSense(ExcaliburCase):
    def runTest(self):
        '''The DAC sense register.'''
        for chip in range(self.chipsPerStripeX*self.chipsPerStripeY):
            self.testDacSelectionRegister("EXCALIBUR", chip+1, "DACSENSE", "dacsense")

class CaseDacExternal(ExcaliburCase):
    def runTest(self):
        '''The DAC external override register.'''
        for chip in range(self.chipsPerStripeX*self.chipsPerStripeY):
            self.testDacSelectionRegister("EXCALIBUR", chip+1, "DACEXTERNAL", "dacexternal")

class CaseThreshold0Dac(ExcaliburCase):
    def runTest(self):
        '''The threshold 0 DAC register.'''
        for chip in range(self.chipsPerStripeX*self.chipsPerStripeY):
            self.testRegister("EXCALIBUR", chip+1, "ANPER:THRESHOLD:0", 0, 512, "threshold0dac")

class CaseThreshold1Dac(ExcaliburCase):
    def runTest(self):
        '''The threshold 1 DAC register.'''
        for chip in range(self.chipsPerStripeX*self.chipsPerStripeY):
            self.testRegister("EXCALIBUR", chip+1, "ANPER:THRESHOLD:1", 0, 512, "threshold1dac")

class CaseThreshold2Dac(ExcaliburCase):
    def runTest(self):
        '''The threshold 2 DAC register.'''
        for chip in range(self.chipsPerStripeX*self.chipsPerStripeY):
            self.testRegister("EXCALIBUR", chip+1, "ANPER:THRESHOLD:2", 0, 512, "threshold2dac")

class CaseThreshold3Dac(ExcaliburCase):
    def runTest(self):
        '''The threshold 3 DAC register.'''
        for chip in range(self.chipsPerStripeX*self.chipsPerStripeY):
            self.testRegister("EXCALIBUR", chip+1, "ANPER:THRESHOLD:3", 0, 512, "threshold3dac")

class CaseThreshold4Dac(ExcaliburCase):
    def runTest(self):
        '''The threshold 4 DAC register.'''
        for chip in range(self.chipsPerStripeX*self.chipsPerStripeY):
            self.testRegister("EXCALIBUR", chip+1, "ANPER:THRESHOLD:4", 0, 512, "threshold4dac")

class CaseThreshold5Dac(ExcaliburCase):
    def runTest(self):
        '''The threshold 5 DAC register.'''
        for chip in range(self.chipsPerStripeX*self.chipsPerStripeY):
            self.testRegister("EXCALIBUR", chip+1, "ANPER:THRESHOLD:5", 0, 512, "threshold5dac")

class CaseThreshold6Dac(ExcaliburCase):
    def runTest(self):
        '''The threshold 6 DAC register.'''
        for chip in range(self.chipsPerStripeX*self.chipsPerStripeY):
            self.testRegister("EXCALIBUR", chip+1, "ANPER:THRESHOLD:6", 0, 512, "threshold6dac")

class CaseThreshold7Dac(ExcaliburCase):
    def runTest(self):
        '''The threshold 7 DAC register.'''
        for chip in range(self.chipsPerStripeX*self.chipsPerStripeY):
            self.testRegister("EXCALIBUR", chip+1, "ANPER:THRESHOLD:7", 0, 512, "threshold7dac")

class CasePreampDac(ExcaliburCase):
    def runTest(self):
        '''The preamp DAC register.'''
        for chip in range(self.chipsPerStripeX*self.chipsPerStripeY):
            self.testRegister("EXCALIBUR", chip+1, "ANPER:PREAMP", 0, 256, "preampdac")

class CaseIkrumDac(ExcaliburCase):
    def runTest(self):
        '''The ikrum DAC register.'''
        for chip in range(self.chipsPerStripeX*self.chipsPerStripeY):
            self.testRegister("EXCALIBUR", chip+1, "ANPER:IKRUM", 0, 256, "ikrumdac")

class CaseShaperDac(ExcaliburCase):
    def runTest(self):
        '''The shaper DAC register.'''
        for chip in range(self.chipsPerStripeX*self.chipsPerStripeY):
            self.testRegister("EXCALIBUR", chip+1, "ANPER:SHAPER", 0, 256, "shaperdac")

class CaseDiscDac(ExcaliburCase):
    def runTest(self):
        '''The disc DAC register.'''
        for chip in range(self.chipsPerStripeX*self.chipsPerStripeY):
            self.testRegister("EXCALIBUR", chip+1, "ANPER:DISC", 0, 256, "discdac")

class CaseDisclsDac(ExcaliburCase):
    def runTest(self):
        '''The discls DAC register.'''
        for chip in range(self.chipsPerStripeX*self.chipsPerStripeY):
            self.testRegister("EXCALIBUR", chip+1, "ANPER:DISCLS", 0, 256, "disclsdac")

class CaseThresholdnDac(ExcaliburCase):
    def runTest(self):
        '''The thresholdn DAC register.'''
        for chip in range(self.chipsPerStripeX*self.chipsPerStripeY):
            self.testRegister("EXCALIBUR", chip+1, "ANPER:THRESHOLDN", 0, 256, "thresholdndac")

class CaseDacpixelDac(ExcaliburCase):
    def runTest(self):
        '''The dacpixel DAC register.'''
        for chip in range(self.chipsPerStripeX*self.chipsPerStripeY):
            self.testRegister("EXCALIBUR", chip+1, "ANPER:DACPIXEL", 0, 256, "dacpixeldac")

class CaseDelayDac(ExcaliburCase):
    def runTest(self):
        '''The delay DAC register.'''
        for chip in range(self.chipsPerStripeX*self.chipsPerStripeY):
            self.testRegister("EXCALIBUR", chip+1, "ANPER:DELAY", 0, 256, "delaydac")

class CaseTpbufferinDac(ExcaliburCase):
    def runTest(self):
        '''The tpbufferin DAC register.'''
        for chip in range(self.chipsPerStripeX*self.chipsPerStripeY):
            self.testRegister("EXCALIBUR", chip+1, "ANPER:TPBUFFERIN", 0, 256, "tpbufferindac")

class CaseTpbufferoutDac(ExcaliburCase):
    def runTest(self):
        '''The tpbufferout DAC register.'''
        for chip in range(self.chipsPerStripeX*self.chipsPerStripeY):
            self.testRegister("EXCALIBUR", chip+1, "ANPER:TPBUFFEROUT", 0, 256, "tpbufferoutdac")

class CaseRpzDac(ExcaliburCase):
    def runTest(self):
        '''The rpz DAC register.'''
        for chip in range(self.chipsPerStripeX*self.chipsPerStripeY):
            self.testRegister("EXCALIBUR", chip+1, "ANPER:RPZ", 0, 256, "rpzdac")

class CaseGndDac(ExcaliburCase):
    def runTest(self):
        '''The gnd DAC register.'''
        for chip in range(self.chipsPerStripeX*self.chipsPerStripeY):
            self.testRegister("EXCALIBUR", chip+1, "ANPER:GND", 0, 256, "gnddac")

class CaseTprefDac(ExcaliburCase):
    def runTest(self):
        '''The tpref DAC register.'''
        for chip in range(self.chipsPerStripeX*self.chipsPerStripeY):
            self.testRegister("EXCALIBUR", chip+1, "ANPER:TPREF", 0, 256, "tprefdac")

class CaseFbkDac(ExcaliburCase):
    def runTest(self):
        '''The fbk DAC register.'''
        for chip in range(self.chipsPerStripeX*self.chipsPerStripeY):
            self.testRegister("EXCALIBUR", chip+1, "ANPER:FBK", 0, 256, "fbkdac")

class CaseCasDac(ExcaliburCase):
    def runTest(self):
        '''The cas DAC register.'''
        for chip in range(self.chipsPerStripeX*self.chipsPerStripeY):
            self.testRegister("EXCALIBUR", chip+1, "ANPER:CAS", 0, 256, "casdac")

class CaseTprefaDac(ExcaliburCase):
    def runTest(self):
        '''The tprefa DAC register.'''
        for chip in range(self.chipsPerStripeX*self.chipsPerStripeY):
            self.testRegister("EXCALIBUR", chip+1, "ANPER:TPREFA", 0, 256, "tprefadac")

class CaseTprefbDac(ExcaliburCase):
    def runTest(self):
        '''The tprefb DAC register.'''
        for chip in range(self.chipsPerStripeX*self.chipsPerStripeY):
            self.testRegister("EXCALIBUR", chip+1, "ANPER:TPREFB", 0, 256, "tprefbdac")

class CasePixelMask(ExcaliburCase):
    def runTest(self):
        '''The pixel mask.'''
        for chip in range(self.chipsPerStripeX*self.chipsPerStripeY):
            self.testWaveformRegister("EXCALIBUR", chip+1, "PIXEL:MASK", 0, 1, "pixelmask")

class CasePixelTest(ExcaliburCase):
    def runTest(self):
        '''The pixel test.'''
        for chip in range(self.chipsPerStripeX*self.chipsPerStripeY):
            self.testWaveformRegister("EXCALIBUR", chip+1, "PIXEL:TEST", 0, 1, "pixeltest")

class CasePixelGainMode(ExcaliburCase):
    def runTest(self):
        '''The pixel gain mode.'''
        for chip in range(self.chipsPerStripeX*self.chipsPerStripeY):
            self.testWaveformRegister("EXCALIBUR", chip+1, "PIXEL:GAINMODE", 0, 1, "pixelgainmode")

class CasePixelThresholdA(ExcaliburCase):
    def runTest(self):
        '''The pixel gain mode.'''
        for chip in range(self.chipsPerStripeX*self.chipsPerStripeY):
            self.testWaveformRegister("EXCALIBUR", chip+1, "PIXEL:THRESHOLDA", 0, 31, "pixelthresholda")

class CasePixelThresholdB(ExcaliburCase):
    def runTest(self):
        '''The pixel gain mode.'''
        for chip in range(self.chipsPerStripeX*self.chipsPerStripeY):
            self.testWaveformRegister("EXCALIBUR", chip+1, "PIXEL:THRESHOLDB", 0, 31, "pixelthresholdb")

class CaseColourMode(ExcaliburCase):
    def runTest(self):
        '''The colour mode register.'''
        self.testGlobalRegister("EXCALIBUR", "COLOURMODE", 0, 1, "colourmode")

class CaseCounterDepth(ExcaliburCase):
    def runTest(self):
        '''The colour mode register.'''
        self.testGlobalRegister("EXCALIBUR", "COUNTERDEPTH", 0, 3, "counterdepth")

################################################
# Main entry point

if __name__ == "__main__":
    # Create and run the test sequence
    ChipRegistersTestSuite()

    
