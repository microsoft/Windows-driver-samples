/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2015, Microsoft Corporation.

    File:

        AvsCameraDevice.h

    Abstract:

        Device class implementation.  Derived from CCaptureDevice.
        This class overloads our base device class implementation.

    History:

        created 02/24/2015

**************************************************************************/

#pragma once

class CAvsCameraDevice : public CCaptureDevice
{
public:
    //
    // DispatchCreate():
    //
    // This is the Add Device dispatch for the capture device.  It creates
    // the CCaptureDevice and associates it with the device via the bag.
    //
    static
    NTSTATUS
    DispatchCreate (
        _In_ PKSDEVICE Device
    );

public:
    //
    // CCaptureDevice():
    //
    // The capture device class constructor.  Since everything should have
    // been zero'ed by the new operator, don't bother setting anything to
    // zero or NULL.  Only initialize non-NULL, non-0 fields.
    //
    CAvsCameraDevice(
        _In_ PKSDEVICE Device
    )
        : CCaptureDevice( Device )
    {}

    virtual
    NTSTATUS
    Initialize();

protected:
    virtual
    CSensor *
    CreateSensor(
        _In_    const KSFILTER_DESCRIPTOR  *Descriptors
    );

};

// {B27E3887-AD10-4A4E-BFB8-D6765ADD0E38}
#define STATIC_FrontCamera_Filter \
    0xb27e3887, 0xad10, 0x4a4e, 0xbf, 0xb8, 0xd6, 0x76, 0x5a, 0xdd, 0xe, 0x38

// {4EE16166-F358-4F10-8889-93107806B7A7}
#define STATIC_RearCamera_Filter \
    0x4ee16166, 0xf358, 0x4f10, 0x88, 0x89, 0x93, 0x10, 0x78, 0x6, 0xb7, 0xa7

