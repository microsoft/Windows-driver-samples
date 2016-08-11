/*****************************************************************************
 *
 *  segmentation.cpp
 *
 *  Copyright (c) 2003 Microsoft Corporation.  All Rights Reserved.
 *
 *  DESCRIPTION:
 *
 *  CSegFilter is a simple sample segmentation filter, which works together with
 *  the wiadriver. It supports only three hardcoded regions.
 *
 *******************************************************************************/

#include "stdafx.h"

#include "wiaitem.h"

#define COUNTOF(x) (sizeof(x)/sizeof(x[0]))

typedef struct _FILTER_IMAGE {
    double    XPOS;
    double    YPOS;
    double    XEXTENT;
    double    YEXTENT;
    double    DESKEWX;
    double    DESKEWY;
} FILTER_IMAGE;

static FILTER_IMAGE g_FilterImages[] = { { 0.25,   0.5625, 3.3125, 2.75,  0.0, 0.0 },
                                         { 4.0625, 0.5625, 4.1875, 3.625, 0.0, 0.0 },
                                         { 0.547,  4.66,   7.360,  6.173, 6.347, 1.35 } };

static WCHAR *gszChildNameBase = L"Sub Region #";

// {7B6D704B-A4F2-4ecf-8B86-8E0CF1A707F5}
static const GUID CLSID_WiaSegmentationFilter =
{ 0x7b6d704b, 0xa4f2, 0x4ecf, { 0x8b, 0x86, 0x8e, 0xc, 0xf1, 0xa7, 0x7, 0xf5 } };

static LONG g_cLocks = 0;

void LockModule(void)   { InterlockedIncrement(&g_cLocks); }
void UnlockModule(void) { InterlockedDecrement(&g_cLocks); }

class CSegFilter : public IWiaSegmentationFilter
{
public:

    STDMETHODIMP
    QueryInterface(_In_ const IID& iid_requested, _Out_ void** ppInterfaceOut);

    STDMETHODIMP_(ULONG)
    AddRef(void);

    STDMETHODIMP_(ULONG)
    Release(void);

    STDMETHODIMP
    DetectRegions(
                IN  LONG        lFlags,
        _In_    IN  IStream     *pInputStream,
        _In_    IN  IWiaItem2   *pWiaItem);

    CSegFilter() : m_nRefCount(0) {}

private:

    HRESULT
    GenerateChildNames(
                IN  DWORD       cChildNum,
        _Out_   OUT BSTR        *pbstrChildName);

    LONG        m_nRefCount;
};

///
///  QueryInterface
///
STDMETHODIMP
CSegFilter::QueryInterface(_In_ const IID& iid_requested, _Out_ void** ppInterfaceOut)
{
    HRESULT hr = S_OK;

    hr = ppInterfaceOut ? S_OK : E_POINTER;

    if (SUCCEEDED(hr))
    {
        *ppInterfaceOut = NULL;
    }

    //
    //  We support IID_IUnknown and IID_IWiaSegmentationFilter
    //
    if (SUCCEEDED(hr))
    {
        if (IID_IUnknown == iid_requested)
        {
            *ppInterfaceOut = static_cast<IUnknown*>(this);
        }
        else if (IID_IWiaSegmentationFilter == iid_requested)
        {
            *ppInterfaceOut = static_cast<IWiaSegmentationFilter*>(this);
        }
        else
        {
            hr = E_NOINTERFACE;
        }
    }

    if (SUCCEEDED(hr))
    {
        reinterpret_cast<IUnknown*>(*ppInterfaceOut)->AddRef();
    }

    return hr;
}

///
///  AddRef
///
STDMETHODIMP_(ULONG)
CSegFilter::AddRef(void)
{
    if (m_nRefCount == 0)
    {
        LockModule();
    }

    return InterlockedIncrement(&m_nRefCount);
}

///
///  Release
///
STDMETHODIMP_(ULONG)
CSegFilter::Release(void)
{
    ULONG nRetval = InterlockedDecrement(&m_nRefCount);

    if (0 == nRetval)
    {
        delete this;
        UnlockModule();
    }

    return nRetval;
}

/*****************************************************************************
 *
 *  @doc INTERNAL
 *
 *  @func STDMETHODIMP | CSegFilter::DetectRegions | Creates three child items with hardcoded coordinates
 *
 *  @parm   LONG | lFlags |
 *          Currently unused.
 *
 *  @parm   IStream | pInputStream |
 *          The (preview) image on which to perform segmentation. In this example the regions
 *          are hard-coded so we do not read pInputStream
 *
 *  @parm   IWiaItem2 | pWiaItem |
 *          The item under which to create the new child items.
 *
 *  @comm
 *  Creates three child items with hardcoded coordinates. Notes that it sets deskew
 *  properties for one of the child items. This is only for demostration purposes since
 *  the driver cannot perform deskew.
 *
 *  @rvalue S_OK    |
 *              The function succeeded.
 *  @rvalue E_XXX   |
 *              The function failed
 *
 *****************************************************************************/
STDMETHODIMP
CSegFilter::DetectRegions(
            IN  LONG        lFlags,
    _In_    IN  IStream     *pInputStream,
    _In_    IN  IWiaItem2   *pWiaItem)
{
    UNREFERENCED_PARAMETER(lFlags);

    HRESULT     hr;
    CWiaItem    *pIWiaItemWrapper      = NULL;
    IWiaItem2   *pChildIWiaItem        = NULL;
    LONG        xres_dpi               = 0;
    LONG        yres_dpi               = 0;
    LONG        lItemFlags =  WiaItemTypeGenerated |
                              WiaItemTypeTransfer  |
                              WiaItemTypeImage     |
                              WiaItemTypeFile      |
                              WiaItemTypeProgrammableDataSource;

    hr = pWiaItem ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hr))
    {
        hr = pInputStream ? S_OK : E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        pIWiaItemWrapper = new CWiaItem();

        hr = pIWiaItemWrapper ? S_OK : E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        hr = pIWiaItemWrapper->SetIWiaItem(pWiaItem);

        if (SUCCEEDED(hr))
        {
            hr = pIWiaItemWrapper->ReadPropertyLong(WIA_IPS_XRES,&xres_dpi);
        }

        if (SUCCEEDED(hr))
        {
            hr = pIWiaItemWrapper->ReadPropertyLong(WIA_IPS_YRES,&yres_dpi);
        }
    }

    if (SUCCEEDED(hr))
    {
        BSTR            bstrChildName          = NULL;

        for (DWORD i = 0 ; i < COUNTOF(g_FilterImages) ; i++)
        {
            hr = GenerateChildNames(i, &bstrChildName);

            if (SUCCEEDED(hr))
            {
                hr = pWiaItem->CreateChildItem(lItemFlags, COPY_PARENT_PROPERTY_VALUES, bstrChildName, &pChildIWiaItem);
            }

            if (SUCCEEDED(hr))
            {
                hr = pIWiaItemWrapper->SetIWiaItem(pChildIWiaItem);

                if (SUCCEEDED(hr))
                {
                    hr = pIWiaItemWrapper->WritePropertyLong(WIA_IPS_XPOS, (LONG) (g_FilterImages[i].XPOS * xres_dpi));
                }

                if (SUCCEEDED(hr))
                {
                    hr = pIWiaItemWrapper->WritePropertyLong(WIA_IPS_YPOS, (LONG) (g_FilterImages[i].YPOS * yres_dpi));
                }

                if (SUCCEEDED(hr))
                {
                    hr = pIWiaItemWrapper->WritePropertyLong(WIA_IPS_XEXTENT, (LONG) (g_FilterImages[i].XEXTENT * xres_dpi));
                }

                if (SUCCEEDED(hr))
                {
                    hr = pIWiaItemWrapper->WritePropertyLong(WIA_IPS_YEXTENT, (LONG) (g_FilterImages[i].YEXTENT * yres_dpi));
                }

                if (SUCCEEDED(hr))
                {
                    hr = pIWiaItemWrapper->WritePropertyLong(WIA_IPS_DESKEW_X, (LONG) ((g_FilterImages[i].DESKEWX) * xres_dpi));
                }

                if (SUCCEEDED(hr))
                {
                    hr = pIWiaItemWrapper->WritePropertyLong(WIA_IPS_DESKEW_Y, (LONG) ((g_FilterImages[i].DESKEWY) * yres_dpi));
                }
            }

            if(pChildIWiaItem)
            {
                pChildIWiaItem->Release();
                pChildIWiaItem = NULL;
            }

            SysFreeString(bstrChildName);
            bstrChildName = NULL;
        }
    }

    if (pIWiaItemWrapper)
    {
        delete pIWiaItemWrapper;
    }

    return hr;
}

/*****************************************************************************
 *
 *  @doc INTERNAL
 *
 *  @func STDMETHODIMP | CSegFilter::GenerateChildNames | Generates names for a new child item
 *
 *  @parm   DWORD | cChildNum |
 *          The childs number
 *
 *  @parm   BSTR | pbstrChildName |
 *          On successful return contains the name of the child to be created
 *
 *  @comm
 *  Helper function that generates names for a child item # cChildNum
 *
 *  @rvalue S_OK    |
 *              The function succeeded.
 *  @rvalue E_XXX   |
 *              The function failed
 *
 *****************************************************************************/
HRESULT
CSegFilter::GenerateChildNames(
            IN  DWORD       cChildNum,
    _Out_   OUT BSTR        *pbstrChildName)
{
    HRESULT     hr;
    WCHAR       szChildName[20];

    hr = pbstrChildName ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hr))
    {
        hr = StringCchPrintf(szChildName,
                             COUNTOF(szChildName),
                             L"%ws%u",
                             gszChildNameBase,
                             cChildNum);
    }

    if (SUCCEEDED(hr))
    {
        *pbstrChildName = SysAllocString(szChildName);

        hr = (*pbstrChildName) ? S_OK : E_OUTOFMEMORY;
    }

    return hr;
}

/*****************************************************************************
 *
 *  Class Object
 *
 *******************************************************************************/
class CFilterClass : public IClassFactory
{
public:

    STDMETHODIMP
    QueryInterface(_In_ const IID& iid_requested, _Out_ void** ppInterfaceOut)
    {
        HRESULT hr = S_OK;

        hr = ppInterfaceOut ? S_OK : E_POINTER;

        if (SUCCEEDED(hr))
        {
            *ppInterfaceOut = NULL;
        }

        //
        //  We only support IID_IUnknown and IID_IClassFactory
        //
        if (SUCCEEDED(hr))
        {
            if (IID_IUnknown == iid_requested)
            {
                *ppInterfaceOut = static_cast<IUnknown*>(this);
            }
            else if (IID_IClassFactory == iid_requested)
            {
                *ppInterfaceOut = static_cast<IClassFactory*>(this);
            }
            else
            {
                hr = E_NOINTERFACE;
            }
        }

        if (SUCCEEDED(hr))
        {
            reinterpret_cast<IUnknown*>(*ppInterfaceOut)->AddRef();
        }

        return hr;
    }

    STDMETHODIMP_(ULONG)
    AddRef(void)
    {
        LockModule();
        return 2;
    }

    STDMETHODIMP_(ULONG)
    Release(void)
    {
        UnlockModule();
        return 1;
    }

    STDMETHODIMP
    CreateInstance(_In_     IUnknown *pUnkOuter,
                   _In_     REFIID   riid,
                   _Out_    void     **ppv)
    {
        CSegFilter   *pSegFilter = NULL;
        HRESULT      hr;

        hr = ppv ? S_OK : E_POINTER;

        if (SUCCEEDED(hr))
        {
            *ppv = 0;
        }

        if (SUCCEEDED(hr))
        {
            if (pUnkOuter)
            {
                hr = CLASS_E_NOAGGREGATION;
            }
        }

        if (SUCCEEDED(hr))
        {
            pSegFilter = new CSegFilter();

            hr = pSegFilter ? S_OK : E_OUTOFMEMORY;
        }

        if (SUCCEEDED(hr))
        {
            pSegFilter->AddRef();
            hr = pSegFilter->QueryInterface(riid, ppv);
            pSegFilter->Release();
        }

        return hr;
    }

    STDMETHODIMP
    LockServer(BOOL bLock)
    {
        if (bLock)
        {
            LockModule();
        }
        else
        {
            UnlockModule();
        }

        return S_OK;
    }
};

STDAPI DllCanUnloadNow(void)
{
    return (g_cLocks == 0) ? S_OK : S_FALSE;
}

STDAPI DllGetClassObject(_In_           REFCLSID   rclsid,
                         _In_           REFIID     riid,
                         _Outptr_    void       **ppv)
{
    static  CFilterClass s_FilterClass;

    HRESULT     hr;

    hr = ppv ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hr))
    {
        if (rclsid == CLSID_WiaSegmentationFilter)
        {
            hr = s_FilterClass.QueryInterface(riid, ppv);
        }
        else
        {
            *ppv = 0;
            hr = CLASS_E_CLASSNOTAVAILABLE;
        }
    }

    return hr;
}

STDAPI DllUnregisterServer()
{
    return S_OK;
}

STDAPI DllRegisterServer()
{
    return S_OK;
}


