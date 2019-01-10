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
#define DEFAULT_FIX_INTERVAL_SECONDS   1

static const double c_MaximumLatitude = 90.0;
static const double c_MinimumLatitude = -90.0;
static const double c_MaximumLongitude = 180.0;
static const double c_MinimumLongitude = -180.0;

#define INIT_POD_GNSS_STRUCT(pName) \
    memset((pName), 0, sizeof(*(pName))); \
    (pName)->Size = sizeof(*(pName)); \
    (pName)->Version = GNSS_DRIVER_DDK_VERSION

const GNSS_FIXDATA g_DefaultGnssFixData =
{
    sizeof(GNSS_FIXDATA),
    GNSS_DRIVER_DDK_VERSION,
    INVALID_SESSION_ID,
    { 0xCD700000, 0x1D48908 }, // an example date/time, Dec/01/2018
    TRUE,
    STATUS_SUCCESS,
    GNSS_FIXDETAIL_BASIC | GNSS_FIXDETAIL_ACCURACY,
    {
        sizeof(GNSS_FIXDATA_BASIC),
        GNSS_DRIVER_DDK_VERSION,
        // Location: Mount Rainier National Park
        46.852273,
        -121.757468,
        0.0,
        0.0,
        0.0
    },
    {
        sizeof(GNSS_FIXDATA_ACCURACY),
        GNSS_DRIVER_DDK_VERSION,
        10 // Meters accuracy
    }
};
