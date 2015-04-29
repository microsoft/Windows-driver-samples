#pragma once

class ATL_NO_VTABLE CSampleRadioInstance :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSampleRadioInstance>,
    public IRadioInstance,
    public ISampleRadioInstanceInternal
{
public:
    DECLARE_CLASSFACTORY()
    DECLARE_NOT_AGGREGATABLE(CSampleRadioInstance)
    DECLARE_NO_REGISTRY()

    BEGIN_COM_MAP(CSampleRadioInstance)
        COM_INTERFACE_ENTRY(IRadioInstance)
        COM_INTERFACE_ENTRY(ISampleRadioInstanceInternal)
    END_COM_MAP()

    CSampleRadioInstance();

    void FinalRelease();

    static HRESULT CreateInstance(
           _In_  PCWSTR pszKeyName,
           _In_  ISampleRadioManagerInternal *pParentManager,
           _COM_Outptr_ ISampleRadioInstanceInternal **pRadioInstance);

    // IRadioInstance
    IFACEMETHOD(GetRadioManagerSignature)(_Out_ GUID *pguidSignature);
    IFACEMETHOD(GetInstanceSignature)(_Out_ BSTR *pbstrID);
    IFACEMETHOD(GetFriendlyName)(_In_ LCID lcid, _Out_ BSTR *pbstrName);
    IFACEMETHOD(GetRadioState)(_Out_ DEVICE_RADIO_STATE *pRadioState);
    IFACEMETHOD(SetRadioState)(_In_ DEVICE_RADIO_STATE radioState, _In_ UINT32 uTimeoutSec);
    IFACEMETHOD_(BOOL, IsMultiComm)();
    IFACEMETHOD_(BOOL, IsAssociatingDevice)();

    // ISampleRadioInstanceInternal
    IFACEMETHOD(OnSysRadioChange)(_In_ SYSTEM_RADIO_STATE sysRadioSate);

private:
    HRESULT             _Init(_In_ PCWSTR pszKeyName, _In_ ISampleRadioManagerInternal *pParentManager);
    void                _Cleanup();
    void                _OnInstanceUpdate();
    HRESULT             _SetRadioState(_In_ DEVICE_RADIO_STATE radioState);
    static DWORD WINAPI s_ThreadInstanceChange(LPVOID pThis);
    static DWORD WINAPI s_ThreadSetRadio(LPVOID pThis);

    ISampleRadioManagerInternal *_pParentManager;
    CComAutoCriticalSection     _criticalSection;
    CString              _strInstanceId;
    CString              _strInstanceName;
    bool                 _fIsMultiCommDevice;

    CHandle              _hEventThread;
    bool                 _fClosing;   // object is closing
    CHandle              _hRegChangeEvent;
    CRegKey              _hKeyRoot;
    static PCWSTR        s_pszInstanceName;
    static PCWSTR        s_pszInstanceRadioState;
    static PCWSTR        s_pszPreviousRadioState;
    static PCWSTR        s_pszMultiComm;
    static PCWSTR        s_pszAssocatingDevice;
};

typedef struct _SET_DEVICE_RADIO_JOB{
    HRESULT hr;
    CHandle hEvent;
    CSampleRadioInstance *pInstance;
    DEVICE_RADIO_STATE drsTarget;
} SET_DEVICE_RADIO_JOB;

