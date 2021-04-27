#pragma once

#include "CAPXHelper.h"

bool IsCapXAPO(GUID clsid)
{
    bool bIsCapXAPO = false;

    wil::com_ptr_nothrow<IAudioProcessingObject> apo;
    if(SUCCEEDED(CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, __uuidof(IAudioProcessingObject), reinterpret_cast<void**>(&apo))))
    {
        wil::com_ptr_nothrow<IAudioSystemEffects3> capXAPO;
        bIsCapXAPO = wil::try_com_query_to(apo, &capXAPO);
    }

    return bIsCapXAPO;
}

void GetClsidsFromVar(const PROPVARIANT& var, std::vector<GUID>& guids, _Outptr_ PWSTR* ppszFormattedClsids, std::vector<GUID>& capxGuids, _Outptr_ PWSTR* ppszCapXFormattedClsids)
{
    WCHAR formattedClsids[1024] = {};
    PWSTR writeLocation = formattedClsids;
    size_t remainingChars = ARRAYSIZE(formattedClsids);
    const PWSTR formatSeparator = L"|%s";
    bool bIsCapXAPO;
    WCHAR capXformattedClsids[1024] = {};
    PWSTR capXwriteLocation = capXformattedClsids;
    size_t capXremainingChars = ARRAYSIZE(capXformattedClsids);

    for (UINT i = 0; i < var.calpwstr.cElems; i++)
    {
        // populate vector
        GUID clsid = {};
        if (FAILED(CLSIDFromString(var.calpwstr.pElems[i], &clsid)))
        {
            return;
        }

        bIsCapXAPO = IsCapXAPO(clsid);
        try
        {
            bIsCapXAPO ? capxGuids.push_back(clsid) : guids.push_back(clsid);
        }
        CATCH_LOG();

        // build string to log
        if (FAILED(bIsCapXAPO ? StringCchPrintfEx(capXwriteLocation, capXremainingChars, &capXwriteLocation, &capXremainingChars, 0, formatSeparator, var.calpwstr.pElems[i])
                              : StringCchPrintfEx(writeLocation, remainingChars, &writeLocation, &remainingChars, 0, formatSeparator, var.calpwstr.pElems[i])))
        {
            return;
        }
    }

    wil::unique_cotaskmem_string spFormattedClsids = wil::make_cotaskmem_string_nothrow(formattedClsids+1);
    if (spFormattedClsids)
    {
        *ppszFormattedClsids = spFormattedClsids.release();
    }

    wil::unique_cotaskmem_string spCapXFormattedClsids = wil::make_cotaskmem_string_nothrow(capXformattedClsids+1);
    if (spCapXFormattedClsids)
    {
        *ppszCapXFormattedClsids = spCapXFormattedClsids.release();
    }
}


bool EndpointContainsCapx(IMMDevice* pDevice)
{
    // some endpoints have an effects property store
    // grab properties from that
    
    GUID clsidSfx = {};
    bool bIsCapXSfx = false;
    GUID clsidMfx = {};
    bool bIsCapXMfx = false;
    GUID clsidEfx = {};
    bool bIsCapXEfx = false;

    // legacy drivers
    GUID clsidLfx = {};
    bool bIsCapXLfx = false;
    GUID clsidGfx = {};
    bool bIsCapXGfx = false;

    // offload pin
    GUID clsidOffloadSfx = {};
    bool bIsCapXOffloadSfx = false;
    GUID clsidOffloadMfx = {};
    bool bIsCapXOffloadMfx = false;

    // keyword spotter pin
    GUID clsidKeywordSfx = {};
    bool bIsCapXKeywordSfx = false;
    GUID clsidKeywordMfx = {};
    bool bIsCapXKeywordMfx = false;
    GUID clsidKeywordEfx = {};
    bool bIsCapXKeywordEfx = false;

    // // composite effects
    std::vector<GUID> clsidsCompositeSfx;
    wil::unique_cotaskmem_string clsidsCompositeSfxStr;
    std::vector<GUID> clsidsCompositeMfx;
    wil::unique_cotaskmem_string clsidsCompositeMfxStr;
    std::vector<GUID> clsidsCompositeEfx;
    wil::unique_cotaskmem_string clsidsCompositeEfxStr;
    // // CapX composite effects
    std::vector<GUID> clsidsCapXCompositeSfx;
    wil::unique_cotaskmem_string clsidsCapXCompositeSfxStr;
    std::vector<GUID> clsidsCapXCompositeMfx;
    wil::unique_cotaskmem_string clsidsCapXCompositeMfxStr;
    std::vector<GUID> clsidsCapXCompositeEfx;
    wil::unique_cotaskmem_string clsidsCapXCompositeEfxStr;

    wil::com_ptr_nothrow<IMMEndpointInternal> spMMEndpointInternal;
    if (wil::try_com_query_to(pDevice, &spMMEndpointInternal))
    {
        wil::com_ptr_nothrow<IPropertyStore> spFxProperties;
        if ((S_OK == spMMEndpointInternal->TryOpenFXPropertyStore(STGM_READ, &spFxProperties)) && spFxProperties)
        {
            wil::unique_prop_variant varClsidSfx;
            if (SUCCEEDED(spFxProperties->GetValue(PKEY_FX_StreamEffectClsid, &varClsidSfx)) && varClsidSfx.vt == VT_LPWSTR)
            {
                CLSIDFromString(varClsidSfx.pwszVal, &clsidSfx);
                bIsCapXSfx = IsCapXAPO(clsidSfx);
            }

            wil::unique_prop_variant varClsidMfx;
            if (SUCCEEDED(spFxProperties->GetValue(PKEY_FX_ModeEffectClsid, &varClsidMfx)) && varClsidMfx.vt == VT_LPWSTR)
            {
                CLSIDFromString(varClsidMfx.pwszVal, &clsidMfx);
                bIsCapXMfx = IsCapXAPO(clsidMfx);
            }

            wil::unique_prop_variant varClsidEfx;
            if (SUCCEEDED(spFxProperties->GetValue(PKEY_FX_EndpointEffectClsid, &varClsidEfx)) && varClsidEfx.vt == VT_LPWSTR)
            {
                CLSIDFromString(varClsidEfx.pwszVal, &clsidEfx);
                bIsCapXEfx = IsCapXAPO(clsidEfx);
            }

            // legacy drivers
            wil::unique_prop_variant varClsidLfx;
            if (SUCCEEDED(spFxProperties->GetValue(PKEY_FX_PreMixEffectClsid, &varClsidLfx)) && varClsidLfx.vt == VT_LPWSTR)
            {
                CLSIDFromString(varClsidLfx.pwszVal, &clsidLfx);
                bIsCapXLfx = IsCapXAPO(clsidLfx);
            }

            wil::unique_prop_variant varClsidGfx;
            if (SUCCEEDED(spFxProperties->GetValue(PKEY_FX_PostMixEffectClsid, &varClsidGfx)) && varClsidGfx.vt == VT_LPWSTR)
            {
                CLSIDFromString(varClsidGfx.pwszVal, &clsidGfx);
                bIsCapXGfx = IsCapXAPO(clsidGfx);
            }

            // offload
            wil::unique_prop_variant varClsidOffloadSfx;
            if (SUCCEEDED(spFxProperties->GetValue(PKEY_FX_Offload_StreamEffectClsid, &varClsidOffloadSfx)) && varClsidOffloadSfx.vt == VT_LPWSTR)
            {
                CLSIDFromString(varClsidOffloadSfx.pwszVal, &clsidOffloadSfx);
                bIsCapXOffloadSfx = IsCapXAPO(clsidOffloadSfx);
            }

            wil::unique_prop_variant varClsidOffloadMfx;
            if (SUCCEEDED(spFxProperties->GetValue(PKEY_FX_Offload_ModeEffectClsid, &varClsidOffloadMfx)) && varClsidOffloadMfx.vt == VT_LPWSTR)
            {
                CLSIDFromString(varClsidOffloadMfx.pwszVal, &clsidOffloadMfx);
                bIsCapXOffloadMfx = IsCapXAPO(clsidOffloadMfx);
            }

            // keyword
            wil::unique_prop_variant varClsidKeywordSfx;
            if (SUCCEEDED(spFxProperties->GetValue(PKEY_FX_KeywordDetector_StreamEffectClsid, &varClsidKeywordSfx)) && varClsidKeywordSfx.vt == VT_LPWSTR)
            {
                CLSIDFromString(varClsidKeywordSfx.pwszVal, &clsidKeywordSfx);
                bIsCapXKeywordSfx = IsCapXAPO(clsidKeywordSfx);
            }

            wil::unique_prop_variant varClsidKeywordMfx;
            if (SUCCEEDED(spFxProperties->GetValue(PKEY_FX_KeywordDetector_ModeEffectClsid, &varClsidKeywordMfx)) && varClsidKeywordMfx.vt == VT_LPWSTR)
            {
                CLSIDFromString(varClsidKeywordMfx.pwszVal, &clsidKeywordMfx);
                bIsCapXKeywordMfx = IsCapXAPO(clsidKeywordMfx);
            }

            wil::unique_prop_variant varClsidKeywordEfx;
            if (SUCCEEDED(spFxProperties->GetValue(PKEY_FX_KeywordDetector_EndpointEffectClsid, &varClsidKeywordEfx)) && varClsidKeywordEfx.vt == VT_LPWSTR)
            {
                CLSIDFromString(varClsidKeywordEfx.pwszVal, &clsidKeywordEfx);
                bIsCapXKeywordEfx = IsCapXAPO(clsidKeywordEfx);
            }

            // composite effects
            wil::unique_prop_variant varClsidsCompositeSfx;
            if (SUCCEEDED(spFxProperties->GetValue(PKEY_CompositeFX_StreamEffectClsid, &varClsidsCompositeSfx))
                && varClsidsCompositeSfx.vt == (VT_VECTOR | VT_LPWSTR)
                && varClsidsCompositeSfx.calpwstr.cElems > 0)
            {
                GetClsidsFromVar(varClsidsCompositeSfx, clsidsCompositeSfx, &clsidsCompositeSfxStr, clsidsCapXCompositeSfx, &clsidsCapXCompositeSfxStr);
            }

            wil::unique_prop_variant varClsidsCompositeMfx;
            if (SUCCEEDED(spFxProperties->GetValue(PKEY_CompositeFX_ModeEffectClsid, &varClsidsCompositeMfx))
                && varClsidsCompositeMfx.vt == (VT_VECTOR | VT_LPWSTR)
                && varClsidsCompositeMfx.calpwstr.cElems > 0)
            {
                GetClsidsFromVar(varClsidsCompositeMfx, clsidsCompositeMfx, &clsidsCompositeMfxStr, clsidsCapXCompositeMfx, &clsidsCapXCompositeMfxStr);
            }

            wil::unique_prop_variant varClsidsCompositeEfx;
            if (SUCCEEDED(spFxProperties->GetValue(PKEY_CompositeFX_EndpointEffectClsid, &varClsidsCompositeEfx))
                && varClsidsCompositeEfx.vt == (VT_VECTOR | VT_LPWSTR)
                && varClsidsCompositeEfx.calpwstr.cElems > 0)
            {
                GetClsidsFromVar(varClsidsCompositeEfx, clsidsCompositeEfx, &clsidsCompositeEfxStr, clsidsCapXCompositeEfx, &clsidsCapXCompositeEfxStr);
            }
        }
    }

    return ( bIsCapXSfx || bIsCapXMfx || bIsCapXEfx || bIsCapXLfx || bIsCapXGfx || 
             bIsCapXOffloadSfx || bIsCapXOffloadMfx || bIsCapXKeywordSfx || bIsCapXKeywordMfx || bIsCapXKeywordEfx || 
             clsidsCapXCompositeSfx.size() > 0 || clsidsCapXCompositeMfx.size() > 0 || clsidsCapXCompositeMfx.size() > 0 );
}

void EndpointsWithCapx(std::vector<wil::com_ptr<IMMDevice>>& capxEndpoints)
{
    UINT cDevices = 0;
    com_ptr_nothrow<IMMDeviceEnumerator> spEnumerator;
    com_ptr_nothrow<IMMDeviceCollection> spDevices;

    VERIFY_SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&spEnumerator)));
    VERIFY_SUCCEEDED(spEnumerator->EnumAudioEndpoints(eAll, DEVICE_STATE_ACTIVE, &spDevices));
    VERIFY_SUCCEEDED(spDevices->GetCount(&cDevices));

    for (UINT i = 0; i < cDevices; i++)
    {
        wil::com_ptr<IMMDevice> pEndpoint = nullptr;

        VERIFY_SUCCEEDED(spDevices->Item(i, &pEndpoint));

        if (EndpointContainsCapx(pEndpoint.get()))
        {
            capxEndpoints.push_back(pEndpoint.get());
        }
    }
}