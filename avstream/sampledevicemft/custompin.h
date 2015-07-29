#pragma once
#include "stdafx.h"
#include "common.h"

//
// Custom Pin is only an input pin in this sample..It is short circuited back
// to the pipeline. This can have uses in implementing depth pins etc.
// It is not exposed to the down stream i.e. captureengine or the
// WinRT code.
// 
class CCustomPin : public CInPin{
public:

    CCustomPin( 
        _In_ IMFAttributes *pAttributes,
        _In_ ULONG PinId,
        _In_ CMultipinMft *pParent
        );
    STDMETHODIMP SendSample(
            _In_ IMFSample *pSample
        );
    STDMETHODIMP_( DeviceStreamState )SetState(
        _In_ DeviceStreamState
        );
};

STDMETHODIMP CheckCustomPin(
    _In_ CInPin * pin,
    _Inout_ PBOOL  isCustom
    );
