/**************************************************************************
*
*  Copyright © Microsoft Corporation
*
*  File Title:  CapMan.h
*
*  Project:     Production Scanner Driver Sample
*
*  Description: Contains the class declaration for CWIACapabilityManager
*
***************************************************************************/

#pragma once

#define MAX_CAPABILITY_STRING_SIZE_BYTES (sizeof(WCHAR) * MAX_PATH)

class CWIACapabilityManager
{
public:
    CWIACapabilityManager();
    ~CWIACapabilityManager();

public:
    HRESULT
    Initialize(
        _In_ HINSTANCE hInstance);

    void
    Destroy();

    HRESULT
    AddCapability(
             const GUID guidCapability,
             UINT       uiNameResourceID,
             UINT       uiDescriptionResourceID,
             ULONG      ulFlags,
        _In_ LPCWSTR    wszIcon);

    HRESULT
    AllocateCapability(
        _Out_ WIA_DEV_CAP_DRV **ppWIADeviceCapability);

    void
    FreeCapability(
        _In_ WIA_DEV_CAP_DRV *pWIADeviceCapability,
             BOOL             bFreeCapabilityContentOnly = FALSE);

    WIA_DEV_CAP_DRV*
    GetCapabilities();

    WIA_DEV_CAP_DRV*
    GetCommands();

    WIA_DEV_CAP_DRV*
    GetEvents();

    ULONG
    GetNumCapabilities()
    {
        return (m_ulEvents + m_ulCommands);
    }

    ULONG
    GetNumEvents()
    {
        return m_ulEvents;
    }

    ULONG
    GetNumCommands()
    {
        return m_ulCommands;
    }

private:
    HINSTANCE m_hInstance;
    CBasicDynamicArray<WIA_DEV_CAP_DRV> m_CapabilityArray;

    ULONG m_ulEvents;
    ULONG m_ulCommands;
};
