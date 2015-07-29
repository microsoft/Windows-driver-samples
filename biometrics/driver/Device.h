/*++

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
    ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
    THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
    PARTICULAR PURPOSE.

    Copyright (c) Microsoft Corporation. All rights reserved

Module Name:

    Device.h

Abstract:

    This module contains the type definitions of the Biometric
    device driver.

Environment:

    Windows User-Mode Driver Framework (WUDF)

--*/

#pragma once

// 
// TODO: Change this to match your device
//
#define NUM_WBDI_ENDPOINTS      3

// 
// Power policy suspend delay time. 10 seconds.
//
#define WBDI_SUSPEND_DELAY      ((ULONG)(10 * 1000)) 

//
// Struct for passing parameters for capture request completion.
//
typedef struct _CAPTURE_SLEEP_PARAMS
{
    DWORD SleepValue;
    HRESULT Hr;
    DWORD Information;
} CAPTURE_SLEEP_PARAMS, *PCAPTURE_SLEEP_PARAMS;


//
// Class for the Biometric driver.
//

class CBiometricDevice :
    public CComObjectRootEx<CComMultiThreadModel>,
    public IRequestCallbackRequestCompletion,
    public IRequestCallbackCancel,
    public IPnpCallbackHardware
{
public:

    DECLARE_NOT_AGGREGATABLE(CBiometricDevice)

    BEGIN_COM_MAP(CBiometricDevice)
        COM_INTERFACE_ENTRY(IPnpCallbackHardware)
        COM_INTERFACE_ENTRY(IRequestCallbackRequestCompletion)
        COM_INTERFACE_ENTRY(IRequestCallbackCancel)
    END_COM_MAP()

    CBiometricDevice() :
        m_FxDevice(NULL),
        m_IoQueue(NULL),
        m_pIUsbTargetDevice(NULL),
        m_pIUsbInterface(NULL),
        m_pIUsbInputPipe(NULL),
        m_pIUsbOutputPipe(NULL),
        m_pIUsbInterruptPipe(NULL),
        m_PendingRequest(NULL),
        m_Speed(0),
        m_InterruptReadProblem(S_OK),
        m_SleepThread(INVALID_HANDLE_VALUE)
    {
        InitializeCriticalSection(&m_RequestLock);
    }

    ~CBiometricDevice()
    {
        DeleteCriticalSection(&m_RequestLock);
    }

//
// Private data members.
//
private:

    //
    // Weak reference to framework device object.
    //
    IWDFDevice *            m_FxDevice;

    //
    // Weak reference to I/O queue
    //
    PCBiometricIoQueue      m_IoQueue;

    //
    // USB Device I/O Target
    //
    IWDFUsbTargetDevice *   m_pIUsbTargetDevice;

    //
    // USB Interface
    //
    IWDFUsbInterface *      m_pIUsbInterface;

    //
    // USB Input pipe for Reads
    //
    IWDFUsbTargetPipe *     m_pIUsbInputPipe;

    //
    // USB Output pipe for writes
    //
    IWDFUsbTargetPipe *     m_pIUsbOutputPipe;

    //
    // USB interrupt pipe
    //
    IWDFUsbTargetPipe *     m_pIUsbInterruptPipe;

    //
    // Device Speed (Low, Full, High)
    //
    UCHAR                   m_Speed;    

    //
    // If reads stopped because of a transient problem, the error status
    // is stored here.
    //

    HRESULT                 m_InterruptReadProblem;
    
    //
    // Interrupt message buffer 
    //
    
    INTERRUPT_MESSAGE       m_InterruptMessage;

    //
    // Holds a reference to a pending data I/O request.
    //

    IWDFIoRequest           *m_PendingRequest;

    // 
    // Synchronization for m_PendingRequest
    //

    CRITICAL_SECTION        m_RequestLock;

    //
    // Handle to a thread that will sleep before completing a request.
    //
    HANDLE                  m_SleepThread;
    CAPTURE_SLEEP_PARAMS    m_SleepParams;

//
// Private methods.
//
private:

    HRESULT
    Initialize(
        _In_ IWDFDriver *FxDriver,
        _In_ IWDFDeviceInitialize *FxDeviceInit
        );

    //
    // Helper methods
    //

    HRESULT
    CreateUsbIoTargets(
        VOID
        );

    HRESULT
    SetPowerManagement(
        VOID
        );
    
    //
    // Helper functions
    //
    
    HRESULT
    SendControlTransferSynchronously(
        _In_ PWINUSB_SETUP_PACKET SetupPacket,
        _Inout_updates_(BufferLength) PBYTE Buffer,
        _In_ ULONG BufferLength,
        _Out_ PULONG LengthTransferred
        );

    static
    WDF_IO_TARGET_STATE
    GetTargetState(
        IWDFIoTarget * pTarget
        );
    
    HRESULT
    InitiatePendingRead(
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
    CreateInstanceAndInitialize(
        _In_ IWDFDriver *FxDriver,
        _In_ IWDFDeviceInitialize *FxDeviceInit,
        _Out_ CBiometricDevice **Device
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
    // IPnpCallbackHardware
    //

    virtual
    HRESULT
    STDMETHODCALLTYPE
    OnPrepareHardware(
            _In_ IWDFDevice *FxDevice
            );

    virtual
    HRESULT
    STDMETHODCALLTYPE
    OnReleaseHardware(
        _In_ IWDFDevice *FxDevice
        );


    //
    // IRequestCallbackRequestCompletion
    //
    virtual
    void
    STDMETHODCALLTYPE
    OnCompletion(
        _In_ IWDFIoRequest*                 FxRequest,
        _In_ IWDFIoTarget*                  pIoTarget,
        _In_ IWDFRequestCompletionParams*   pParams,
        _In_ PVOID                          pContext
        );

    //
    // IRequestCallbackCancel
    //
    virtual
    VOID
    STDMETHODCALLTYPE
    OnCancel(
        _In_ IWDFIoRequest *pWdfRequest
        );

public:

    //
    // I/O handlers.
    //
    void
    GetIoRequestParams(
        _In_ IWDFIoRequest *FxRequest,
        _Out_ ULONG *MajorControlCode,
        _Outptr_result_bytebuffer_(*InputBufferSizeInBytes) PUCHAR *InputBuffer,
        _Out_ SIZE_T *InputBufferSizeInBytes,
        _Outptr_result_bytebuffer_(*OutputBufferSizeInBytes) PUCHAR *OutputBuffer,
        _Out_ SIZE_T *OutputBufferSizeInBytes
        );

    void
    OnGetAttributes(
        _Inout_ IWDFIoRequest *FxRequest
        );

    void
    OnReset(
        _Inout_ IWDFIoRequest *FxRequest
        );

    void
    OnCalibrate(
        _Inout_ IWDFIoRequest *FxRequest
        );

    void
    OnGetSensorStatus(
        _Inout_ IWDFIoRequest *FxRequest
        );

    void
    OnCaptureData(
        _Inout_ IWDFIoRequest *FxRequest
        );

    void
    OnUpdateFirmware(
        _Inout_ IWDFIoRequest *FxRequest
        );
 
    void
    OnGetSupportedAlgorithms(
        _Inout_ IWDFIoRequest *FxRequest
        );

    void
    OnGetIndicator(
        _Inout_ IWDFIoRequest *FxRequest
        );

    void
    OnSetIndicator(
        _Inout_ IWDFIoRequest *FxRequest
        );
 
    void
    OnControlUnit(
        _Inout_ IWDFIoRequest *FxRequest
        );

    VOID
    CompletePendingRequest(
        HRESULT hr,
        DWORD   information
        );

    inline PCAPTURE_SLEEP_PARAMS
    GetCaptureSleepParams()
    {
        return &m_SleepParams;
    }

};


