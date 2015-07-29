#pragma once

#include <windows.h>
#include <initguid.h>
#include <wdf.h>
#include <ntintsafe.h>

#include "PointOfServiceCommonTypes.h"
#include "PointOfServiceDriverInterface.h"

#include "PosCx.h"

#include <new>

#ifdef __cplusplus
extern "C" {
#endif
DRIVER_INITIALIZE DriverEntry;
#ifdef __cplusplus
}
#endif

#define SCANNER_INTERFACE_TAG ((ULONG) '0SCB')
