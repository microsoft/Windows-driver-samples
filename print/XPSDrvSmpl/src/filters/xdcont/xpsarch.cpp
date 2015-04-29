/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   xpsarch.cpp

Abstract:

   Implementation of the XPS archive class. This class is responsible for providing
   an interface to clients that removes the potentially interleaved nature of an
   XPS document. This allows clients to manipulate files using the parts full name
   instead of having to worry about the semantics of interleaved parts.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "xdexcept.h"
#include "xpsarch.h"

typedef HRESULT (*GetClassObject)(REFCLSID, REFIID, LPVOID FAR*);

static PCSTR szPartNameFormatString = "%s/[%i].piece";
static PCSTR szLastPartNameFormatString = "%s/[%i].last.piece";

//
// GUIDs for the archive handler
//
CONST GUID CLSID_PKArchiveHandler = {0x5a0f4115, 0xd4d3, 0x401e, {0x80, 0x71, 0xa4, 0x40, 0xd6, 0xd0, 0x70, 0x92}};
CONST GUID IID_IPKArchive = {0xbdbbdf56, 0xc742, 0x4efd, {0x80, 0x75, 0xaf, 0x2c, 0x7b, 0x24, 0x7f, 0x38}};

/*++

Routine Name:

    CXPSArchive::CXPSArchive

Routine Description:

    CXPSArchive class constructor

Arguments:

    pReadStream    - Pointer to the print read stream
    pWriteStream   - Pointer to the print write stream

Return Value:

    None
    Throws CXDException(HRESULT) on an error

--*/
CXPSArchive::CXPSArchive(
    _In_ IPrintReadStream*  pReadStream,
    _In_ IPrintWriteStream* pWriteStream
    ) :
    m_pWriteStream(pWriteStream),
    m_pPkArchive(NULL),
    m_hPkArch(NULL)
{
    HRESULT hr = S_OK;

    //
    // Get the current directory to load the pk archive library
    //
    DWORD cchName = 0;
    TCHAR* szFileName = new(std::nothrow) TCHAR[MAX_PATH];

    if (SUCCEEDED(hr = CHECK_POINTER(szFileName, E_OUTOFMEMORY)))
    {
        cchName = GetModuleFileName(g_hInstance, szFileName, MAX_PATH);

        if (cchName == 0)
        {
            hr = GetLastErrorAsHResult();
        }
    }

    if (SUCCEEDED(hr) &&
        cchName > 0)
    {
        //
        // Remove the filespec
        //
        PathRemoveFileSpec(szFileName);

        try
        {
            //
            // Append the PK archive DLL name
            //
            CStringXD cstrPath(szFileName);
            cstrPath += TEXT("\\pkarch.dll");

            //
            // Try to load the PK archive DLL
            //
            m_hPkArch = LoadLibrary(cstrPath);
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }

    if (szFileName != NULL)
    {
        delete[] szFileName;
        szFileName = NULL;
    }

    if (SUCCEEDED(hr))
    {
        //
        // Get DllGetClassObject from the PK archive and instantiate the PK archive handler
        //
        if (m_hPkArch != NULL)
        {
            GetClassObject pfnGetClassObject = reinterpret_cast<GetClassObject>(GetProcAddress(m_hPkArch, "DllGetClassObject"));

            if (SUCCEEDED(hr = CHECK_POINTER(pfnGetClassObject, E_NOINTERFACE)) &&
                SUCCEEDED(hr = pfnGetClassObject(CLSID_PKArchiveHandler, IID_IPKArchive, reinterpret_cast<LPVOID*>(&m_pPkArchive))))
            {
                hr = CHECK_POINTER(m_pPkArchive, E_NOINTERFACE);
            }
        }
        else
        {
            hr = E_NOINTERFACE;
        }
    }

    //
    // Initialise the IO streams
    //
    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_POINTER(pReadStream, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pWriteStream, E_POINTER)) &&
        SUCCEEDED(hr = m_pPkArchive->SetReadStream(pReadStream)) &&
        SUCCEEDED(hr = m_pPkArchive->SetWriteStream(m_pWriteStream)))
    {
        //
        // We can now process the read stream to create the file index
        //
        hr = m_pPkArchive->ProcessReadStream();
    }

    if (FAILED(hr))
    {
        throw CXDException(hr);
    }
}

/*++

Routine Name:

    CXPSArchive::~CXPSArchive

Routine Description:

    CXPSArchive class destructor

Arguments:

    None

Return Value:

    None

--*/
CXPSArchive::~CXPSArchive()
{
    if (m_pPkArchive != NULL)
    {
        m_pPkArchive->Close();
        m_pPkArchive = NULL;
    }

    if (m_pWriteStream != NULL)
    {
        m_pWriteStream->Close();
    }

    if (m_hPkArch != NULL)
    {
        FreeLibrary(m_hPkArch);
    }
}

/*++

Routine Name:

    CXPSArchive::InitialiseFile

Routine Description:

    This routine intialises the named file ready for processing
    or sending on. This entails locating all the PK archive files
    that constitute the XPS part.

Arguments:

    szFileName - The name of the part to intialise

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXPSArchive::InitialiseFile(
    _In_z_ PCSTR szFileName
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(szFileName, E_POINTER)))
    {
        try
        {
            CStringXDA cstrFileName(szFileName);

            //
            // Open the file handler - if it is still in use this will fail
            //
            if (SUCCEEDED(hr = m_XpsFile.Open(szFileName)))
            {
                //
                // Find the record or the parts comprising the record
                //
                hr = AddFile(szFileName);
            }
            else
            {
                RIP("File handler has not been closed correctly\n");
            }
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }

    ERR_ON_HR_EXC(hr, E_ELEMENT_NOT_FOUND);
    return hr;
}

/*++

Routine Name:

    CXPSArchive::GetFileStream

Routine Description:

    This routine retrieves the read stream for an XPS part.

Arguments:

    szFileName   - The name of the XPS part the stream is required for
    ppFileStream - Pointer to an ISequentialStream pointer that recieves the stream

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXPSArchive::GetFileStream(
    _In_z_      PCSTR               szFileName,
    _Outptr_ ISequentialStream** ppFileStream
    )
{
    HRESULT hr = S_OK;

    //
    // Initialise the current XPS file
    //
    if (SUCCEEDED(hr = CHECK_POINTER(ppFileStream, E_POINTER)) &&
        SUCCEEDED(hr = InitialiseFile(szFileName)))
    {
        //
        // Query the XPS file for the file stream
        //
        *ppFileStream = NULL;
        hr = m_XpsFile.QueryInterface(IID_ISequentialStream, reinterpret_cast<PVOID*>(ppFileStream));
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CXPSArchive::CloseCurrent

Routine Description:

    This routine closes the currently opened XPS file

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXPSArchive::CloseCurrent(
    VOID
    )
{
    return m_XpsFile.Close();
}

/*++

Routine Name:

    CXPSArchive::SendCurrentFile

Routine Description:

    This routine sends the current initialised file and requests the
    PK archive handler compresses it according to the requested method

Arguments:

    eCompType - The requested compression type

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXPSArchive::SendCurrentFile(
    _In_ ECompressionType eCompType
    )
{
    HRESULT hr = S_OK;

    ULONG cb = 0;
    PVOID pv = NULL;
    PSTR  pName = NULL;

    //
    // Get the data buffer and file name
    //
    if (SUCCEEDED(hr = m_XpsFile.GetBuffer(&pv, &cb)) &&
        SUCCEEDED(hr = m_XpsFile.GetFileName(&pName)))
    {
        hr = SendFile(pName, pv, cb, eCompType);
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CXPSArchive::SendCurrentFile

Routine Description:

    This routine sends the current initialised file by requesting the
    PK archive handler send the original constituent PK records

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXPSArchive::SendCurrentFile(
    VOID
    )
{
    HRESULT hr = S_OK;

    XPSPartStack* pPartStack = NULL;
    PSTR  pName = NULL;

    if (SUCCEEDED(hr = CHECK_POINTER(m_pPkArchive, E_NOINTERFACE)) &&
        SUCCEEDED(hr = m_XpsFile.GetFileParts(&pPartStack)) &&
        SUCCEEDED(hr = m_XpsFile.GetFileName(&pName)))
    {
        try
        {
            //
            // Send all file parts on to the PK archive
            //
            if (!m_sentList[pName])
            {
                XPSPartStack::const_iterator iterParts = pPartStack->begin();

                for (;
                     iterParts != pPartStack->end() && SUCCEEDED(hr);
                     iterParts++)
                {
                    hr = m_pPkArchive->SendFile(iterParts->first);
                }

                if (SUCCEEDED(hr))
                {
                    m_sentList[pName] = TRUE;
                }
            }
        }
        catch (exception& DBG_ONLY(e))
        {
            ERR(e.what());
            hr = E_FAIL;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CXPSArchive::SendFile

Routine Description:

    This routine sends a file defined by it's name and constituent data. The
    routine passes the name and buffer to the PK archive handler to compress
    and add to the archive.

Arguments:

    szFileName - The name of the part
    pBuffer    - The buffer containing the part data
    cbBuffer   - The size of the data buffer
    eCompType  - The requested compression type

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXPSArchive::SendFile(
    _In_z_                PCSTR            szFileName,
    _In_reads_bytes_(cbBuffer) PVOID            pBuffer,
    _In_                  ULONG            cbBuffer,
    _In_                  ECompressionType eCompType
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(m_pPkArchive, E_NOINTERFACE)) &&
        SUCCEEDED(hr = CHECK_POINTER(szFileName, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pBuffer, E_POINTER)))
    {
        if (eCompType == CompDeflated ||
            eCompType == CompNone)
        {
            try
            {
                //
                // Get the PK archive to compress and send the file on
                //
                if (!m_sentList[szFileName])
                {
                    if (SUCCEEDED(hr = m_pPkArchive->SendFile(szFileName, pBuffer, cbBuffer, eCompType)))
                    {
                        m_sentList[szFileName] = TRUE;
                    }
                }
            }
            catch (exception& DBG_ONLY(e))
            {
                ERR(e.what());
                hr = E_FAIL;
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CXPSArchive::AddFile

Routine Description:

    This routine adds the PK file or constiuent PK files to the XPS file
    ready for retrieval of the read stream. This hides the XPS piece handling
    from the client which just adds the part by it's full un-interleaved name

Arguments:

    szFileName - The name of the part

Return Value:

    HRESULT
    S_OK                - On success
    E_ELEMENT_NOT_FOUND - When part not present or badly formed in the container
    E_*                 - On error

--*/
HRESULT
CXPSArchive::AddFile(
    _In_z_ PCSTR szFileName
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(m_pPkArchive, E_NOINTERFACE)) &&
        SUCCEEDED(hr = CHECK_POINTER(szFileName, E_POINTER)))
    {
        //
        // Find the part or parts comprising the file
        //
        NameIndex* pNameIndex = NULL;

        if (SUCCEEDED(hr = m_pPkArchive->GetNameIndex(&pNameIndex)))
        {
            try
            {
                NameIndex::const_iterator iterNameIndex = pNameIndex->find(szFileName);
                if (iterNameIndex != pNameIndex->end())
                {
                    //
                    // There is a single part
                    //
                    hr = m_XpsFile.AddFilePart(CStringXDA(szFileName), iterNameIndex->second);
                }
                else
                {
                    //
                    // This could be a multipart interleaved file
                    //
                    UINT cPart = 0;
                    do
                    {
                        CStringXDA cstrPart;
                        cstrPart.Format(szPartNameFormatString, szFileName, cPart);

                        iterNameIndex = pNameIndex->find(cstrPart);
                        if (iterNameIndex != pNameIndex->end())
                        {
                            //
                            // We found the next part
                            //
                            hr = m_XpsFile.AddFilePart(CStringXDA(szFileName), iterNameIndex->second);
                            cPart++;
                        }
                        else
                        {
                            cstrPart.Format(szLastPartNameFormatString, szFileName, cPart);
                            iterNameIndex = pNameIndex->find(cstrPart);

                            if (iterNameIndex != pNameIndex->end())
                            {
                                //
                                // We found the last part
                                //
                                hr = m_XpsFile.AddFilePart(CStringXDA(szFileName), iterNameIndex->second);
                                cPart++;
                                break;
                            }
                            else
                            {
                                //
                                // The element is either not well constructed (i.e. missing a piece or the last
                                // part) or it does not exist
                                //
                                hr = E_ELEMENT_NOT_FOUND;
                            }
                        }
                    }
                    while (SUCCEEDED(hr));
                }
            }
            catch (CXDException& e)
            {
                hr = e;
            }
            catch (exception& DBG_ONLY(e))
            {
                ERR(e.what());
                hr = E_FAIL;
            }
        }
    }

    ERR_ON_HR_EXC(hr, E_ELEMENT_NOT_FOUND);
    return hr;
}
