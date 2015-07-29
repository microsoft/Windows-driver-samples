/////////////////////////////////////////////////////////////////////////////
//
// Sauron.h : Declaration of the CSauron
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __SAURON_H_
#define __SAURON_H_

#include "resource.h"
#include "effects.h"
#include "luminous.h"

// preset values
enum {
    PRESET_BARS = 0,
    PRESET_FLASH,
    PRESET_COUNT
};

/////////////////////////////////////////////////////////////////////////////
// CSauron
class ATL_NO_VTABLE CSauron :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CSauron, &CLSID_Sauron>,
    public IDispatchImpl<ISauron, &IID_ISauron, &LIBID_SAURONLib>,
    public IWMPEffects
{
private:
    COLORREF    m_clrForeground;    // foreground color
    LONG        m_nPreset;          // current preset
    CLuminous     *m_Luminous;           // light control

    HRESULT WzToColor(const WCHAR *pwszColor, COLORREF *pcrColor);
    HRESULT ColorToWz( BSTR* pbstrColor, COLORREF crColor);
    DWORD SwapBytes(DWORD dwRet);

public:
    CSauron();
    ~CSauron();

DECLARE_REGISTRY_RESOURCEID(IDR_SAURON)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSauron)
    COM_INTERFACE_ENTRY(ISauron)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IWMPEffects)
END_COM_MAP()

public:

    // CComCoClass Overrides
    HRESULT FinalConstruct();
    void FinalRelease();

    // ISauron
    STDMETHOD(get_foregroundColor)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_foregroundColor)(/*[in]*/ BSTR newVal);

    // IEffects
    STDMETHOD(Render)(TimedLevel *pLevels, HDC hdc, RECT *rc);
    STDMETHOD(MediaInfo)(LONG lChannelCount, LONG lSampleRate, BSTR bstrTitle);
    STDMETHOD(GetCapabilities)(DWORD * pdwCapabilities);
    STDMETHOD(GoFullscreen)(BOOL /* fFullScreen */) { return E_NOTIMPL; };
    STDMETHOD(RenderFullScreen)(TimedLevel * /* pLevels */) { return E_NOTIMPL; };
    STDMETHOD(DisplayPropertyPage)(HWND /* hwndOwner */) { return E_NOTIMPL; };
    STDMETHOD(GetTitle)(BSTR *bstrTitle);
    STDMETHOD(GetPresetTitle)(LONG nPreset, BSTR *bstrPresetTitle);
    STDMETHOD(GetPresetCount)(LONG *pnPresetCount);
    STDMETHOD(SetCurrentPreset)(LONG nPreset);
    STDMETHOD(GetCurrentPreset)(LONG *pnPreset);
};

#endif //__SAURON_H_
