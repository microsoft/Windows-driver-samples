#pragma once

typedef struct {
    CString    strRadioInstanceID;
    bool       fExisting;
    CComPtr<ISampleRadioInstanceInternal> spRadioInstance;
} INSTANCE_LIST_OBJ;

EXTERN_C const IID IID_IMediaRadioManagerNotifySink;

class ATL_NO_VTABLE CSampleRadioManager :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSampleRadioManager, &CLSID_SampleRadioManager>,
    public IConnectionPointContainerImpl<CSampleRadioManager>,
    public IConnectionPointImpl<CSampleRadioManager, &IID_IMediaRadioManagerNotifySink>,
    public IMediaRadioManager,
    public ISampleRadioManagerInternal
{
public:

    DECLARE_CLASSFACTORY()
    DECLARE_NOT_AGGREGATABLE(CSampleRadioManager)

    DECLARE_NO_REGISTRY()

    BEGIN_COM_MAP(CSampleRadioManager)
        COM_INTERFACE_ENTRY(IMediaRadioManager)
        COM_INTERFACE_ENTRY(ISampleRadioManagerInternal)
        COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
    END_COM_MAP()

    BEGIN_CONNECTION_POINT_MAP(CSampleRadioManager)
        CONNECTION_POINT_ENTRY(IID_IMediaRadioManagerNotifySink)
    END_CONNECTION_POINT_MAP()

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    CSampleRadioManager();

    HRESULT FinalConstruct();
    void    FinalRelease();

    // IMediaRadioManager interface
    IFACEMETHOD(GetRadioInstances)(_Out_ IRadioInstanceCollection **ppCollection);
    IFACEMETHOD(OnSystemRadioStateChange)(_In_ SYSTEM_RADIO_STATE sysRadioState, _In_ UINT32 uTimeoutSec);

    // ISampleRadioManagerInternal interface
    IFACEMETHOD(OnInstanceRadioChange)(_In_ BSTR bstrRadioInstanceID, _In_ DEVICE_RADIO_STATE radioState);

private:
    HRESULT _FireEventOnInstanceAdd(_In_ IRadioInstance *pRadioInstance);
    HRESULT _FireEventOnInstanceRemove(_In_ BSTR bstrRadioInstanceID);
    HRESULT _FireEventOnInstanceRadioChange(_In_ BSTR bstrRadioInstanceID, _In_ DEVICE_RADIO_STATE radioState);

    void    _Cleanup();
    HRESULT _AddInstance(_In_ PCWSTR pszKeyName, _Out_opt_ IRadioInstance **ppRadioInstance);
    void    _OnInstanceAddRemove();
    HRESULT _SetSysRadioState(_In_ SYSTEM_RADIO_STATE sysRadioState);
    static DWORD WINAPI s_ThreadRegChange(LPVOID pThis);
    static DWORD WINAPI s_ThreadSetSysRadio(LPVOID pThis);

    CComAutoCriticalSection       _criticalSection;
    CAtlList<INSTANCE_LIST_OBJ *> _listRadioInstances;

    CRegKey  _hKeyRoot;
    CHandle  _hRegChangeEvent;
    CHandle  _hEventThread;
    bool     _fClosing;   // object is closing

    static PCWSTR s_pszRootKey;
};

typedef struct _SET_SYS_RADIO_JOB{
    HRESULT hr;
    CHandle hEvent;
    CSampleRadioManager *pSampleRM;
    SYSTEM_RADIO_STATE  srsTarget;
} SET_SYS_RADIO_JOB;

