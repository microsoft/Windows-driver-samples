//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1998 - 2003  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:   Intrface.cpp
//
//
//  PURPOSE:  Interface for User Mode COM Customization DLL.
//
//
//  Functions:
//          IPrintOemUni2 interface functions
//          COemCF class factory
//          Registration functions: DllCanUnloadNow, DllGetClassObject
//
//  History:
//          06/26/03    xxx created.
//
//

#include "precomp.h"
#include <INITGUID.H>
#include <PRCOMOEM.H>

#include "oemprean.h"
#include "debug.h"
#include "intrface.h"

// This indicates to Prefast that this is a usermode driver file.
_Analysis_mode_(_Analysis_code_type_user_driver_);

// ==================================================================
// Internal Globals
//
static long g_cComponents;      // Count of active components
static long g_cServerLocks;     // Count of locks
//
// ==================================================================

// ==================================================================
// The purpose of this array is to inform UNIDRV of the callbacks
// that are implemented in this driver.
//
// Note that there is *NO* order dependency in this array. New
// index values and their corresponding callbacks can be placed
// anywhere within the list as needed.
//
static const DRVFN s_aOemHookFuncs[] =
{
    // The following are defined in ddihook.cpp.
    //
    {INDEX_DrvAlphaBlend, (PFN)OEMAlphaBlend},
    {INDEX_DrvBitBlt, (PFN)OEMBitBlt},
    {INDEX_DrvCopyBits, (PFN)OEMCopyBits},
    {INDEX_DrvDitherColor, (PFN)OEMDitherColor},
    {INDEX_DrvFillPath, (PFN)OEMFillPath},
    {INDEX_DrvFontManagement, (PFN)OEMFontManagement},
    {INDEX_DrvGetGlyphMode, (PFN)OEMGetGlyphMode},
    {INDEX_DrvGradientFill, (PFN)OEMGradientFill},
    {INDEX_DrvLineTo, (PFN)OEMLineTo},
    {INDEX_DrvPaint, (PFN)OEMPaint},
    {INDEX_DrvPlgBlt, (PFN)OEMPlgBlt},
    {INDEX_DrvQueryAdvanceWidths, (PFN)OEMQueryAdvanceWidths},
    {INDEX_DrvQueryFont, (PFN)OEMQueryFont},
    {INDEX_DrvQueryFontData, (PFN)OEMQueryFontData},
    {INDEX_DrvQueryFontTree, (PFN)OEMQueryFontTree},
    {INDEX_DrvRealizeBrush, (PFN)OEMRealizeBrush},
    {INDEX_DrvStretchBlt, (PFN)OEMStretchBlt},
    {INDEX_DrvStretchBltROP, (PFN)OEMStretchBltROP},
    {INDEX_DrvStrokeAndFillPath, (PFN)OEMStrokeAndFillPath},
    {INDEX_DrvStrokePath, (PFN)OEMStrokePath},
    {INDEX_DrvTextOut, (PFN)OEMTextOut},
    {INDEX_DrvTransparentBlt, (PFN)OEMTransparentBlt},
    {INDEX_DrvStartDoc, (PFN)OEMStartDoc},
    {INDEX_DrvEndDoc, (PFN)OEMEndDoc},
    {INDEX_DrvStartPage, (PFN)OEMStartPage},
    {INDEX_DrvSendPage, (PFN)OEMSendPage},
    {INDEX_DrvStartBanding, (PFN)OEMStartBanding},
    {INDEX_DrvNextBand, (PFN)OEMNextBand},
    {INDEX_DrvEscape, (PFN)OEMEscape}
};


////////////////////////////////////////////////////////////////////////////////
//
// COemUni2 body
//
COemUni2::COemUni2()
{
    m_cRef = 1;

    // Increment COM component count.
    InterlockedIncrement(&g_cComponents);

    m_pOEMHelp = NULL;

}

COemUni2::~COemUni2()
{
    // Make sure that helper interface is released.
    if(NULL != m_pOEMHelp)
    {
        m_pOEMHelp->Release();
        m_pOEMHelp = NULL;
    }

    // If this instance of the object is being deleted, then the reference
    // count should be zero.
    assert(0 == m_cRef);
}

HRESULT __stdcall
COemUni2::
QueryInterface(
    const IID&  iid,
    void**      ppv
    )
{
    VERBOSE(L"COemUni2::QueryInterface() entry.");

    if (iid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown*>(this);
        VERBOSE(DLLTEXT("COemUni2::QueryInterface IUnknown.\r\n"));
    }
    else if (iid == IID_IPrintOemUni2)
    {
        VERBOSE(DLLTEXT("COemUni2::QueryInterface IPrintOemUni2.\r\n"));

        *ppv = static_cast<IPrintOemUni2*>(this);
    }
    else
    {
        WARNING(DLLTEXT("COemUni2::QueryInterface NULL. Returning E_NOINTERFACE.\r\n"));

        *ppv = NULL;
        return E_NOINTERFACE;
    }
    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

ULONG __stdcall
COemUni2::
AddRef(
    VOID
    )
{
    VERBOSE(L"COemUni2::AddRef() entry.");

    return InterlockedIncrement(&m_cRef);
}

_At_(this, __drv_freesMem(object)) 
ULONG __stdcall
COemUni2::
Release(
    VOID
    )
{
    VERBOSE(L"COemUni2::Release() entry.");
    ULONG   cRef    = InterlockedDecrement(&m_cRef);

    if (cRef == 0)
    {
        delete this;
        return 0;
    }
    return cRef;
}

HRESULT __stdcall
COemUni2::
CommandCallback(
    PDEVOBJ     pdevobj,
    DWORD       dwCallbackID,
    DWORD       dwCount,
    PDWORD      pdwParams,
    OUT INT     *piResult
    )

/*++

Routine Description:

    Implementation of IPrintOemUni::CommandCallback

    The IPrintOemUni::CommandCallback method is used
    to provide dynamically generated printer commands
    for Unidrv-supported printers.

    Please refer to DDK documentation for more details.

Arguments:

    pdevobj - pointer to a DEVOBJ structure.
    dwCallbackID - value representing the printer command's
        *CallbackID attribute in the printer's generic printer
        description (GPD) file.
    dwCount - value representing the number of elements
        in the array pointed to by pdwParams. Can be 0.
    pdwParams - pointer to an array of DWORD-sized parameters
        containing values specified by the printer commands
        *Params attribute in the printer's GPD file. Can be NULL.
    piResult - Receives a method-supplied result value.

Return Value:

    S_OK The operation succeeded.
    E_FAIL The operation failed.
    E_NOTIMPL The method is not implemented.


--*/

{
    VERBOSE(L"COemUni2::CommandCallback() entry.");

    UNREFERENCED_PARAMETER(pdevobj);
    UNREFERENCED_PARAMETER(dwCallbackID);
    UNREFERENCED_PARAMETER(dwCount);
    UNREFERENCED_PARAMETER(pdwParams);

    *piResult = 0;

    return S_OK;
}

HRESULT __stdcall
COemUni2::
Compression(
    PDEVOBJ     pdevobj,
    PBYTE       pInBuf,
    PBYTE       pOutBuf,
    DWORD       dwInLen,
    DWORD       dwOutLen,
    OUT INT     *piResult
    )

/*++

Routine Description:

    Implementation of IPrintOemUni::Compression

    The IPrintOemUni::Compression method can be used
    with Unidrv-supported printers to provide a customized
    bitmap compression method.

    Please refer to DDK documentation for more details.

Arguments:

    pdevobj - pointer to a DEVOBJ structure.
    pInBuf - pointer to input scan line data.
    pOutBuf - pointer to an output buffer to receive
        compressed scan line data.
    dwInLen - length of the input data.
    dwOutLen - length of the output buffer.
    piResult - Receives a method-supplied result value. If the
        operation succeeds, this value should be the number
        of compressed bytes, which must not be larger than
        the value received for dwOutLen. If an error occurs,
        or if the method cannot compress, the result value
        should be -1.

Return Value:

    S_OK The operation succeeded.
    E_FAIL The operation failed.
    E_NOTIMPL The method is not implemented.


--*/

{
    VERBOSE(L"COemUni2::Compression() entry.");

    UNREFERENCED_PARAMETER(pdevobj);
    UNREFERENCED_PARAMETER(pInBuf);
    UNREFERENCED_PARAMETER(pOutBuf);
    UNREFERENCED_PARAMETER(dwInLen);
    UNREFERENCED_PARAMETER(dwOutLen);
    UNREFERENCED_PARAMETER(piResult);

    return E_NOTIMPL;
}

HRESULT __stdcall
COemUni2::
DevMode(
    DWORD           dwMode,
    POEMDMPARAM pOemDMParam
    )

/*++

Routine Description:

    Implementation of IPrintOemUni::DevMode

    The IPrintOemUni::DevMode method, provided by
    rendering plug-ins for Unidrv, performs operations
    on private DEVMODE members.

    A rendering plug-in for Unidrv MUST implement the
    IPrintOemUni::DevMode method.

    Please refer to DDK documentation for more details.

Arguments:

    dwMode - caller-supplied constant. Refer to the docs
        for more information.
    pOemDMParam - pointer to an OEMDMPARAM structure.

Return Value:

    S_OK The operation succeeded.
    E_FAIL The operation failed.


--*/

{
    VERBOSE(L"COemUni2:DevMode entry.");

    DBG_OEMDMPARAM(DBG_VERBOSE, L"pOemDMParam", pOemDMParam);

    return hrOEMDevMode(dwMode, pOemDMParam);
}

HRESULT __stdcall
COemUni2::
DisableDriver(
    VOID
    )

/*++

Routine Description:

    Implementation of IPrintOemUni::DisableDriver

    The IPrintOemUni::DisableDriver method allows a rendering
    plug-in for Unidrv to free resources that were allocated by the
    plug-in's IPrintOemUni::EnableDriver method. This is the last
    IPrintOemUni interface method that is called before the rendering
    plug-in is unloaded.

    A rendering plug-in for Unidrv MUST implement the
    IPrintOemUni::DisableDriver method.

    Please refer to DDK documentation for more details.

Arguments:

    VOID

Return Value:

    S_OK The operation succeeded.
    E_FAIL The operation failed.


--*/

{
    VERBOSE(L"COemUni2::DisableDriver() entry.");

    // Release reference to Printer Driver's interface.
    //
    if (this->m_pOEMHelp)
    {
        this->m_pOEMHelp->Release();
        this->m_pOEMHelp = NULL;
    }

    return S_OK;
}

HRESULT __stdcall
COemUni2::
DisablePDEV(
    PDEVOBJ     pdevobj
    )

/*++

Routine Description:

    Implementation of IPrintOemUni::DisablePDEV

    The IPrintOemUni::DisablePDEV method performs the same types of
    operations as the DrvDisablePDEV function. Its purpose is to allow a
    rendering plug-in to delete the private PDEV structure that is pointed
    to by the DEVOBJ structure's pdevOEM member. This PDEV structure
    is one that was allocated by the plug-in's IPrintOemUni::EnablePDEV method.

    A rendering plug-in for Unidrv MUST implement the
    IPrintOemUni::DisablePDEV method.

    Please refer to DDK documentation for more details.

Arguments:

    pdevobj - pointer to a DEVOBJ structure.

Return Value:

    S_OK The operation succeeded.
    E_FAIL The operation failed.


--*/

{
    VERBOSE(L"COemUni2::DisablePDEV() entry.");

    POEMPDEV pOemPDEV = static_cast<POEMPDEV>(pdevobj->pdevOEM);
    delete pOemPDEV;

    return S_OK;
}

HRESULT __stdcall
COemUni2::
DownloadCharGlyph(
    PDEVOBJ         pdevobj,
    PUNIFONTOBJ     pUFObj,
    HGLYPH          hGlyph,
    PDWORD          pdwWidth,
    OUT DWORD       *pdwResult
    )

/*++

Routine Description:

    Implementation of IPrintOemUni::DownloadCharGlyph

    The IPrintOemUni::DownloadCharGlyph method enables a
    rendering plug-in for Unidrv to send a character glyph for
    a specified soft font to the printer.

    Please refer to DDK documentation for more details.

Arguments:

    pdevobj - pointer to a DEVOBJ structure.
    pUFObj - pointer to a UNIFONTOBJ structure.
    hGlyph - glyph handle.
    pdwWidth - pointer to receive the method-supplied width of the character.
    pdwResult - Receives a method-supplied value representing the amount
        of printer memory, in bytes, required to store the character glyph.
        If the operation fails, the returned value should be zero.

Return Value:

    S_OK The operation succeeded.
    E_FAIL The operation failed.
    E_NOTIMPL The method is not implemented.


--*/

{
    VERBOSE(L"COemUni2::DownloadCharGlyph() entry.");

    UNREFERENCED_PARAMETER(pdevobj);
    UNREFERENCED_PARAMETER(pUFObj);
    UNREFERENCED_PARAMETER(hGlyph);
    UNREFERENCED_PARAMETER(pdwWidth);
    UNREFERENCED_PARAMETER(pdwResult);

    return E_NOTIMPL;
}

HRESULT __stdcall
COemUni2::
DownloadFontHeader(
    PDEVOBJ         pdevobj,
    PUNIFONTOBJ     pUFObj,
    OUT DWORD       *pdwResult
    )

/*++

Routine Description:

    Implementation of IPrintOemUni::DownloadFontHeader

    The IPrintOemUni::DownloadFontHeader method allows a
    rendering plug-in for Unidrv to send a font's header
    information to a printer.

    Please refer to DDK documentation for more details.

Arguments:

    pdevobj - pointer to a DEVOBJ structure.
    pUFObj - pointer to a UNIFONTOBJ structure.
    pdwResult - Receives a method-supplied value representing the amount
        of printer memory, in bytes, required to store the font header
        information. If the operation fails, the returned value should be zero.

Return Value:

    S_OK The operation succeeded.
    E_FAIL The operation failed.
    E_NOTIMPL The method is not implemented.


--*/

{
    VERBOSE(L"COemUni2::DownloadFontHeader() entry.");

    UNREFERENCED_PARAMETER(pdevobj);
    UNREFERENCED_PARAMETER(pUFObj);
    UNREFERENCED_PARAMETER(pdwResult);

    return E_NOTIMPL;
}

HRESULT __stdcall
COemUni2::
DriverDMS(
    PVOID       pDevObj,
    PVOID       pBuffer,
    DWORD       cbSize,
    PDWORD      pcbNeeded
    )

/*++

Routine Description:

    Implementation of IPrintOemUni::DriverDMS

    The IPrintOemUni::DriverDMS method allows a rendering
    plug-in for Unidrv to indicate that it uses a device-managed
    drawing surface instead of the default GDI-managed surface.

    A rendering plug-in for Unidrv MUST implement the
    IPrintOemUni::DriverDMS method.

    Please refer to DDK documentation for more details. For general info
    on device managed surfaces and how to handle them, please refer to
    the section titled "Handling Device-Managed Surfaces".

Arguments:

    pDevObj - pointer to a DEVOBJ structure.
    pBuffer - pointer to a buffer to receive method-specified flags.
    cbSize - size, in bytes, of the buffer pointed to by pBuffer.
    pcbNeeded - pointer to a location to receive the required
        minimum pBuffer size.

Return Value:

    S_OK The operation succeeded.
    E_FAIL The operation failed.


--*/

{
    VERBOSE(L"COemUni2::DriverDMS() entry.");

    UNREFERENCED_PARAMETER(pDevObj);
    UNREFERENCED_PARAMETER(pBuffer);
    UNREFERENCED_PARAMETER(cbSize);
    UNREFERENCED_PARAMETER(pcbNeeded);

    return E_NOTIMPL;
}

HRESULT __stdcall
COemUni2::
EnableDriver(
    DWORD               dwDriverVersion,
    DWORD               cbSize,
    PDRVENABLEDATA      pded
    )

/*++

Routine Description:

    Implementation of IPrintOemUni::EnableDriver

    The IPrintOemUni::EnableDriver method allows a rendering
    plug-in for Unidrv to hook out some graphics DDI functions.

    The IPrintOemUni::EnableDriver method allows a rendering
    plug-in to perform the same types of operations as the
    DrvEnableDriver function. Like the DrvEnableDriver function,
    the IPrintOemUni::EnableDriver method is responsible for
    providing addresses of internally supported graphics DDI functions,
    or DDI hook functions.

    The method should fill the supplied DRVENABLEDATA structure
    and allocate an array of DRVFN structures. It should fill the
    array with pointers to hooking functions, along with winddi.h-defined
    index values that identify the hooked out graphics DDI functions.

    A rendering plug-in for Unidrv MUST implement the
    IPrintOemUni::EnableDriver method.

    Please refer to DDK documentation for more details.

Arguments:

    DriverVersion - interface version number. This value is
        defined by PRINTER_OEMINTF_VERSION, in printoem.h.
    cbSize - size, in bytes, of the structure pointed to by pded.
    pded - pointer to a DRVENABLEDATA structure.

Return Value:

    S_OK The operation succeeded.
    E_FAIL The operation failed.


--*/

{
    UNREFERENCED_PARAMETER(dwDriverVersion);
    UNREFERENCED_PARAMETER(cbSize);

    VERBOSE(L"COemUni2::EnableDriver() entry.");

    pded->iDriverVersion =  PRINTER_OEMINTF_VERSION;
    pded->c = sizeof(s_aOemHookFuncs) / sizeof(DRVFN);
    pded->pdrvfn = (DRVFN *) s_aOemHookFuncs;

    // Even if nothing is done, need to return S_OK so
    // that DisableDriver() will be called, which releases
    // the reference to the Printer Driver's interface.
    //
    return S_OK;
}

HRESULT __stdcall
COemUni2::
EnablePDEV(
    PDEVOBJ             pdevobj,
    _In_ PWSTR          pPrinterName,
    ULONG               cPatterns,
    HSURF               *phsurfPatterns,
    ULONG               cjGdiInfo,
    GDIINFO             *pGdiInfo,
    ULONG               cjDevInfo,
    DEVINFO             *pDevInfo,
    DRVENABLEDATA       *pded,
    OUT PDEVOEM     *ppDevOem
    )

/*++

Routine Description:

    Implementation of IPrintOemUni::EnablePDEV

    The IPrintOemUni::EnablePDEV method performs the same types
    of operations as the DrvEnablePDEV function that is exported
    by a printer graphics DLL. Its purpose is to allow a rendering
    plug-in to create its own PDEV structure. For more information
    about PDEV structures, see "Customized PDEV Structures" in the DDK docs.

    A rendering plug-in for Unidrv MUST implement the
    IPrintOemUni::EnablePDEV method.

    Please refer to DDK documentation for more details.

Arguments:

    pdevobj - pointer to a DEVOBJ structure.
    pPrinterName - pointer to a text string representing the
        logical address of the printer.
    cPatterns - value representing the number of HSURF-typed
        surface handles contained in the buffer pointed to by
        phsurfPatterns.
    phsurfPatterns - pointer to a buffer that is large enough to
        contain cPatterns number of HSURF-typed surface handles.
        The handles represent surface fill patterns.
    cjGdiInfo - value representing the size of the structure pointed
        to by pGdiInfo.
    pGdiInfo - pointer to a GDIINFO structure.
    cjDevInfo - value representing the size of the structure pointed
        to by pDevInfo.
    pDevInfo - pointer to a DEVINFO structure.
    pded - pointer to a DRVENABLEDATA structure containing the
        addresses of the printer driver's graphics DDI hooking functions.
    pDevOem - Receives a method-supplied pointer to a private PDEV structure.

Return Value:

    S_OK The operation succeeded.
    E_FAIL The operation failed.

    If the operation fails, the method should call SetLastError to set an error code.


--*/

{
    VERBOSE(L"COemUni2::EnablePDEV() entry.");

    UNREFERENCED_PARAMETER(pdevobj);
    UNREFERENCED_PARAMETER(pPrinterName);
    UNREFERENCED_PARAMETER(cPatterns);
    UNREFERENCED_PARAMETER(phsurfPatterns);
    UNREFERENCED_PARAMETER(cjGdiInfo);
    UNREFERENCED_PARAMETER(cjDevInfo);

    #ifndef DBG
    UNREFERENCED_PARAMETER(pGdiInfo);
    UNREFERENCED_PARAMETER(pDevInfo);
    #endif

    // Allocate an instance of our private PDEV.
    //
    POEMPDEV pOemPDEV = new COemPDEV();
    *ppDevOem = pOemPDEV;

    if (NULL == pOemPDEV)
    {
        return E_FAIL;
    }

    pOemPDEV->InitializeDDITable(pded);

    DBG_GDIINFO(DBG_VERBOSE, L"pGdiInfo", pGdiInfo);
    DBG_DEVINFO(DBG_VERBOSE, L"pDevInfo", pDevInfo);

    pOemPDEV->bPreAnalysis = FALSE;         // Initialize the pre-analysis flag

    return S_OK;
}

HRESULT __stdcall
COemUni2::
FilterGraphics(
    PDEVOBJ     pdevobj,
    PBYTE       pBuf,
    DWORD       dwLen
    )

/*++

Routine Description:

    Implementation of IPrintOemUni::FilterGraphics

    The IPrintOemUni::FilterGraphics method can be used with
    Unidrv-supported printers to modify scan line data and send
    it to the spooler.

    Please refer to DDK documentation for more details.

Arguments:

    pdevobj - pointer to a DEVOBJ structure.
    pBuf - pointer to a buffer containing scan line data to be printed.
    dwLen - value representing the length, in bytes, of the data
        pointed to by pBuf.

Return Value:

    S_OK The operation succeeded.
    E_FAIL The operation failed.
    E_NOTIMPL The method is not implemented.


--*/

{
    DWORD dwResult;

    VERBOSE(L"COemUni2::FilterGraphics() entry.");

    m_pOEMHelp->DrvWriteSpoolBuf(pdevobj, pBuf, dwLen, &dwResult);

    if (dwResult == dwLen)
        return S_OK;
    else
        return S_FALSE;
}

HRESULT __stdcall
COemUni2::
GetImplementedMethod(
    _In_ PSTR pMethodName
    )

/*++

Routine Description:

    Implementation of IPrintOemUni::GetImplementedMethod

    The IPrintOemUni::GetImplementedMethod method is used by
    Unidrv to determine which IPrintOemUni interface methods
    have been implemented by a rendering plug-in.

    A rendering plug-in for Unidrv MUST implement the
    IPrintOemUni::GetImplemented method.

    Please refer to DDK documentation for more details.

Arguments:

    pMethodName - pointer to a string representing the name of an
        IPrintOemUni interface method, such as "ImageProcessing"
        for IPrintOemUni::ImageProcessing or "FilterGraphics" for
        IPrintOemUni::FilterGraphics.

Return Value:

    S_OK The operation succeeded (the specified method is implemented).
    S_FALSE The operation failed (the specified method is not implemented).


--*/

{
    HRESULT Result = S_FALSE;

    VERBOSE(L"COemUni2::GetImplementedMethod() entry.");
    VERBOSE(TEXT("\tCOemUni2::%hs: "), pMethodName);

    // Unidrv only calls GetImplementedMethod for optional
    // methods.  The required methods are assumed to be
    // supported.

    // Return S_OK for supported function (i.e. implemented),
    // and S_FALSE for functions that aren't supported (i.e. not implemented).
    switch (*pMethodName)
    {
        case 'C':
            if (!strcmp(NAME_CommandCallback, pMethodName))
            {
                Result = S_OK;
            }
            break;
        case 'F':
            if (!strcmp(NAME_FilterGraphics, pMethodName))
            {
                Result = S_OK;
            }
            break;
    }

    VERBOSE(Result == S_OK ? TEXT("Supported\r\n") : TEXT("NOT supported\r\n"));

    return Result;
}

HRESULT __stdcall
COemUni2::
GetInfo(
    DWORD       dwMode,
    PVOID       pBuffer,
    DWORD       cbSize,
    PDWORD      pcbNeeded
    )

/*++

Routine Description:

    Implementation of IPrintOemUni::GetInfo

    A rendering plug-in's IPrintOemUni::GetInfo method returns
    identification information.

    A rendering plug-in for Unidrv MUST implement the
    IPrintOemUni::GetInfo method.

    Please refer to DDK documentation for more details.

Arguments:

    dwMode - Contains one of the following caller-supplied integer constants.
        OEMGI_GETSIGNATURE - The method must return a unique four-byte
            identification signature. The plug-in must also place this signature
            in OPTITEM structures, as described in the description of the
            OEMCUIPPARAM. structure's pOEMOptItems member.
        OEMGI_GETVERSION - The method must return the user interface
            plug-in's version number as a DWORD. The version format is
            developer-defined.
    pBuffer - pointer to memory allocated to receive the information specified
        by dwInfo.
    cbSize - size of the buffer pointed to by pBuffer.
    pcbNeeded - pointer to a location to receive the number of bytes written
        into the buffer pointed to by pBuffer.

Return Value:

    S_OK The operation succeeded (the specified method is implemented).
    S_FALSE The operation failed (the specified method is not implemented).


--*/

{
    PWSTR pszTag = L"COemUni2::GetInfo entry.";

    switch(dwMode)
    {
        case OEMGI_GETSIGNATURE: 
            pszTag = L"COemUni2::GetInfo entry. [OEMGI_GETSIGNATURE]"; 
            break;
        case OEMGI_GETVERSION: 
            pszTag = L"COemUni2::GetInfo entry. [OEMGI_GETVERSION]"; 
            break;
    }

    VERBOSE(pszTag);

    // Validate parameters.
    if( (NULL == pcbNeeded)
        ||
        ( (OEMGI_GETSIGNATURE != dwMode)
        &&
        (OEMGI_GETVERSION != dwMode) ) )
    {
        WARNING(DLLTEXT("COemUni2::GetInfo() exit pcbNeeded is NULL! ERROR_INVALID_PARAMETER.\r\n"));
        SetLastError(ERROR_INVALID_PARAMETER);
        return E_FAIL;
    }

    // Set expected buffer size.
    *pcbNeeded = sizeof(DWORD);

    // Check buffer size is sufficient.
    if((cbSize < *pcbNeeded) || (NULL == pBuffer))
    {
        VERBOSE(TEXT("COemUni2::GetInfo() exit insufficient buffer!\r\n"));
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return E_FAIL;
    }

    switch(dwMode)
    {
        // OEM DLL Signature
        case OEMGI_GETSIGNATURE:
            *(PDWORD)pBuffer = OEM_SIGNATURE;
            break;

        // OEM DLL version
        case OEMGI_GETVERSION:
            *(PDWORD)pBuffer = OEM_VERSION;
            break;

        // dwMode not supported.
        default:
            // Set written bytes to zero since nothing was written.
            WARNING(DLLTEXT("COemUni2::GetInfo() exit mode not supported.\r\n"));
            *pcbNeeded = 0;
            SetLastError(ERROR_NOT_SUPPORTED);
            return E_FAIL;
    }

    VERBOSE(TEXT("COemUni2::GetInfo() exit S_OK, (*pBuffer is %#x).\r\n"), *(PDWORD)pBuffer);

    return S_OK;
}

HRESULT __stdcall
COemUni2::
HalftonePattern(
    PDEVOBJ     pdevobj,
    PBYTE       pHTPattern,
    DWORD       dwHTPatternX,
    DWORD       dwHTPatternY,
    DWORD       dwHTNumPatterns,
    DWORD       dwCallbackID,
    PBYTE       pResource,
    DWORD       dwResourceSize
    )

/*++

Routine Description:

    Implementation of IPrintOemUni::HalftonePattern

    The IPrintOemUni::HalftonePattern method can be used with
    Unidrv-supported printers to create or modify a halftone
    pattern before it is used in a halftoning operation.

    Please refer to DDK documentation for more details.

Arguments:

    pdevobj - pointer to a DEVOBJ structure.
    pHTPattern - pointer to a buffer to receive the method-supplied
        halftone pattern. Buffer size, in bytes, is:
        (((dwHTPatternX * dwHTPatternY) + 3)/4) * 4 * dwHTNumPatterns.
    dwHTPatternX - length, in pixels, of the halftone pattern, as specified
        by the GPD file's *HTPatternSize attribute.
    dwHTPatternY - height, in pixels, of the halftone pattern, as specified
        by the GPD file's *HTPatternSize attribute.
    dwHTNumPatterns - number of patterns, as specified by the GPD file's
        *HTNumPatterns attribute. This can be 1 or 3.
    dwCallbackID - value identifying the halftone method, as specified by
        the GPD file's *HTCallbackID attribute.
    pResource - pointer to a buffer containing a halftone pattern, as
        specified by the GPD file's *rcHTPatternID attribute. This can be NULL.
    dwResourceSize - size of the halftone pattern contained in the buffer
        pointed to by pResource. This is zero if pResource is NULL.

Return Value:

    S_OK The operation succeeded.
    E_FAIL The operation failed.
    E_NOTIMPL The method is not implemented.


--*/

{
    VERBOSE(L"COemUni2::HalftonePattern() entry.");

    UNREFERENCED_PARAMETER(pdevobj);
    UNREFERENCED_PARAMETER(pHTPattern);
    UNREFERENCED_PARAMETER(dwHTPatternX);
    UNREFERENCED_PARAMETER(dwHTPatternY);
    UNREFERENCED_PARAMETER(dwHTNumPatterns);
    UNREFERENCED_PARAMETER(dwCallbackID);
    UNREFERENCED_PARAMETER(pResource);
    UNREFERENCED_PARAMETER(dwResourceSize);

    return E_NOTIMPL;
}

HRESULT __stdcall
COemUni2::
ImageProcessing(
    PDEVOBJ                 pdevobj,
    PBYTE                   pSrcBitmap,
    PBITMAPINFOHEADER       pBitmapInfoHeader,
    PBYTE                   pColorTable,
    DWORD                   dwCallbackID,
    PIPPARAMS               pIPParams,
    OUT PBYTE               *ppbResult
    )

/*++

Routine Description:

    Implementation of IPrintOemUni::ImageProcessing

    The IPrintOemUni::ImageProcessing method can be used with
    Unidrv-supported printers to modify image bitmap data, in order
    to perform color formatting or halftoning. The method can return
    the modified bitmap to Unidrv or send it directly to the print spooler.

    Please refer to DDK documentation for more details.

Arguments:

    pdevobj - pointer to a DEVOBJ structure.
    pSrcBitmap - pointer to an input device-independent bitmap (DIB).
    pBitmapInfoHeader - pointer to a BITMAPINFOHEADER structure
        that describes the bitmap pointed to by pSrcBitmap.
    pColorTable - pointer to a color table. Used only if the output format is
        eight bits per pixel.
    dwCallbackID - value assigned to the *IPCallbackID attribute of the
        currently selected option for the ColorMode feature.
    pIPParams - pointer to an IPPARAMS structure.
    ppbResult -
        If the method returns the converted DIB to Unidrv:
            If the conversion succeeds, the method should return a
                pointer to a buffer containing the converted DIB.
                Otherwise it should return NULL.
        If the method sends the converted DIB to the print spooler:
            If the operation succeeds, the method should return TRUE.
                Otherwise it should return FALSE.

Return Value:

    S_OK The operation succeeded.
    E_FAIL The operation failed.
    E_NOTIMPL The method is not implemented.


--*/

{
    VERBOSE(L"COemUni2::ImageProcessing() entry.");

    UNREFERENCED_PARAMETER(pdevobj);
    UNREFERENCED_PARAMETER(pSrcBitmap);
    UNREFERENCED_PARAMETER(pBitmapInfoHeader);
    UNREFERENCED_PARAMETER(pColorTable);
    UNREFERENCED_PARAMETER(dwCallbackID);
    UNREFERENCED_PARAMETER(pIPParams);
    UNREFERENCED_PARAMETER(ppbResult);

    return E_NOTIMPL;
}

HRESULT __stdcall
COemUni2::
MemoryUsage(
    PDEVOBJ             pdevobj,
    POEMMEMORYUSAGE     pMemoryUsage
    )

/*++

Routine Description:

    Implementation of IPrintOemUni::MemoryUsage

    The IPrintOemUni::MemoryUsage method can be used with
    Unidrv-supported printers to specify the amount of memory
    required for use by a rendering plug-in's IPrintOemUni::ImageProcessing
    method.

    Please refer to DDK documentation for more details.

Arguments:

    pdevobj - pointer to a DEVOBJ structure.
    pMemoryUsage - pointer to an OEMMEMORYUSAGE structure.

Return Value:

    S_OK The operation succeeded.
    E_FAIL The operation failed.
    E_NOTIMPL The method is not implemented.


--*/

{
    VERBOSE(L"COemUni2::MemoryUsage() entry.");

    UNREFERENCED_PARAMETER(pdevobj);
    UNREFERENCED_PARAMETER(pMemoryUsage);

    return E_NOTIMPL;
}

HRESULT __stdcall
COemUni2::
OutputCharStr(
    PDEVOBJ         pdevobj,
    PUNIFONTOBJ     pUFObj,
    DWORD           dwType,
    DWORD           dwCount,
    PVOID           pGlyph
    )

/*++

Routine Description:

    Implementation of IPrintOemUni::OutputCharStr

    The IPrintOemUni::OutputCharStr method enables a rendering
    plug-in to control the printing of font glyphs.

    Please refer to DDK documentation for more details.

Arguments:

    pdevobj - pointer to a DEVOBJ structure.
    pUFObj - pointer to a UNIFONTOBJ structure.
    dwType - value indicating the type of glyph specifier array
        pointed to by pGlyph. Valid values are as follows:
        TYPE_GLYPHHANDLE The pGlyph array elements are glyph
            handles of type HGLYPH.
        TYPE_GLYPHID The pGlyph array elements are glyph
            identifiers of type DWORD.
    dwCount - value representing the number of glyph specifiers in
        the array pointed to by pGlyph.
    pGlyph - pointer to an array of glyph specifiers, where the array
        element type is indicated by dwType.

Return Value:

    S_OK The operation succeeded.
    E_FAIL The operation failed.
    E_NOTIMPL The method is not implemented.


--*/

{
    VERBOSE(L"COemUni2::OutputCharStr() entry.");

    UNREFERENCED_PARAMETER(pdevobj);
    UNREFERENCED_PARAMETER(pUFObj);
    UNREFERENCED_PARAMETER(dwType);
    UNREFERENCED_PARAMETER(dwCount);
    UNREFERENCED_PARAMETER(pGlyph);

    return E_NOTIMPL;
}

HRESULT __stdcall
COemUni2::
PublishDriverInterface(
    IUnknown *pIUnknown
    )

/*++

Routine Description:

    Implementation of IPrintOemUni::PublishDriverInterface

    The IPrintOemUni::PublishDriverInterface method allows a
    rendering plug-in for Unidrv to obtain the Unidrv driver's
    IPrintOemDriverUni interface.

    A rendering plug-in for Unidrv MUST implement the
    IPrintOemUni::PublishDriverInterface method and the method
    must return S_OK, or the driver will not call the plug-in's other
    IPrintOemUni interface methods.

    Please refer to DDK documentation for more details.

Arguments:

    pIUnknown - pointer to the IUnknown interface of the driver's
        IPrintOemDriverUni COM interface.

Return Value:

    S_OK The operation succeeded.
    E_FAIL The operation failed.


--*/

{
    VERBOSE(L"COemUni2::PublishDriverInterface() entry.");

    // Need to store pointer to Driver Helper functions, if we already haven't.
    if (this->m_pOEMHelp == NULL)
    {
        HRESULT hResult;

        // Get Interface to Helper Functions.
        hResult = pIUnknown->QueryInterface(IID_IPrintOemDriverUni, (void** ) &(this->m_pOEMHelp));

        if(!SUCCEEDED(hResult))
        {
            // Make sure that interface pointer reflects interface query failure.
            this->m_pOEMHelp = NULL;

            return E_FAIL;
        }
    }

    return S_OK;
}

HRESULT __stdcall
COemUni2::
ResetPDEV(
    PDEVOBJ     pdevobjOld,
    PDEVOBJ     pdevobjNew
    )

/*++

Routine Description:

    Implementation of IPrintOemUni::ResetPDEV

    A rendering plug-in's IPrintOemUni::ResetPDEV method performs
    the same types of operations as the DrvResetPDEV function. During
    the processing of an application's call to the Platform SDK ResetDC
    function, the IPrintOemUni::ResetPDEV method is called by Unidrv's
    DrvResetPDEV function. For more information about when DrvResetPDEV
    is called, see its description in the DDK docs.

    The rendering plug-in's private PDEV structure's address is contained
    in the pdevOEM member of the DEVOBJ structure pointed to by pdevobjOld.
    The IPrintOemUni::ResetPDEV method should use relevant members of this
    old structure to fill in the new structure, which is referenced through pdevobjNew.

    A rendering plug-in for Unidrv MUST implement the
    IPrintOemUni::ResetPDEV method.

    Please refer to DDK documentation for more details.

Arguments:

    pdevobjOld - pointer to a DEVOBJ structure containing current PDEV information.
    pdevobjNew - pointer to a DEVOBJ structure into which the method should place
        new PDEV information.

Return Value:

    S_OK The operation succeeded.
    E_FAIL The operation failed.


--*/

{
    VERBOSE(L"COemUni2::ResetPDEV() entry.");

    UNREFERENCED_PARAMETER(pdevobjOld);
    UNREFERENCED_PARAMETER(pdevobjNew);

    return S_OK;
}

HRESULT __stdcall
COemUni2::
SendFontCmd(
    PDEVOBJ         pdevobj,
    PUNIFONTOBJ     pUFObj,
    PFINVOCATION    pFInv
    )

/*++

Routine Description:

    Implementation of IPrintOemUni::SendFontCmd

    The IPrintOemUni::SendFontCmd method enables a rendering
    plug-in to modify a printer's font selection command and then
    send it to the printer.

    Please refer to DDK documentation for more details.

Arguments:

    pdevobj - pointer to a DEVOBJ structure.
    pUFObj - pointer to a UNIFONTOBJ structure.
    pFInv - pointer to an FINVOCATION structure.

Return Value:

    S_OK The operation succeeded.
    E_FAIL The operation failed.
    E_NOTIMPL The method is not implemented.


--*/

{
    VERBOSE(L"COemUni2::SendFontCmd() entry.");

    UNREFERENCED_PARAMETER(pdevobj);
    UNREFERENCED_PARAMETER(pUFObj);
    UNREFERENCED_PARAMETER(pFInv);

    return E_NOTIMPL;
}

HRESULT __stdcall
COemUni2::
TextOutAsBitmap(
    SURFOBJ         *pso,
    STROBJ          *pstro,
    FONTOBJ         *pfo,
    CLIPOBJ         *pco,
    RECTL           *prclExtra,
    RECTL           *prclOpaque,
    BRUSHOBJ        *pboFore,
    BRUSHOBJ        *pboOpaque,
    POINTL          *pptlOrg,
    MIX             mix
    )

/*++

Routine Description:

    Implementation of IPrintOemUni::TextOutAsBitmap

    The IPrintOemUni::TextOutAsBitmap method allows a rendering
    plug-in to create a bitmap image of a text string, in case a
    downloadable font is not available.

    Please refer to DDK documentation for more details.

Arguments:

    pso - Defines the surface on which to be written.
    pstro - Defines the glyphs to be rendered and their positions
    pfo - Specifies the font to be used
    pco - Defines the clipping path
    prclExtra - A NULL-terminated array of rectangles to be filled
    prclOpaque - Specifies an opaque rectangle
    pboFore - Defines the foreground brush
    pboOpaque - Defines the opaque brush
    pptlOrg - Pointer to POINT struct , defining th origin
    mix - Specifies the foreground and background ROPs for pboFore

Return Value:

    S_OK The operation succeeded.
    E_FAIL The operation failed.
    E_NOTIMPL The method is not implemented.


--*/

{
    VERBOSE(L"COemUni2::TextOutAsBitmap() entry.");

    UNREFERENCED_PARAMETER(pso);
    UNREFERENCED_PARAMETER(pstro);
    UNREFERENCED_PARAMETER(pfo);
    UNREFERENCED_PARAMETER(pco);
    UNREFERENCED_PARAMETER(prclExtra);
    UNREFERENCED_PARAMETER(prclOpaque);
    UNREFERENCED_PARAMETER(pboFore);
    UNREFERENCED_PARAMETER(pboOpaque);
    UNREFERENCED_PARAMETER(pptlOrg);
    UNREFERENCED_PARAMETER(mix);

    return E_NOTIMPL;
}

HRESULT __stdcall
COemUni2::
TTDownloadMethod(
    PDEVOBJ         pdevobj,
    PUNIFONTOBJ     pUFObj,
    OUT DWORD       *pdwResult
    )

/*++

Routine Description:

    Implementation of IPrintOemUni::TTDownloadMethod

    The IPrintOemUni::TTDownloadMethod method enables a rendering
    plug-in to indicate the format that Unidrv should use for a specified
    TrueType soft font.

    Please refer to DDK documentation for more details.

Arguments:

    pdevobj - pointer to a DEVOBJ structure.
    pUFObj - pointer to a UNIFONTOBJ structure.
    pdwResult - Receives one of the following method-supplied constant values:
        TTDOWNLOAD_BITMAP Unidrv should download the specified font as bitmaps.
        TTDOWNLOAD_DONTCARE Unidrv can select the font format.
        TTDOWNLOAD_GRAPHICS Unidrv should print TrueType fonts as graphics,
            instead of downloading the font.
        TTDOWNLOAD_TTOUTLINE Unidrv should download the specified font as outlines.

Return Value:

    S_OK The operation succeeded.
    E_FAIL The operation failed.
    E_NOTIMPL The method is not implemented.


--*/

{
    VERBOSE(L"COemUni2::TTDownloadMethod() entry.");

    UNREFERENCED_PARAMETER(pdevobj);
    UNREFERENCED_PARAMETER(pUFObj);
    UNREFERENCED_PARAMETER(pdwResult);

    return E_NOTIMPL;
}

HRESULT __stdcall
COemUni2::
TTYGetInfo(
    PDEVOBJ     pdevobj,
    DWORD       dwInfoIndex,
    PVOID       pOutputBuf,
    DWORD       dwSize,
    DWORD       *pcbcNeeded
    )

/*++

Routine Description:

    Implementation of IPrintOemUni::TTYGetInfo

    The IPrintOemUni::TTYGetInfo method enables a rendering plug-in
    to supply Unidrv with information relevant to text-only printers.

    Please refer to DDK documentation for more details.

Arguments:

    pdevobj - pointer to a DEVOBJ structure.
    dwInfoIndex - constant identifying the type of information being requested.
        The following constant values are defined:
            OEMTTY_INFO_CODEPAGE - The pOutputBuf parameter points to a
                DWORD in which the method should return the number of the
                code page to be used.
            OEMTTY_INFO_MARGINS - The pOutputBuf parameter points to a
                RECT structure in which the method should return page margin
                widths, in tenths of millimeters (for example, 20 represents 2 mm).
                If the entire page is printable, all margin values must be 0.
            OEMTTY_INFO_NUM_UFMS - The pOutputBuf parameter points to a
                DWORD in which the method should return the number of resource
                IDs of the UFMs for 10, 12, and 17 CPI fonts. To actually obtain
                these resource IDs, perform a query using OEMTTY_INFO_UFM_IDS.
            OEMTTY_INFO_UFM_IDS - The pOutputBuf parameter points to an array
                of DWORDs of sufficient size to hold the number of resource IDs of
                the UFMs for 10, 12, and 17 CPI fonts. (This number is obtained by
                using OEMTTY_INFO_NUM_UFMS in a query.) The method should
                return the resource IDs of the UFMs for 10,12, and 17 CPI fonts.
    pOutputBuf - pointer to a buffer to receive the requested information.
    dwSize - size, in bytes, of the buffer pointed to by pOutputBuf.
    pcbcNeeded - pointer to a location to receive the number of bytes written into
        the buffer pointed to by pOutputBuf. If the number of bytes required is
        smaller than the number specified by dwSize, the method should supply
        the required size and return E_FAIL.

Return Value:

    S_OK The operation succeeded.
    E_FAIL The operation failed.
    E_NOTIMPL The method is not implemented.


--*/

{
    VERBOSE(L"COemUni2::TTYGetInfo() entry.");

    UNREFERENCED_PARAMETER(pdevobj);
    UNREFERENCED_PARAMETER(dwInfoIndex);
    UNREFERENCED_PARAMETER(pOutputBuf);
    UNREFERENCED_PARAMETER(dwSize);
    UNREFERENCED_PARAMETER(pcbcNeeded);

    return E_NOTIMPL;
}

HRESULT __stdcall
COemUni2::
WritePrinter(
    PDEVOBJ     pdevobj,
    PVOID       pBuf,
    DWORD       cbBuffer,
    PDWORD      pcbWritten
    )

/*++

Routine Description:

    Implementation of IPrintOemUni2::WritePrinter

    The IPrintOemUni2::WritePrinter method, if supported, enables a
    rendering plug-in to capture all output data generated by a Unidrv
    driver. If this method is not supported, the output data would
    otherwise be sent to the spooler in a call to the spooler's WritePrinter API.

    Please refer to DDK documentation for more details.

Arguments:

    pdevobj - pointer to a DEVOBJ structure.
    pBuf - pointer to the first byte of an array of bytes that contains
        the output data generated by the Unidrv driver.
    cbBuffer - size, in bytes, of the array pointed to by pBuf.
    pcbWritten - pointer to a DWORD value that receives the number
        of bytes of data that were successfully sent to the plug-in.

Return Value:

    S_OK The operation succeeded.
    E_FAIL The operation failed.
    E_NOTIMPL The method is not implemented.


--*/

{
    VERBOSE(L"COemUni2::WritePrinter() entry.");

    UNREFERENCED_PARAMETER(pdevobj);
    UNREFERENCED_PARAMETER(pBuf);
    UNREFERENCED_PARAMETER(cbBuffer);
    UNREFERENCED_PARAMETER(pcbWritten);

    return E_NOTIMPL;
}


////////////////////////////////////////////////////////////////////////////////
//
// oem class factory
//
class COemCF : public IClassFactory
{
public:
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj);
    
    STDMETHOD_(ULONG,AddRef)(THIS);
    
    // the _At_ tag here tells prefast that once release 
    // is called, the memory should not be considered leaked
    _At_(this, __drv_freesMem(object)) 
    STDMETHOD_(ULONG,Release)(THIS);

    // *** IClassFactory methods ***
    STDMETHOD(CreateInstance)(THIS_
                            LPUNKNOWN pUnkOuter,
                            REFIID riid,
                            LPVOID FAR* ppvObject);
    STDMETHOD(LockServer)(THIS_ BOOL bLock);


    // Constructor
    COemCF(): m_cRef(1) { };

    // Destructor
    ~COemCF() { };

protected:
    LONG m_cRef;

};

///////////////////////////////////////////////////////////
//
// Class factory body
//
HRESULT __stdcall
COemCF::
QueryInterface(
    const IID&  iid,
    void**      ppv)
{
    VERBOSE(L"COemCF::QueryInterface entry.");
    if ((iid == IID_IUnknown) || (iid == IID_IClassFactory))
    {
        *ppv = static_cast<COemCF*>(this);
    }
    else
    {
        *ppv = NULL;
#if DBG && defined(USERMODE_DRIVER)
        TCHAR szOutput[80] = {0};
        StringFromGUID2(iid, szOutput, _countof(szOutput)); // can not fail!
        VERBOSE(TEXT("COemCF::QueryInterface %s not supported.\r\n"), szOutput);
#endif
        return E_NOINTERFACE;
    }
    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

ULONG __stdcall
COemCF::
AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

_At_(this, __drv_freesMem(object)) 
ULONG __stdcall
COemCF::
Release()
{
//  ASSERT( 0 != m_cRef);
    ULONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
    {
        delete this;
    }
    return cRef;
}

// IClassFactory implementation
HRESULT __stdcall
COemCF::
CreateInstance(
    IUnknown*   pUnknownOuter,
    const IID&  iid,
    void**      ppv)
{
    VERBOSE(L"Class factory:Create component.");

    if (ppv == NULL)
    {
        return E_POINTER;
    }
    *ppv = NULL;

    // Cannot aggregate.
    if (pUnknownOuter != NULL)
    {
        return CLASS_E_NOAGGREGATION;
    }

    // Create component.
    COemUni2* pOemCP = new COemUni2;
    if (pOemCP == NULL)
    {
        return E_OUTOFMEMORY;
    }

    // Get the requested interface.
    HRESULT hr = pOemCP->QueryInterface(iid, ppv);

    // Release the IUnknown pointer.
    // (If QueryInterface failed, component will delete itself.)
    pOemCP->Release();
    return hr;
}

// LockServer
HRESULT __stdcall
COemCF::
LockServer(
    BOOL    bLock
    )
{
    if (bLock)
    {
        InterlockedIncrement(&g_cServerLocks);
    }
    else
    {
        InterlockedDecrement(&g_cServerLocks);
    }
    return S_OK;
}


//
// Registration functions
//

//
// Can DLL unload now?
//
STDAPI DllCanUnloadNow(void)
{
    VERBOSE(L"DllCanUnloadNow entry.");

    //
    // To avoid leaving OEM DLL still in memory when Unidrv or Pscript drivers
    // are unloaded, Unidrv and Pscript driver ignore the return value of
    // DllCanUnloadNow of the OEM DLL, and always call FreeLibrary on the OEMDLL.
    //
    // If OEM DLL spins off a working thread that also uses the OEM DLL, the
    // thread needs to call LoadLibrary and FreeLibraryAndExitThread, otherwise
    // it may crash after Unidrv or Pscript calls FreeLibrary.
    //

    if ((g_cComponents == 0) && (g_cServerLocks == 0))
    {
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}

//
// Get class factory
//
STDAPI  DllGetClassObject(
    _In_ REFCLSID clsid, 
    _In_ REFIID iid, 
    _Outptr_ LPVOID* ppv
    )
{
    VERBOSE(L"DllGetClassObject:\tCreate class factory.");

    if (ppv == NULL)
    {
        return E_POINTER;
    }
    *ppv = NULL;

    // Can we create this component?
    if (clsid != CLSID_OEMRENDER)
    {
        ERR(ERRORTEXT("DllGetClassObject:\tClass not available!\r\n"));
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    // Create class factory.
    COemCF* pFontCF = new COemCF;  // Reference count set to 1 in constructor
    if (pFontCF == NULL)
    {
        ERR(ERRORTEXT("DllGetClassObject:\tOut of Memory!\r\n"));
        return E_OUTOFMEMORY;
    }

    // Get requested interface.
    HRESULT hrResult = pFontCF->QueryInterface(iid, ppv);
    pFontCF->Release();

    return hrResult;
}

