/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2001, Microsoft Corporation.

    File:

        device.h

    Abstract:

    This file contains the declaration of CCaptureDevice.

    CCaptureDevice is the base device class object.  It handles basic PnP
    requests and constructs the filter factories.

    History:

        created 3/9/2001

**************************************************************************/

//
//  Forward references...
//
class CSensor;

struct CSensorContext
{
    PWSTR                       Name;           //  What to call this class of filter.
    const KSFILTER_DESCRIPTOR  *Descriptor;     //  The description of the filter.
    AcpiPldPanel                AcpiPosition;   //  Where the device is located.
};

class CCaptureDevice
{
protected:

    //
    // The AVStream device we're associated with.
    //
    PKSDEVICE m_Device;

    //
    // Since we don't have physical hardware, this provides the hardware
    // simulation.  m_HardwareSimulation provides the fake ISR, fake DPC,
    // etc...  m_Synthesizer provides RGB24 and UYVY image synthesis and
    // overlay in software.
    //

    //
    // Number of pins with resources acquired.  This is used as a locking
    // mechanism for resource acquisition on the device.
    //

    // The capture pin.  When we complete scatter / gather mappings, we
    // notify the capture pin.
    //

    //  Number of Filter descriptors & filter factories.
    size_t  m_FilterDescriptorCount;

    //  Pointer to an array of filter descriptor pointers.
    //  Typically it's one sensor for each filter factory.
    CSensorContext  *m_Context;

    //  The sensor state for each camera.
    CSensor        **m_Sensor;

    //
    // The Dma adapter object we acquired through IoGetDmaAdapter() during
    // Pnp start.  This must be initialized with AVStream in order to perform
    // Dma directly into the capture buffers.
    //
    PADAPTER_OBJECT m_DmaAdapterObject;

    //
    // The number of map registers returned from IoGetDmaAdapter().
    //
    ULONG m_NumberOfMapRegisters;

    //
    // Cleanup():
    //
    // This is the free callback for the bagged capture device.  Not providing
    // one will call ExFreePool, which is not what we want for a constructed
    // C++ object.  This simply deletes the capture device.
    //
    static
    void
    Cleanup (
        _In_ CCaptureDevice *CapDevice
    )
    {
        delete CapDevice;
    }

    //
    //  Prepare():
    //
    //  Called by CCaptureDevice::DispatchCreate to finish AddDevice processing,
    //  once the object is created.
    //
    //  This is a seperate helper function to allow the derived DispatchCreate
    //  implementation to be as simple as is feasible.
    //
    NTSTATUS
    Prepare();

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

    //
    // DispatchStart():
    //
    // This is the Pnp Start dispatch for the capture device.  It simply
    // bridges to PnpStart() in the context of the CCaptureDevice.
    //
    static
    NTSTATUS
    DispatchStart (
        _In_ PKSDEVICE Device,
        _In_ PIRP Irp,
        _In_ PCM_RESOURCE_LIST TranslatedResourceList,
        _In_ PCM_RESOURCE_LIST UntranslatedResourceList
    );

    //
    // DispatchStop():
    //
    // This is the Pnp stop dispatch for the capture device.  It simply
    // bridges to PnpStop() in the context of the CCaptureDevice.
    //
    static
    void
    DispatchStop(
        _In_ PKSDEVICE Device,
        _In_ PIRP Irp
    );

    static
    NTSTATUS
    DispatchPostStart(
        _In_ PKSDEVICE Device
    );

    static
    NTSTATUS
    DispatchQueryStop(
        _In_ PKSDEVICE Device,
        _In_ PIRP Irp
    );

    static
    void
    DispatchCancelStop(
        _In_ PKSDEVICE Device,
        _In_ PIRP Irp
    );

    static
    NTSTATUS
    DispatchQueryRemove(
        _In_ PKSDEVICE Device,
        _In_ PIRP Irp
    );

    static
    void
    DispatchCancelRemove(
        _In_ PKSDEVICE Device,
        _In_ PIRP Irp
    );

    static
    void
    DispatchRemove(
        _In_ PKSDEVICE Device,
        _In_ PIRP Irp
    );

    static
    void
    DispatchSurpriseRemoval(
        _In_ PKSDEVICE Device,
        _In_ PIRP Irp
    );

    static
    NTSTATUS
    DispatchQueryCapabilities(
        _In_ PKSDEVICE Device,
        _In_ PIRP Irp,
        _Inout_ PDEVICE_CAPABILITIES Capabilities
    );

    static
    NTSTATUS
    DispatchQueryPower(
        _In_ PKSDEVICE Device,
        _In_ PIRP Irp,
        _In_ DEVICE_POWER_STATE DeviceTo,
        _In_ DEVICE_POWER_STATE DeviceFrom,
        _In_ SYSTEM_POWER_STATE SystemTo,
        _In_ SYSTEM_POWER_STATE SystemFrom,
        _In_ POWER_ACTION Action
    );

    static
    void
    DispatchSetPower(
        _In_ PKSDEVICE Device,
        _In_ PIRP Irp,
        _In_ DEVICE_POWER_STATE To,
        _In_ DEVICE_POWER_STATE From
    );

    static
    NTSTATUS
    DispatchQueryInterface(
        _In_ PKSDEVICE Device,
        _In_ PIRP Irp
    );

protected:
    //
    // PnpStart():
    //
    // This is the Pnp start routine for our simulated hardware.  Note that
    // DispatchStart bridges to here in the context of the CCaptureDevice.
    //
    virtual
    NTSTATUS
    PnpStart (
        _In_ PCM_RESOURCE_LIST TranslatedResourceList,
        _In_ PCM_RESOURCE_LIST UntranslatedResourceList
    );

    //
    // PnpStop():
    //
    // This is the Pnp stop routine for our simulated hardware.  Note that
    // DispatchStop bridges to here in the context of the CCaptureDevice.
    //
    virtual
    void
    PnpStop (
        _In_ PIRP Irp
    );

    virtual
    NTSTATUS
    PnpPostStart();

    virtual
    NTSTATUS
    PnpQueryStop(
        _In_ PIRP Irp
    );

    virtual
    void
    PnpCancelStop(
        _In_ PIRP Irp
    );

    virtual
    NTSTATUS
    PnpQueryRemove(
        _In_ PIRP Irp
    );

    virtual
    void
    PnpCancelRemove(
        _In_ PIRP Irp
    );

    virtual
    void
    PnpRemove(
        _In_ PIRP Irp
    );

    virtual
    void
    PnpSurpriseRemoval(
        _In_ PIRP Irp
    );

    virtual
    NTSTATUS
    PnpQueryCapabilities(
        _In_ PIRP Irp,
        _Inout_ PDEVICE_CAPABILITIES Capabilities
    );

    virtual
    NTSTATUS
    PnpQueryPower(
        _In_ PIRP Irp,
        _In_ DEVICE_POWER_STATE DeviceTo,
        _In_ DEVICE_POWER_STATE DeviceFrom,
        _In_ SYSTEM_POWER_STATE SystemTo,
        _In_ SYSTEM_POWER_STATE SystemFrom,
        _In_ POWER_ACTION Action
    );

    virtual
    void
    PnpSetPower(
        _In_ PIRP Irp,
        _In_ DEVICE_POWER_STATE To,
        _In_ DEVICE_POWER_STATE From
    );

    virtual
    NTSTATUS
    PnpQueryInterface(
        _In_ PIRP Irp
    );

protected:
    virtual
    CSensor *
    CreateSensor(
        _In_    const KSFILTER_DESCRIPTOR  *Descriptors
    );

public:
    //
    // CCaptureDevice():
    //
    // The capture device class constructor.  Since everything should have
    // been zero'ed by the new operator, don't bother setting anything to
    // zero or NULL.  Only initialize non-NULL, non-0 fields.
    //
    CCaptureDevice (
        _In_ PKSDEVICE Device
    );

    //
    // ~CCaptureDevice():
    //
    // The capture device destructor.
    //
    virtual
    ~CCaptureDevice();

    virtual
    NTSTATUS
    Initialize();

    //
    //  Recast():
    //
    //  Helper function.  Used to convert from a PKSDEVICE to a CCaptureDevice *
    //
    static
    inline
    CCaptureDevice *
    Recast(
        _In_ PKSDEVICE Device
    )
    {
        return reinterpret_cast <CCaptureDevice *> (Device -> Context);
    }

    //
    // QueryInterruptTime():
    //
    // Determine the frame number that this frame corresponds to.
    //
    ULONG
    QueryInterruptTime (LONG ID,
                        _In_ PKSPIN Pin
                       );

    virtual
    PDEVICE_OBJECT
    GetDeviceObject();

    PKSDEVICE
    GetKsDevice()
    {
        return m_Device;
    }

    ULONG
    GetFilterIndex(PKSFILTER Filter);

    ULONG
    GetFilterCount()
    {
        return (ULONG) m_FilterDescriptorCount;
    }

    PCWSTR
    GetFilterName(ULONG  FilterIndex)
    {
        return (FilterIndex < m_FilterDescriptorCount) ?  m_Context[FilterIndex].Name : nullptr;
    }

    CSensor *
    GetSensor( PKSFILTER Filter )
    {
        ULONG Index = GetFilterIndex(Filter);

        return ( Index < m_FilterDescriptorCount ) ? m_Sensor[ Index ] : nullptr;
    }

};

//
// Define a dispatch table for a capture pin.  It provides notifications
// about creation, closure, processing, data formats, etc...
//
#define DEFINE_CAMERA_KSDEVICE_DISPATCH( Table, Class )                        \
const                                                                   \
KSDEVICE_DISPATCH                                                       \
Table =                                                                 \
{                                                                       \
    Class::DispatchCreate,              /* Pnp Add Device         */    \
    Class::DispatchStart,               /* Pnp Start              */    \
    Class::DispatchPostStart,           /* Post-Start             */    \
    Class::DispatchQueryStop,           /* Pnp Query Stop         */    \
    Class::DispatchCancelStop,          /* Pnp Cancel Stop        */    \
    Class::DispatchStop,                /* Pnp Stop               */    \
    Class::DispatchQueryRemove,         /* Pnp Query Remove       */    \
    Class::DispatchCancelRemove,        /* Pnp Cancel Remove      */    \
    Class::DispatchRemove,              /* Pnp Remove             */    \
    Class::DispatchQueryCapabilities,   /* Pnp Query Capabilities */    \
    Class::DispatchSurpriseRemoval,     /* Pnp Surprise Removal   */    \
    Class::DispatchQueryPower,          /* Power Query Power      */    \
    Class::DispatchSetPower,            /* Power Set Power        */    \
    Class::DispatchQueryInterface       /* Pnp Query Interface    */    \
};

