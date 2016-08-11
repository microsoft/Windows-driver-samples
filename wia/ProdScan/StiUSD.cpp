/**************************************************************************
*
*  Copyright © Microsoft Corporation
*
*  File Name:   StiUSD.cpp
*
*  Description: Contains the IStiUSD interface implementation for
*               the Production Scanner Driver Sample
*
*
***************************************************************************/

#include "stdafx.h"


/**************************************************************************\
*
* Helper for IStiUSD::Initialize implementation. Placeholder where the driver
* would initialize its communication interface with the scanner device (register
* itself to receive event notifications from the device, read the scanner device
* configuration). Also initializes the the available transfer formats and the
* capabilities needed for the sample driver.
*
* Parameters:
*
*    wszDevicePath - device path name
*    hDeviceKey    - device key in Registry
*
* Return Value:
*
*    S_OK if it suceeds, a standard COM error code if initialization fails
*
\**************************************************************************/

HRESULT CWiaDriver::InitializeDeviceConnection(
    _In_ LPCWSTR wszDevicePath,
    _In_ HKEY    hDeviceKey)
{
    HRESULT hr = S_OK;

    WIAEX_TRACE_BEGIN;

    //
    // Validate parameters:
    //
    if ((!wszDevicePath) || (!hDeviceKey))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08X", hr));
    }

    //
    // Initialize here the connection with the scanner device identified by wszDevicePath.
    //
    // For example:
    //
    // if (SUCCEEDED(hr))
    // {
    //     hr = m_ScannerDevice.Initialize(wszDevicePath);
    //     if (FAILED(hr))
    //     {
    //         WIAEX_ERROR((g_hInst, "Failed to initialize connection with scanner device described by %ws, hr = 0x%08X", wszDevicePath, hr));
    //     }
    // }
    //

    //
    // Initialize the valid format information arrays, matching the current scanner configuration if available:
    //
    if (SUCCEEDED(hr))
    {
        hr = InitializeFormatInfoArrays();
        if(FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_FORMAT_INFO arrays, hr = 0x%08X", hr));
        }
    }

    //
    // Initialize the page size array used by this driver (only one list, for portrait orientation).
    //
    // This sample driver supports Letter, and also pretends (without actually doing it) custom
    // and auto-detect document sizes:
    //
    if (SUCCEEDED(hr))
    {
        //
        // WIA_PAGE_CUSTOM is always supported:
        //
        m_lPortraitSizesArray.Append(WIA_PAGE_CUSTOM);

        //
        // WIA_PAGE_AUTO is also supported if the scanner reports that it supports automatic document size detection:
        //
        m_lPortraitSizesArray.Append(WIA_PAGE_AUTO);

        //
        // Other standard page sizes:
        //
        GetValidPageSizes(MAX_SCAN_AREA_WIDTH, MAX_SCAN_AREA_HEIGHT, MIN_SCAN_AREA_WIDTH, MIN_SCAN_AREA_HEIGHT, TRUE, m_lPortraitSizesArray);
    }

    //
    // Create the capability manager which will be initialized (uniquely per
    // driver session) during the first IWiaMiniDrv::drvGetCapabilities call:
    //
    if (SUCCEEDED(hr))
    {
        hr = m_tCapabilityManager.Initialize(g_hInst);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize the WIA driver capability manager object, hr = 0x%08X", hr));
        }
    }

    WIAEX_TRACE_FUNC_HR;

    return hr;
}


/**************************************************************************\
*
* Implements IStiUSD::Initialize. Initializes an instance of the COM object
* that defines the IStiUSD interface. When this method is called the driver
* receives a pointer to an IStiDeviceControl COM interface.
*
* Parameters:
*
*    pIStiDevControl - caller-supplied pointer to the IStiDeviceControl interface
*    dwStiVersion    - caller-supplied STI version number
*                      (value defined as STI_VERSION_x in sti.h)
*    hParametersKey  - caller-supplied handle to the registry key under
*                      which device-specific information is to be stored.
*
* Return Value:
*
*    If the operation succeeds, the method must return S_OK. Otherwise, it
*    should return one of the STIERR-prefixed error codes  defined in stierr.h
*    or another standard COM error code.
*
\**************************************************************************/

HRESULT CWiaDriver::Initialize(
    _In_ PSTIDEVICECONTROL pIStiDevControl,
    DWORD                  dwStiVersion,
    _In_ HKEY              hParametersKey)
{
    UNREFERENCED_PARAMETER(dwStiVersion);

    HRESULT hr = S_OK;
    DWORD cchDevicePath = sizeof(m_wszDevicePath) / sizeof(WCHAR);

    WIAEX_TRACE_BEGIN;

    //
    // Validate parameters:
    //
    if ((!pIStiDevControl) || (!hParametersKey))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08X", hr));
    }

    //
    // Get the device path name from PnP:
    //
    if (SUCCEEDED(hr))
    {
        memset(m_wszDevicePath, 0, sizeof(m_wszDevicePath));

        hr = pIStiDevControl->GetMyDevicePortName(m_wszDevicePath, cchDevicePath);

        if(FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "IStiDeviceControl::GetMyDevicePortName failed, hr = 0x%08X", hr));
        }
    }

    //
    // Initialize the connection with the scanner:
    //
    if (SUCCEEDED(hr))
    {
        m_hDeviceKey = hParametersKey;

        hr = InitializeDeviceConnection(m_wszDevicePath, m_hDeviceKey);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "InitializeDeviceConnection for DevicePath %ws failed, hr = 0x%08X", m_wszDevicePath, hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        m_hrLastEdviceError = STI_ERROR_NO_ERROR;
        m_bInitialized = TRUE;
    }
    else
    {
        m_hrLastEdviceError = hr;
        m_bInitialized = FALSE;
    }

    WIAEX_TRACE((g_hInst, "IStiUSD::Initialize 0x%08X", hr));

    return hr;
}

/**************************************************************************\
*
* Implements IStiUSD::GetCapabilities. Returns the still image
* device's capabilities.
*
* Parameters:
*
*    pDevCaps - caller-supplied pointer to an empty STI_USD_CAPS structure
*
* Return Value:
*
*    If the operation succeeds, the method must return S_OK. Otherwise, it
*    should return one of the STIERR-prefixed error codes defined in stierr.h
*    or another standard COM error code.
*
\**************************************************************************/

HRESULT CWiaDriver::GetCapabilities(
    _Out_ PSTI_USD_CAPS pDevCaps)
{
    HRESULT hr = S_OK;

    WIAEX_TRACE_BEGIN;

    if (!pDevCaps)
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08X",hr));
    }

    if (SUCCEEDED(hr))
    {
        //
        // The sample driver supports device notifications (required), known also as interrupt events.
        // Polling (optional) it is not needed so STI_GENCAP_POLLING_NEEDED is not reported here:
        //

        memset(pDevCaps, 0, sizeof(STI_USD_CAPS));

        pDevCaps->dwVersion = STI_VERSION_3;
        pDevCaps->dwGenericCaps = STI_GENCAP_WIA | STI_USD_GENCAP_NATIVE_PUSHSUPPORT | STI_GENCAP_NOTIFICATIONS;

        WIAS_TRACE((g_hInst, "Device capabilities: 0x%08X", pDevCaps->dwGenericCaps));
    }

    if (FAILED(hr))
    {
        m_hrLastEdviceError = hr;
    }

    WIAEX_TRACE((g_hInst, "IStiUSD::GetCapabilities 0x%08X", hr));

    return hr;
}

/**************************************************************************\
*
* Implements IStiUSD::GetStatus. Returns the status for the still image device.
* GetStatus is called by the WIA service for two major operations:
*
*   1. Checking device ON-LINE status.
*   2. Polling for device events (like a push button event)
*
* Parameters:
*
*    pDevStatus - caller-supplied pointer to an STI_DEVICE_STATUS structure
*
* Return Value:
*
*    If the operation succeeds, the method must return S_OK. Otherwise, it
*    should return one of the STIERR-prefixed error codes defined in stierr.h
*    or another standard COM error code.
*
\**************************************************************************/

HRESULT CWiaDriver::GetStatus(
    _Inout_ PSTI_DEVICE_STATUS pDevStatus)
{
    HRESULT hr = S_OK;

    if (!pDevStatus)
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08X",hr));
    }

    //
    // A driver may be requested to report one or both of the following:
    //
    // STI_DEVSTATUS_EVENTS_STATE - The driver should fill in the dwEventHandlingState member
    // STI_DEVSTATUS_ONLINE_STATE - The driver should fill in the dwOnlineState member
    //
    // In this case STI_DEVSTATUS_EVENTS_STATE is not expected nor supported as this driver
    // does not support polling events (if the driver previously set the STI_GENCAP_POLLING_NEEDED
    // flag in the device's STI_DEV_CAPS structure, the IStiUSD::GetStatus method is the means
    // by which the Event Monitor determines if a still image device event has occurred;
    // the Event Monitor will call the method, specifying STI_DEVSTATUS_EVENT_STATE
    // in the supplied STI_DEVICE_STATUS structure; the driver must poll the device
    // and set STI_EVENTHANDLING_PENDING if an event has occurred)
    //
    // If the caller specifies STI_DEVSTATUS_ONLINE_STATE in the supplied
    // STI_DEVICE_STATUS structure, the driver should set the appropriate flag
    // in the STI_DEVICE_STATUS structure's dwOnlineState member.
    //

    if (SUCCEEDED(hr))
    {
        pDevStatus->dwOnlineState = 0;
        pDevStatus->dwHardwareStatusCode = 0;
        pDevStatus->dwEventHandlingState = 0;

        //
        // STI_DEVSTATUS_ONLINE_STATE:
        //
        if (pDevStatus->StatusMask & STI_DEVSTATUS_ONLINE_STATE)
        {
            //
            // This sample driver is always online and ready:
            //
            pDevStatus->dwOnlineState = STI_ONLINESTATE_OPERATIONAL;
        }

        //
        // STI_DEVSTATUS_EVENTS_STATE:
        //
        else if (pDevStatus->StatusMask & STI_DEVSTATUS_EVENTS_STATE)
        {
            //
            // Polled events are not supported so we don't have to return anyting here:
            //
            pDevStatus->dwEventHandlingState &= ~STI_EVENTHANDLING_PENDING;
        }
    }

    if (FAILED(hr))
    {
        m_hrLastEdviceError = hr;
    }

    WIAS_TRACE((g_hInst, "IStiUSD::GetStatus 0x%08X", hr));

    return hr;
}

/**************************************************************************\
*
* Implements IStiUSD::DeviceReset. Resets the still image device to a known,
* initialized state. The sample driver will return S_OK without to execute
* any reset operation. The success-do-nothing approach is needed to ensure
* compatibility with the requirements for the IStiUSD interface implementation.
*
* Parameters:
*
*    None
*
* Return Value:
*
*    If the operation succeeds, the method must return S_OK. Otherwise, it
*    should return one of the STIERR-prefixed error codes defined in stierr.h
*    or another standard COM error code. The sample driver always returns S_OK.
*
\**************************************************************************/

HRESULT CWiaDriver::DeviceReset()
{
    WIAS_TRACE((g_hInst, "IStiUSD::DeviceReset 0x%08X", S_OK));
    return S_OK;
}

/**************************************************************************\
*
* Implements IStiUSD::Diagnostic. If the driver is initialized when Diagnostic
* is called the driver should attempt to communicate with the device and
* determine if the device is online and operational. This sample driver
* does not do any special device validation.
*
* Parameters:
*
*    pBuffer - caller-supplied pointer to an STI_DIAG structure to receive
*              testing status information
*
* Return Value:
*
*    If the operation succeeds, the method must return S_OK. Otherwise, it
*    should return one of the STIERR-prefixed error codes defined in stierr.h
*    or another standard COM error code.
*
\**************************************************************************/

HRESULT CWiaDriver::Diagnostic(
    _Inout_ LPDIAG pBuffer)
{
    HRESULT hr = S_OK;

    WIAEX_TRACE((g_hInst, "IStiUSD::Diagnostic.."));

    if ((!pBuffer) || (pBuffer->dwSize < sizeof(STI_DIAG)) || (pBuffer->sErrorInfo.dwSize < sizeof(STI_ERROR_INFO)))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameter (buffer: %p, size: %u bytes (needed: %u), error: %u bytes (needed: %u)), hr = 0x%08X",
            pBuffer, pBuffer ? pBuffer->dwSize : 0, sizeof(STI_DIAG),
            pBuffer ? pBuffer->sErrorInfo.dwSize : 0, sizeof(STI_ERROR_INFO), hr));
        m_hrLastEdviceError = STIERR_INVALID_PARAM;
    }

    if (SUCCEEDED(hr))
    {
        pBuffer->dwVendorDiagCode = 0;
        pBuffer->dwStatusMask = 0;
        pBuffer->sErrorInfo.dwGenericError = NOERROR;
        pBuffer->sErrorInfo.dwVendorError = 0;
        memset(pBuffer->sErrorInfo.szExtendedErrorText, 0, sizeof(pBuffer->sErrorInfo.szExtendedErrorText));

        if (STI_DIAGCODE_HWPRESENCE == pBuffer->dwBasicDiagCode)
        {
            WIAEX_TRACE((g_hInst, "STI_DIAGCODE_HWPRESENCE.."));
        }
        else
        {
            WIAEX_ERROR((g_hInst, "Unknown basic diag code requested (and ignored): %u", pBuffer->dwBasicDiagCode));
        }
    }

    WIAEX_TRACE((g_hInst, "IStiUSD::Diagnostic 0x%08X", hr));

    return hr;
}

/**************************************************************************\
*
* Implements IStiUSD::Escape. This method allows an application to send
* directly to the device a proprietary command ID and optionally send
* and receive data.
*
* The sample driver returns STIERR_UNSUPPORTED.
*
* Parameters:
*
*    EscapeFunction - caller-supplied, vendor-defined, DWORD-sized value
*                     representing an I/O operation
*    lpInData       - caller-supplied pointer to a buffer containing data
*                     sent to the device
*    cbInDataSize   - caller-supplied length, in bytes, of the buffer
*                     pointed to by lpInData
*    pOutData       - caller-supplied pointer to a memory buffer to
*                     receive data from the device
*    cbOutDataSize  - caller-supplied length, in bytes, of the buffer
*                     pointed to by lpOutData
*    pdwActualData  - receives the number of bytes actually written to pOutData
*
* Return Value:
*
*    If the operation succeeds, the method must return S_OK. Otherwise, it
*    should return one of the STIERR-prefixed error codes defined in stierr.h
*    or another standard COM error code. This driver returns STIERR_UNSUPPORTED.
*
\**************************************************************************/

HRESULT CWiaDriver::Escape(
    STI_RAW_CONTROL_CODE               EscapeFunction,
    _In_reads_bytes_(cbInDataSize)
    LPVOID                             lpInData,
    DWORD                              cbInDataSize,
    _Out_writes_bytes_(cbOutDataSize)
    LPVOID                             pOutData,
    DWORD                              cbOutDataSize,
    _Out_ LPDWORD                      pdwActualData)
{
    UNREFERENCED_PARAMETER(EscapeFunction);
    UNREFERENCED_PARAMETER(cbInDataSize);
    UNREFERENCED_PARAMETER(lpInData);
    UNREFERENCED_PARAMETER(cbOutDataSize);
    UNREFERENCED_PARAMETER(pOutData);

    WIAEX_ERROR((g_hInst, "This method is not implemented or supported for this driver"));

    HRESULT hr = STIERR_UNSUPPORTED;

    if (pdwActualData)
    {
        *pdwActualData = 0;
    }

    m_hrLastEdviceError = hr;

    WIAEX_TRACE((g_hInst, "IStiUSD::Escape 0x%08X", hr));

    return hr;
}

/**************************************************************************\
*
* Implements IStiUSD::LockDevice. The sample driver returns S_OK. This is
* a must considering that the driver cannot function otherwise (the WIA Service
* expects this method to succeed for a properly installed driver and a working
* scanner device).
*
* Parameters:
*
*    None
*
* Return Value:
*
*    If the operation succeeds, the method must return S_OK. Otherwise, it
*    should return one of the STIERR-prefixed error codes defined in stierr.h
*    or another standard COM error code. This sample driver returns S_OK.
*
\**************************************************************************/

HRESULT CWiaDriver::LockDevice()
{
    return S_OK;
}

/**************************************************************************\
*
* Implements IStiUSD::UnLockDevice. The sample driver returns S_OK. This is
* a must considering that the driver cannot function otherwise (the WIA Service
* expects this method to succeed for a properly installed driver and a
* working scanner device).
*
* Parameters:
*
*    None
*
* Return Value:
*
*    If the operation succeeds, the method must return S_OK. Otherwise, it
*    should return one of the STIERR-prefixed error codes defined in stierr.h
*    or another standard COM error code. This sample driver returns S_OK.
*
\**************************************************************************/

HRESULT CWiaDriver::UnLockDevice()
{
    return S_OK;
}

/**************************************************************************\
*
* Implements IStiUSD::RawReadData. Reads data from the still image device.
*
* Parameters:
*
*    lpBuffer          - caller-supplied pointer to a buffer to receive data
*                        read from the device.
*    lpdwNumberOfBytes - caller-supplied pointer to a DWORD. The caller loads
*                        the DWORD with the number of bytes in the buffer pointed
*                        to by lpBuffer. The driver must replace this value with
*                        the number of bytes actually read.
*    lpOverlapped      - optional, caller-supplied pointer to an OVERLAPPED structure
*                        (described in the Microsoft Windows SDK documentation).
*
*
* Return Value:
*
*    If the operation succeeds, the method must return S_OK. Otherwise, it
*    should return one of the STIERR-prefixed error codes defined in stierr.h
*    or another standard COM error code. This driver returns STIERR_UNSUPPORTED.
*
\**************************************************************************/

HRESULT CWiaDriver::RawReadData(
    _Out_writes_bytes_(*lpdwNumberOfBytes)
             LPVOID        lpBuffer,
    _Inout_  LPDWORD       lpdwNumberOfBytes,
    _In_opt_ LPOVERLAPPED  lpOverlapped)
{
    UNREFERENCED_PARAMETER(lpBuffer);
    UNREFERENCED_PARAMETER(lpdwNumberOfBytes);
    UNREFERENCED_PARAMETER(lpOverlapped);

    WIAEX_ERROR((g_hInst, "This method is not implemented or supported for this driver"));

    HRESULT hr = STIERR_UNSUPPORTED;

    m_hrLastEdviceError = hr;

    WIAEX_TRACE((g_hInst, "IStiUSD::RawReadData 0x%08X", hr));

    return hr;
}

/**************************************************************************\
*
* Implements IStiUSD::RawWriteData. Writes data to the still image device.
*
* Parameters:
*
*    lpBuffer        - caller-supplied pointer to a buffer containing data
*                      to be sent to the device.
*    dwNumberOfBytes - caller-supplied number of bytes to be written; this is
*                      the number of bytes in the buffer pointed to by lpBuffer.
*    lpOverlapped    - optional, caller-supplied pointer to an OVERLAPPED structure
*                      (described in the Microsoft Windows SDK documentation).
*
* Return Value:
*
*    If the operation succeeds, the method must return S_OK. Otherwise, it
*    should return one of the STIERR-prefixed error codes defined in stierr.h
*    or another standard COM error code. This driver returns STIERR_UNSUPPORTED.
*
\**************************************************************************/

HRESULT CWiaDriver::RawWriteData(
    _In_reads_bytes_(dwNumberOfBytes)
             LPVOID       lpBuffer,
             DWORD        dwNumberOfBytes,
    _In_opt_ LPOVERLAPPED lpOverlapped)
{
    UNREFERENCED_PARAMETER(lpBuffer);
    UNREFERENCED_PARAMETER(dwNumberOfBytes);
    UNREFERENCED_PARAMETER(lpOverlapped);

    WIAEX_ERROR((g_hInst, "This method is not implemented or supported for this driver"));

    HRESULT hr = STIERR_UNSUPPORTED;

    m_hrLastEdviceError = hr;

    WIAEX_TRACE((g_hInst, "IStiUSD::RawWriteData 0x%08X", hr));

    return hr;
}

/**************************************************************************\
*
* Implements IStiUSD::RawReadCommand. Reads command information from
* the still image device.
*
* Parameters:
*
*    lpBuffer          - caller-supplied pointer to a buffer to receive the
*                        command read from the device.
*    lpdwNumberOfBytes - caller-supplied pointer to a DWORD. The caller loads
*                        the DWORD with the number of bytes in the buffer pointed
*                        to by lpBuffer; The driver must replace this value with
*                        the number of bytes actually read.
*    lpOverlapped      - optional, caller-supplied pointer to an OVERLAPPED structure
*                        (described in the Microsoft Windows SDK documentation).
*
* Return Value:
*
*    If the operation succeeds, the method must return S_OK. Otherwise, it
*    should return one of the STIERR-prefixed error codes defined in stierr.h
*    or another standard COM error code. This driver returns STIERR_UNSUPPORTED.
*
\**************************************************************************/

HRESULT CWiaDriver::RawReadCommand(
    _Out_writes_bytes_(*lpdwNumberOfBytes)
             LPVOID       lpBuffer,
    _Inout_  LPDWORD      lpdwNumberOfBytes,
    _In_opt_ LPOVERLAPPED lpOverlapped)
{
    UNREFERENCED_PARAMETER(lpBuffer);
    UNREFERENCED_PARAMETER(lpdwNumberOfBytes);
    UNREFERENCED_PARAMETER(lpOverlapped);

    WIAEX_ERROR((g_hInst, "This method is not implemented or supported for this driver"));

    HRESULT hr = STIERR_UNSUPPORTED;

    m_hrLastEdviceError = hr;

    WIAEX_TRACE((g_hInst, "IStiUSD::RawReadCommand 0x%08X", hr));

    return hr;
}

/**************************************************************************\
*
* Implements IStiUSD::RawWriteCommand. Writes command information to
* the still image device.
*
* Parameters:
*
*    lpBuffer        - caller-supplied pointer to a buffer containing data
*                      to be sent to the device.
*    dwNumberOfBytes - caller-supplied number of bytes to be written; this is
*                      the number of bytes in the buffer pointed to by lpBuffer.
*    lpOverlapped    - optional, caller-supplied pointer to an OVERLAPPED structure
*                      (described in the Microsoft Windows SDK documentation).
*
* Return Value:
*
*    If the operation succeeds, the method must return S_OK. Otherwise, it
*    should return one of the STIERR-prefixed error codes defined in stierr.h
*    or another standard COM error code. This driver returns STIERR_UNSUPPORTED.
*
\**************************************************************************/

HRESULT CWiaDriver::RawWriteCommand(
    _In_reads_bytes_(dwNumberOfBytes)
             LPVOID        lpBuffer,
             DWORD         dwNumberOfBytes,
    _In_opt_ LPOVERLAPPED  lpOverlapped)
{
    UNREFERENCED_PARAMETER(lpBuffer);
    UNREFERENCED_PARAMETER(dwNumberOfBytes);
    UNREFERENCED_PARAMETER(lpOverlapped);

    WIAEX_ERROR((g_hInst, "This method is not implemented or supported for this driver"));

    HRESULT hr = STIERR_UNSUPPORTED;

    m_hrLastEdviceError = hr;

    WIAEX_TRACE((g_hInst, "IStiUSD::RawWriteCommand 0x%08X", hr));

    return hr;
}

/**************************************************************************\
*
* Implements IStiUSD::GetLastError. Returns the last known error associated
* with the still image device. The driver should record the last STIERR_ error
* code (defined in stierr.h) or generic Win32 error code (retrieved using the
* GetLastError Win32 API) encountered during its operation.
*
* Parameters:
*
*    pdwLastDeviceError - caller-supplied pointer to a buffer in which
*                         the error code will be stored
*
* Return Value:
*
*    If the operation succeeds, the method must return S_OK. Otherwise, it
*    should return one of the STIERR-prefixed error codes defined in stierr.h
*    or another standard COM error code.
*
\**************************************************************************/

HRESULT CWiaDriver::GetLastError(
    _Out_ LPDWORD pdwLastDeviceError)
{
    //
    // When this method is called the driver should also check the scanner state
    // (as when re-initializing the current WIA_DPS_DOCUMENT_HANDLING_STATUS).
    //
    HRESULT hr = S_OK;

    if (!pdwLastDeviceError)
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08X", hr));
        m_hrLastEdviceError = STIERR_INVALID_PARAM;
    }

    if (SUCCEEDED(hr))
    {
        *pdwLastDeviceError = WIN32_FROM_HRESULT(m_hrLastEdviceError);
    }

    WIAEX_TRACE((g_hInst, "IStiUSD::GetLastError 0x%08X (%u, 0x%08X)", hr, m_hrLastEdviceError, m_hrLastEdviceError));

    return hr;
}

/**************************************************************************\
*
* Implements IStiUSD::GetLastErrorInfo. Returns information about the last
* known error associated with a still image device. The driver should record
* the last STIERR_ error code (defined in stierr.h) or generic Win32 error code
* (retrieved using the GetLastError Win32 API) encountered during its operation.
* This error code would be the only information packaged in the STI_ERROR_INFO
* to be returned by this method for the sample driver. The sample driver does
* not provide a vendor error ID and no additional text error description.
*
* Parameters:
*
*    pLastErrorInfo - caller-supplied pointer to an STI_ERROR_INFO structure
*                     to receive error information
*
* Return Value:
*
*    If the operation succeeds, the method must return S_OK. Otherwise, it
*    should return one of the STIERR-prefixed error codes defined in stierr.h
*    or another standard COM error code.
*
\**************************************************************************/

HRESULT CWiaDriver::GetLastErrorInfo(
    _Out_ STI_ERROR_INFO *pLastErrorInfo)
{
    HRESULT hr = S_OK;

    if (!pLastErrorInfo)
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08X",hr));
        m_hrLastEdviceError = STIERR_INVALID_PARAM;
    }

    if (SUCCEEDED(hr))
    {
        memset(pLastErrorInfo, 0, sizeof(STI_ERROR_INFO));
        pLastErrorInfo->dwGenericError = WIN32_FROM_HRESULT(m_hrLastEdviceError);
    }

    WIAEX_TRACE((g_hInst, "IStiUSD::GetLastErrorInfo 0x%08X (%u, 0x%08X)", hr, m_hrLastEdviceError, m_hrLastEdviceError));

    //
    // If successful reset the recorded error to not report it more than once:
    //
    if (SUCCEEDED(hr))
    {
        m_hrLastEdviceError = STI_ERROR_NO_ERROR;
    }

    return hr;
}
