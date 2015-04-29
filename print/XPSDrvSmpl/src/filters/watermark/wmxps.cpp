/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   wmxps.cpp

Abstract:

   Watermark XPS markup class implementation. The CWatermarkMarkup class is responsible
   for creating a stream that contains markup loaded from a resource.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdexcept.h"
#include "wmxps.h"

using XDPrintSchema::PageWatermark::EWatermarkOption;
using XDPrintSchema::PageWatermark::VectorWatermark;

/*++

Routine Name:

    CWatermarkMarkup::CWatermarkMarkup

Routine Description:

    Constructor for the CWatermarkMarkup vector management class
    which sets the member variables to sensible defaults and
    ensures the current watermark is a vector based watermark

Arguments:

    wmProps    - Object containing the PrintTicket settings
    resourceID - Resource ID for the watermark

Return Value:

    None
    Throws CXDException(HRESULT) on an error

--*/
CWatermarkMarkup::CWatermarkMarkup(
    _In_ CONST CWMPTProperties& wmProps,
    _In_ CONST INT              resourceID
    ) :
    m_WMProps(wmProps),
    m_resourceID(resourceID)
{
    HRESULT hr = S_OK;

    EWatermarkOption wmType;

    if (SUCCEEDED(hr = m_WMProps.GetType(&wmType)))
    {
        if (wmType != VectorWatermark)
        {
            hr = E_INVALIDARG;
        }
    }

    if (FAILED(hr))
    {
        throw CXDException(hr);
    }
}

/*++

Routine Name:

    CWatermarkMarkup::~CWatermarkMarkup

Routine Description:

    Default destructor for the font management class

Arguments:

    None

Return Value:

    None

--*/
CWatermarkMarkup::~CWatermarkMarkup()
{
}

/*++

Routine Name:

    CWatermarkMarkup::GetImageDimensions

Routine Description:

    Method to obtain the width and height for the watermark vector image

Arguments:

    pDimensions - Pointer to a structure which will hold the vector image dimensions

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWatermarkMarkup::GetImageDimensions(
    _Out_ SizeF* pDimensions
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pDimensions, E_POINTER)))
    {
        //
        // Currently fixed to A4 dimensions
        //
        pDimensions->Width  = 793.76f;
        pDimensions->Height = 1122.56f;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWatermarkMarkup::CreateXPSStream

Routine Description:

    Method for writing out the vector image to the container

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWatermarkMarkup::CreateXPSStream(
    VOID
    )
{
    HRESULT hr = S_OK;

    if (m_pXPSStream == NULL)
    {
        HRSRC hrSrc = FindResourceEx(g_hInstance,
                                     RT_RCDATA,
                                     MAKEINTRESOURCE(m_resourceID),
                                     MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
        if (hrSrc != NULL)
        {
            //
            // Load the resource
            //
            HGLOBAL hXPSRes = LoadResource(g_hInstance, hrSrc);

            if (hXPSRes != NULL)
            {
                if (SUCCEEDED(hr = CreateStreamOnHGlobal(NULL, TRUE, &m_pXPSStream)))
                {
                    //
                    // Retrieve the XPS data and the size of the data
                    //
                    PVOID pXPSData  = LockResource(hXPSRes);
                    DWORD cbXPSData = SizeofResource(g_hInstance, hrSrc);

                    if (SUCCEEDED(hr = CHECK_POINTER(pXPSData, E_FAIL)) &&
                        cbXPSData > 0)
                    {
                        ULONG cbTotalWritten = 0;

                        //
                        // Write the size of the data to the stream.
                        // This is required as the stream will be read back using the CComBSTR ReadFromStream() method
                        //
                        DWORD cbXPSDataWithTerm = cbXPSData + sizeof(OLECHAR);

                        if (SUCCEEDED(hr = m_pXPSStream->Write(&cbXPSDataWithTerm, sizeof(cbXPSDataWithTerm), &cbTotalWritten)))
                        {
                            //
                            // Write the data to the stream
                            // Note the XPS data content must be in Unicode format to be compatible with
                            // the CComBSTR ReadFromStream() method.
                            //
                            ULONG cbWritten = 0;

                            if (SUCCEEDED(hr = m_pXPSStream->Write(pXPSData, cbXPSData, &cbWritten)))
                            {
                                cbTotalWritten += cbWritten;

                                hr = m_pXPSStream->Write(OLESTR("\0"), sizeof(OLECHAR), &cbWritten);
                                cbTotalWritten += cbWritten;
                            }
                        }
                    }
                }

                FreeResource(hXPSRes);
            }
            else
            {
                hr = GetLastErrorAsHResult();
            }
        }
        else
        {
            hr = GetLastErrorAsHResult();
        }
    }

    //
    // Make sure the stream is pointing back at the start of the data
    //
    if (SUCCEEDED(hr))
    {
        LARGE_INTEGER cbMoveFromStart = {0};

        hr = m_pXPSStream->Seek(cbMoveFromStart,
                                STREAM_SEEK_SET,
                                NULL);
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWatermarkMarkup::GetStream

Routine Description:

    Method for obtaining a stream to write the vector image out to

Arguments:

    ppStream - Pointer to a pointer to the stream

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CWatermarkMarkup::GetStream(
    _Out_ IStream** ppStream
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(ppStream, E_POINTER)) &&
        SUCCEEDED(hr = CreateXPSStream()))
    {
        *ppStream = m_pXPSStream;
    }

    ERR_ON_HR(hr);
    return hr;
}
