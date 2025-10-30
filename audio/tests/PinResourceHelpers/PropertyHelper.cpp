// ------------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// Module Name:
//
//  PropertyHelper.cpp
//
// Abstract:
//
//  Implementation for common helpers of endpoint property
//
// -------------------------------------------------------------------------------
#include "PreComp.h"
#include <AVEndpointKeys.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <devpkey.h>
#include <PropertyHelper.h>
#include <imagehlp.h>

using namespace WEX::Common;
using namespace WEX::Logging;
using namespace WEX::TestExecution;

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// GetConnectorId
//
// Retrieve the connector id from the property store and checks if the connector exists
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT GetConnectorId
(
    IMMDevice*              pDevice,
    EndpointConnectorType   eConnectorType,
    bool*                   hasConnector,
    UINT*                   pConnectorId
)

















































///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// GetEndpointFriendlyName
//
// Retrieve the endpoint friendly name from the property store
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT GetEndpointFriendlyName
(
    IMMDevice*              pDevice,
    LPWSTR*                 ppwszEndpointName
)
{
    HRESULT                                     hr = S_OK;
    wil::com_ptr_nothrow<IPropertyStore>        spPropertyStore;
    wil::unique_prop_variant                    var;
    wil::unique_cotaskmem_string                pwszName;

    *ppwszEndpointName = NULL;

    // Retrieve the endpoint friendly name from the propertey store
    if (!VERIFY_SUCCEEDED(hr = pDevice->OpenPropertyStore(STGM_READ, &spPropertyStore))) {
        return hr;
    }
    if (!VERIFY_SUCCEEDED(hr = spPropertyStore->GetValue(PKEY_Device_FriendlyName, &var))) {
        return hr;
    }
    if (!VERIFY_IS_TRUE(var.vt == VT_LPWSTR)) {
        hr = E_FAIL;
        return hr;
    }

    pwszName = wil::make_cotaskmem_string_nothrow(var.pwszVal);
    if (!VERIFY_IS_TRUE(pwszName.get() && wcslen(pwszName.get()) != 0)) {
        hr = E_FAIL;
        return hr;
    }

    *ppwszEndpointName = pwszName.release();

    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// GetAudioFilterAsDevice
//
// Get the audio filter from MMDevice. This method is called to activate IKsControl, IKsFormatSupport or IKsGetProposedFormat.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT GetAudioFilterAsDevice
(
    IMMDevice* pDevice,
    IMMDevice** ppAudioFilterAsDevice
)



































///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// GetCachedProcessingModes
//
// Read the processing mode characteristics from the property store and get all processing modes. This method applies to host and keyword detector pins only.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT GetCachedProcessingModes
(
    IMMDevice*              pDevice,
    EndpointConnectorType   eConnectorType,
    ULONG                   *pCount,
    AUDIO_SIGNALPROCESSINGMODE **ppModes
)
















































































































///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// GetProcessingModes
//
// Query for connector's processing modes property
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT GetProcessingModes
(
    IMMDevice*              pDevice,
    UINT                    pinId,
    ULONG                   *pCount,
    AUDIO_SIGNALPROCESSINGMODE **ppModes
)
{
    HRESULT                                     hr = S_OK;
    wil::com_ptr_nothrow<IMMDevice>             spAudioFilter;
    wil::com_ptr_nothrow<IKsControl>            spKsControl;
    KSP_PIN                                     pinProp;
    ULONG                                       cbNeeded = 0;
    ULONG                                       cbProperty;
    PKSMULTIPLE_ITEM                            pProperty = NULL;
    ULONG                                       cbReturned = 0;
    AUDIO_SIGNALPROCESSINGMODE*                 pModes = NULL;
    CComHeapPtr<AUDIO_SIGNALPROCESSINGMODE>     spModes;

    *pCount = 0;
    *ppModes = NULL;

    // Get the audio filter
    if (!VERIFY_SUCCEEDED(hr = GetAudioFilterAsDevice(pDevice, &spAudioFilter))) {
        return hr;
    }

    // Activate IKsControl
    if (!VERIFY_SUCCEEDED(hr = spAudioFilter->Activate(__uuidof(IKsControl), CLSCTX_ALL, NULL, (VOID**)&spKsControl))) {
        return hr;
    }

    // Read the audio signal processing mode property
    pinProp.Property.Set = KSPROPSETID_AudioSignalProcessing;
    pinProp.Property.Id = KSPROPERTY_AUDIOSIGNALPROCESSING_MODES;
    pinProp.Property.Flags = KSPROPERTY_TYPE_GET;
    pinProp.PinId = pinId & PARTID_MASK;
    pinProp.Reserved = 0;

    // Determined the needed size
    if (!VERIFY_SUCCEEDED(hr = spKsControl->KsProperty(&pinProp.Property, sizeof(KSP_PIN), NULL, 0, &cbNeeded))) {
        return hr;
    }
    if (!VERIFY_IS_TRUE(cbNeeded > 0) || !VERIFY_IS_TRUE((cbNeeded - sizeof(KSMULTIPLE_ITEM)) % sizeof(GUID) == 0)) {
        hr = E_NOTFOUND;
        return hr;
    }

    cbProperty = cbNeeded;
    pProperty = (PKSMULTIPLE_ITEM)CoTaskMemAlloc(cbNeeded);

    if (!VERIFY_IS_NOT_NULL(pProperty)) {
        hr = E_OUTOFMEMORY;
        cbProperty = 0;
        return hr;
    }

    // Query the processing mode property
    if (!VERIFY_SUCCEEDED(hr = spKsControl->KsProperty(&pinProp.Property, sizeof(KSP_PIN), pProperty, cbNeeded, &cbReturned))) {
        goto Exit;
    }

    if (!VERIFY_IS_NOT_NULL(pProperty)) {
        hr = E_NOTFOUND;
        goto Exit;
    }

    if (!VERIFY_IS_TRUE(cbProperty >= (sizeof(KSMULTIPLE_ITEM) + pProperty->Count * sizeof(GUID)))) {
        hr = E_NOTFOUND;
        goto Exit;
    }

    pModes = reinterpret_cast<GUID*>(pProperty + 1);

    if (!VERIFY_IS_TRUE(spModes.Allocate(pProperty->Count)))
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    memcpy(spModes, pModes, sizeof(AUDIO_SIGNALPROCESSINGMODE)*(pProperty->Count));

    *pCount = pProperty->Count;
    *ppModes = spModes.Detach();

Exit:
    if (pProperty != NULL) {
        CoTaskMemFree(pProperty);
    }
    cbProperty = 0;

    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// GetCachedSupportedFormatRecords
//
// Read the processing mode characteristics from the property store and get all supported formats for processing mode. This method applies to host and keyword detector pins only.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT GetCachedSupportedFormatRecords
(
    IMMDevice*              pDevice,
    EndpointConnectorType   eConnectorType,
    AUDIO_SIGNALPROCESSINGMODE mode,
    ULONG *pCount,
    FORMAT_RECORD **ppFormatRecords
)























































































































































///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// GetSupportedFormatRecords
//
// For pins do not have cached supported formats information, such as offload pin, we provide our predefined format list and check whether each is supported by the pin.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT GetSupportedFormatRecords
(
    IMMDevice*                  pDevice,
    UINT                        pinId,
    EndpointConnectorType       eConnectorType,
    AUDIO_SIGNALPROCESSINGMODE  mode,
    STACKWISE_DATAFLOW          dataFlow,
    ULONG                       *pCount,
    FORMAT_RECORD               **ppFormatRecords
)
{
    HRESULT                             hr = S_OK;
    FORMAT_RECORD                       *pFormatRecords;
    UINT                                cFormatRecords = 0;
    CComHeapPtr<FORMAT_RECORD>          spFormatRecords;
    BOOL                                isSupported = false;
    WAVEFORMATEX                        *pWfx;
    UINT32                              u32DefaultPeriodicityInFrames = 0;
    UINT32                              u32FundamentalPeriodicityInFrames = 0;
    UINT32                              u32MinPeriodicityInFrames = 0;
    UINT32                              u32MaxPeriodicityInFrames = 0;
    UINT32                              u32MaxPeriodicityInFramesExtended = 0;

    // Init return
    *pCount = 0;
    *ppFormatRecords = NULL;

    pFormatRecords = new FORMAT_RECORD[COUNT_FORMATS];
    if (!VERIFY_IS_NOT_NULL(pFormatRecords)) {
        hr = E_OUTOFMEMORY;
        return hr;
    }
    memset(pFormatRecords, 0, sizeof(FORMAT_RECORD)*COUNT_FORMATS);

    for (ULONG i = 0; i < COUNT_FORMATS; i++) {
        pWfx = (WAVEFORMATEX*)&ListofFormats[i];

        // Check if format is supported
        if (!VERIFY_SUCCEEDED(hr = IsFormatSupported(pDevice, pinId, pWfx, &isSupported))) {
            return hr;
        }

        // If the format is supported, we discover the periodicity characteristics for the format and store them along with the format in our FORMAT_RECORD struct.
        if (isSupported) {
            if (!VERIFY_SUCCEEDED(hr = DiscoverPeriodicityCharacteristicsForFormat(pDevice, eConnectorType, mode, pWfx, dataFlow, &u32DefaultPeriodicityInFrames, &u32FundamentalPeriodicityInFrames, &u32MinPeriodicityInFrames, &u32MaxPeriodicityInFrames, &u32MaxPeriodicityInFramesExtended))) {
                return hr;
            }

            pFormatRecords[cFormatRecords].defaultPeriodInFrames = u32DefaultPeriodicityInFrames;
            pFormatRecords[cFormatRecords].fundamentalPeriodInFrames = u32FundamentalPeriodicityInFrames;
            pFormatRecords[cFormatRecords].minPeriodInFrames = u32MinPeriodicityInFrames;
            pFormatRecords[cFormatRecords].maxPeriodInFrames = u32MaxPeriodicityInFrames;
            pFormatRecords[cFormatRecords].maxPeriodInFramesExtended = u32MaxPeriodicityInFramesExtended;
            memcpy(&pFormatRecords[cFormatRecords].wfxEx, &ListofFormats[i], sizeof(WAVEFORMATEX) + ListofFormats[i].Format.cbSize);
            cFormatRecords++;
        }
    }

    if (!VERIFY_IS_TRUE(spFormatRecords.Allocate(cFormatRecords))) {
        hr = E_OUTOFMEMORY;
        return hr;
    }

    memcpy(spFormatRecords, pFormatRecords, sizeof(FORMAT_RECORD)*cFormatRecords);
    delete[] pFormatRecords;

    *pCount = cFormatRecords;
    *ppFormatRecords = spFormatRecords.Detach();

    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// IsFormatSupported
//
// Wrap up the IKsFormatSupport::IsFormatSupported method.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT IsFormatSupported
(
    IMMDevice*              pDevice,
    UINT                    pinId,
    WAVEFORMATEX            *pWfx,
    BOOL*                   pbSupported
)
{
    HRESULT                                 hr = S_OK;
    wil::com_ptr_nothrow<IMMDevice>         spAudioFilter;
    wil::com_ptr_nothrow<IDeviceTopology>   spAudioFilterTopology;
    wil::com_ptr_nothrow<IPart>             spPart;
    wil::com_ptr_nothrow<IKsFormatSupport>  spFormatSupport;
    PKSDATAFORMAT_WAVEFORMATEXTENSIBLE      pKsRequestedFormat = NULL;
    PKSDATAFORMAT                           pKsFormat = NULL;
    KSDATAFORMAT_WAVEFORMATEXTENSIBLE       KsWfex = {};
    DWORD                                   cbKsFormat = 0;

    // Init return
    *pbSupported = FALSE;

    // Get the audio filter
    if (!VERIFY_SUCCEEDED(hr = GetAudioFilterAsDevice(pDevice, &spAudioFilter))) {
        return hr;
    }

    // Activate an IDeviceTopology on the filter
    if (!VERIFY_SUCCEEDED(hr = spAudioFilter->Activate(__uuidof(IDeviceTopology), CLSCTX_ALL, NULL, (void**)&spAudioFilterTopology))) {
        return hr;
    }

    // Get the connector
    if (!VERIFY_SUCCEEDED(hr = spAudioFilterTopology->GetPartById(pinId, &spPart))) {
        return hr;
    }

    // Activate IKsFormatSupport interface
    if (!VERIFY_SUCCEEDED(hr = spPart->Activate(CLSCTX_ALL, __uuidof(IKsFormatSupport), (void**)&spFormatSupport))) {
        return hr;
    }

    // Convert to KSDATAFORMAT_WAVEFORMATEX
    if (!VERIFY_SUCCEEDED(hr = CreateKSFormatFromWFXFormat(pWfx, (KSDATAFORMAT_WAVEFORMATEX**)&pKsRequestedFormat))) {
        return hr;
    }

    if (pKsRequestedFormat->DataFormat.FormatSize < sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE))
    {
        memcpy(&KsWfex, pKsRequestedFormat, pKsRequestedFormat->DataFormat.FormatSize);
        pKsFormat = reinterpret_cast<KSDATAFORMAT *>(&KsWfex);
        cbKsFormat = sizeof(KsWfex);
    }
    else
    {
        pKsFormat = reinterpret_cast<KSDATAFORMAT *>(pKsRequestedFormat);
        cbKsFormat = pKsRequestedFormat->DataFormat.FormatSize;
    }

    // Expect that pbFormat is really a KSDATAFORMAT
    if (!VERIFY_IS_TRUE(cbKsFormat >= sizeof(KSDATAFORMAT))) {
        hr = E_INVALIDARG;
        return hr;
    }

    // validate return pointers (this is an externally callable method)
    if (!VERIFY_IS_TRUE(pKsFormat)) {
        hr = E_POINTER;
        return hr;
    }

    if (!VERIFY_SUCCEEDED(hr = spFormatSupport->IsFormatSupported(pKsFormat, cbKsFormat, pbSupported))) {
        return hr;
    }

    CoTaskMemFree(pKsRequestedFormat);

    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// DiscoverPeriodicityCharacteristicsForFormat
//
// Discover the periodicity characteristics for given format.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT DiscoverPeriodicityCharacteristicsForFormat
(
    IMMDevice*                  pDevice,
    EndpointConnectorType       eConnectorType,
    AUDIO_SIGNALPROCESSINGMODE  mode,
    WAVEFORMATEX                *pWfx,
    STACKWISE_DATAFLOW          dataFlow,
    UINT32                      *pDefaultPeriodicityInFrames,
    UINT32                      *pFundamentalPeriodicityInFrames,
    UINT32                      *pMinPeriodicityInFrames,
    UINT32                      *pMaxPeriodicityInFrames,
    UINT32                      *pMaxPeriodicityInFramesExtended
)
{
    HRESULT                                     hr = S_OK;
    HNSTIME                                     hnsDefaultPeriod = kSystemDefaultPeriod;
    UINT32                                      ActualPeriodicityInFrames;

    // Verify format is not null
    if (!VERIFY_IS_NOT_NULL(pWfx)) {
        hr = E_INVALIDARG;
        return hr;
    }

    *pDefaultPeriodicityInFrames =
        *pFundamentalPeriodicityInFrames =
        *pMinPeriodicityInFrames =
        *pMaxPeriodicityInFrames = HNSTIME_TO_FRAMES_DOUBLE(hnsDefaultPeriod, pWfx->nSamplesPerSec);

    // Check to see whether the endpoint supports the default periodicity and return the actual periodicity
    if (!VERIFY_SUCCEEDED(hr = CheckConnectorSupportForPeriodicity(pDevice, eConnectorType, mode, pWfx, dataFlow, hnsDefaultPeriod, &ActualPeriodicityInFrames))) {
        return hr;
    }

    // Use what we get as the default, max, min and fundamental periodicity
    *pDefaultPeriodicityInFrames =
        *pFundamentalPeriodicityInFrames =
        *pMinPeriodicityInFrames =
        *pMaxPeriodicityInFrames = ActualPeriodicityInFrames;

    // Max periodicity is capped to the default periodicity for all clients except extended periodicity clients
    *pMaxPeriodicityInFramesExtended = *pMaxPeriodicityInFrames;
    if (*pMaxPeriodicityInFrames > *pDefaultPeriodicityInFrames)
    {
        *pMaxPeriodicityInFrames = *pDefaultPeriodicityInFrames;
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CheckConnectorSupportForPeriodicity
//
// Check to see if the connector supports the specified periodicity and return the actual periodicity.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CheckConnectorSupportForPeriodicity
(
    IMMDevice*                  pDevice,
    EndpointConnectorType       eConnectorType,
    AUDIO_SIGNALPROCESSINGMODE  mode,
    WAVEFORMATEX                *pWfx,
    STACKWISE_DATAFLOW          dataFlow,
    HNSTIME                     RequestedPeriodicity,
    UINT32                      *pActualPeriodicityInFrames
)









































































































































///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// GetCachedDefaultFormat
//
// Read the default device format from the property store. Use audio engine device format for all other pin types. For Keyword Detector pin, it has a different default device format.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT GetCachedDefaultFormat
(
    IMMDevice*              pDevice,
    EndpointConnectorType   eConnectorType,
    WAVEFORMATEX**          ppDefaultFormat
)






























///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// GetProposedFormatForProcessingMode
//
// Wrap up the IKsGetProposedFormat::GetProposedFormatForMode method.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT GetProposedFormatForProcessingMode
(
    IMMDevice*                  pDevice,
    UINT                        pinId,
    AUDIO_SIGNALPROCESSINGMODE  mode,
    WAVEFORMATEX                **ppProposedFormat
)
{
    HRESULT                                     hr = S_OK;
    wil::com_ptr_nothrow<IMMDevice>             spAudioFilter;
    wil::com_ptr_nothrow<IDeviceTopology>       spAudioFilterTopology;
    wil::com_ptr_nothrow<IPart>                 spPart;



    PKSDATAFORMAT                               pKsProposedFormat = NULL;
    WAVEFORMATEXTENSIBLE*                       pWfxEx = NULL;

    // Init return
    *ppProposedFormat = NULL;

    if (!VERIFY_SUCCEEDED(hr = GetAudioFilterAsDevice(pDevice, &spAudioFilter))) {
        return hr;
    }

    // Activate an IDeviceTopology on the filter
    if (!VERIFY_SUCCEEDED(hr = spAudioFilter->Activate(__uuidof(IDeviceTopology), CLSCTX_ALL, NULL, (void**)&spAudioFilterTopology))) {
        return hr;
    }

    // Get the connector
    if (!VERIFY_SUCCEEDED(hr = spAudioFilterTopology->GetPartById(pinId, &spPart))) {
        return hr;
    }

    // Retrieve the proposed format for a specific processing mode and store it in pKsProposedFormat...












    // Allocate memory for ppOffloadDefaultFormat and copy pKsDataDeviceFormat to ppOffloadDefaultFormat
    pWfxEx = (WAVEFORMATEXTENSIBLE *)CoTaskMemAlloc(sizeof(WAVEFORMATEX) + ((PKSDATAFORMAT_WAVEFORMATEX)pKsProposedFormat)->WaveFormatEx.cbSize);
    if (!VERIFY_IS_NOT_NULL(pWfxEx)) {
        CoTaskMemFree(pKsProposedFormat);
        hr = E_OUTOFMEMORY;
        return hr;
    }
    memcpy(pWfxEx, &(((PKSDATAFORMAT_WAVEFORMATEX)pKsProposedFormat)->WaveFormatEx), sizeof(WAVEFORMATEX) + ((PKSDATAFORMAT_WAVEFORMATEX)pKsProposedFormat)->WaveFormatEx.cbSize);
    *ppProposedFormat = (WAVEFORMATEX*)pWfxEx;

    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// GetCurrentAvailiablePinInstanceCount
//
// Query for the connector's pin instance count property.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT GetAvailiablePinInstanceCount
(
    IMMDevice*          pDevice,
    UINT                pinId,
    UINT32*             pAvailablePinInstanceCount
)
{
    HRESULT                                     hr = S_OK;
    wil::com_ptr_nothrow<IMMDevice>             spAudioFilter;
    wil::com_ptr_nothrow<IKsControl>            spKsControl;
    KSP_PIN                                     pinProp;
    KSPIN_CINSTANCES                            pinInstances;
    ULONG                                       cbReturned = 0;

    *pAvailablePinInstanceCount = 0;

    if (!VERIFY_SUCCEEDED(hr = GetAudioFilterAsDevice(pDevice, &spAudioFilter))) {
        return hr;
    }

    if (!VERIFY_SUCCEEDED(hr = spAudioFilter->Activate(__uuidof(IKsControl), CLSCTX_ALL, NULL, (VOID**)&spKsControl))) {
        return hr;
    }

    // Read the pin instance count
    pinProp.Property.Set = KSPROPSETID_Pin;
    pinProp.Property.Id = KSPROPERTY_PIN_CINSTANCES;
    pinProp.Property.Flags = KSPROPERTY_TYPE_GET;
    pinProp.PinId = pinId & PARTID_MASK;
    pinProp.Reserved = 0;

    if (!VERIFY_SUCCEEDED(hr = spKsControl->KsProperty(&pinProp.Property, sizeof(KSP_PIN), &pinInstances, sizeof(KSPIN_CINSTANCES), &cbReturned))) {
        return hr;
    }

    *pAvailablePinInstanceCount = pinInstances.PossibleCount - pinInstances.CurrentCount;

    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// GetDriverPathViaService
//
// Get the full path to the driver .sys file by the service name
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT GetDriverPathViaService
(
    LPWSTR             ServiceName,
    LPWSTR             DriverFullPath,
    UINT               cchFullPath
)
{
    HRESULT                             hr = S_OK;
    wil::unique_schandle                scManager;
    wil::unique_schandle                scService;

    scManager = wil::unique_schandle(OpenSCManagerW(nullptr, nullptr, GENERIC_READ));
    if (!VERIFY_IS_NOT_NULL(scManager.get()))
    {
        hr = E_OUTOFMEMORY;
        return hr;
    }

    scService = wil::unique_schandle(OpenServiceW(scManager.get(), ServiceName, GENERIC_READ));
    if (!VERIFY_IS_NOT_NULL(scService.get()))
    {
        hr = E_OUTOFMEMORY;
        return hr;
    }

    DWORD BytesRequired;
    if (!VERIFY_IS_TRUE(!QueryServiceConfigW(scService.get(), nullptr, 0, &BytesRequired) && ERROR_INSUFFICIENT_BUFFER == GetLastError()))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }

    wil::unique_cotaskmem_ptr<BYTE>     pBuff;
    pBuff.reset((BYTE*)CoTaskMemAlloc(BytesRequired));
    if (!VERIFY_IS_NOT_NULL(pBuff))
    {
        hr = E_OUTOFMEMORY;
        return hr;
    }

    LPQUERY_SERVICE_CONFIGW ServiceConfig = (LPQUERY_SERVICE_CONFIGW)pBuff.get();
    DWORD BytesReturned;

    if (!VERIFY_IS_TRUE(QueryServiceConfigW(scService.get(), ServiceConfig, BytesRequired, &BytesReturned)))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }

    if (!VERIFY_IS_NOT_NULL(ServiceConfig->lpBinaryPathName))
    {
        hr = E_UNEXPECTED;
        return hr;
    }

    if (!VERIFY_IS_TRUE('\0' != ServiceConfig->lpBinaryPathName[0]))
    {
        hr = E_UNEXPECTED;
        return hr;
    }

    if (!VERIFY_SUCCEEDED(hr = GetFullPathFromImagePath(ServiceConfig->lpBinaryPathName, DriverFullPath, cchFullPath)))
    {
        return hr;
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// GetFullPathFromImagePath
//
// Get the full driver path from the image path
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT GetFullPathFromImagePath
(
    LPWSTR              ImagePath,
    LPWSTR              DriverFullPath,
    UINT                cchFullPath
)
{
    HRESULT hr = S_OK;
    //
    // First, check if the ImagePath uses either of the well-known kernel
    // DosDevices prefixes.  If so, skip over those first since
    // GetFileAttributes would succeed, leading us to think it is a valid DOS
    // path, and GetFullPathName handles these incorrectly anyways.  Don't
    // bother handling the user DosDevices formats since it would be invalid to
    // specify that format for the ImagePath of a kernel module.
    //
    LPWSTR pImagePath = (LPWSTR)ImagePath;

    const wchar_t* DosDevicesPath =  L"\\DosDevices\\";
    const wchar_t* QuestionPath = L"\\??\\";
    if (_wcsnicmp(pImagePath,
        DosDevicesPath,
        static_cast<int>(wcslen(DosDevicesPath))) == 0)
    {
        pImagePath += wcslen(DosDevicesPath);
    }
    else if (_wcsnicmp(pImagePath,
        QuestionPath,
        static_cast<int>(wcslen(QuestionPath))) == 0)
    {
        pImagePath += wcslen(QuestionPath);
    }

    //
    // Check if the ImagePath happens to be a valid full path.
    //
    if (GetFileAttributesW(pImagePath) != 0xFFFFFFFF)
    {
        if (!VERIFY_IS_TRUE(GetFullPathNameW(pImagePath, cchFullPath, DriverFullPath, nullptr)))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            return hr;
        }
        return hr;
    }

    //
    // If the ImagePath starts with "\SystemRoot" or "%SystemRoot%" then
    // remove those values.
    //
    LPWSTR pRelativePath = (LPWSTR)pImagePath;

    const wchar_t* SystemRootPath = L"\\SystemRoot\\";
    const wchar_t* SystemRootVariablePath = L"%SystemRoot%\\";

    if (_wcsnicmp(pRelativePath,
        SystemRootPath,
        static_cast<int>(wcslen(SystemRootPath))) == 0)
    {
        pRelativePath += wcslen(SystemRootPath);
    }
    else if (_wcsnicmp(pRelativePath,
        SystemRootVariablePath,
        static_cast<int>(wcslen(SystemRootVariablePath))) == 0)
    {
        pRelativePath += wcslen(SystemRootVariablePath);
    }

    //
    // At this point pRelativePath should point to the image path relative to
    // the windows directory.
    //
    WCHAR WindowsPath[MAX_PATH];
    if (!VERIFY_IS_TRUE(GetSystemWindowsDirectoryW(WindowsPath, MAX_PATH)))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }

    if (!VERIFY_SUCCEEDED(hr = StringCchPrintfW(DriverFullPath, cchFullPath, L"%s\\%s", WindowsPath, pRelativePath)))
    {
        return hr;
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CheckImports
//
// Check if the driver imports a specific module
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CheckImports
(
    LPWSTR              DriverPath,
    LPSTR               ModuleNameToCheck,
    LPSTR               MethodNameToCheck,
    bool*               pIsImported
)
{
    HRESULT hr = S_OK;

    *pIsImported = FALSE;

    CHAR szDriverPath[MAX_PATH];
    size_t bytesConverted = 0;
    if (!VERIFY_IS_TRUE(0 == wcstombs_s(&bytesConverted, szDriverPath, MAX_PATH, DriverPath, _TRUNCATE)))
    {
        hr = E_FAIL;
        return hr;
    }

    LOADED_IMAGE image;
    //Load the image
    if (!VERIFY_IS_TRUE(MapAndLoad(szDriverPath, NULL, &image, TRUE, TRUE)))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }

    auto imageCleanup = wil::scope_exit([&]()
    {
        (VOID)UnMapAndLoad(&image);
    });

    //Get the Import Directory
    ULONG importDescriptorSize;
    PIMAGE_SECTION_HEADER sectionHeader;
    PIMAGE_IMPORT_DESCRIPTOR importDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)ImageDirectoryEntryToDataEx(image.MappedAddress,
        FALSE,
        IMAGE_DIRECTORY_ENTRY_IMPORT,
        &importDescriptorSize,
        &sectionHeader);
    if (!VERIFY_IS_NOT_NULL(importDescriptor))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }

    //Iterate through each directory entry
    while (!(importDescriptor->Characteristics == 0 &&
        importDescriptor->TimeDateStamp == 0 &&
        importDescriptor->ForwarderChain == 0 &&
        importDescriptor->Name == 0 &&
        importDescriptor->FirstThunk == 0))
    {
        CHAR* str = (PCHAR)ImageRvaToVa(ImageNtHeader(image.MappedAddress),
            image.MappedAddress,
            importDescriptor->Name,
            NULL);
        if (!str || !*str)
        {
            continue;
        }

        if (_stricmp(str, ModuleNameToCheck) == 0)
        {
            // If a specific method name is provided, check for the specific method
            if (MethodNameToCheck)
            {
                PIMAGE_THUNK_DATA thunk;
                //Iterate through the INT(Import Name Table)
                if (importDescriptor->OriginalFirstThunk == 0)
                {
                    thunk = nullptr;
                }
                else
                {
                    thunk = (PIMAGE_THUNK_DATA)ImageRvaToVa(ImageNtHeader(image.MappedAddress),
                        image.MappedAddress,
                        importDescriptor->OriginalFirstThunk,
                        NULL);
                }

                while (thunk && thunk->u1.Ordinal != 0)
                {
                    if (!IMAGE_SNAP_BY_ORDINAL(thunk->u1.Ordinal))
                    {
                        PIMAGE_IMPORT_BY_NAME importName = (PIMAGE_IMPORT_BY_NAME)ImageRvaToVa(ImageNtHeader(image.MappedAddress),
                            image.MappedAddress,
                            (ULONG)thunk->u1.ForwarderString,
                            NULL);
                        if (importName && _stricmp(importName->Name, MethodNameToCheck) == 0)
                        {
                            *pIsImported = TRUE;
                        }
                    }

                    thunk++;
                }
            }
            else
            {
                *pIsImported = TRUE;
            }
            
            break;
        }

        importDescriptor++;
    }

    return hr;

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// IsPortCls
//
// Check if audio device is PortCls
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT IsPortCls
(
    IMMDevice*          pDevice,
    bool*               pIsPortCls
)
{
    HRESULT                                 hr = S_OK;
    wil::com_ptr_nothrow<IMMDevice>         spDevnodeDevice;
    wil::com_ptr_nothrow<IPropertyStore>    spPnpProperties;
    wil::unique_prop_variant                varDeviceService;

    *pIsPortCls = false;

    // Find the associated PnP device 
    if (!VERIFY_SUCCEEDED(hr = GetPnpDevnodeFromMMDevice(pDevice, &spDevnodeDevice))) {
        return hr;
    }

    // Open pnp device property store
    if (!VERIFY_SUCCEEDED(hr = spDevnodeDevice->OpenPropertyStore(STGM_READ, &spPnpProperties))) {
        return hr;
    }

    // Read the DEVPKEY_Device_Service
    if (!VERIFY_SUCCEEDED(hr = spPnpProperties->GetValue((REFPROPERTYKEY)DEVPKEY_Device_Service, &varDeviceService))) {
        return hr;
    }

    // If DEVPKEY_Device_Service is empty, it likely means a raw PDO, which means the device is handled by the parent FDO.
    // For practical purposes, this would most likely mean a non-PortCls pin, which is what we were trying to determine, so it is OK.
    if (varDeviceService.vt == VT_EMPTY) {
        return S_OK;
    }

    if (!VERIFY_IS_TRUE(varDeviceService.vt == VT_LPWSTR && varDeviceService.pwszVal != nullptr)) {
        hr = E_FAIL;
        return hr;
    }

    WCHAR FullPath[MAX_PATH];
    if (!VERIFY_SUCCEEDED(hr = GetDriverPathViaService(varDeviceService.pwszVal, FullPath, MAX_PATH))) {
        return hr;
    }

    if (!VERIFY_SUCCEEDED(hr = CheckImports(FullPath, "portcls.sys", "PcNewPort", pIsPortCls))) {
        return hr;
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// IsAVStream
//
// Check if audio device is AVStream
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT IsAVStream
(
    IMMDevice*          pDevice,
    bool*               pIsAVStream
)
{
    HRESULT                                 hr = S_OK;
    wil::com_ptr_nothrow<IMMDevice>         spDevnodeDevice;
    wil::com_ptr_nothrow<IPropertyStore>    spPnpProperties;
    wil::unique_prop_variant                varDeviceService;

    *pIsAVStream = false;

    // Find the associated PnP device 
    if (!VERIFY_SUCCEEDED(hr = GetPnpDevnodeFromMMDevice(pDevice, &spDevnodeDevice))) {
        return hr;
    }

    // Open pnp device property store
    if (!VERIFY_SUCCEEDED(hr = spDevnodeDevice->OpenPropertyStore(STGM_READ, &spPnpProperties))) {
        return hr;
    }

    // Read the DEVPKEY_Device_Service
    if (!VERIFY_SUCCEEDED(hr = spPnpProperties->GetValue((REFPROPERTYKEY)DEVPKEY_Device_Service, &varDeviceService))) {
        return hr;
    }

    // If DEVPKEY_Device_Service is empty, it likely means a raw PDO, which means the device is handled by the parent FDO.
    // For practical purposes, this would most likely mean a non-AVStream pin, which is what we were trying to determine, so it is OK.
    if (varDeviceService.vt == VT_EMPTY) {
        return S_OK;
    }

    if (!VERIFY_IS_TRUE(varDeviceService.vt == VT_LPWSTR && varDeviceService.pwszVal != nullptr)) {
        hr = E_FAIL;
        return hr;
    }

    WCHAR FullPath[MAX_PATH];
    if (!VERIFY_SUCCEEDED(hr = GetDriverPathViaService(varDeviceService.pwszVal, FullPath, MAX_PATH))) {
        return hr;
    }

    if (!VERIFY_SUCCEEDED(hr = CheckImports(FullPath, "ks.sys", nullptr, pIsAVStream))) {
        return hr;
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// IsBluetooth
//
// Check if audio device is Bluetooth
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT IsBluetooth
(
    IMMDevice* pDevice,
    bool* pIsBluetooth
)
{
    HRESULT                                 hr = S_OK;
    wil::com_ptr_nothrow<IPropertyStore>    spPropertyStore;
    wil::unique_prop_variant                varIsBluetooth;

    *pIsBluetooth = false;

    // Open pnp device property store
    if (!VERIFY_SUCCEEDED(hr = pDevice->OpenPropertyStore(STGM_READ, &spPropertyStore))) {
        return hr;
    }

    // Read the PKEY_Endpoint_IsBluetooth
    hr = spPropertyStore->GetValue((REFPROPERTYKEY)PKEY_Endpoint_IsBluetooth, &varIsBluetooth);
    if (hr != S_OK) {
        return hr;
    }

    if (varIsBluetooth.vt == VT_BOOL)
    {
        *pIsBluetooth = varIsBluetooth.boolVal == VARIANT_TRUE ? true : false;
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// IsSideband
//
// Check if audio device is side band
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT IsSideband
(
    IMMDevice* pDevice,
    bool* pIsSideband
)
{
    HRESULT                                 hr = S_OK;
    wil::com_ptr_nothrow<IPropertyStore>    spPropertyStore;
    wil::unique_prop_variant                varIsSideband;

    *pIsSideband = false;

    // Open pnp device property store
    if (!VERIFY_SUCCEEDED(hr = pDevice->OpenPropertyStore(STGM_READ, &spPropertyStore))) {
        return hr;
    }

    // Read the PKEY_Endpoint_IsBluetooth
    hr = spPropertyStore->GetValue((REFPROPERTYKEY)PKEY_Endpoint_IsSideband, &varIsSideband);
    if (hr != S_OK) {
        return hr;
    }

    if (varIsSideband.vt == VT_BOOL)
    {
        *pIsSideband = varIsSideband.boolVal == VARIANT_TRUE ? true : false;
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// IsMVA
//
// Check if audio device is side band
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT IsMVA
(
    EndpointConnectorType eConnectorType,
    IMMDevice* pDevice,
    bool* pIsMVA
)
{
    HRESULT hr = S_OK;
    wil::com_ptr_nothrow<IMMDevice>             adapterDevice;
    wil::com_ptr_nothrow<IKsControl>            ksControl;
    ULONG                                       ulReturned = 0;
    ULONG                                       propertySupportFlags = 0;
    KSSOUNDDETECTORPROPERTY                     vam2Property = {0};

    if (!VERIFY_IS_TRUE(pDevice != nullptr)) {
        hr = E_FAIL;
        return hr;
    }

    if (!VERIFY_IS_TRUE(pIsMVA != nullptr)) {
        hr = E_FAIL;
        return hr;
    }

    // If it isn't a keyword connector, it's not MVA, we're done.
    if (eConnectorType != eKeywordDetectorConnector)
    {
        *pIsMVA = FALSE;
        return S_OK;
    }

    if (!VERIFY_SUCCEEDED(hr = GetAudioFilterAsDevice(pDevice, &adapterDevice)))
    {
        return hr;
    }

    if (!VERIFY_SUCCEEDED(hr = adapterDevice->Activate(__uuidof(IKsControl), CLSCTX_ALL, NULL, (VOID**) &ksControl))) {
        return hr;
    }

    // At this point we know that it is a keyword detector connector and we have the required interface to call into the driver
    // to see if it's MVA. If it's not MVA, it's SVA, so mark it as SVA here.
    *pIsMVA = false;

    vam2Property.Property.Set = KSPROPSETID_SoundDetector2;
    vam2Property.Property.Id = KSPROPERTY_SOUNDDETECTOR_SUPPORTEDPATTERNS;
    vam2Property.Property.Flags = KSPROPERTY_TYPE_BASICSUPPORT;
    vam2Property.EventId = GUID_NULL;

    hr = ksControl->KsProperty(
                (PKSPROPERTY)&vam2Property,
                sizeof(vam2Property),
                &propertySupportFlags,
                sizeof(propertySupportFlags),
                &ulReturned);
    if (SUCCEEDED(hr) && 
        propertySupportFlags == (KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT))
    {
        // Success, we've identified this as a connector which supports MVA, mark it.
        *pIsMVA = true;
    }

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// GetPnpDevnodeFromMMDeivce
//
// Get pnp devnode from mm device
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT GetPnpDevnodeFromMMDevice
(
    IMMDevice* pDevice,
    IMMDevice** pDevnodeDevice
)
{
    HRESULT                                     hr = S_OK;
    wil::com_ptr_nothrow<IMMDeviceEnumerator>   spEnumerator;
    wil::com_ptr_nothrow<IPropertyStore>        spProps;
    wil::com_ptr_nothrow<IMMDevice>             spDevnodeAsDevice;
    wil::com_ptr_nothrow<IPropertyStore>        spDevnodeProps;
    wil::unique_prop_variant                    varDevnode;

    // Create an enumerator
    if (!VERIFY_SUCCEEDED(hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void **)&spEnumerator))) {
        return hr;
    }

    // Read the PKEY_Endpoint_Devnode
    if (!VERIFY_SUCCEEDED(hr = pDevice->OpenPropertyStore(STGM_READ, &spProps))) {
        return hr;
    }
    if (!VERIFY_SUCCEEDED(hr = spProps->GetValue(PKEY_Endpoint_Devnode, &varDevnode))) {
        return hr;
    }

    // Get the devnode as an IMMDevice
    if (!VERIFY_SUCCEEDED(hr = spEnumerator->GetDevice(varDevnode.pwszVal, &spDevnodeAsDevice))) {
        return hr;
    }
    *pDevnodeDevice = spDevnodeAsDevice.detach();

    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// VerifyAllEndpointsPluggedIn
//
// Verify there is no unplugged endpoint
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
void VerifyAllEndpointsPluggedIn()
{
    wil::com_ptr_nothrow<IMMDeviceEnumerator>    spEnumerator;
    wil::com_ptr_nothrow<IMMDeviceCollection>    spEndpoints;
    UINT                                         cDevices = 0;

    SetVerifyOutput verifySettings(VerifyOutputSettings::LogOnlyFailures);
    DisableVerifyExceptions disable;

    VERIFY_SUCCEEDED(::CoInitializeEx(NULL, COINIT_MULTITHREADED));

    // Create IMMDevice Enumerator
    VERIFY_SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void **)&spEnumerator));

    // Enumerate all unplugged endpoints
    VERIFY_SUCCEEDED(spEnumerator->EnumAudioEndpoints(eAll, DEVICE_STATE_UNPLUGGED, &spEndpoints));
    VERIFY_SUCCEEDED(spEndpoints->GetCount(&cDevices));

    if (!VERIFY_IS_TRUE(cDevices == 0))
    {
        Log::Comment(L"Following unplugged audio device(s) found, please plug in unplugged device(s).");

        for (UINT i = 0; i < cDevices; i++) {
            wil::com_ptr_nothrow<IMMDevice> spEndpoint;
            wil::unique_cotaskmem_string id;
            wil::unique_cotaskmem_string friendlyName;

            VERIFY_SUCCEEDED(spEndpoints->Item(i, &spEndpoint));
            VERIFY_SUCCEEDED(spEndpoint->GetId(&id));
            VERIFY_SUCCEEDED(GetEndpointFriendlyName(spEndpoint.get(), &friendlyName));

            Log::Comment(String().Format(L"Device: %s (%s) is unplugged.", friendlyName.get(), id.get()));
        }
    }
}
