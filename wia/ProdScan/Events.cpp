/**************************************************************************
*
*  Copyright © Microsoft Corporation
*
*  Title: Events.cpp
*
*  Description: This file contains the IStiUSD methods used for WIA/STI
*               events and helper CWiaDriver methods which are event related.
*
***************************************************************************/

#include "stdafx.h"

/**************************************************************************\
*
* Implements IStiUSD::SetNotificationHandle. Specifies an event handle
* that the driver should use to inform the caller of device events.
* Typically called by the WIA service (the Still Image Event Monitor).
* Also called by the driver itself to control the WIA event mechanism.
*
* The WIA service will pass in a valid handle (created using CreateEvent()),
* indicating that it wants the WIA driver to signal this handle when an
* event occurs in the hardware.
*
* NULL can be passed to this SetNotificationHandle() method. NULL indicates
* that the WIA driver is to STOP all device activity, and exit any event
* wait operations.
*
* Parameters:
*
*    hEvent - HANDLE to an event created by the WIA service using CreateEvent()
*             This parameter can be NULL, indicating that all previous event
*             waiting should be stopped.
*
* Return Value:
*
*    If the operation succeeds, the method should return S_OK.
*    Otherwise, it should return one of the STIERR-prefixed error
*    codes defined in stierr.h.
*
\**************************************************************************/
HRESULT CWiaDriver::SetNotificationHandle(
    _In_opt_ HANDLE hEvent)
{
    if ((hEvent) && (INVALID_HANDLE_VALUE != hEvent))
    {
        //
        // Enable STI/WIA scan events for this driver:
        //

        //
        // This event is created and owned by the caller, which may be
        // either this driver itself or the WIA service. The owner must
        // close the event handle that it owns when calling this method
        // with a different parameter. If we attempt to close the event
        // handle here we cause an exception on checked builds. In the
        // extreme case that the event is not closed by its owner it
        // must be closed by the system when the process (in this case
        // the WIA service process) terminates.
        //
        // if (m_hWiaEvent)
        // {
        //     CloseHandle(m_hWiaEvent);
        //     m_hWiaEvent = NULL;
        // }
        //
        m_hWiaEvent = hEvent;

        //
        // Refresh also the backup copy of the event handle:
        //
        m_hWiaEventStoredCopy = m_hWiaEvent;
    }
    else
    {
        //
        // Disable STI/WIA scan events for this driver - reset
        // the event handle but keep its backup copy, if any
        //
        m_hWiaEvent = NULL;

    }

    return S_OK;
}

/**************************************************************************\
*
* Implements IStiUSD::GetNotificationData. Returns a description of the most
* recent event that occurred on the still image device. If no events have
* occurred since the last time the method was called, the method should return
* STIERR_NOEVENTS. If an event occurred, the driver should return the GUID
* associated with it (STINOTIFY::guidNotificationCode).
*
* GetNotificationData is called both for polled events and interrupt events
* (this driver supports only interrupt events):
*
* 1. [POLLING EVENTS] IStiUSD::GetStatus() reported that there was an event
* pending, by setting the STI_EVENTHANDLING_PENDING flag in the STI_DEVICE_STATUS
* structure. Polling events mechanism is not supported by this driver.
*
* 2. [INTERRUPT EVENTS] The hEvent handle passed in by IStiUSD::SetNotificationHandle()
* was signaled by the hardware, or by calling SetEvent() directly. This is the
* WIA event mechanism this driver supports.
*
* The driver is responsible for filling out the STINOTIFY structure members:
*
* dwSize - size of the STINOTIFY structure.
*
* guidNotificationCode - GUID that represents the event that is to be responded to.
* This should be set to GUID_NULL if no event is to be sent.  This will tell the
* WIA service that no event really happened.
*
* abNotificationData - OPTIONAL - vendor specific information. This data is limited
* to 64 bytes of data ONLY. Not used by this driver.
*
* Parameters:
*
*    hEvent - caller-supplied handle to a Win32 event, created by calling CreateEvent
*
* Return Value:
*
*    If the operation succeeds, the method should return S_OK if there is an event
*    signaled or STIERR_NOEVENTS if there are no events. If an error occurrs
*    it should return one of the other STIERR-prefixed error codes defined in stierr.h.
*
\**************************************************************************/
HRESULT CWiaDriver::GetNotificationData(
    _Out_ LPSTINOTIFY lpNotify)
{
    HRESULT hr = S_OK;

    if (!lpNotify)
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08X",hr));
    }

    //
    // This sample driver does not signal WIA scan events so we'll return here STIERR_NOEVENTS:
    //
    if (SUCCEEDED(hr))
    {
        hr = STIERR_NOEVENTS;
    }

    //
    // A real driver would check if the Hardware device signaled a device event
    // and if so, fill in the lpNotify response data as following:
    //
    // if (SUCCEEDED(hr))
    // {
    //      memset(lpNotify, 0, sizeof(STINOTIFY));
    //      lpNotify->dwSize = sizeof(STINOTIFY);
    //      lpNotify->guidNotificationCode = guidEvent;
    // }
    //
    // where guidEvent would be one of the following:
    //
    // WIA_EVENT_SCAN_IMAGE
    // WIA_EVENT_DEVICE_NOT_READY
    // WIA_EVENT_DEVICE_READY
    // WIA_EVENT_FLATBED_LID_OPEN
    // WIA_EVENT_FLATBED_LID_CLOSED
    // WIA_EVENT_FEEDER_LOADED
    // WIA_EVENT_FEEDER_EMPTIED
    // WIA_EVENT_COVER_OPEN
    // WIA_EVENT_COVER_CLOSED
    //
    // or any other WIA_NOTIFICATION_EVENT and/or WIA_ACTION_EVENT reported to
    // IWiaMiniDrv::drvGetCapabilities (see CWiaDriver::drvGetCapabilities).
    //

    if (FAILED(hr) && (STIERR_NOEVENTS != hr))
    {
        m_hrLastEdviceError = hr;
    }

    return hr;
}
