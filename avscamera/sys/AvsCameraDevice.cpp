/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2015, Microsoft Corporation.

    File:

        AvsCameraDevice.cpp

    Abstract:

        Device class implementation.  Derived from CCaptureDevice.
        This class overloads our base device class implementation.

    History:

        created 02/24/2015

**************************************************************************/

#include "Common.h"

/**************************************************************************

    PAGEABLE CODE

**************************************************************************/

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA

//
// DispatchCreate():
//
// This is the Add Device dispatch for the capture device.  It creates
// the CCaptureDevice and associates it with the device via the bag.
//
NTSTATUS
CAvsCameraDevice::
DispatchCreate (
    _In_ PKSDEVICE Device
)
{
    PAGED_CODE();

    DBG_ENTER("()");

    CAvsCameraDevice *CapDevice = new (NonPagedPoolNx, 'eviD') CAvsCameraDevice (Device);
    if( !CapDevice)
    {
        //
        //  Return failure if we couldn't create the device.
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS Status = CapDevice->Prepare();
    if (!NT_SUCCESS (Status))
    {
        //  Init failure.
        delete CapDevice;
    }

    DBG_LEAVE("()");
    return Status;
}

NTSTATUS
CAvsCameraDevice::
Initialize()
{
    PAGED_CODE();

    static
    CSensorContext Context[] =
    {
        { L"RFC", &AvsCameraFilterDescriptor,    AcpiPldPanelBack },
        { L"FFC", &AvsCameraFilterDescriptorFFC, AcpiPldPanelFront }
    };

    m_FilterDescriptorCount = SIZEOF_ARRAY(Context);
    m_Context = Context;

    return CCaptureDevice::Initialize();
}

CSensor *
CAvsCameraDevice::
CreateSensor(
    _In_    const KSFILTER_DESCRIPTOR  *Descriptors
)
{
    PAGED_CODE();

    return new (NonPagedPoolNx, 'sneS') CSensorSimulation( this, Descriptors );
}

