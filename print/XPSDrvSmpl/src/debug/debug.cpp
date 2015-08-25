/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   debug.cpp

Abstract:

   Debug implementations.

--*/

#include "precomp.h"
#include "debug.h"
#include "xdstring.h"

/*++

Routine Name:

    RealDebugMessage

Routine Description:

    This routine takes a debug message and va_list and outputs the message via OutputDebugString

Arguments:

    dwSize     - Maximum size of the debug message in number of characters
    pszMessage - The debug message string
    arglist    - The arg list for the debug message string

Return Value:

    BOOL
    TRUE  - On success
    FALSE - On error

--*/
BOOL
RealDebugMessage(
    _In_ DWORD   dwSize,
    _In_ PCSTR   pszMessage,
         va_list arglist
    )
{
    HRESULT hr = S_OK;
    PSTR    pszMsgBuf;

    if (NULL == pszMessage ||
        0 == dwSize)
    {
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        //
        // Allocate memory for message buffer.
        //
        pszMsgBuf = new(std::nothrow) CHAR[dwSize + 1];

        if (NULL != pszMsgBuf)
        {
            //
            // Pass the variable parameters to wvsprintf to be formated.
            //
            hr = StringCbVPrintfA(pszMsgBuf, (dwSize + 1) * sizeof(CHAR), pszMessage, arglist);

            //
            // Dump string to debug output.
            //
            OutputDebugStringA(pszMsgBuf);

            //
            // Clean up.
            //
            delete[] pszMsgBuf;
            pszMsgBuf = NULL;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return SUCCEEDED(hr);
}

/*++

Routine Name:

    DbgPrint

Routine Description:

    This routine takes a format string and arguments and outputs as a debug string

Arguments:

    pszFormatString - Format string for the debug message
    ...             - argument list

Return Value:

    BOOL
    TRUE  - On success
    FALSE - On error

--*/
BOOL
DbgPrint(
    _In_ PCSTR pszFormatString,
    ...
    )
{
    BOOL    bResult;
    va_list VAList;

    //
    // Pass the variable parameters to RealDebugMessage to be processed.
    //
    va_start(VAList, pszFormatString);
    bResult = RealDebugMessage(0x8000, pszFormatString, VAList);
    va_end(VAList);

    return bResult;
}

/*++

Routine Name:

    DbgDOMDoc

Routine Description:

    This routine outputs an XML DOM document to the debug output stream

Arguments:

    pszMessage - Debug message
    pDomDoc    - DOM document to be output

Return Value:

    None.

--*/
VOID
DbgDOMDoc(
    _In_ PCSTR             pszMessage,
    _In_ IXMLDOMDocument2* pDomDoc
    )
{
    try
    {
        CComBSTR xml;

        if (pDomDoc != NULL &&
            SUCCEEDED(pDomDoc->get_xml(&xml)))
        {
            CStringXDA ansi(xml);

            if (pszMessage != NULL)
            {
                DbgPrint("%s%s\n", pszMessage, ansi.GetBuffer());
            }
            else
            {
                DbgPrint("%s\n", ansi.GetBuffer());
            }
        }
    }
    catch (CXDException&)
    {
    }
}

