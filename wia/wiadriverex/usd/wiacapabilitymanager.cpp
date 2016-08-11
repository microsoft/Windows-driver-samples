/*****************************************************************************
 *
 *  wiacapabilitymanager.cpp
 *
 *  Copyright (c) 2003 Microsoft Corporation.  All Rights Reserved.
 *
 *  DESCRIPTION:
 *
 *  Helper class for WIA capabilities
 *  
 *******************************************************************************/

#include "stdafx.h"
#include <strsafe.h>

CWIACapabilityManager::CWIACapabilityManager()
{

}

CWIACapabilityManager::~CWIACapabilityManager()
{
    Destroy();
}

HRESULT CWIACapabilityManager::Initialize(_In_ HINSTANCE hInstance)
{
    HRESULT hr = E_INVALIDARG;
    if(hInstance)
    {
        m_hInstance = hInstance;
        hr = S_OK;
    }
    else
    {
        WIAS_ERROR((g_hInst, "Invalid parameters were passed"));
    }
    return hr;
}

void CWIACapabilityManager::Destroy()
{
    WIAS_TRACE((g_hInst,"Array contents"));
    for(INT i = 0; i < m_CapabilityArray.Size(); i++)
    {
        FreeCapability(&m_CapabilityArray[i],TRUE);
    }
    m_CapabilityArray.Destroy();
}

HRESULT CWIACapabilityManager::AddCapability(const GUID    guidCapability,
                                             UINT          uiNameResourceID,
                                             UINT          uiDescriptionResourceID,
                                             ULONG         ulFlags,
                                             _In_ LPWSTR   wszIcon)
{
    HRESULT hr = S_OK;

    WIA_DEV_CAP_DRV *pWIADeviceCapability = NULL;
    hr = AllocateCapability(&pWIADeviceCapability);
    if((SUCCEEDED(hr)&& (pWIADeviceCapability)))
    {
        pWIADeviceCapability->ulFlags = ulFlags;
        *pWIADeviceCapability->guid   = guidCapability;

        CBasicStringWide cswCapabilityString;

        //
        // Load capability name from resource
        //

        if(cswCapabilityString.LoadString(uiNameResourceID,m_hInstance))
        {
            hr = StringCbCopyW(pWIADeviceCapability->wszName,
                               MAX_CAPABILITY_STRING_SIZE_BYTES,
                               cswCapabilityString.String());
            if(FAILED(hr))
            {
                WIAS_ERROR((g_hInst, "Failed to copy source string (%ws) to destination string, hr = 0x%lx",cswCapabilityString.String(),hr));
            }
        }
        else
        {
            hr = E_FAIL;
            WIAS_ERROR((g_hInst, "Failed to load the device capability name string from DLL resource, hr = 0x%lx",hr));
        }

        //
        // Load capability description from resource
        //

        if(cswCapabilityString.LoadString(uiDescriptionResourceID,m_hInstance))
        {
            hr = StringCbCopyW(pWIADeviceCapability->wszDescription,
                               MAX_CAPABILITY_STRING_SIZE_BYTES,
                               cswCapabilityString.String());
            if(FAILED(hr))
            {
                WIAS_ERROR((g_hInst, "Failed to copy source string (%ws) to destination string, hr = 0x%lx",cswCapabilityString.String(),hr));
            }
        }
        else
        {
            hr = E_FAIL;
            WIAS_ERROR((g_hInst, "Failed to load the device capability description string from DLL resource, hr = 0x%lx",hr));
        }

        //
        // Copy icon location string
        //

        cswCapabilityString = wszIcon;

        if(cswCapabilityString.Length())
        {
            hr = StringCbCopyW(pWIADeviceCapability->wszIcon,
                               MAX_CAPABILITY_STRING_SIZE_BYTES,
                               cswCapabilityString.String());
            if(FAILED(hr))
            {
                WIAS_ERROR((g_hInst, "Failed to copy source string (%ws) to destination string, hr = 0x%lx",cswCapabilityString.String(),hr));
            }
        }
        else
        {
            hr = E_FAIL;
            WIAS_ERROR((g_hInst, "Failed to load the device capability icon location string from DLL resource, hr = 0x%lx",hr));
        }

        if(SUCCEEDED(hr))
        {
            if((pWIADeviceCapability->ulFlags == WIA_NOTIFICATION_EVENT) ||
               (pWIADeviceCapability->ulFlags == WIA_ACTION_EVENT))
            {
                //
                // The capability being added is an event, so always add it to the beginning of the array
                //

                m_CapabilityArray.Insert(*pWIADeviceCapability,0);
            }
            else
            {
                //
                // The capability being added is a command, so always add it to the end of the array
                //

                m_CapabilityArray.Append(*pWIADeviceCapability);
            }
        }

        if(pWIADeviceCapability)
        {
            CoTaskMemFree(pWIADeviceCapability);
            pWIADeviceCapability = NULL;
        }
    }
    return hr;
}

HRESULT CWIACapabilityManager::AllocateCapability(_Out_ WIA_DEV_CAP_DRV **ppWIADeviceCapability)
{
    HRESULT hr = E_INVALIDARG;
    if(ppWIADeviceCapability)
    {
        *ppWIADeviceCapability                = NULL;
        WIA_DEV_CAP_DRV *pWIADeviceCapability = NULL;

#pragma prefast(suppress:__WARNING_MEMORY_LEAK, "Freed when calling FreeCapability(*ppWIADeviceCapability).")
        pWIADeviceCapability = (WIA_DEV_CAP_DRV*)CoTaskMemAlloc(sizeof(WIA_DEV_CAP_DRV));
        if(pWIADeviceCapability)
        {
            memset(pWIADeviceCapability,0,sizeof(WIA_DEV_CAP_DRV));
            //
            // attempt to allocate the GUID member of the structure
            //
			
#pragma prefast(suppress:__WARNING_MEMORY_LEAK, "Freed when calling FreeCapability(*ppWIADeviceCapability).")
            pWIADeviceCapability->guid = (GUID*)CoTaskMemAlloc(sizeof(GUID));
            if(pWIADeviceCapability->guid)
            {
                *pWIADeviceCapability->guid = GUID_NULL;
                hr = S_OK;
            }
            else
            {
                hr = E_OUTOFMEMORY;
                WIAS_ERROR((g_hInst, "Failed to allocate memory for GUID member of WIA_DEV_CAP_DRV structure, hr = 0x%lx",hr));
            }

            //
            // attempt to allocate the LPOLESTR name member of the structure
            //

            if(SUCCEEDED(hr))
            {
#pragma prefast(suppress:__WARNING_MEMORY_LEAK, "Freed when calling FreeCapability(*ppWIADeviceCapability).")
                pWIADeviceCapability->wszName = (LPOLESTR)CoTaskMemAlloc(MAX_CAPABILITY_STRING_SIZE_BYTES);
                if(pWIADeviceCapability->wszName)
                {
                    memset(pWIADeviceCapability->wszName,0,MAX_CAPABILITY_STRING_SIZE_BYTES);
                    hr = S_OK;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    WIAS_ERROR((g_hInst, "Failed to allocate memory for LPOLESTR (wszName) member of WIA_DEV_CAP_DRV structure, hr = 0x%lx",hr));
                }
            }

            //
            // attempt to allocate the LPOLESTR description member of the structure
            //

            if(SUCCEEDED(hr))
            {
#pragma prefast(suppress:__WARNING_MEMORY_LEAK, "Freed when calling FreeCapability(*ppWIADeviceCapability).")
                pWIADeviceCapability->wszDescription = (LPOLESTR)CoTaskMemAlloc(MAX_CAPABILITY_STRING_SIZE_BYTES);
                if(pWIADeviceCapability->wszDescription)
                {
                    memset(pWIADeviceCapability->wszDescription,0,MAX_CAPABILITY_STRING_SIZE_BYTES);
                    hr = S_OK;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    WIAS_ERROR((g_hInst, "Failed to allocate memory for LPOLESTR (wszDescription) member of WIA_DEV_CAP_DRV structure, hr = 0x%lx",hr));
                }
            }

            //
            // attempt to allocate the LPOLESTR icon member of the structure
            //

            if(SUCCEEDED(hr))
            {
#pragma prefast(suppress:__WARNING_MEMORY_LEAK, "Freed when calling FreeCapability(*ppWIADeviceCapability).")
                pWIADeviceCapability->wszIcon = (LPOLESTR)CoTaskMemAlloc(MAX_CAPABILITY_STRING_SIZE_BYTES);
                if(pWIADeviceCapability->wszIcon)
                {
                    memset(pWIADeviceCapability->wszIcon,0,MAX_CAPABILITY_STRING_SIZE_BYTES);
                    hr = S_OK;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    WIAS_ERROR((g_hInst, "Failed to allocate memory for LPOLESTR (wszIcon) member of WIA_DEV_CAP_DRV structure, hr = 0x%lx",hr));
                }
            }

            if(SUCCEEDED(hr))
            {
                *ppWIADeviceCapability = pWIADeviceCapability;
            }
            else
            {
                FreeCapability(pWIADeviceCapability);
                pWIADeviceCapability = NULL;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
            WIAS_ERROR((g_hInst, "Failed to allocate memory for WIA_DEV_CAP_DRV structure, hr = 0x%lx",hr));
        }
    }
    return hr;
}

void CWIACapabilityManager::FreeCapability(_In_ WIA_DEV_CAP_DRV *pWIADeviceCapability, 
                                                BOOL            bFreeCapabilityContentOnly)
{
    if(pWIADeviceCapability)
    {
        if(pWIADeviceCapability->guid)
        {
            CoTaskMemFree(pWIADeviceCapability->guid);
            pWIADeviceCapability->guid = NULL;
        }

        if(pWIADeviceCapability->wszName)
        {
            CoTaskMemFree(pWIADeviceCapability->wszName);
            pWIADeviceCapability->wszName = NULL;
        }

        if(pWIADeviceCapability->wszDescription)
        {
            CoTaskMemFree(pWIADeviceCapability->wszDescription);
            pWIADeviceCapability->wszDescription = NULL;
        }

        if(pWIADeviceCapability->wszIcon)
        {
            CoTaskMemFree(pWIADeviceCapability->wszIcon);
            pWIADeviceCapability->wszIcon = NULL;
        }

        if(bFreeCapabilityContentOnly == FALSE)
        {
            CoTaskMemFree(pWIADeviceCapability);
            pWIADeviceCapability = NULL;
        }
    }
    else
    {
        WIAS_ERROR((g_hInst, "Invalid parameters were passed, caller attempted to free a NULL WIA_DEV_CAP_DRV structure"));
    }
}

HRESULT CWIACapabilityManager::DeleteCapability(const GUID  guidCapability,
                                                ULONG       ulFlags)
{
    UNREFERENCED_PARAMETER(guidCapability);
    UNREFERENCED_PARAMETER(ulFlags);

    return E_NOTIMPL;
}

LONG CWIACapabilityManager::GetNumCapabilities()
{
    return (LONG)m_CapabilityArray.Size();
}

LONG CWIACapabilityManager::GetNumCommands()
{
    LONG lNumCommands = 0;
    for(INT i = 0; i < m_CapabilityArray.Size(); i++)
    {
        if((m_CapabilityArray[i].ulFlags != WIA_NOTIFICATION_EVENT) &&
           (m_CapabilityArray[i].ulFlags != WIA_ACTION_EVENT))
        {
            lNumCommands++;
        }
    }
    return lNumCommands;
}

LONG CWIACapabilityManager::GetNumEvents()
{
    LONG lNumEvents = 0;
    for(INT i = 0; i < m_CapabilityArray.Size(); i++)
    {
        if((m_CapabilityArray[i].ulFlags == WIA_NOTIFICATION_EVENT) ||
           (m_CapabilityArray[i].ulFlags == WIA_ACTION_EVENT))
        {
            lNumEvents++;
        }
    }
    return lNumEvents;
}

WIA_DEV_CAP_DRV* CWIACapabilityManager::GetCapabilities()
{
    return &m_CapabilityArray[0];
}

WIA_DEV_CAP_DRV* CWIACapabilityManager::GetCommands()
{
    return &m_CapabilityArray[GetNumEvents()];
}

WIA_DEV_CAP_DRV* CWIACapabilityManager::GetEvents()
{
    return &m_CapabilityArray[0];
}

