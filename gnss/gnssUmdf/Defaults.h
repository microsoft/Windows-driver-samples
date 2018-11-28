/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    defaults.h

Abstract:

    This module contains the defaults used within the Umdf Gnss Driver.

Environment:

    driver and application

--*/

#pragma once

#define GNSS_DRIVER_DDK_VERSION GNSS_DRIVER_VERSION_4
#define INVALID_SESSION_ID (0xFFFFFFFF)
#define DEFAULT_FIX_INTERVAL_SECONDS   1 // 1 sec

static const double c_MaximumLatitude = 90.0;
static const double c_MinimumLatitude = -90.0;
static const double c_MaximumLongitude = 180.0;
static const double c_MinimumLongitude = -180.0;

const GNSS_FIXDATA g_DefaultGnssFixData =
{
    sizeof(GNSS_FIXDATA),
    GNSS_DRIVER_DDK_VERSION,
    INVALID_SESSION_ID,
    { 0x299a64f0, 0x01d3120f },  // an example date, 8/10/2017
    TRUE,
    STATUS_SUCCESS,
    GNSS_FIXDETAIL_BASIC | GNSS_FIXDETAIL_ACCURACY | GNSS_FIXDETAIL_SATELLITE,
    {
        sizeof(GNSS_FIXDATA_BASIC),
        GNSS_DRIVER_DDK_VERSION,
        45.519653,  // an example location, Portland OR, USA
        -122.667703,
        0.0,
        0.0,
        0.0
    },
    {
        sizeof(GNSS_FIXDATA_ACCURACY),
        GNSS_DRIVER_DDK_VERSION,
        10 // Meters accuracy
    },
    {
        sizeof(GNSS_FIXDATA_SATELLITE),
        GNSS_DRIVER_DDK_VERSION,
        4, // satellite count
        {
            { 1, TRUE, 0, 0, 10 },
            { 2, FALSE, 0, 0, 20 },
            { 3, FALSE, 0, 0, 30 },
            { 4, FALSE, 0, 0, 40 },
        }
    }
};
