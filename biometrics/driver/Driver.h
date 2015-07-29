/*++

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
    ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
    THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
    PARTICULAR PURPOSE.

    Copyright (c) Microsoft Corporation. All rights reserved

Module Name:

    Driver.h

Abstract:

    This module contains the type definitions for the Biometric
    driver callback class.

Environment:

    Windows User-Mode Driver Framework (WUDF)

--*/

#pragma once

//
// This class handles driver events for the skeleton sample.  In particular
// it supports the OnDeviceAdd event, which occurs when the driver is called
// to setup per-device handlers for a new device stack.
//

EXTERN_C const CLSID CLSID_BiometricUsbSample;

class CBiometricDriver :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CBiometricDriver, &CLSID_BiometricUsbSample>,
    public IDriverEntry
{
public:

    CBiometricDriver()
    {
    }

    DECLARE_NO_REGISTRY()

    DECLARE_NOT_AGGREGATABLE(CBiometricDriver)

    BEGIN_COM_MAP(CBiometricDriver)
        COM_INTERFACE_ENTRY(IDriverEntry)
    END_COM_MAP()

//
// Public methods
//
public:

    //
    // IDriverEntry methods
    //

    virtual
    HRESULT
    STDMETHODCALLTYPE
    OnInitialize(
        _In_ IWDFDriver *FxWdfDriver
        )
    {
        UNREFERENCED_PARAMETER(FxWdfDriver);
        return S_OK;
    }

    virtual
    HRESULT
    STDMETHODCALLTYPE
    OnDeviceAdd(
        _In_ IWDFDriver *FxWdfDriver,
        _In_ IWDFDeviceInitialize *FxDeviceInit
        );

    virtual
    VOID
    STDMETHODCALLTYPE
    OnDeinitialize(
        _In_ IWDFDriver *FxWdfDriver
        )
    {
        UNREFERENCED_PARAMETER(FxWdfDriver);
        return;
    }

};

OBJECT_ENTRY_AUTO(CLSID_BiometricUsbSample, CBiometricDriver)
