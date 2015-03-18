#include "stdafx.h"

#include "WpdService.tmh"

WpdService::WpdService() : m_pContactsService(NULL)
{
}

WpdService::~WpdService()
{

}

HRESULT WpdService::Initialize(_In_ FakeDevice* pDevice)
{
    if (pDevice == NULL)
    {
        return E_POINTER;
    }
    m_pContactsService = pDevice->GetContactsService();
    m_ServiceMethods.Initialize(m_pContactsService);
    m_ServiceCapabilities.Initialize(m_pContactsService);
    return S_OK;
}

HRESULT WpdService::DispatchWpdMessage(
    _In_    REFPROPERTYKEY         Command,
    _In_    IPortableDeviceValues* pParams,
    _In_    IPortableDeviceValues* pResults)
{
    HRESULT     hr                  = S_OK;
    LPWSTR      pszRequestFilename  = NULL;

    // Get the request filename to process the service message
    hr = pParams->GetStringValue(PRIVATE_SAMPLE_DRIVER_REQUEST_FILENAME, &pszRequestFilename);
    if (FAILED(hr))
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "Failed to get the required requested filename");
    }

    if (hr == S_OK)
    {    
        hr = CheckRequestFilename(pszRequestFilename);
        CHECK_HR(hr, "Unknown request filename %ws received", pszRequestFilename);
    }

    if (hr == S_OK)
    {
        if (Command.fmtid == WPD_CATEGORY_SERVICE_CAPABILITIES)
        {
            hr = m_ServiceCapabilities.DispatchWpdMessage(Command, pParams, pResults);            
        }
        else if (IsEqualPropertyKey(Command, WPD_COMMAND_SERVICE_METHODS_START_INVOKE))
        {
            hr = m_ServiceMethods.OnStartInvoke(pParams, pResults);
        }
        else if (IsEqualPropertyKey(Command, WPD_COMMAND_SERVICE_METHODS_END_INVOKE))
        {
            hr = m_ServiceMethods.OnEndInvoke(pParams, pResults);
        }
        else if (IsEqualPropertyKey(Command, WPD_COMMAND_SERVICE_METHODS_CANCEL_INVOKE))
        {
            hr = m_ServiceMethods.OnCancelInvoke(pParams, pResults);
        }
        else if (IsEqualPropertyKey(Command, WPD_COMMAND_SERVICE_COMMON_GET_SERVICE_OBJECT_ID))
        {
            hr = OnGetServiceObjectID(pszRequestFilename, pParams, pResults);
        }
        else
        {
            hr = E_NOTIMPL;
            CHECK_HR(hr, "Unknown command %ws.%d received",CComBSTR(Command.fmtid), Command.pid);
        }
    }

    CoTaskMemFree(pszRequestFilename);

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_SERVICE_COMMON_GET_SERVICE_OBJECT_ID 
 *  command.
 *
 *  The parameters sent to us are:
 *  None
 *
 *  The driver should:
 *  - Return the objectID associated with the filename. 
 *
 */
HRESULT WpdService::OnGetServiceObjectID(
    _In_    LPCWSTR                pszRequestFilename,
    _In_    IPortableDeviceValues* pParams,
    _In_    IPortableDeviceValues* pResults)
{
    HRESULT hr = S_OK;

    if((pParams    == NULL) ||
       (pResults   == NULL))
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    // For simplicity, the request filename is the same as the service object ID
    hr = pResults->SetStringValue(WPD_PROPERTY_SERVICE_OBJECT_ID, pszRequestFilename);
    CHECK_HR(hr, "Failed to set WPD_PROPERTY_COMMON_OBJECT_IDS");

    return hr;
}

HRESULT WpdService::CheckRequestFilename(
    _In_    LPCWSTR pszRequestFilename)
{
    HRESULT     hr                 = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    CAtlStringW strRequestFilename = pszRequestFilename;

    // For simplicity, the request filename happens to be the same as the service object ID
    if (strRequestFilename.CompareNoCase(m_pContactsService->GetRequestFilename()) == 0)
    {
        hr = S_OK;
    }
    else
    {
        CHECK_HR(hr, "Unknown request filename %ws received", pszRequestFilename);
    }

    return hr;
}
