/**************************************************************************
*
*  Copyright © Microsoft Corporation
*
*  File Title:  CapMan.cpp
*
*  Project:     Production Scanner Driver Sample
*
*  Description: Contains the implementation for the CWIACapabilityManager
*               class, a helper class for managing WIA capabilities
*
***************************************************************************/

#include "stdafx.h"

/**************************************************************************\
*
* CWIACapabilityManager constructor
*
\**************************************************************************/

CWIACapabilityManager::CWIACapabilityManager()
{
    m_ulEvents = 0;
    m_ulCommands = 0;
}

/**************************************************************************\
*
* CWIACapabilityManager destructor
*
\**************************************************************************/

CWIACapabilityManager::~CWIACapabilityManager()
{
    Destroy();
}

/**************************************************************************\
*
* Parameters:
*
*    hInstance - module instance handle used for driver resources
*
* Return Value:
*
*    S_OK or a standard COM error code
*
\**************************************************************************/

HRESULT
CWIACapabilityManager::Initialize(
    _In_ HINSTANCE hInstance)
{
    HRESULT hr = S_OK;

    if (!hInstance)
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08X", hr));
    }

    if (SUCCEEDED(hr))
    {
        m_hInstance = hInstance;
    }

    return hr;
}

/**************************************************************************\
*
* Parameters:
*
*    None
*
* Return Value:
*
*    None
*
\**************************************************************************/

void
CWIACapabilityManager::Destroy()
{
    INT nCapabilities = m_CapabilityArray.Size();

    for (INT i = 0; i < nCapabilities; i++)
    {
        FreeCapability(&m_CapabilityArray[i], TRUE);
    }
    m_CapabilityArray.Destroy();

    m_ulEvents = 0;
    m_ulCommands = 0;
}

/**************************************************************************\
*
* Parameters:
*
*   guidCapability          - capability identifier
*   uiNameResourceID        - capability name
*   uiDescriptionResourceID - capability description
*   ulFlags                 - capability flags
*   wszIcon                 - capability icon
*
* Return Value:
*
*    S_OK or a standard COM error code
*
\**************************************************************************/

HRESULT
CWIACapabilityManager::AddCapability(
         const GUID guidCapability,
         UINT       uiNameResourceID,
         UINT       uiDescriptionResourceID,
         ULONG      ulFlags,
    _In_ LPCWSTR    wszIcon)
{
    HRESULT hr = S_OK;
    WIA_DEV_CAP_DRV *pWIADeviceCapability = NULL;
    WCHAR wCapabilityString[MAX_PATH] = {};

    if (!wszIcon)
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08X", hr));
    }

    //
    // Allocate memory for the new capability structure:
    //
    if (SUCCEEDED(hr))
    {
        hr = AllocateCapability(&pWIADeviceCapability);
        if (SUCCEEDED(hr) && (!pWIADeviceCapability))
        {
            hr = E_POINTER;
            WIAEX_ERROR((g_hInst, "AllocateCapability failed to return a valid pointer, hr = 0x%08X", hr));
        }
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Cannot allocate memory for capability, hr = 0x%08X", hr));
        }
    }

    //
    // Copy the capability flags and GUID identifier:
    //
    if (SUCCEEDED(hr))
    {
        pWIADeviceCapability->ulFlags = ulFlags;
        *pWIADeviceCapability->guid = guidCapability;
    }

    //
    // Load and copy the capability name from resources:
    //
    if (SUCCEEDED(hr))
    {
        if (LoadString(m_hInstance, uiNameResourceID, wCapabilityString, ARRAYSIZE(wCapabilityString)))
        {
            hr = StringCbCopyW(pWIADeviceCapability->wszName, MAX_CAPABILITY_STRING_SIZE_BYTES, wCapabilityString);
            if(FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to copy source string (%ws) to destination string, hr = 0x%08X", wCapabilityString, hr));
            }
        }
        else
        {
            hr = E_FAIL;
            WIAEX_ERROR((g_hInst, "Failed to load the device capability name string %u from resources, hr = 0x%08X", uiNameResourceID, hr));
        }
    }

    //
    // Load and copy the capability description from resources:
    //
    if (SUCCEEDED(hr))
    {
        memset(wCapabilityString, 0, sizeof(wCapabilityString));
        if (LoadString(m_hInstance, uiDescriptionResourceID, wCapabilityString, ARRAYSIZE(wCapabilityString)))
        {
            hr = StringCbCopyW(pWIADeviceCapability->wszDescription, MAX_CAPABILITY_STRING_SIZE_BYTES, wCapabilityString);
            if(FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to copy source string (%ws) to destination string, hr = 0x%08X", wCapabilityString, hr));
            }
        }
        else
        {
            hr = E_FAIL;
            WIAEX_ERROR((g_hInst, "Failed to load the device capability description string %u from DLL resource, hr = 0x%08X",
                uiDescriptionResourceID, hr));
        }
    }

    //
    // Copy the icon location:
    //
    if (SUCCEEDED(hr))
    {
        memset(pWIADeviceCapability->wszIcon, 0, MAX_CAPABILITY_STRING_SIZE_BYTES);

        hr = StringCbCopyW(pWIADeviceCapability->wszIcon, MAX_CAPABILITY_STRING_SIZE_BYTES, wszIcon);
        if(FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to copy source string (%ws) to destination string, hr = 0x%08X", wszIcon, hr));
        }
    }

    //
    // Add the new capability item to the array:
    //
    if(SUCCEEDED(hr))
    {
        if ((WIA_NOTIFICATION_EVENT & pWIADeviceCapability->ulFlags) || (WIA_ACTION_EVENT & pWIADeviceCapability->ulFlags))
        {
            //
            // Event capabilities are inserted at the beginning of the array:
            //
            m_CapabilityArray.Insert(*pWIADeviceCapability, 0);

            m_ulEvents += 1;
        }
        else
        {
            //
            // Command capabilities are appended at the end of the array:
            //
            m_CapabilityArray.Append(*pWIADeviceCapability);

            m_ulCommands += 1;
        }

        if ((m_ulEvents + m_ulCommands) != (ULONG)m_CapabilityArray.Size())
        {
            WIAEX_ERROR((g_hInst, "Counted %u capabilities (%u events, %u commands), recorded %u..",
                m_ulEvents + m_ulCommands, m_ulEvents, m_ulCommands, m_CapabilityArray.Size()));
        }
    }

    //
    // Clean up:
    //

    if (pWIADeviceCapability)
    {
        CoTaskMemFree(pWIADeviceCapability);
        pWIADeviceCapability = NULL;
    }

    return hr;
}

/**************************************************************************\
*
* Parameters:
*
*   ppWIADeviceCapability - pointer to capability object to allocate
*
* Return Value:
*
*    S_OK or a standard COM error code
*
\**************************************************************************/

HRESULT
CWIACapabilityManager::AllocateCapability(
    _Out_ WIA_DEV_CAP_DRV **ppWIADeviceCapability)
{
    HRESULT hr = S_OK;
    WIA_DEV_CAP_DRV *pWIADeviceCapability = NULL;

    if (!ppWIADeviceCapability)
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08X", hr));
    }

    if (SUCCEEDED(hr))
    {
        *ppWIADeviceCapability = NULL;

#pragma prefast(suppress:__WARNING_MEMORY_LEAK, "pWIADeviceCapability is freed with FreeCapability when the call fails")
        pWIADeviceCapability = (WIA_DEV_CAP_DRV*)CoTaskMemAlloc(sizeof(WIA_DEV_CAP_DRV));
        if (!pWIADeviceCapability)
        {
            hr = E_OUTOFMEMORY;
            WIAEX_ERROR((g_hInst, "Failed to allocate memory for WIA_DEV_CAP_DRV structure, hr = 0x%08X", hr));
        }
        else
        {
            //
            // Successfully created WIA_DEV_CAP_DRV, now initialize to 0.
            //
            memset(pWIADeviceCapability, 0, sizeof(WIA_DEV_CAP_DRV));
        }
    }

    if (SUCCEEDED(hr))
    {
#pragma prefast(suppress:__WARNING_MEMORY_LEAK, "Freed with FreeCapability when the call fails")
        pWIADeviceCapability->guid = (GUID*)CoTaskMemAlloc(sizeof(GUID));
        if (!pWIADeviceCapability->guid)
        {
            hr = E_OUTOFMEMORY;
            WIAEX_ERROR((g_hInst, "Failed to allocate memory for GUID member of WIA_DEV_CAP_DRV structure, hr = 0x%08X", hr));
        }
    }

    if (SUCCEEDED(hr))
    {
#pragma prefast(suppress:__WARNING_MEMORY_LEAK, "Freed with FreeCapability when the call fails")
        pWIADeviceCapability->wszName = (LPOLESTR)CoTaskMemAlloc(MAX_CAPABILITY_STRING_SIZE_BYTES);
        if (!pWIADeviceCapability->wszName)
        {
            hr = E_OUTOFMEMORY;
            WIAEX_ERROR((g_hInst, "Failed to allocate memory for LPOLESTR (wszName) member of WIA_DEV_CAP_DRV structure, hr = 0x%08X", hr));
        }
    }

    if (SUCCEEDED(hr))
    {
#pragma prefast(suppress:__WARNING_MEMORY_LEAK, "Freed with FreeCapability when the call fails")
        pWIADeviceCapability->wszDescription = (LPOLESTR)CoTaskMemAlloc(MAX_CAPABILITY_STRING_SIZE_BYTES);
        if (!pWIADeviceCapability->wszDescription)
        {
            hr = E_OUTOFMEMORY;
            WIAEX_ERROR((g_hInst, "Failed to allocate memory for LPOLESTR (wszDescription) member of WIA_DEV_CAP_DRV structure, hr = 0x%08X", hr));
        }
    }

    if (SUCCEEDED(hr))
    {
#pragma prefast(suppress:__WARNING_MEMORY_LEAK, "Freed with FreeCapability when the call fails")
        pWIADeviceCapability->wszIcon = (LPOLESTR)CoTaskMemAlloc(MAX_CAPABILITY_STRING_SIZE_BYTES);
        if (!pWIADeviceCapability->wszIcon)
        {
            hr = E_OUTOFMEMORY;
            WIAEX_ERROR((g_hInst, "Failed to allocate memory for LPOLESTR (wszIcon) member of WIA_DEV_CAP_DRV structure, hr = 0x%08X", hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        *pWIADeviceCapability->guid = GUID_NULL;
        memset(pWIADeviceCapability->wszName, 0, MAX_CAPABILITY_STRING_SIZE_BYTES);
        memset(pWIADeviceCapability->wszDescription, 0, MAX_CAPABILITY_STRING_SIZE_BYTES);
        memset(pWIADeviceCapability->wszIcon, 0, MAX_CAPABILITY_STRING_SIZE_BYTES);

        *ppWIADeviceCapability = pWIADeviceCapability;
    }
    else if (pWIADeviceCapability)
    {
        FreeCapability(pWIADeviceCapability);
        pWIADeviceCapability = NULL;
    }

    return hr;
}

/**************************************************************************\
*
* Parameters:
*
*   pWIADeviceCapability       - pointer to capability object to free
*   bFreeCapabilityContentOnly - TRUE to free only the capability content
*                                or FALSE to free the entire object instance
*
* Return Value:
*
*    S_OK or a standard COM error code
*
\**************************************************************************/

void
CWIACapabilityManager::FreeCapability(
    _In_ WIA_DEV_CAP_DRV *pWIADeviceCapability,
         BOOL             bFreeCapabilityContentOnly)
{
    if (pWIADeviceCapability)
    {
        if (pWIADeviceCapability->guid)
        {
            CoTaskMemFree(pWIADeviceCapability->guid);
            pWIADeviceCapability->guid = NULL;
        }

        if (pWIADeviceCapability->wszName)
        {
            CoTaskMemFree(pWIADeviceCapability->wszName);
            pWIADeviceCapability->wszName = NULL;
        }

        if (pWIADeviceCapability->wszDescription)
        {
            CoTaskMemFree(pWIADeviceCapability->wszDescription);
            pWIADeviceCapability->wszDescription = NULL;
        }

        if (pWIADeviceCapability->wszIcon)
        {
            CoTaskMemFree(pWIADeviceCapability->wszIcon);
            pWIADeviceCapability->wszIcon = NULL;
        }

        if (!bFreeCapabilityContentOnly)
        {
            CoTaskMemFree(pWIADeviceCapability);
        }
    }
    else
    {
        WIAEX_ERROR((g_hInst, "Invalid parameter, caller attempted to free a NULL WIA_DEV_CAP_DRV structure"));
    }
}

/**************************************************************************\
*
* Parameters:
*
*   None
*
* Return Value:
*
*   Pointer to the capability manager's list of capabilities
*
\**************************************************************************/

WIA_DEV_CAP_DRV*
CWIACapabilityManager::GetCapabilities()
{
    WIA_DEV_CAP_DRV* pCapabilities = NULL;

    if ((!m_ulEvents) && (!m_ulCommands))
    {
        WIAEX_ERROR((g_hInst, "No capabilities to return"));
        pCapabilities = NULL;
    }
    else
    {
        pCapabilities = &m_CapabilityArray[0];
    }

    return pCapabilities;
}

/**************************************************************************\
*
* Parameters:
*
*   None
*
* Return Value:
*
*   Pointer to the capability manager's list of command capabilities
*
\**************************************************************************/

WIA_DEV_CAP_DRV*
CWIACapabilityManager::GetCommands()
{
    WIA_DEV_CAP_DRV* pCommands = NULL;

    if (!m_ulCommands)
    {
        WIAEX_ERROR((g_hInst, "No commands to return"));
        pCommands = NULL;
    }
    else
    {
        //
        // Command capabilities are stored at the end of the capability list, after the event capabilities:
        //
        pCommands =  &m_CapabilityArray[m_ulEvents];
    }

    return pCommands;
}

/**************************************************************************\
*
* Parameters:
*
*   None
*
* Return Value:
*
*   Pointer to the capability manager's list of event capabilities
*
\**************************************************************************/

WIA_DEV_CAP_DRV*
CWIACapabilityManager::GetEvents()
{
    WIA_DEV_CAP_DRV* pEvents = NULL;

    if (!m_ulEvents)
    {
        WIAEX_ERROR((g_hInst, "No events to return"));
        pEvents = NULL;
    }
    else
    {
        //
        // Event capabilities are stored at the beginning of the capability list:
        //
        pEvents = &m_CapabilityArray[0];
    }

    return pEvents;
}
