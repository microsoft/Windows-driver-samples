//Copyright (C) Microsoft Corporation, All Rights Reserved
//
//Abstract:
//
//      This module contains the constant definitions for the accelerometer's
//      register interface and default property values.

#pragma once

#include "WTypesbase.h"



// Register interface
#define ADXL345_DEVID                       0x00
#define ADXL345_THRESH_TAP                  0x1D
#define ADXL345_OFFSET_X                    0x1E
#define ADXL345_OFFSET_Y                    0x1F
#define ADXL345_OFFSET_Z                    0x20
#define ADXL345_DURATION_TAP                0x21
#define ADXL345_LATENT_TAP                  0x22
#define ADXL345_WINDOW_TAP                  0x23
#define ADXL345_THRESH_ACT                  0x24
#define ADXL345_THRESH_INACT                0x25
#define ADXL345_TIME_INACT                  0x26
#define ADXL345_ACT_INACT_CTL               0x27
#define ADXL345_THRESH_FF                   0x28
#define ADXL345_TIME_FF                     0x29
#define ADXL345_TAP_AXES                    0x2A
#define ADXL345_ACT_TAP_STATUS              0x2B
#define ADXL345_BW_RATE                     0x2C
#define ADXL345_POWER_CTL                   0x2D
#define ADXL345_INT_ENABLE                  0x2E
#define ADXL345_INT_MAP                     0x2F
#define ADXL345_INT_SOURCE                  0x30
#define ADXL345_DATA_FORMAT                 0x31
#define ADXL345_DATA_X0                     0x32
#define ADXL345_DATA_X1                     0x33
#define ADXL345_DATA_Y0                     0x34
#define ADXL345_DATA_Y1                     0x35
#define ADXL345_DATA_Z0                     0x36
#define ADXL345_DATA_Z1                     0x37
#define ADXL345_FIFO_CTL                    0x38
#define ADXL345_FIFO_STATUS                 0x39

// ACT_INACT_CTL register bits
#define ADXL345_ACT_INACT_CTL_ACT_ACDC      0x80
#define ADXL345_ACT_INACT_CTL_ACT_X         0x40
#define ADXL345_ACT_INACT_CTL_ACT_Y         0x20
#define ADXL345_ACT_INACT_CTL_ACT_Z         0x10
#define ADXL345_ACT_INACT_CTL_INACT_ACDC    0x08
#define ADXL345_ACT_INACT_CTL_INACT_X       0x04
#define ADXL345_ACT_INACT_CTL_INACT_Y       0x02
#define ADXL345_ACT_INACT_CTL_INACT_Z       0x01

// POWER_CTL register bits
#define ADXL345_POWER_CTL_STANDBY           0x00
#define ADXL345_POWER_CTL_MEASURE           0x08

// INT_ENABLE | MAP | SOURCE register bits
#define ADXL345_INT_MASK                    0xFF
#define ADXL345_INT_DATA_READY              0x80
#define ADXL345_INT_SINGLE_TAP              0x40
#define ADXL345_INT_DOUBLE_TAP              0x20
#define ADXL345_INT_ACTIVITY                0x10
#define ADXL345_INT_INACTIVITY              0x08
#define ADXL345_INT_FREEFALL                0x04
#define ADXL345_INT_WATERMARK               0x02
#define ADXL345_INT_OVERRUN                 0x01

// DATA_FORMAT register bits
#define ADXL345_DATA_FORMAT_FULL_RES        0x08
#define ADXL345_DATA_FORMAT_JUSTIFY_RIGHT   0x00
#define ADXL345_DATA_FORMAT_JUSTIFY_LEFT    0x04
#define ADXL345_DATA_FORMAT_RANGE_MASK      0x03
#define ADXL345_DATA_FORMAT_RANGE_2G        0x00
#define ADXL345_DATA_FORMAT_RANGE_4G        0x01
#define ADXL345_DATA_FORMAT_RANGE_8G        0x02
#define ADXL345_DATA_FORMAT_RANGE_16G       0x03

// DATA register bits - the data report includes adjacent data registers, for a total of 6 bytes
#define ADXL345_DATA_REPORT_SIZE_BYTES      6

// FIFO_CTL register bits
#define ADXL345_FIFO_CTL_MODE_MASK          0xC0
#define ADXL345_FIFO_CTL_MODE_BYPASS        0x00
#define ADXL345_FIFO_CTL_MODE_FIFO          0x40
#define ADXL345_FIFO_CTL_MODE_STREAM        0x80
#define ADXL345_FIFO_CTL_MODE_TRIGGER       0xC0

// Bus address
const unsigned short SENSOR_ACCELEROMETER_BUS_ADDRESS[] = L"1D";

// Default property values
typedef struct _DATA_RATE
{
    ULONG  DataRateInterval;
    BYTE   RateCode;
} DATA_RATE, *PDATA_RATE;

const unsigned short SENSOR_ACCELEROMETER_NAME[] = L"Accelerometer";
const unsigned short SENSOR_ACCELEROMETER_DESCRIPTION[] = L"Accelerometer Sensor";
const unsigned short SENSOR_ACCELEROMETER_ID[] = L"ADXL345";
const unsigned short SENSOR_ACCELEROMETER_MANUFACTURER[] = L"Analog Devices";
const unsigned short SENSOR_ACCELEROMETER_MODEL[] = L"ADXL345";
const unsigned short SENSOR_ACCELEROMETER_SERIAL_NUMBER[] = L"0123456789=0123456789";

const DOUBLE ACCELEROMETER_MIN_ACCELERATION_G = -16.0;
const DOUBLE ACCELEROMETER_MAX_ACCELERATION_G = 16.0;
const DOUBLE ACCELEROMETER_RESOLUTION_ACCELERATION_G = 0.004;

const ULONG ACCELEROMETER_MIN_REPORT_INTERVAL = 10;
const ULONG DEFAULT_ACCELEROMETER_REPORT_INTERVAL = 100;
const ULONG CURRENT_REPORT_INTERVAL_NOT_SET = 0;
const DATA_RATE ACCELEROMETER_SUPPORTED_DATA_RATES[] = { { 160, 0x06 }, { 80, 0x07 }, { 40, 0x08 }, { 20, 0x09 }, { 10, 0x0A } };
const ULONG ACCELEROMETER_SUPPORTED_DATA_RATES_COUNT = ARRAYSIZE(ACCELEROMETER_SUPPORTED_DATA_RATES);

const DOUBLE ACCELEROMETER_CHANGE_SENSITIVITY_RESOLUTION = 0.0625;
const DOUBLE ACCELEROMETER_MIN_CHANGE_SENSITIVITY = 0.0625;
const DOUBLE ACCELEROMETER_MAX_CHANGE_SENSITIVITY = 16.0;
const DOUBLE ACCELEROMETER_DEFAULT_AXIS_THRESHOLD = ACCELEROMETER_MIN_CHANGE_SENSITIVITY / ACCELEROMETER_CHANGE_SENSITIVITY_RESOLUTION;
const DOUBLE DEFAULT_ACCELEROMETER_CHANGE_SENSITIVITY = 0.0625;
const DOUBLE CHANGE_SENSITIVITY_NOT_SET = -1.0;