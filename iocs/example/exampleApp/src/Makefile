TOP=../..

include $(TOP)/configure/CONFIG

# ------------------------
# Build an IOC Application
# ------------------------

PROD_IOC = example

# sim.dbd will be installed into <top>/dbd
DBD += example.dbd

# sim.dbd will be created from these files
example_DBD += base.dbd
example_DBD += drvAsynSerialPort.dbd
example_DBD += drvAsynIPPort.dbd
example_DBD += asyn.dbd
example_DBD += elcomat3000.dbd

# sim_registerRecordDeviceDriver.cpp will be created
# sim.dbd
example_SRCS += example_registerRecordDeviceDriver.cpp

# These two lines are needed for non-vxWorks builds, such as Linux
example_SRCS_DEFAULT += exampleMain.cpp
example_SRCS_vxWorks += -nil-

# xxx Support Module
example_LIBS += asyn
example_LIBS += elcomat3000

# We need to link this IOC Application against the EPICS Base libraries
example_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES

