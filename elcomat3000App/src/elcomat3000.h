/*
 * elcomat3000.h
 *
 *  Created on: 21 Jul 2011
 *      Author: fgz73762
 */

#ifndef ELCOMAT3000_H_
#define ELCOMAT3000_H_

#include <epicsEvent.h>
#include "asynPortDriver.h"
#include "epicsThread.h"

class elcomat3000 : public asynPortDriver, public epicsThreadRunable 
{
public:
    elcomat3000(const char *portName, const char* serialPortName, int serialPortAddress);
    virtual ~elcomat3000();

    /* These are the methods that we override from asynPortDriver */
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    virtual asynStatus drvUserCreate(asynUser* pasynUser, const char* drvInfo,
        const char** pptypeName, size_t* psize);

    /* These are the methods that we override from epicsThreadRunable */
    virtual void run();

    /* These are the methods new to the class */
    void parseData(const char* text);
    bool getToken(const char* text, size_t* pos, char* token, size_t maxTokenLen);

protected:
    /* Our data */
    double currentSumX;
    double currentSumY;
    double currentSumXSquared;
    double currentSumYSquared;
    int currentSamples;
    char serialPortName[80];
    int serialPortAddress;
    epicsThread thread;
    asynUser* serialPortUser;

    /* Parameter indices */
    int FIRST_PARAM;
    int index_averageSize;
    int index_connected;
    int index_xValue;
    int index_yValue;
    int index_xStdDev;
    int index_yStdDev;
    int index_samplePeriod;
    int index_serialNumber;
    int index_day;
    int index_month;
    int index_year;
    int index_focalLength;
    int index_running;
    int index_xSamples;
    int index_ySamples;
    int index_xInvalid;
    int index_yInvalid;
    int index_go;
    int index_stop;
    int index_streamingMode;
    int LAST_PARAM;
};


#endif /* ELCOMAT3000_H_ */
