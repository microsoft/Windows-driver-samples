/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   saxhndlr.cpp

Abstract:

   Default sax handler implementation. Provides default implementations
   for the ISAXContentHandler. This allows derived classes to only need
   to implement the methods that are required.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "xdexcept.h"
#include "saxhndlr.h"
#include "widetoutf8.h"

/*++

Routine Name:

    CSaxHandler::CSaxHandler

Routine Description:

    CSaxHandler class constructor

Arguments:

    None

Return Value:

    None

--*/
CSaxHandler::CSaxHandler() :
    CUnknown<ISAXContentHandler>(IID_ISAXContentHandler)
{
}

/*++

Routine Name:

    CSaxHandler::~CSaxHandler

Routine Description:

    CSaxHandler class destructor

Arguments:

    None

Return Value:

    None

--*/
CSaxHandler::~CSaxHandler()
{
}

/*++

Routine Name:

    CSaxHandler::putDocumentLocator

Routine Description:

    This routine is the default implementation for ISAXContentHandler::putDocumentLocator

Arguments:

    Unused

Return Value:

    HRESULT
    S_OK - Always succeeds

--*/
HRESULT STDMETHODCALLTYPE
CSaxHandler::putDocumentLocator(
    ISAXLocator *
    )
{
    return S_OK;
}

/*++

Routine Name:

    CSaxHandler::startDocument

Routine Description:

    This routine is the default implementation for ISAXContentHandler::startDocument

Arguments:

    Unused

Return Value:

    HRESULT
    S_OK - Always succeeds

--*/
HRESULT STDMETHODCALLTYPE
CSaxHandler::startDocument(
    void
    )
{
    return S_OK;
}

/*++

Routine Name:

    CSaxHandler::endDocument

Routine Description:

    This routine is the default implementation for ISAXContentHandler::endDocument

Arguments:

    Unused

Return Value:

    HRESULT
    S_OK - Always succeeds

--*/
HRESULT STDMETHODCALLTYPE
CSaxHandler::endDocument(
    void
    )
{
    return S_OK;
}

/*++

Routine Name:

    CSaxHandler::startPrefixMapping

Routine Description:

    This routine is the default implementation for ISAXContentHandler::startPrefixMapping

Arguments:

    Unused

Return Value:

    HRESULT
    S_OK - Always succeeds

--*/
HRESULT STDMETHODCALLTYPE
CSaxHandler::startPrefixMapping(
    CONST wchar_t*,
    INT,
    CONST wchar_t*,
    INT
    )
{
    return S_OK;
}

/*++

Routine Name:

    CSaxHandler::endPrefixMapping

Routine Description:

    This routine is the default implementation for ISAXContentHandler::endPrefixMapping

Arguments:

    Unused

Return Value:

    HRESULT
    S_OK - Always succeeds

--*/
HRESULT STDMETHODCALLTYPE
CSaxHandler::endPrefixMapping(
    CONST wchar_t*,
    INT
    )
{
    return S_OK;
}

/*++

Routine Name:

    CSaxHandler::startElement

Routine Description:

    This routine is the default implementation for ISAXContentHandler::startElement

Arguments:

    Unused

Return Value:

    HRESULT
    S_OK - Always succeeds

--*/
HRESULT STDMETHODCALLTYPE
CSaxHandler::startElement(
    CONST wchar_t*,
    INT,
    CONST wchar_t*,
    INT,
    CONST wchar_t*,
    INT,
    _In_ ISAXAttributes*
    )
{
    return S_OK;
}

/*++

Routine Name:

    CSaxHandler::endElement

Routine Description:

    This routine is the default implementation for ISAXContentHandler::endElement

Arguments:

    Unused

Return Value:

    HRESULT
    S_OK - Always succeeds

--*/
HRESULT STDMETHODCALLTYPE
CSaxHandler::endElement(
    CONST wchar_t*,
    INT,
    CONST wchar_t*,
    INT,
    CONST wchar_t*,
    INT
    )
{
    return S_OK;
}

/*++

Routine Name:

    CSaxHandler::characters

Routine Description:

    This routine is the default implementation for ISAXContentHandler::characters

Arguments:

    Unused

Return Value:

    HRESULT
    S_OK - Always succeeds

--*/
HRESULT STDMETHODCALLTYPE
CSaxHandler::characters(
    CONST wchar_t*,
    INT
    )
{
    return S_OK;
}

/*++

Routine Name:

    CSaxHandler::ignorableWhitespace

Routine Description:

    This routine is the default implementation for ISAXContentHandler::ignorableWhitespace

Arguments:

    Unused

Return Value:

    HRESULT
    S_OK - Always succeeds

--*/
HRESULT STDMETHODCALLTYPE
CSaxHandler::ignorableWhitespace(
    CONST wchar_t*,
    INT
    )
{
    return S_OK;
}

/*++

Routine Name:

    CSaxHandler::processingInstruction

Routine Description:

    This routine is the default implementation for ISAXContentHandler::processingInstruction

Arguments:

    Unused

Return Value:

    HRESULT
    S_OK - Always succeeds

--*/
HRESULT STDMETHODCALLTYPE
CSaxHandler::processingInstruction(
    CONST wchar_t*,
    INT,
    CONST wchar_t*,
    INT
    )
{
    return S_OK;
}

/*++

Routine Name:

    CSaxHandler::skippedEntity

Routine Description:

    This routine is the default implementation for ISAXContentHandler::skippedEntity

Arguments:

    Unused

Return Value:

    HRESULT
    S_OK - Always succeeds

--*/
HRESULT STDMETHODCALLTYPE
CSaxHandler::skippedEntity(
    CONST wchar_t*,
    INT
    )
{
    return S_OK;
}

/*++

Routine Name:

    CSaxHandler::WriteToPrintStream

Routine Description:

    This routine converts a CStringXDW buffer to UTF-8 string to be
    written to the write stream provided. This overload handles accepts
    an IPrintWriteStream to write the data out.

Arguments:

    pcstrOut - Pointer to the Atl CStringXDW containing the mark-up to be written
    pWriter  - Pointer to the print write stream to write to

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CSaxHandler::WriteToPrintStream(
    _In_ CStringXDW*          pcstrOut,
    _In_ IPrintWriteStream* pWriter
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pcstrOut, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pWriter, E_POINTER)))
    {
        ULONG cbWritten = 0;
        PVOID pData = NULL;
        ULONG cbData = 0;

        try
        {
            CWideToUTF8 wideToUTF8(pcstrOut);

            if (SUCCEEDED(hr = wideToUTF8.GetBuffer(&pData, &cbData)))
            {
                hr = pWriter->WriteBytes(pData, cbData, &cbWritten);
            }
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CSaxHandler::WriteToPrintStream

Routine Description:

    This routine converts a CStringXDW buffer to UTF-8 string to be
    written to the write stream provided. This overload handles accepts
    an ISequentialStream to write the data out.

Arguments:

    pcstrOut - Pointer to the Atl CStringXDW containing the mark-up to be written
    pWriter  - Pointer to the print write stream to write to

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CSaxHandler::WriteToPrintStream(
    _In_ CStringXDW*          pcstrOut,
    _In_ ISequentialStream* pWriter
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pcstrOut, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pWriter, E_POINTER)))
    {
        ULONG cbWritten = 0;
        PVOID pData = NULL;
        ULONG cbData = 0;

        try
        {
            CWideToUTF8 wideToUTF8(pcstrOut);

            if (SUCCEEDED(hr = wideToUTF8.GetBuffer(&pData, &cbData)))
            {
                hr = pWriter->Write(pData, cbData, &cbWritten);
            }
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}



/*++

Routine Name:

    CSaxHandler::EscapeEntity

Routine Description:

    The UnicodeString mark-up could contain characters that need to be escaped (the
    SAX handler is passed the characters and not the original escapes).
    The list of characters which need to be escaped are defined in the XML standard
    and may change in future revisions.
    This routine takes a BSTR and replaces all instances of these characters with
    their escaped versions.

Arguments:

    pStr - Pointer to the BSTR to be converted

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CSaxHandler::EscapeEntity(
    _Inout_ BSTR* pStr
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pStr, E_POINTER)))
    {
        try
        {
            if (SysStringLen(*pStr) > 0)
            {
                CStringXDW cstrStr(*pStr);

                cstrStr.Replace(L"&", L"&amp;");
                cstrStr.Replace(L"<", L"&lt;");
                cstrStr.Replace(L">", L"&gt;");
                cstrStr.Replace(L"\"", L"&quot;");
                cstrStr.Replace(L"'", L"&apos;");

                SysFreeString(*pStr);
                *pStr = cstrStr.AllocSysString();
            }
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

