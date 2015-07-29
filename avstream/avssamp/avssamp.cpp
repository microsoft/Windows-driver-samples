/**************************************************************************

    AVStream Filter-Centric Sample

    Copyright (c) 1999 - 2001, Microsoft Corporation

    File:

        avssamp.cpp

    Abstract:

        This is the main file for the filter-centric sample.

    History:

        created 6/18/01

**************************************************************************/

#include "avssamp.h"

/**************************************************************************

    INITIALIZATION CODE

**************************************************************************/


extern "C" DRIVER_INITIALIZE DriverEntry;

extern "C"
NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
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
            &CaptureDeviceDescriptor
            );

}

/**************************************************************************

    DESCRIPTOR AND DISPATCH LAYOUT

**************************************************************************/

//
// FilterDescriptors:
//
// The table of filter descriptors that this device supports.  Each one of
// these will be used as a template to create a filter-factory on the device.
//
DEFINE_KSFILTER_DESCRIPTOR_TABLE (FilterDescriptors) {
    &CaptureFilterDescriptor
};

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
CaptureDeviceDescriptor = {
    //
    // Since this is a software sample (filter-centric filters usually are
    // software kinds of transforms), we really don't care about device level
    // notifications and work.  The default behavior done on behalf of us
    // by AVStream will be quite sufficient.
    //
    NULL,
    SIZEOF_ARRAY (FilterDescriptors),
    FilterDescriptors,
    KSDEVICE_DESCRIPTOR_VERSION
};

