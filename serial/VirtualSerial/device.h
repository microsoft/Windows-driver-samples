/*++

Copyright (C) Microsoft Corporation, All Rights Reserved

Module Name:

    Device.h

Abstract:

    This module contains the type definitions for the UMDF VirtualSerial sample
    driver's device callback class.

Environment:

    Windows User-Mode Driver Framework (WUDF)

--*/

#pragma once
#include "initguid.h"
#include "serial.h"

DEFINE_GUID(GUID_DEVINTERFACE_MODEM, 0x2c7089aa, 0x2e0e, 0x11d1, 0xb1, 0x14, 0x00, 0xc0, 0x4f, 0xc2, 0xaa, 0xe4);
DEFINE_GUID(GUID_DEVINTERFACE_COMPORT, 0x86e0d1e0L, 0x8089, 0x11d0, 0x9c, 0xe4, 0x08, 0x00, 0x3e, 0x30, 0x1f, 0x73);

#define SYMBOLIC_LINK_NAME_PREFIX   L"\\DosDevices\\Global\\"

//
// Class for the VirtualSerial driver.
//

class CMyDevice : 
    public CUnknown,
    public IObjectCleanup
{

//
// Private data members.
//
private:

    IWDFDevice *m_FxDevice;

    //
    // Baud rate
    //
    ULONG m_BaudRate;

    //
    // Modem control register
    //
    ULONG m_MCR;

    //
    // FIFO control register
    //
    ULONG m_FCR;

    //
    // Line control register
    //
    ULONG m_LCR;

    //
    // Valid data mask
    //
    UCHAR m_ValidDataMask;
    
    //
    // Timeouts
    //
    SERIAL_TIMEOUTS m_Timeouts;
    
    //
    // Legacy hardware key property store
    //
    BOOL m_CreatedLegacyHardwareKey;
    
    IWDFNamedPropertyStore2 * m_LegacyHardwarePropertyStore;
    
    PWCHAR m_PdoName;
//
// Private methods.
//

private:

    CMyDevice(
        VOID
        ) :
        m_FxDevice(NULL),
        m_BaudRate(0),
        m_MCR(0),
        m_FCR(0),
        m_LCR(0),
        m_ValidDataMask(0),
        m_CreatedLegacyHardwareKey(FALSE),
        m_LegacyHardwarePropertyStore(NULL),
        m_PdoName(NULL)
    {
        ZeroMemory(&m_Timeouts, sizeof(SERIAL_TIMEOUTS));
    }

    HRESULT
    Initialize(
        _In_ IWDFDriver *FxDriver,
        _In_ IWDFDeviceInitialize *FxDeviceInit
        );
        
    HRESULT
    CMyDevice::GetPdoName(
        _Out_ LPWSTR *PdoName
        );
//
// Public methods
//
public:

    //
    // The factory method used to create an instance of this driver.
    //

    static
    HRESULT
    CreateInstance(
        _In_ IWDFDriver *FxDriver,
        _In_ IWDFDeviceInitialize *FxDeviceInit,
        _Out_ PCMyDevice *Device
        );

    HRESULT
    Configure(
        VOID
        );

//
// COM methods
//
public:

    //
    // IUnknown methods.
    //

    virtual
    ULONG
    STDMETHODCALLTYPE
    AddRef(
        VOID
        )
    {
        return __super::AddRef();
    }

    _At_(this, __drv_freesMem(object))
    virtual
    ULONG
    STDMETHODCALLTYPE
    Release(
        VOID
       )
    {
        return __super::Release();
    }

    virtual
    HRESULT
    STDMETHODCALLTYPE
    QueryInterface(
        _In_ REFIID InterfaceId,
        _Out_ PVOID *Object
        );
        
    //
    // IObjectCleanup
    //
    IObjectCleanup *
    QueryIObjectCleanup(
        VOID
        )
    {
        AddRef();
        return static_cast<IObjectCleanup *>(this);
    }    
    
    virtual
    void
    STDMETHODCALLTYPE
    OnCleanup(
        IWDFObject*  pWdfObject
        );
    
        
    ULONG
    GetBaudRate(
        VOID
        )
    {
        return m_BaudRate;
    }

    void
    SetBaudRate(
        _In_ ULONG BaudRate
        )
    {
        m_BaudRate = BaudRate;

        return;
    }

    ULONG *
    GetModemControlRegisterPtr(
        VOID
        )
    {
        return &m_MCR;
    }

    ULONG *
    GetFifoControlRegisterPtr(
        VOID
        )
    {
        return &m_FCR;
    }

    ULONG *
    GetLineControlRegisterPtr(
        VOID
        )
    {
        return &m_LCR;
    }

    VOID
    SetValidDataMask(
        _In_ UCHAR Mask
        )
    {
        m_ValidDataMask = Mask;
    }        
    
    VOID
    SetTimeouts(
        _In_ SERIAL_TIMEOUTS Timeouts
        )
    {
        m_Timeouts = Timeouts;
    }
    
    VOID
    GetTimeouts(
        _Out_ SERIAL_TIMEOUTS *Timeouts
        )
    {
       *Timeouts = m_Timeouts;;
    }
    
};
