TOP=../..

include $(TOP)/configure/CONFIG

# -------------------------------
# Build an Diamond Support Module
# -------------------------------

#LIBRARY_IOC += xxx

# xxxRecord.h will be created from xxxRecord.dbd
#DBDINC += xxx.h

# xxxRecord.dbd will be installed into <top>/dbd
#DBD += xxx.dbd

# The following are compiled and added to the support library
#xxx_SRCS += xxxCodeA.c
#xxx_SRCS += xxxCodeB.c

# We need to link against the EPICS Base libraries
#xxx_LIBS += $(EPICS_BASE_IOC_LIBS)


# ------------------------
# Build an IOC Application
# ------------------------

PROD_IOC = elcomat3000

# elcomat3000.dbd will be installed into <top>/dbd
DBD += elcomat3000.dbd

# elcomat3000.dbd will be created from these files
elcomat3000_DBD += base.dbd
#elcomat3000_DBD += xxx.dbd

# elcomat3000_registerRecordDeviceDriver.cpp will be created
# elcomat3000.dbd
elcomat3000_SRCS += elcomat3000_registerRecordDeviceDriver.cpp

# These two lines are needed for non-vxWorks builds, such as Linux
elcomat3000_SRCS_DEFAULT += elcomat3000Main.cpp
elcomat3000_SRCS_vxWorks += -nil-

# Add locally compiled object code
#elcomat3000_SRCS +=

# The following adds object code from base/src/vxWorks
elcomat3000_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# This line says that this IOC Application depends on the
# xxx Support Module
#elcomat3000_LIBS += xxx

# We need to link this IOC Application against the EPICS Base libraries
elcomat3000_LIBS += $(EPICS_BASE_IOC_LIBS)

# ---------------------------------------------------

# NOTE: To build SNL programs, SNCSEQ must be defined
# in the <top>/configure/RELEASE file
#
# The difference between a file with an "st" extension
# and one with an "stt" extension is:
# The "st" extension files get passed through the C preprocessor
# whereas the "stt" files do not.

#ifneq ($(SNCSEQ),)
    # This builds sncExample as a component of elcomat3000
#    elcomat3000_SNCFLAGS += +r
#    elcomat3000_DBD      += sncExample.dbd
#    elcomat3000_SRCS     += sncExample.stt
#    elcomat3000_LIBS     += seq pv

    # The following builds sncExample as a standalone application
#    PROD_HOST           += sncExample
#    sncExample_SNCFLAGS += +m
#    sncExample_SRCS     += sncExample.stt
#    sncExample_LIBS     += seq pv
    # Here, we need to specify the dependency on EPICS Base libraries
    # again because this is a standalone application and
    # not part of elcomat3000
#    sncExample_LIBS     += $(EPICS_BASE_HOST_LIBS)
#endif

include $(TOP)/configure/RULES
