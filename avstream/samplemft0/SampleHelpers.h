//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved


#pragma once
#include "stdafx.h"

#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)
#define __WFILE__ WIDEN(__FILE__)


#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(WPPCameraExtensionMFt_Sample, \
        (31A88041,BBF0,4D2C,A752,16EB1E51859C), \
        WPP_DEFINE_BIT(AllMessages) \
    )

#define SAFEFREE(x) \
    if(x) { \
        free(x); \
        x = NULL; \
    }
#define SAFERELEASE(x) \
    if(x) { \
        x->Release(); \
        x = NULL; \
    }

#define CHK_LOG_BRK(exp) \
    if(FAILED(hr = (exp)))  { \
        wprintf(L"HR=%08x File: %S Ln:  %d\n", hr, __FILE__, __LINE__); \
        break; \
    }

#define CHK_NULL_BRK(exp) \
    if((exp) == NULL) { \
        hr = E_OUTOFMEMORY; \
        wprintf(L"HR=%08x File: %S Ln:  %d\n", hr, __FILE__, __LINE__); \
        break; \
    }
#define CHK_NULL_PTR_BRK(exp) \
    if((exp) == NULL) { \
        hr = E_INVALIDARG; \
        wprintf(L"HR=%08x File: %S Ln:  %d\n", hr, __FILE__, __LINE__); \
        break; \
    }

#define CHK_BOOL_BRK(exp) \
    if(!exp) { \
        hr = E_FAIL; \
        wprintf(L"HR=%08x File: %S Ln:  %d\n", hr, __FILE__, __LINE__); \
        break; \
    }


class VideoBufferLock
{
public:
    VideoBufferLock(IMFMediaBuffer *pBuffer) : m_p2DBuffer(NULL)
    {
        m_pBuffer = pBuffer;
        m_pBuffer->AddRef();

        // Query for the 2-D buffer interface. OK if this fails.
        m_pBuffer->QueryInterface(IID_IMF2DBuffer, (void**)&m_p2DBuffer);
    }

    ~VideoBufferLock()
    {
        UnlockBuffer();
        SAFERELEASE(m_pBuffer);
        SAFERELEASE(m_p2DBuffer);
    }

    // LockBuffer:
    // Locks the buffer. Returns a pointer to scan line 0 and returns the stride.

    // The caller must provide the default stride as an input parameter, in case
    // the buffer does not expose IMF2DBuffer. You can calculate the default stride
    // from the media type.

    HRESULT LockBuffer(
        LONG  lDefaultStride,    // Minimum stride (with no padding).
        DWORD dwHeightInPixels,  // Height of the image, in pixels.
        BYTE  **ppbScanLine0,    // Receives a pointer to the start of scan line 0.
        LONG  *plStride          // Receives the actual stride.
        )
    {
        HRESULT hr = S_OK;

        // Use the 2-D version if available.
        if (m_p2DBuffer)
        {
            hr = m_p2DBuffer->Lock2D(ppbScanLine0, plStride);
        }
        else
        {
            // Use non-2D version.
            BYTE *pData = NULL;

            hr = m_pBuffer->Lock(&pData, NULL, NULL);
            if (SUCCEEDED(hr))
            {
                *plStride = lDefaultStride;
                if (lDefaultStride < 0)
                {
                    // Bottom-up orientation. Return a pointer to the start of the
                    // last row *in memory* which is the top row of the image.
                    *ppbScanLine0 = pData + abs(lDefaultStride) * (dwHeightInPixels - 1);
                }
                else
                {
                    // Top-down orientation. Return a pointer to the start of the
                    // buffer.
                    *ppbScanLine0 = pData;
                }
            }
        }
        return hr;
    }

    HRESULT UnlockBuffer()
    {
        if (m_p2DBuffer)
        {
            return m_p2DBuffer->Unlock2D();
        }
        else
        {
            return m_pBuffer->Unlock();
        }
    }

private:
    IMFMediaBuffer  *m_pBuffer;
    IMF2DBuffer     *m_p2DBuffer;
};

