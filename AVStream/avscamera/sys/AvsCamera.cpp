/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2014, Microsoft Corporation.

    File:

        AvsCamera.cpp

    Abstract:

        Sample Camera driver initialization.

    History:

        created 5/15/2014

**************************************************************************/

#include "Common.h"

/**************************************************************************

    DESCRIPTOR AND DISPATCH LAYOUT

**************************************************************************/

//
// CaptureDeviceDispatch:
//
// This is the dispatch table for the capture device.  Plug and play
// notifications as well as power management notifications are dispatched
// through this table.
//
DEFINE_CAMERA_KSDEVICE_DISPATCH( AvsCameraDispatch, CAvsCameraDevice );

//
// CaptureDeviceDescriptor:
//
// This is the device descriptor for the capture device.  It points to the
// dispatch table and contains a list of filter descriptors that describe
// filter-types that this device supports.  Note that the filter-descriptors
// can be created dynamically and the factories created via
// KsCreateFilterFactory as well.
//
const
KSDEVICE_DESCRIPTOR
AvsCameraDeviceDescriptor =
{
    &AvsCameraDispatch,
    0,
    NULL
};

/**************************************************************************

    INITIALIZATION CODE

**************************************************************************/


extern "C" DRIVER_INITIALIZE DriverEntry;

extern "C"
NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)

/*++

Routine Description:

    Driver entry point.  Pass off control to the AVStream initialization
    function (KsInitializeDriver) and return the status code from it.

Arguments:

    DriverObject -
        The WDM driver object for our driver

    RegistryPath -
        The registry path for our registry info

Return Value:

    As from KsInitializeDriver

--*/

{
    //
    // Simply pass the device descriptor and parameters off to AVStream
    // to initialize us.  This will cause filter factories to be set up
    // at add & start.  Everything is done based on the descriptors passed
    // here.
    //
    return
        KsInitializeDriver (
            DriverObject,
            RegistryPath,
            &AvsCameraDeviceDescriptor
        );

}

