#include "stdafx.h"
#include "WpdNetworkConfig.tmh"

WpdNetworkConfig::WpdNetworkConfig()
{

}

WpdNetworkConfig::~WpdNetworkConfig()
{

}

HRESULT WpdNetworkConfig::Initialize(_In_   FakeDevice *pFakeDevice)
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


HRESULT WpdNetworkConfig::DispatchWpdMessage(_In_    REFPROPERTYKEY          Command,
                                             _In_    IPortableDeviceValues*  pParams,
                                             _In_    IPortableDeviceValues*  pResults)
{
    HRESULT hr = S_OK;

    if (hr == S_OK)
    {
        if (Command.fmtid != WPD_CATEGORY_NETWORK_CONFIGURATION)
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "This object does not support this command category %ws",CComBSTR(Command.fmtid));
        }
    }

    if (hr == S_OK)
    {
        if(IsEqualPropertyKey(Command, WPD_COMMAND_PROCESS_WIRELESS_PROFILE))
        {
            hr = OnProcessWFCObject(pParams, pResults);
            CHECK_HR(hr, "Failed to commit WFC file");
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
 *  This method is called when we receive a WPD_COMMAND_PROCESS_WIRELESS_PROFILE
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_OBJECT_ID: identifies the object to process.
 *
 *  The driver should:
 *  -
 */
HRESULT WpdNetworkConfig::OnProcessWFCObject(
    _In_    IPortableDeviceValues*  pParams,
    _In_    IPortableDeviceValues*  pResults)
{
    HRESULT hr = S_OK;
    LPWSTR  pszObjectID = NULL;

    UNREFERENCED_PARAMETER(pResults);

    // Get the Object ID
    hr = pParams->GetStringValue(WPD_PROPERTY_OBJECT_PROPERTIES_OBJECT_ID, &pszObjectID);
    if (hr == S_OK)
    {
        FakeContent* pElement = NULL;

        // Check if the object exists
        if(m_pFakeDevice->GetContent(pszObjectID, &pElement))
        {
            // Check if the object is of the proper content type
            if (IsEqualGUID(pElement->ContentType, WPD_CONTENT_TYPE_WIRELESS_PROFILE))
            {
                BYTE Buffer[16];
                DWORD dwNumBytesRead = 0;

                // Perform minimal validation on the object contents (just read some bytes for this sample)
                hr = pElement->ReadData(WPD_RESOURCE_DEFAULT, 0, Buffer, sizeof(Buffer), &dwNumBytesRead);
                CHECK_HR(hr, "Failed to read resource data for %ws.%d", CComBSTR(WPD_RESOURCE_DEFAULT.fmtid), WPD_RESOURCE_DEFAULT.pid);
            }
            else
            {
                hr = E_INVALIDARG;
                CHECK_HR(hr, "Invalid ObjectID for OnProcessWFCObject [%ws]", pszObjectID);
            }
        }
        else
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "Invalid ObjectID [%ws]", pszObjectID);
        }

        CoTaskMemFree(pszObjectID);
    }
    else
    {
        CHECK_HR(hr, "Missing or invalid value for WPD_PROPERTY_OBJECT_PROPERTIES_OBJECT_ID");
    }

    return hr;
}


