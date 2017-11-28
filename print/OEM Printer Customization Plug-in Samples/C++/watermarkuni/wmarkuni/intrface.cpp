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

#include "precomp.h"

#include "wmarkuni.h"
#include "debug.h"
#include "intrface.h"
#include "name.h"

// indicate to prefast that this is a user-mode component.
_Analysis_mode_(_Analysis_code_type_user_driver_);

////////////////////////////////////////////////////////
//      Internal Globals
////////////////////////////////////////////////////////

static long g_cComponents = 0;     // Count of active components
static long g_cServerLocks = 0;    // Count of locks

////////////////////////////////////////////////////////
//      Internal Constants
////////////////////////////////////////////////////////

///////////////////////////////////////////////////////
// Warning: the following array order must match the
//          order in enum ENUMHOOKS.
///////////////////////////////////////////////////////

static const DRVFN OEMHookFuncs[] =
{
    { INDEX_DrvRealizeBrush,        (PFN) OEMRealizeBrush        },
    { INDEX_DrvDitherColor,         (PFN) OEMDitherColor         },
    { INDEX_DrvCopyBits,            (PFN) OEMCopyBits            },
    { INDEX_DrvBitBlt,              (PFN) OEMBitBlt              },
    { INDEX_DrvStretchBlt,          (PFN) OEMStretchBlt          },
    { INDEX_DrvTextOut,             (PFN) OEMTextOut             },
    { INDEX_DrvStrokePath,          (PFN) OEMStrokePath          },
    { INDEX_DrvFillPath,            (PFN) OEMFillPath            },
    { INDEX_DrvStrokeAndFillPath,   (PFN) OEMStrokeAndFillPath   },
    { INDEX_DrvPaint,               (PFN) OEMPaint               },
    { INDEX_DrvLineTo,              (PFN) OEMLineTo              },
    { INDEX_DrvStartPage,           (PFN) OEMStartPage           },
    { INDEX_DrvSendPage,            (PFN) OEMSendPage            },
    { INDEX_DrvEscape,              (PFN) OEMEscape              },
    { INDEX_DrvStartDoc,            (PFN) OEMStartDoc            },
    { INDEX_DrvEndDoc,              (PFN) OEMEndDoc              },
    { INDEX_DrvNextBand,            (PFN) OEMNextBand            },
    { INDEX_DrvStartBanding,        (PFN) OEMStartBanding        },
    { INDEX_DrvQueryFont,           (PFN) OEMQueryFont           },
    { INDEX_DrvQueryFontTree,       (PFN) OEMQueryFontTree       },
    { INDEX_DrvQueryFontData,       (PFN) OEMQueryFontData       },
    { INDEX_DrvQueryAdvanceWidths,  (PFN) OEMQueryAdvanceWidths  },
    { INDEX_DrvFontManagement,      (PFN) OEMFontManagement      },
    { INDEX_DrvGetGlyphMode,        (PFN) OEMGetGlyphMode        },
    { INDEX_DrvStretchBltROP,       (PFN) OEMStretchBltROP       },
    { INDEX_DrvPlgBlt,              (PFN) OEMPlgBlt              },
    { INDEX_DrvTransparentBlt,      (PFN) OEMTransparentBlt      },
    { INDEX_DrvAlphaBlend,          (PFN) OEMAlphaBlend          },
    { INDEX_DrvGradientFill,        (PFN) OEMGradientFill        },
};

////////////////////////////////////////////////////////////////////////////////
//
// IWmarkUni body
//
IWmarkUni::~IWmarkUni()
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


HRESULT __stdcall IWmarkUni::QueryInterface(const IID& iid, void** ppv)
{
    if (iid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown*>(this);
        VERBOSE(DLLTEXT("IWmarkUni::QueryInterface IUnknown.\r\n"));
    }
    else if (iid == IID_IPrintOemUni)
    {
        *ppv = static_cast<IPrintOemUni*>(this);
        VERBOSE(DLLTEXT("IWmarkUni::QueryInterface IPrintOemUni.\r\n"));
    }
    else
    {
        *ppv = NULL;
        VERBOSE(DLLTEXT("IWmarkUni::QueryInterface interface not supported.\r\n"));
        return E_NOINTERFACE;
    }
    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

ULONG __stdcall IWmarkUni::AddRef()
{
    VERBOSE(DLLTEXT("IWmarkUni::AddRef() entry.\r\n"));
    return InterlockedIncrement(&m_cRef);
}

_At_(this, __drv_freesMem(object)) 
ULONG __stdcall IWmarkUni::Release()
{
   VERBOSE(DLLTEXT("IWmarkUni::Release() entry.\r\n"));
   ASSERT( 0 != m_cRef);
   ULONG cRef = InterlockedDecrement(&m_cRef);
   if (0 == cRef)
   {
      delete this;

   }
   return cRef;
}


HRESULT __stdcall IWmarkUni::GetInfo (
    DWORD   dwMode,
    PVOID   pBuffer,
    DWORD   cbSize,
    PDWORD  pcbNeeded)
{
    VERBOSE(DLLTEXT("IWmarkUni::GetInfo entry.\r\n"));

    // Validate parameters.
    if( (NULL == pcbNeeded)
        ||
        ( (OEMGI_GETSIGNATURE != dwMode)
          &&
          (OEMGI_GETVERSION != dwMode)
          &&
          (OEMGI_GETPUBLISHERINFO != dwMode)
        )
      )
    {
        WARNING(DLLTEXT("IWmarkUni::GetInfo() exit pcbNeeded is NULL! ERROR_INVALID_PARAMETER.\r\n"));
        SetLastError(ERROR_INVALID_PARAMETER);
        return E_FAIL;
    }

    // Set expected buffer size.
    if(OEMGI_GETPUBLISHERINFO != dwMode)
    {
        *pcbNeeded = sizeof(DWORD);
    }
    else
    {
        *pcbNeeded = sizeof(PUBLISHERINFO);
        return E_FAIL;
    }

    // Check buffer size is sufficient.
    if((cbSize < *pcbNeeded) || (NULL == pBuffer))
    {
        VERBOSE(DLLTEXT("IWmarkUni::GetInfo() exit insufficient buffer!\r\n"));
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
        case OEMGI_GETPUBLISHERINFO:
        default:
            // Set written bytes to zero since nothing was written.
            WARNING(DLLTEXT("IWmarkUni::GetInfo() exit mode not supported.\r\n"));
            *pcbNeeded = 0;
            SetLastError(ERROR_NOT_SUPPORTED);
            return E_FAIL;
    }

    VERBOSE(DLLTEXT("IWmarkUni::GetInfo() exit S_OK.\r\n"));

    return S_OK;
}

HRESULT __stdcall IWmarkUni::PublishDriverInterface(
    IUnknown *pIUnknown)
{
    VERBOSE(DLLTEXT("IWmarkUni::PublishDriverInterface() entry.\r\n"));

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


HRESULT __stdcall IWmarkUni::EnableDriver(
    DWORD          dwDriverVersion,
    DWORD          cbSize,
    PDRVENABLEDATA pded)
{
    VERBOSE(DLLTEXT("IWmarkUni::EnableDriver() entry.\r\n"));

    UNREFERENCED_PARAMETER(dwDriverVersion);
    UNREFERENCED_PARAMETER(cbSize);

    // List DDI functions that are hooked.
    pded->iDriverVersion =  PRINTER_OEMINTF_VERSION;
    pded->c = sizeof(OEMHookFuncs) / sizeof(DRVFN);
    pded->pdrvfn = (DRVFN *) OEMHookFuncs;

    // Even if nothing is done, need to return S_OK so
    // that DisableDriver() will be called, which releases
    // the reference to the Printer Driver's interface.
    // If error occurs, return E_FAIL.
    return S_OK;
}

HRESULT __stdcall IWmarkUni::DisableDriver(VOID)
{
    VERBOSE(DLLTEXT("IWmarkUni::DisaleDriver() entry.\r\n"));

    // Release reference to Printer Driver's interface.
    if (this->m_pOEMHelp)
    {
        this->m_pOEMHelp->Release();
        this->m_pOEMHelp = NULL;
    }

    return S_OK;
}

HRESULT __stdcall IWmarkUni::DisablePDEV(
    PDEVOBJ         pdevobj)
{
    VERBOSE(DLLTEXT("IWmarkUni::DisablePDEV() entry.\r\n"));

    assert(NULL != pdevobj->pdevOEM);
    delete pdevobj->pdevOEM;

    return S_OK;
};

HRESULT __stdcall IWmarkUni::EnablePDEV(
    PDEVOBJ         pdevobj,
    _In_ PWSTR      pPrinterName,
    ULONG           cPatterns,
    HSURF          *phsurfPatterns,
    ULONG           cjGdiInfo,
    GDIINFO        *pGdiInfo,
    ULONG           cjDevInfo,
    DEVINFO        *pDevInfo,
    DRVENABLEDATA  *pded,
    OUT PDEVOEM    *pDevOem)
{
    UNREFERENCED_PARAMETER(pdevobj);
    UNREFERENCED_PARAMETER(pPrinterName);
    UNREFERENCED_PARAMETER(cPatterns);
    UNREFERENCED_PARAMETER(phsurfPatterns);
    UNREFERENCED_PARAMETER(cjGdiInfo);
    UNREFERENCED_PARAMETER(pGdiInfo);
    UNREFERENCED_PARAMETER(cjDevInfo);
    UNREFERENCED_PARAMETER(pDevInfo);

    VERBOSE(DLLTEXT("IWmarkUni::EnablePDEV() entry.\r\n"));

    POEMPDEV    poempdev;
    INT         i, j;
    DWORD       dwDDIIndex;
    PDRVFN      pdrvfn;

    //
    // Allocate the OEMDev
    //
    poempdev = new OEMPDEV;
    if (NULL == poempdev)
    {
        return NULL;
    }

    //
    // Fill in OEMDEV
    //

    for (i = 0; i < MAX_DDI_HOOKS; i++)
    {
        //
        // search through Unidrv's hooks and locate the function ptr
        //
        dwDDIIndex = OEMHookFuncs[i].iFunc;
        for (j = pded->c, pdrvfn = pded->pdrvfn; j > 0; j--, pdrvfn++)
        {
            if (dwDDIIndex == pdrvfn->iFunc)
            {
                poempdev->pfnUnidrv[i] = pdrvfn->pfn;
                break;
            }
        }
        if (j == 0)
        {
            //
            // Didn't find the hook. This could mean Unidrv doesn't hook this DDI but allows OEMs to hook it out.
            //
            poempdev->pfnUnidrv[i] = NULL;
        }

    }

    *pDevOem = (POEMPDEV) poempdev;

    return (NULL != *pDevOem ? S_OK : E_FAIL);
}


HRESULT __stdcall IWmarkUni::ResetPDEV(
    PDEVOBJ         pdevobjOld,
    PDEVOBJ        pdevobjNew)
{
    VERBOSE(DLLTEXT("IWmarkUni::ResetPDEV() entry.\r\n"));

    UNREFERENCED_PARAMETER(pdevobjOld);
    UNREFERENCED_PARAMETER(pdevobjNew);

    return S_OK;
}


HRESULT __stdcall IWmarkUni::DevMode(
    DWORD       dwMode,
    POEMDMPARAM pOemDMParam)
{
    VERBOSE(DLLTEXT("IWmarkUni:DevMode entry.\n"));
    return hrOEMDevMode(dwMode, pOemDMParam);
}

HRESULT __stdcall IWmarkUni::GetImplementedMethod(_In_ PSTR pMethodName)
{
    HRESULT Result = S_FALSE;


    VERBOSE(DLLTEXT("IWmarkUni::GetImplementedMethod() entry.\r\n"));

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
            else if (!strcmp(NAME_Compression, pMethodName))
            {
                Result = S_FALSE;
            }
            break;

        case 'D':
            if (!strcmp(NAME_DownloadFontHeader, pMethodName))
            {
                Result = S_FALSE;
            }
            else if (!strcmp(NAME_DownloadCharGlyph, pMethodName))
            {
                Result = S_FALSE;
            }
            break;

        case 'F':
            if (!strcmp(NAME_FilterGraphics, pMethodName))
            {
                Result = S_OK;
            }
            break;

        case 'H':
            if (!strcmp(NAME_HalftonePattern, pMethodName))
            {
                Result = S_FALSE;
            }
            break;

        case 'I':
            if (!strcmp(NAME_ImageProcessing, pMethodName))
            {
                Result = S_OK;
            }
            break;

        case 'M':
            if (!strcmp(NAME_MemoryUsage, pMethodName))
            {
                Result = S_FALSE;
            }
            break;

        case 'O':
            if (!strcmp(NAME_OutputCharStr, pMethodName))
            {
                Result = S_FALSE;
            }
            break;

        case 'S':
            if (!strcmp(NAME_SendFontCmd, pMethodName))
            {
                Result = S_FALSE;
            }
            break;

        case 'T':
            if (!strcmp(NAME_TextOutAsBitmap, pMethodName))
            {
                Result = S_FALSE;
            }
            else if (!strcmp(NAME_TTDownloadMethod, pMethodName))
            {
                Result = S_FALSE;
            }
            else if (!strcmp(NAME_TTYGetInfo, pMethodName))
            {
                Result = S_FALSE;
            }
            break;

        case 'W':
            if(!strcmp(NAME_WritePrinter, pMethodName))
            {
                Result = S_FALSE;
            }
            break;
    }

    return Result;
}

HRESULT __stdcall IWmarkUni::CommandCallback(
    PDEVOBJ     pdevobj,
    DWORD       dwCallbackID,
    DWORD       dwCount,
    PDWORD      pdwParams,
    OUT INT     *piResult)
{
    VERBOSE(DLLTEXT("IWmarkUni::CommandCallback() entry.\r\n"));

    UNREFERENCED_PARAMETER(pdevobj);
    UNREFERENCED_PARAMETER(dwCallbackID);
    UNREFERENCED_PARAMETER(dwCount);
    UNREFERENCED_PARAMETER(pdwParams);

    *piResult = 0;

    return S_OK;
}

HRESULT __stdcall IWmarkUni::ImageProcessing(
    PDEVOBJ             pdevobj,
    PBYTE               pSrcBitmap,
    PBITMAPINFOHEADER   pBitmapInfoHeader,
    PBYTE               pColorTable,
    DWORD               dwCallbackID,
    PIPPARAMS           pIPParams,
    OUT PBYTE           *ppbResult)
{
    VERBOSE(DLLTEXT("IWmarkUni::ImageProcessing() entry.\r\n"));

    UNREFERENCED_PARAMETER(pdevobj);
    UNREFERENCED_PARAMETER(pSrcBitmap);
    UNREFERENCED_PARAMETER(pBitmapInfoHeader);
    UNREFERENCED_PARAMETER(pColorTable);
    UNREFERENCED_PARAMETER(dwCallbackID);
    UNREFERENCED_PARAMETER(pIPParams);
    UNREFERENCED_PARAMETER(ppbResult);

    return S_OK;
}

HRESULT __stdcall IWmarkUni::FilterGraphics(
    PDEVOBJ     pdevobj,
    PBYTE       pBuf,
    DWORD       dwLen)
{
    DWORD dwResult;
    VERBOSE(DLLTEXT("IWmarkUni::FilterGraphis() entry.\r\n"));
    m_pOEMHelp->DrvWriteSpoolBuf(pdevobj, pBuf, dwLen, &dwResult);

    if (dwResult == dwLen)
        return S_OK;
    else
        return E_FAIL;
}

HRESULT __stdcall IWmarkUni::Compression(
    PDEVOBJ     pdevobj,
    PBYTE       pInBuf,
    PBYTE       pOutBuf,
    DWORD       dwInLen,
    DWORD       dwOutLen,
    OUT INT     *piResult)
{
    VERBOSE(DLLTEXT("IWmarkUni::Compression() entry.\r\n"));

    UNREFERENCED_PARAMETER(pdevobj);
    UNREFERENCED_PARAMETER(pInBuf);
    UNREFERENCED_PARAMETER(pOutBuf);
    UNREFERENCED_PARAMETER(dwInLen);
    UNREFERENCED_PARAMETER(dwOutLen);
    UNREFERENCED_PARAMETER(piResult);

    // When implemented, the return from GetImplementedMethod
    // for this method name must be S_OK.

    return E_NOTIMPL;
}


HRESULT __stdcall IWmarkUni::HalftonePattern(
    PDEVOBJ     pdevobj,
    PBYTE       pHTPattern,
    DWORD       dwHTPatternX,
    DWORD       dwHTPatternY,
    DWORD       dwHTNumPatterns,
    DWORD       dwCallbackID,
    PBYTE       pResource,
    DWORD       dwResourceSize)
{
    VERBOSE(DLLTEXT("IWmarkUni::HalftonePattern() entry.\r\n"));

    UNREFERENCED_PARAMETER(pdevobj);
    UNREFERENCED_PARAMETER(pHTPattern);
    UNREFERENCED_PARAMETER(dwHTPatternX);
    UNREFERENCED_PARAMETER(dwHTPatternY);
    UNREFERENCED_PARAMETER(dwHTNumPatterns);
    UNREFERENCED_PARAMETER(dwCallbackID);
    UNREFERENCED_PARAMETER(pResource);
    UNREFERENCED_PARAMETER(dwResourceSize);

    // When implemented, the return from GetImplementedMethod
    // for this method name must be S_OK.

    return E_NOTIMPL;
}

HRESULT __stdcall IWmarkUni::MemoryUsage(
    PDEVOBJ         pdevobj,
    POEMMEMORYUSAGE pMemoryUsage)
{
    VERBOSE(DLLTEXT("IWmarkUni::MemoryUsage() entry.\r\n"));

    UNREFERENCED_PARAMETER(pdevobj);
    UNREFERENCED_PARAMETER(pMemoryUsage);

    // When implemented, the return from GetImplementedMethod
    // for this method name must be S_OK.

    return E_NOTIMPL;
}

HRESULT __stdcall IWmarkUni::DownloadFontHeader(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    OUT DWORD   *pdwResult)
{
    VERBOSE(DLLTEXT("IWmarkUni::DownloadFontHeader() entry.\r\n"));

    UNREFERENCED_PARAMETER(pdevobj);
    UNREFERENCED_PARAMETER(pUFObj);
    UNREFERENCED_PARAMETER(pdwResult);

    // When implemented, the return from GetImplementedMethod
    // for this method name must be S_OK.

    return E_NOTIMPL;
}

HRESULT __stdcall IWmarkUni::DownloadCharGlyph(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    HGLYPH      hGlyph,
    PDWORD      pdwWidth,
    OUT DWORD   *pdwResult)
{
    VERBOSE(DLLTEXT("IWmarkUni::DownloadCharGlyph() entry.\r\n"));

    UNREFERENCED_PARAMETER(pdevobj);
    UNREFERENCED_PARAMETER(pUFObj);
    UNREFERENCED_PARAMETER(hGlyph);
    UNREFERENCED_PARAMETER(pdwWidth);
    UNREFERENCED_PARAMETER(pdwResult);

    // When implemented, the return from GetImplementedMethod
    // for this method name must be S_OK.

    return E_NOTIMPL;
}

HRESULT __stdcall IWmarkUni::TTDownloadMethod(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    OUT DWORD   *pdwResult)
{
    VERBOSE(DLLTEXT("IWmarkUni::TTDownloadMethod() entry.\r\n"));

    UNREFERENCED_PARAMETER(pdevobj);
    UNREFERENCED_PARAMETER(pUFObj);
    UNREFERENCED_PARAMETER(pdwResult);

    // When implemented, the return from GetImplementedMethod
    // for this method name must be S_OK.

    return E_NOTIMPL;
}

HRESULT __stdcall IWmarkUni::OutputCharStr(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    DWORD       dwType,
    DWORD       dwCount,
    PVOID       pGlyph)
{
    VERBOSE(DLLTEXT("IWmarkUni::OutputCharStr() entry.\r\n"));

    UNREFERENCED_PARAMETER(pdevobj);
    UNREFERENCED_PARAMETER(pUFObj);
    UNREFERENCED_PARAMETER(dwType);
    UNREFERENCED_PARAMETER(dwCount);
    UNREFERENCED_PARAMETER(pGlyph);

    // When implemented, the return from GetImplementedMethod
    // for this method name must be S_OK.

    return E_NOTIMPL;
}

HRESULT __stdcall IWmarkUni::SendFontCmd(
    PDEVOBJ      pdevobj,
    PUNIFONTOBJ  pUFObj,
    PFINVOCATION pFInv)
{
    VERBOSE(DLLTEXT("IWmarkUni::SendFontCmd() entry.\r\n"));

    UNREFERENCED_PARAMETER(pdevobj);
    UNREFERENCED_PARAMETER(pUFObj);
    UNREFERENCED_PARAMETER(pFInv);

    // When implemented, the return from GetImplementedMethod
    // for this method name must be S_OK.

    return E_NOTIMPL;
}

HRESULT __stdcall IWmarkUni::DriverDMS(
    PVOID   pDevObj,
    PVOID   pBuffer,
    DWORD   cbSize,
    PDWORD  pcbNeeded)
{
    VERBOSE(DLLTEXT("IWmarkUni::DriverDMS() entry.\r\n"));

    UNREFERENCED_PARAMETER(pDevObj);
    UNREFERENCED_PARAMETER(pBuffer);
    UNREFERENCED_PARAMETER(cbSize);
    UNREFERENCED_PARAMETER(pcbNeeded);

    return E_NOTIMPL;
}

HRESULT __stdcall IWmarkUni::TextOutAsBitmap(
    SURFOBJ    *pso,
    STROBJ     *pstro,
    FONTOBJ    *pfo,
    CLIPOBJ    *pco,
    RECTL      *prclExtra,
    RECTL      *prclOpaque,
    BRUSHOBJ   *pboFore,
    BRUSHOBJ   *pboOpaque,
    POINTL     *pptlOrg,
    MIX         mix)
{
    VERBOSE(DLLTEXT("IWmarkUni::TextOutAsBitmap() entry.\r\n"));

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

    // When implemented, the return from GetImplementedMethod
    // for this method name must be S_OK.

    return E_NOTIMPL;
}

HRESULT __stdcall IWmarkUni::TTYGetInfo(
    PDEVOBJ     pdevobj,
    DWORD       dwInfoIndex,
    PVOID       pOutputBuf,
    DWORD       dwSize,
    DWORD       *pcbcNeeded)
{
    VERBOSE(DLLTEXT("IWmarkUni::TTYGetInfo() entry.\r\n"));

    UNREFERENCED_PARAMETER(pdevobj);
    UNREFERENCED_PARAMETER(dwInfoIndex);
    UNREFERENCED_PARAMETER(pOutputBuf);
    UNREFERENCED_PARAMETER(dwSize);
    UNREFERENCED_PARAMETER(pcbcNeeded);

    // When implemented, the return from GetImplementedMethod
    // for this method name must be S_OK.

    return E_NOTIMPL;
}


////////////////////////////////////////////////////////////////////////////////
//
// oem class factory
//
class IOemCF : public IClassFactory
{
public:
    // *** IUnknown methods ***

    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);

    STDMETHOD_(ULONG,AddRef)  (THIS);

    // the _At_ tag here tells prefast that once release 
    // is called, the memory should not be considered leaked
    _At_(this, __drv_freesMem(object)) 
    STDMETHOD_(ULONG,Release) (THIS);

    // *** IClassFactory methods ***
    STDMETHOD(CreateInstance) (THIS_
                               LPUNKNOWN pUnkOuter,
                               REFIID riid,
                               LPVOID FAR* ppvObject);
    STDMETHOD(LockServer)     (THIS_ BOOL bLock);


    // Constructor
    IOemCF(): m_cRef(1) { };
    ~IOemCF() { };

protected:
    LONG m_cRef;

};

///////////////////////////////////////////////////////////
//
// Class factory body
//
HRESULT __stdcall IOemCF::QueryInterface(const IID& iid, void** ppv)
{
    if ((iid == IID_IUnknown) || (iid == IID_IClassFactory))
    {
        *ppv = static_cast<IOemCF*>(this);
    }
    else
    {
        *ppv = NULL;
        VERBOSE(DLLTEXT("IOemCF::QueryInterface interface not supported.\r\n"));
        return E_NOINTERFACE;
    }
    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

ULONG __stdcall IOemCF::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

_At_(this, __drv_freesMem(object)) 
ULONG __stdcall IOemCF::Release()
{
   ASSERT( 0 != m_cRef);
   ULONG cRef = InterlockedDecrement(&m_cRef);
   if (0 == cRef)
   {
      delete this;

   }
   return cRef;
}

// IClassFactory implementation
HRESULT __stdcall IOemCF::CreateInstance(IUnknown* pUnknownOuter,
                                           const IID& iid,
                                           void** ppv)
{
    //VERBOSE(DLLTEXT("Class factory:\t\tCreate component."));

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
    IWmarkUni* pOemCP = new IWmarkUni;
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
HRESULT __stdcall IOemCF::LockServer(BOOL bLock)
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
STDAPI DllCanUnloadNow()
{
    //
    // To avoid leaving OEM DLL still in memory when Unidrv or Pscript drivers
    // are unloaded, Unidrv and Pscript driver ignore the return value of
    // DllCanUnloadNow of the OEM DLL, and always call FreeLibrary on the OEMDLL.
    //
    // If OEM DLL spins off a working thread that also uses the OEM DLL, the
    // thread needs to call LoadLibrary and FreeLibraryAndExitThread, otherwise
    // it may crash after Unidrv or Pscript calls FreeLibrary.
    //

    VERBOSE(DLLTEXT("DllCanUnloadNow entered.\r\n"));

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
    _Outptr_ LPVOID* ppv)
{
    VERBOSE(DLLTEXT("DllGetClassObject:\tCreate class factory.\r\n"));

    if (ppv == NULL)
    {
        return E_POINTER;
    }
    *ppv = NULL;

    // Can we create this component?
    if (clsid != CLSID_OEMRENDER)
    {
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    // Create class factory.
    IOemCF* pFontCF = new IOemCF;  // Reference count set to 1
                                         // in constructor
    if (pFontCF == NULL)
    {
        return E_OUTOFMEMORY;
    }

    // Get requested interface.
    HRESULT hr = pFontCF->QueryInterface(iid, ppv);
    pFontCF->Release();

    return hr;
}

