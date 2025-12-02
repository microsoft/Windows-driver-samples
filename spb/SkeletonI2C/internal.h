/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name: 

    internal.h

Abstract:

    This module contains the common internal type and function
    definitions for the SPB controller driver.

Environment:

    kernel-mode only

Revision History:

--*/

#ifndef _INTERNAL_H_
#define _INTERNAL_H_

#pragma warning(push)
#pragma warning(disable:4512)
#pragma warning(disable:4480)

#define SI2C_POOL_TAG ((ULONG) 'C2IS')

/////////////////////////////////////////////////
//
// Common includes.
//
/////////////////////////////////////////////////

#include <initguid.h>
#include <ntddk.h>
#include <wdm.h>
#include <wdf.h>
#include <ntstrsafe.h>

#include "SPBCx.h"
#include "i2ctrace.h"


/////////////////////////////////////////////////
//
// Hardware definitions.
//
/////////////////////////////////////////////////

#include "skeletoni2c.h"

/////////////////////////////////////////////////
//
// Resource and descriptor definitions.
//
/////////////////////////////////////////////////

#include "reshub.h"

//
// I2C Serial peripheral bus descriptor
//

#include "pshpack1.h"

typedef struct _PNP_I2C_SERIAL_BUS_DESCRIPTOR {
    PNP_SERIAL_BUS_DESCRIPTOR SerialBusDescriptor;
    ULONG ConnectionSpeed;
    USHORT SlaveAddress;
    // follwed by optional Vendor Data
    // followed by PNP_IO_DESCRIPTOR_RESOURCE_NAME
} PNP_I2C_SERIAL_BUS_DESCRIPTOR, *PPNP_I2C_SERIAL_BUS_DESCRIPTOR;

#include "poppack.h"

#define I2C_SERIAL_BUS_TYPE 0x01
#define I2C_SERIAL_BUS_SPECIFIC_FLAG_10BIT_ADDRESS 0x0001

/////////////////////////////////////////////////
//
// Settings.
//
/////////////////////////////////////////////////

//
// Power settings.
//

#define MONITOR_POWER_ON         1
#define MONITOR_POWER_OFF        0

#define IDLE_TIMEOUT_MONITOR_ON  1000
#define IDLE_TIMEOUT_MONITOR_OFF 100

//
// Target settings.
//

typedef enum ADDRESS_MODE
{
    AddressMode7Bit,
    AddressMode10Bit
}
ADDRESS_MODE, *PADDRESS_MODE;

typedef struct PBC_TARGET_SETTINGS
{
    // TODO: Update this structure to include other
    //       target settings needed to configure the
    //       controller (i.e. connection speed, phase/
    //       polarity for SPI).

    ADDRESS_MODE                  AddressMode;
    USHORT                        Address;
    ULONG                         ConnectionSpeed;
}
PBC_TARGET_SETTINGS, *PPBC_TARGET_SETTINGS;


//
// Transfer settings. 
//

typedef enum BUS_CONDITION
{
    BusConditionFree,
    BusConditionBusy,
    BusConditionDontCare
}
BUS_CONDITION, *PBUS_CONDITION;

typedef struct PBC_TRANSFER_SETTINGS
{
    // TODO: Update this structure to include other
    //       settings needed to configure the controller 
    //       for a specific transfer.

    BUS_CONDITION                  BusCondition;
    BOOLEAN                        IsStart;
    BOOLEAN                        IsEnd;
}
PBC_TRANSFER_SETTINGS, *PPBC_TRANSFER_SETTINGS;

/////////////////////////////////////////////////
//
// Context definitions.
//
/////////////////////////////////////////////////

typedef struct PBC_DEVICE   PBC_DEVICE,   *PPBC_DEVICE;
typedef struct PBC_TARGET   PBC_TARGET,   *PPBC_TARGET;
typedef struct PBC_REQUEST  PBC_REQUEST,  *PPBC_REQUEST;

//
// Device context.
//

struct PBC_DEVICE 
{
    // TODO: Update this structure with variables that 
    //       need to be stored in the device context.

    // Handle to the WDF device.
    WDFDEVICE                      FxDevice;

    // Structure mapped to the controller's
    // register interface.
    PSKELETONI2C_REGISTERS         pRegisters;
    ULONG                          RegistersCb;
    PHYSICAL_ADDRESS               pRegistersPhysicalAddress;

    // Target that the controller is currently
    // configured for. In most cases this value is only
    // set when there is a request being handled, however,
    // it will persist between lock and unlock requests.
    // There cannot be more than one current target.
    PPBC_TARGET                    pCurrentTarget;
    
    // Variables to track enabled interrupts
    // and status between ISR and DPC.
    WDFINTERRUPT                   InterruptObject;
    ULONG                          InterruptMask;
    ULONG                          InterruptStatus;

    // Controller driver spinlock.
    WDFSPINLOCK                    Lock;

    // Delay timer used to stall between transfers.
    WDFTIMER                       DelayTimer;

    // The power setting callback handle
    PVOID                          pMonitorPowerSettingHandle;
};

//
// Target context.
//

struct PBC_TARGET 
{
    // TODO: Update this structure with variables that 
    //       need to be stored in the target context.

    // Handle to the SPB target.
    SPBTARGET                      SpbTarget;

    // Target specific settings.
    PBC_TARGET_SETTINGS            Settings;
    
    // Current request associated with the 
    // target. This value should only be non-null
    // when this target is the controller's current
    // target.
    PPBC_REQUEST                   pCurrentRequest;
};

//
// Request context.
//

struct PBC_REQUEST 
{
    // TODO: Update this structure with variables that 
    //       need to be stored in the request context.

    //
    // Variables that persist for the lifetime of
    // the request. Specifically these apply to an
    // entire sequence request (not just a single transfer).
    //

    // Handle to the SPB request.
    SPBREQUEST                     SpbRequest;

    // SPB request type.
    SPB_REQUEST_TYPE               Type;

    // Number of transfers in sequence and 
    // index of the current one.
    ULONG                          TransferCount; 
    ULONG                          TransferIndex;

    // Total bytes transferred.
    size_t                         TotalInformation;

    // Current status of the request.
    NTSTATUS                       Status;
    BOOLEAN                        bIoComplete;


    //
    // Variables that are reused for each transfer within
    // a [sequence] request.
    //

    // Pointer to the transfer buffer and length.
    size_t                         Length;
    PMDL                           pMdlChain;

    // Position of the current transfer within
    // the sequence and its associated controller
    // settings.
    SPB_REQUEST_SEQUENCE_POSITION  SequencePosition;
    PBC_TRANSFER_SETTINGS          Settings;

    // Direction of the current transfer.
    SPB_TRANSFER_DIRECTION         Direction;

    // Time to delay before starting transfer.
    ULONG                          DelayInUs;

    // Interrupt flag indicating data is ready to
    // be transferred.
    ULONG                          DataReadyFlag; 

    // Bytes read/written in the current transfer.
    size_t                         Information;
};

//
// Declate contexts for device, target, and request.
//

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(PBC_DEVICE,  GetDeviceContext);
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(PBC_TARGET,  GetTargetContext);
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(PBC_REQUEST, GetRequestContext);

#pragma warning(pop)

#endif // _INTERNAL_H_
