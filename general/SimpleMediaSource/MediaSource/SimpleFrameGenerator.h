//
// Copyright (C) Microsoft Corporation. All rights reserved.
//

#pragma once
#ifndef SIMPLE_FRAME_GENERATOR_H
#define SIMPLE_FRAME_GENERATOR_H

class SimpleFrameGenerator
{
public:
    SimpleFrameGenerator() = default;
    ~SimpleFrameGenerator() {};

    HRESULT Initialize(_In_ IMFMediaType* pMediaType);

    HRESULT CreateFrame(
        _Inout_updates_bytes_(len) BYTE* pBuf,
        _In_ DWORD len,
        _In_ LONG pitch,
        _In_ ULONG rgbMask);

    // pixel format converter
    static void RGB24ToYUY2(int R, int G, int B, BYTE* pY, BYTE* pU, BYTE* pV);
    static void RGB24ToY(int R, int G, int B, BYTE* pY);
    static void RGB32ToNV12(BYTE RGB1[8], BYTE RGB2[8], BYTE* pY1, BYTE* pY2, BYTE* pUV);

    static HRESULT RGB32ToNV12Frame(_Inout_updates_bytes_(len) BYTE* pbBuff, ULONG cbBuff, long stride, UINT width, UINT height, BYTE* pbBuffOut, ULONG cbBuffOut, long strideOut);

private: 
    HRESULT _CreateRGB32Frame(
        _Inout_updates_bytes_(len) BYTE* pBuf,
        _In_ DWORD len,
        _In_ LONG pitch,
        _In_ DWORD width,
        _In_ DWORD height,
        _In_ ULONG rgbMask);

    UINT32 m_width = 0;
    UINT32 m_height = 0;
    GUID m_subType = GUID_NULL;

};

#endif

