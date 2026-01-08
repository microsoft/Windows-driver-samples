/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    powerlimitclient_drvinterface.h

Abstract:

    This module contains the interfaces used to communicate with the simulate
    power limit client driver stack.

--*/

//--------------------------------------------------------------------- Pragmas

#pragma once

//--------------------------------------------------------------------- Defines

//
// IOCTLs to control the client driver
//

#define POWERLIMITCLIENT_IOCTL(_index_) \
    CTL_CODE(FILE_DEVICE_UNKNOWN, _index_, METHOD_BUFFERED, FILE_WRITE_DATA)

//
// IOCTL_POWERLIMIT_CLIENT_QUERY_LIMIT_COUNT
// - Output: ULONG, number of supported power limit parameters.
//

#define IOCTL_POWERLIMIT_CLIENT_QUERY_LIMIT_COUNT         POWERLIMITCLIENT_IOCTL(0x800)

//
// IOCTL_POWERLIMIT_CLIENT_QUERY_ATTRIBUTES
// - Output: POWER_LIMIT_ATTRIBUTES[], attributes of supported power limit parameters.
//

#define IOCTL_POWERLIMIT_CLIENT_QUERY_ATTRIBUTES          POWERLIMITCLIENT_IOCTL(0x801)

//
// IOCTL_POWERLIMIT_CLIENT_QUERY_LIMITS
// - Output: POWER_LIMIT_VALUE[], values of supported power limit parameters.
//

#define IOCTL_POWERLIMIT_CLIENT_QUERY_LIMITS              POWERLIMITCLIENT_IOCTL(0x802)

//
// Each domain supports PowerLimitContinuous/Burst/BurstTimeParameter.
//

#define PLCLIENT_DEFAULT_LIMIT_COUNT_PER_DOMAIN          3UL
#define PLCLIENT_DEFAULT_DOMAIN_COUNT                    2UL
#define PLCLIENT_DEFAULT_LIMIT_COUNT                     PLCLIENT_DEFAULT_LIMIT_COUNT_PER_DOMAIN * PLCLIENT_DEFAULT_DOMAIN_COUNT
#define PLCLIENT_DEFAULT_MAX_VALUE                       50000UL
#define PLCLIENT_DEFAULT_MIN_VALUE                       1000UL
