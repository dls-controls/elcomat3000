#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsMutex.h>
#include <epicsString.h>
#include <epicsStdio.h>
#include <epicsMutex.h>
#include <epicsExport.h>
#include <iocsh.h>
#include "elcomat3000.h"
#include "asynOctetSyncIO.h"
#include "asynCommonSyncIO.h"

static const char *driverName = "elcomat3000";

/* Create the parameter name strings to be used in records INP/OUT fields to map records to specific parameters */
#define elcomat3000_averageSize_str    "AVERAGESIZE"
#define elcomat3000_samplePeriod_str   "SAMPLEPERIOD"
#define elcomat3000_connected_str      "CONNECTED"
#define elcomat3000_xValue_str         "XVALUE"
#define elcomat3000_yValue_str         "YVALUE"
#define elcomat3000_xStdDev_str        "XSTDDEV"
#define elcomat3000_yStdDev_str        "YSTDDEV"
#define elcomat3000_serialNumber_str   "SERIALNUMBER"
#define elcomat3000_day_str            "DAY"
#define elcomat3000_month_str          "MONTH"
#define elcomat3000_year_str           "YEAR"
#define elcomat3000_focalLength_str    "FOCALLENGTH"
#define elcomat3000_running_str        "RUNNING"
#define elcomat3000_xSamples_str       "XSAMPLES"
#define elcomat3000_ySamples_str       "YSAMPLES"
#define elcomat3000_xInvalid_str       "XINVALID"
#define elcomat3000_yInvalid_str       "YINVALID"
#define elcomat3000_go_str             "GO"
#define elcomat3000_stop_str           "STOP"
#define elcomat3000_streamingMode_str  "STREAMING_MODE"
#define NUM_PARAMS (&LAST_PARAM - &FIRST_PARAM - 1)

/* String terminators */
#define INPUT_EOS "\r"
#define OUTPUT_EOS "\n"

/* arc seconds to micro radians conversion factor */
#define ARCSECONDTOURAD 4.84813681

/*
 * Constructor
 */
elcomat3000::elcomat3000(const char* portName, const char* serialPortName, int serialPortAddress) 
    : asynPortDriver(portName, 1 /*maxAddr*/, NUM_PARAMS,
        asynInt32Mask | asynFloat64Mask | asynDrvUserMask, 
        asynInt32Mask | asynFloat64Mask, 
        0, /*ASYN_CANBLOCK=0, ASYN_MULTIDEVICE=0 */
        1, /*autoConnect*/
        0, /*default priority */
        0) /*default stack size*/
    , thread(*this, "rxthread", epicsThreadGetStackSize(epicsThreadStackMedium))
{
    /* create the parameters */
    createParam(elcomat3000_averageSize_str, asynParamInt32, &index_averageSize);
    createParam(elcomat3000_connected_str, asynParamInt32, &index_connected);
    createParam(elcomat3000_xValue_str, asynParamFloat64, &index_xValue);
    createParam(elcomat3000_yValue_str, asynParamFloat64, &index_yValue);
    createParam(elcomat3000_xStdDev_str, asynParamFloat64, &index_xStdDev);
    createParam(elcomat3000_yStdDev_str, asynParamFloat64, &index_yStdDev);
    createParam(elcomat3000_samplePeriod_str, asynParamFloat64, &index_samplePeriod);
    createParam(elcomat3000_serialNumber_str, asynParamInt32, &index_serialNumber);
    createParam(elcomat3000_day_str, asynParamInt32, &index_day);
    createParam(elcomat3000_month_str, asynParamInt32, &index_month);
    createParam(elcomat3000_year_str, asynParamInt32, &index_year);
    createParam(elcomat3000_focalLength_str, asynParamInt32, &index_focalLength);
    createParam(elcomat3000_running_str, asynParamInt32, &index_running);
    createParam(elcomat3000_xSamples_str, asynParamInt32, &index_xSamples);
    createParam(elcomat3000_ySamples_str, asynParamInt32, &index_ySamples);
    createParam(elcomat3000_xInvalid_str, asynParamInt32, &index_xInvalid);
    createParam(elcomat3000_yInvalid_str, asynParamInt32, &index_yInvalid);
    createParam(elcomat3000_go_str, asynParamInt32, &index_go);
    createParam(elcomat3000_stop_str, asynParamInt32, &index_stop);
    createParam(elcomat3000_streamingMode_str, asynParamInt32, &index_streamingMode);
   
    /* set some default values */
    setIntegerParam(index_averageSize, 10);
    setIntegerParam(index_connected, 0);
    setDoubleParam(index_xValue, 0.0);
    setDoubleParam(index_yValue, 0.0);
    setDoubleParam(index_xStdDev, 0.0);
    setDoubleParam(index_yStdDev, 0.0);
    setDoubleParam(index_samplePeriod, 0.1);
    setIntegerParam(index_serialNumber, 0);
    setIntegerParam(index_day, 0);
    setIntegerParam(index_month, 0);
    setIntegerParam(index_year, 0);
    setIntegerParam(index_focalLength, 0);
    setIntegerParam(index_running, 0);
    setIntegerParam(index_xSamples, 0);
    setIntegerParam(index_ySamples, 0);
    setIntegerParam(index_xInvalid, 0);
    setIntegerParam(index_yInvalid, 0);
    setIntegerParam(index_go, 0);
    setIntegerParam(index_stop, 0);
    setIntegerParam(index_streamingMode, 0);

    /* Initialise averaging parameters */
    currentSumX = 0.0;
    currentSumY = 0.0;
    currentSamples = 0;

    /* Record serial port information */
    strcpy(this->serialPortName, serialPortName);
    this->serialPortAddress = serialPortAddress;

    /* Start the thread */
    thread.start();
}

/*
 * Destructor
 */
elcomat3000::~elcomat3000()
{
}

/*
 * Override of the thread run virtual.  This is the function
 * that will be run for the thread.  Receive data from the device.
 */
void elcomat3000::run()
{
    size_t nwrite;
    size_t nread;
    int eomReason;
    char rxBuffer[200];
    asynStatus status;

    /* Connect to the serial port */
    asynStatus asynRtn = pasynOctetSyncIO->connect(serialPortName, serialPortAddress, &serialPortUser, NULL);
    if(asynRtn != asynSuccess) 
    {
        printf("Failed to connect to serial port=%s error=%d\n",
            serialPortName, asynRtn);
    }
    else
    {
        lock();
        setIntegerParam(index_connected, 1);
        callParamCallbacks();
        unlock();
    }
    pasynOctetSyncIO->setInputEos(serialPortUser, INPUT_EOS, strlen(INPUT_EOS));
    pasynOctetSyncIO->setOutputEos(serialPortUser, OUTPUT_EOS, strlen(OUTPUT_EOS));
    pasynOctetSyncIO->write(serialPortUser, "s", 1, 3.0, &nwrite);
    pasynOctetSyncIO->flush(serialPortUser);

    /* Read device configuration */
    status = pasynOctetSyncIO->writeRead(serialPortUser, "d", 1, rxBuffer, 199, 3.0, &nwrite, &nread, &eomReason);
    if(status == asynSuccess)
    {
        lock();
        parseData(rxBuffer);
        callParamCallbacks();
        unlock();
    }
    else
    {
        printf("Configuration read failed\n");
    }
    
    bool going = true;
    while(going)
    {
        /* Get operating mode information */
        int running;
        getIntegerParam(index_running, &running);
        int streamingMode;
        getIntegerParam(index_streamingMode, &streamingMode);
        /* Are we streaming? */
        /* This is actually a bit of a yukky wait when we are not running.  There should be
           a semaphore to wait on instead. */
        if(!running || !streamingMode)
        {
            /* Wait for the next sample */
            double samplePeriod;
            getDoubleParam(index_samplePeriod, &samplePeriod);
            epicsThreadSleep(samplePeriod);
        }
        /* Are we running? */
        if(running)
        {
            /* Get a sample */
            if(streamingMode)
            {
                if(currentSamples == 0)
                {
                    /* If this is the first sample, start the streaming mode */
                    status = pasynOctetSyncIO->writeRead(serialPortUser, "A", 1, rxBuffer, 199, 3.0, &nwrite, &nread, &eomReason);
                }
                else
                {
                    /* Otherwise, just read the next message */
                    status = pasynOctetSyncIO->read(serialPortUser, rxBuffer, 199, 3.0, &nread, &eomReason);
                }
            }
            else
            {
                /* Trigger the next sample */
                status = pasynOctetSyncIO->writeRead(serialPortUser, "a", 1, rxBuffer, 199, 3.0, &nwrite, &nread, &eomReason);
            }
            /* Process the sample */
            if(status == asynSuccess)
            {
                lock();
                parseData(rxBuffer);
                callParamCallbacks();
                unlock();
            }
            else
            {
                printf("Sample read failed\n");
            }
            /* Terminate the streaming mode if we have collected enough samples */
            if(streamingMode)
            {
                getIntegerParam(index_running, &running);
                if(!running)
                {
                    pasynOctetSyncIO->write(serialPortUser, "s", 1, 3.0, &nwrite);
                    pasynOctetSyncIO->flush(serialPortUser);
                }
            }
        }
    }
}

bool elcomat3000::getToken(const char* text, size_t* pos, char* token, size_t maxTokenLen)
{
    bool result = false;
    size_t tokenPos = 0;
    enum {SKIPPING, ADDING, DONE} state = SKIPPING;
    while(state != DONE && *pos < strlen(text))
    {
        char ch = text[*pos];
        (*pos)++;
        if(ch > ' ')
        {
            if(tokenPos < (maxTokenLen - 1))
            {
                token[tokenPos] = ch;
                tokenPos++;
                result = true;
            }
            state = ADDING;
        }
        else if(state == ADDING)
        {
            state = DONE;
            (*pos)--;
        }
        else if(state == SKIPPING)
        {
        }
    }
    token[tokenPos] = '\0';
    //printf("{%s}", token);
    return result;
}

void elcomat3000::parseData(const char* text)
{
    size_t pos = 0;
    char token[200];
    char* endptr;
    //printf("Parsing: \"%s\"\n", text);
    /* Get the message type */
    if(getToken(text, &pos, token, 200))
    {
        if(strcmp(token, "8") == 0)
        {
            /* The device information message */
            /* Get the serial number */
            if(getToken(text, &pos, token, 200))
            {
                setIntegerParam(index_serialNumber, atoi(token));
            }
            /* Get the day */
            if(getToken(text, &pos, token, 200))
            {
                setIntegerParam(index_day, atoi(token));
            }
            /* Get the month */
            if(getToken(text, &pos, token, 200))
            {
                setIntegerParam(index_month, atoi(token));
            }
            /* Get the year */
            if(getToken(text, &pos, token, 200))
            {
                setIntegerParam(index_year, atoi(token));
            }
            /* Get the focal length */
            if(getToken(text, &pos, token, 200))
            {
                setIntegerParam(index_focalLength, atoi(token));
            }
        }
        else
        {
            /* All other messages have the same format */
            double x;
            double y;
            bool xValid = false;
            bool yValid = false;
            /* Get the status code */
            if(getToken(text, &pos, token, 200))
            {
                /* Handle the status codes */
                //printf("Status = %s\n", token);
                if(strlen(token) >= 3)
                {
                    if(token[2] == '1')
                    {
                        xValid = true;
                    }
                    if(token[2] == '2')
                    {
                        yValid = true;
                    }
                    if(token[2] == '3')
                    {
                        xValid = true;
                        yValid = true;
                    }
      
                }
            }
            /* Get the x value */
            if(getToken(text, &pos, token, 200))
            {
                x = strtod(token, &endptr) * ARCSECONDTOURAD;
                /* Get the y value */
                if(*endptr == '\0' && getToken(text, &pos, token, 200))
                {
                    y = strtod(token, &endptr) * ARCSECONDTOURAD;
                    if(*endptr == '\0')
                    {
                        int xSamples;
                        int ySamples;
                        getIntegerParam(index_xSamples, &xSamples);
                        getIntegerParam(index_ySamples, &ySamples);
                        if(xValid)
                        {
                            currentSumX += x;
                            currentSumXSquared += x*x;
                            xSamples++;
                            setIntegerParam(index_xSamples, xSamples);
                        }
                        else
                        {
                            int xInvalid;
                            getIntegerParam(index_xInvalid, &xInvalid);
                            xInvalid++;
                            setIntegerParam(index_xInvalid, xInvalid);
                        }
                        if(yValid)
                        {
                            currentSumY += y;
                            currentSumYSquared += y*y;
                            ySamples++;
                            setIntegerParam(index_ySamples, ySamples);
                        }
                        else
                        {
                            int yInvalid;
                            getIntegerParam(index_yInvalid, &yInvalid);
                            yInvalid++;
                            setIntegerParam(index_yInvalid, yInvalid);
                        }
                        currentSamples++;
                        int averageSize;
                        getIntegerParam(index_averageSize, &averageSize);
                        if(currentSamples >= averageSize)
                        {
                            if(averageSize > 0)
                            {
                                setDoubleParam(index_xValue, currentSumX / xSamples);
                                setDoubleParam(index_yValue, currentSumY / ySamples);
                                setDoubleParam(index_xStdDev, sqrt(currentSumXSquared/xSamples - 
                                          (currentSumX*currentSumX) / (xSamples*xSamples) ) );
                                setDoubleParam(index_yStdDev, sqrt(currentSumYSquared/ySamples - 
                                          (currentSumY*currentSumY) / (ySamples*ySamples) ) );
                            }
                            setIntegerParam(index_running, 0);
                        }
                    }
                }
            }
        }
    }
    //printf("\n");
}

/*
 * Called by asynManager to pass a pasynUser structure and drvInfo string to the driver
 */
asynStatus elcomat3000::drvUserCreate(asynUser* pasynUser, const char* drvInfo,
    const char** pptypeName, size_t* psize)
{
    /* Base class does most of the work, including writing to the parameter library and doing call backs */
    asynStatus status = asynPortDriver::drvUserCreate(pasynUser, drvInfo, pptypeName, psize);
    return status;
}

/** Called when asyn clients call pasynInt32->write().
  * This function performs actions for some parameters
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus elcomat3000::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    /* Base class does most of the work, including writing to the parameter library and doing call backs */
    asynStatus status = asynPortDriver::writeInt32(pasynUser, value);

    /* Any work we need to do */
    int parameter = pasynUser->reason;
    if(parameter == index_averageSize)
    {
        /* Restart the average */
        currentSumX = 0.0;
        currentSumY = 0.0;
        currentSamples = 0;
    }
    if(parameter == index_go)
    {
        int go;
        getIntegerParam(index_go, &go);
        if(go)
        {
            setIntegerParam(index_running, 1);
            setIntegerParam(index_xSamples, 0);
            setIntegerParam(index_ySamples, 0);
            setIntegerParam(index_xInvalid, 0);
            setIntegerParam(index_yInvalid, 0);
            currentSumX = 0.0;
            currentSumY = 0.0;
            currentSumXSquared = 0.0;
            currentSumYSquared = 0.0;
            currentSamples = 0;
            callParamCallbacks();
        }
    }
    if(parameter == index_stop)
    {
        int stop;
        getIntegerParam(index_stop, &stop);
        if(stop)
        {
            setIntegerParam(index_running, 0);
            callParamCallbacks();
        }
    }
    return status;
}

/** Configuration command, called directly or from iocsh */
extern "C" int elcomat3000Config(const char *portName, const char* serialPortName, int serialPortAddress) 
{
    new elcomat3000(portName, serialPortName, serialPortAddress);
    return(asynSuccess);
}

/** Code for iocsh registration */
static const iocshArg elcomat3000ConfigArg0 = {"Port name", iocshArgString};
static const iocshArg elcomat3000ConfigArg1 = {"Serial port name", iocshArgString};
static const iocshArg elcomat3000ConfigArg2 = {"Serial port address", iocshArgInt};
static const iocshArg* const elcomat3000ConfigArgs[] =  {&elcomat3000ConfigArg0, &elcomat3000ConfigArg1, &elcomat3000ConfigArg2};
static const iocshFuncDef configelcomat3000 = {"elcomat3000Config", 3, elcomat3000ConfigArgs};
static void configelcomat3000CallFunc(const iocshArgBuf *args)
{
    elcomat3000Config(args[0].sval, args[1].sval, args[2].ival);
}

static void elcomat3000Register(void)
{
    iocshRegister(&configelcomat3000, configelcomat3000CallFunc);
}

extern "C" { epicsExportRegistrar(elcomat3000Register); }


