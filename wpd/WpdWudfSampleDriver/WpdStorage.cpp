#include "stdafx.h"
#include "WpdStorage.tmh"

WpdStorage::WpdStorage()
{

}

WpdStorage::~WpdStorage()
{

}

HRESULT WpdStorage::Initialize(_In_ FakeDevice *pFakeDevice)
{

    HRESULT hr = S_OK;

    if(pFakeDevice == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }
    m_pFakeDevice = pFakeDevice;
    return hr;
}


HRESULT WpdStorage::DispatchWpdMessage(_In_ REFPROPERTYKEY          Command,
                                       _In_ IPortableDeviceValues*  pParams,
                                       _In_ IPortableDeviceValues*  pResults)
{

    HRESULT hr = S_OK;

    if (hr == S_OK)
    {
        if (Command.fmtid != WPD_CATEGORY_STORAGE)
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "This object does not support this command category %ws",CComBSTR(Command.fmtid));
        }
    }

    if (hr == S_OK)
    {
        if (IsEqualPropertyKey(Command, WPD_COMMAND_STORAGE_FORMAT))
        {
            hr = OnFormat(pParams, pResults);
            CHECK_HR(hr, "Failed to format storage");
        }
        else
        {
            hr = E_NOTIMPL;
            CHECK_HR(hr, "This object does not support this command id %d", Command.pid);
        }
    }
    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_STORAGE_FORMAT
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_STORAGE_OBJECT_ID: identifies the storage object to format.
 *
 *  The driver should:
 *  - Format the storage identified by WPD_PROPERTY_STORAGE_OBJECT_ID.
 */
HRESULT WpdStorage::OnFormat(
    _In_ IPortableDeviceValues*  pParams,
    _In_ IPortableDeviceValues*  pResults)
{
    HRESULT hr                  = S_OK;
    LPWSTR  pszObjectID         = NULL;

    UNREFERENCED_PARAMETER(pResults);

    // Get the Object ID
    hr = pParams->GetStringValue(WPD_PROPERTY_STORAGE_OBJECT_ID, &pszObjectID);
    if (hr != S_OK)
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "Missing string value for WPD_PROPERTY_STORAGE_OBJECT_ID");
    }

    // Format this storage
    if (hr == S_OK)
    {
        hr = m_pFakeDevice->FormatStorage(pszObjectID, pParams);
        CHECK_HR(hr, "Failed to process format command on [%ws]", pszObjectID);
    }

    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
    CoTaskMemFree(pszObjectID);

    return hr;
}


