////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Framework_PowerStates.h
//
//   Abstract:
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef FRAMEWORK_POWER_STATES_H
#define FRAMEWORK_POWER_STATES_H

VOID UnregisterPowerStateChangeCallback(_Inout_ DEVICE_EXTENSION* pDeviceExtension);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS RegisterPowerStateChangeCallback(_Inout_ DEVICE_EXTENSION* pDeviceExtension);

#endif /// FRAMEWORK_POWER_STATES_H