/*++

Copyright (c) Microsoft Corporation, All Rights Reserved

Module Name:

    queue.h

Abstract:

    This file defines the queue callback interface.

Environment:

    Windows User-Mode Driver Framework (WUDF)

--*/

#pragma once

//
// Queue Callback Object.
//

class CMyQueue : 
    public CUnknown
{
protected:    
    //
    // Unreferenced pointer to the partner Fx device.
    //

    IWDFIoQueue *m_FxQueue;

    //
    // Unreferenced pointer to the parent device.
    //

    PCMyDevice m_Device;

    HRESULT
    Initialize(
        _In_ WDF_IO_QUEUE_DISPATCH_TYPE DispatchType,
        _In_ bool Default,
        _In_ bool PowerManaged
        );

protected:

    CMyQueue(
        _In_ PCMyDevice Device
        );

    virtual ~CMyQueue();

    HRESULT
    Configure(
        VOID
        );

public:
    
    IWDFIoQueue *
    GetFxQueue(
        VOID
        )
    {
        return m_FxQueue;
    }


    PCMyDevice
    GetDevice(
        VOID
        )
    {
        return m_Device;
    }
    
    //
    // IUnknown
    //

    STDMETHOD_(ULONG,AddRef) (VOID) {return CUnknown::AddRef();}

    _At_(this, __drv_freesMem(object))
    STDMETHOD_(ULONG,Release) (VOID) {return CUnknown::Release();}

    STDMETHOD_(HRESULT, QueryInterface)(
        _In_ REFIID InterfaceId, 
        _Outptr_ PVOID *Object
        );
};
