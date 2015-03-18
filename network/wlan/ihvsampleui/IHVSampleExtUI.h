//
// Copyright (C) Microsoft Corporation 2005
// IHV UI Extension sample
//

#ifndef _IHVSAMPLEEXTUI_H_
#define _IHVSAMPLEEXTUI_H_

#include "precomp.h"

// object ref count
extern long g_objRefCount;

//lock count on server
extern long g_serverLock;

#define IHV_KEY_LENGTH 64

#define MAX_IHV_CIPHERS 6
#define MAX_IHV_AUTHS 6

// IHV Auth types
typedef enum _IHV_AUTH_TYPE {
    IHVAuthV1, 
    IHVAuthV2,
    IHVAuthV3,
    IHVAuthInvalid
} IHV_AUTH_TYPE, *PIHV_AUTH_TYPE;

// IHV cipher types
typedef enum _IHV_CIPHER_TYPE {
    None,
	IHVCipher1,
	IHVCipher2,
	IHVCipher3,
	IHVCipherInvalid
} IHV_CIPHER_TYPE, *PIHV_CIPHER_TYPE;

// structure holding valid ciphers for a given auth
typedef struct _IHV_AUTH_CIPHERS {
    IHV_AUTH_TYPE IHVAuth;
	DWORD dwCipherCount;
    IHV_CIPHER_TYPE IHVCiphers[MAX_IHV_CIPHERS];
} IHV_AUTH_CIPHERS, *PIHV_AUTH_CIPHERS;

// structure for all auths and corresponding ciphers
typedef struct _IHV_AUTH_CIPHER_CAPABILITY {
     DWORD dwAuthCount;
	 IHV_AUTH_CIPHERS IhvAuthCiphers[MAX_IHV_AUTHS];
} IHV_AUTH_CIPHER_CAPABILITY, *PIHV_AUTH_CIPHER_CAPABILITY;

typedef struct _IHV_SECURITY_CONFIG {
    IHV_AUTH_TYPE Auth;
	IHV_CIPHER_TYPE Cipher;
} IHV_SECURITY_CONFIG, *PIHV_SECURITY_CONFIG;

extern IHV_AUTH_CIPHER_CAPABILITY g_IHVOneXExtCapability; 

extern LPWSTR g_IHVAuthFriendlyName[];
extern LPWSTR g_IHVCipherFriendlyName[];


#define PROP_COUNT_CONNECTION   1

#define PROP_COUNT_SECURITY     2
#define PROP_COUNT_SEC_CIPHERS  2
typedef enum _IHV_SECURITY_TYPE {
    IHVSecurityV1, 
    IHVSecurityV2,
    IHVSecurityInvalid
} IHV_SECURITY_TYPE, *PIHV_SECURITY_TYPE;
static LPWSTR wstrSecurityTypes[] = { L"IHV Security v1", L"IHV Security v2" };

#define PROP_COUNT_KEYEXTENSION 3
static LPWSTR wstrAuthArray[] = { L"IHVAuth Open-with-1X", L"IHVAuth v1", L"IHVAuth v2" };




#define IHV_CIPHER_COUNT 3
static LPWSTR wstrCipherArray[] = { L"None", L"IHVCipher v1", L"IHVCipher v2" };
static LPWSTR wstr1XCipherArray[] = { L"IHVCipher WEP"};
static BSTR bstrCipherArray[IHV_CIPHER_COUNT] = {0};

typedef struct _IHV_CIPHERS_FOR_AUTH_INFO
{
    DOT11_EXT_UI_PROPERTY_DISPLAY_INFO displayInfo[IHV_CIPHER_COUNT];
} IHV_CIPHERS_FOR_AUTH_INFO;
static DOT11_EXT_UI_PROPERTY_DISPLAY_INFO cipherOne = {1, DOT11_EXT_UI_DISPLAY_INFO_CIPHER, 0};
static DOT11_EXT_UI_PROPERTY_DISPLAY_INFO cipherTwo = {2, DOT11_EXT_UI_DISPLAY_INFO_CIPHER, 0};
static DOT11_EXT_UI_PROPERTY_DISPLAY_INFO cipherThree = {3, DOT11_EXT_UI_DISPLAY_INFO_CIPHER, 0};
static DOT11_EXT_UI_PROPERTY_DISPLAY_INFO ciphersInfoArray[MAX_IHV_AUTHS][MAX_IHV_CIPHERS] = {0}; 


//////// structures for balloon /////////

struct IHV_UI_REQUEST
{
    char title[80];
    char help[80];

    IHV_UI_REQUEST()
    {
        memset(this, 0, sizeof(IHV_UI_REQUEST));
    }
};

struct IHV_UI_RESPONSE
{
    char key[100];
    int num[150];

    IHV_UI_RESPONSE()
    {
        memset(this, 0, sizeof(IHV_UI_RESPONSE));
    }
};


class CDot11SampleExtUI: public IDot11SampleExtUI, public IWizardExtension, public IObjectWithSite
{
public:
    CDot11SampleExtUI();

    ~CDot11SampleExtUI();
    
    // IUnknown Implementation
    BEGIN_INTERFACE_TABLE()
        IMPLEMENTS_INTERFACE(IDot11ExtUI)
        IMPLEMENTS_INTERFACE(IDot11SampleExtUI)
        IMPLEMENTS_INTERFACE(IWizardExtension)
        IMPLEMENTS_INTERFACE(IObjectWithSite)
    END_INTERFACE_TABLE();

    // Used to get the IHV friendly name
    STDMETHODIMP
    GetDot11ExtUIFriendlyName(BSTR* bstrFriendlyName);

    // Used to display an IHV specific connection page
    STDMETHODIMP
    GetDot11ExtUIProperties(
        DOT11_EXT_UI_PROPERTY_TYPE ExtType,
        ULONG *pcExtensions,
        IDot11ExtUIProperty **ppDot11ExtUIProperty
        );

    STDMETHODIMP
    GetDot11ExtUIBalloonText(
	    BSTR pIHVUIRequest, // the UI request structure from IHV
	    BSTR* pwszBalloonText // the balloon text to be displayed
	    );

    HRESULT
    CreateConnectionProperties(
        ULONG *pcExtensions,
        IDot11ExtUIProperty **ppDot11ExtUIProperty
        );
    
    HRESULT
    CreateSecurityProperties(
        ULONG *pcExtensions,
        IDot11ExtUIProperty **ppDot11ExtUIProperty
        );

    HRESULT
    CreateKeyProperties(
        ULONG *pcExtensions,
        IDot11ExtUIProperty **ppDot11ExtUIProperty
        );

    // IObjectWithSite
    STDMETHOD (SetSite) (
        IUnknown* pUnkSite
        );

    STDMETHOD (GetSite) (
        REFIID riid, 
        void** ppvSite
        );

    //IWizardExtension
    STDMETHOD (AddPages) (
        HPROPSHEETPAGE* aPages, 
        UINT cPages, 
        UINT *pnPagesAdded
        );

    STDMETHOD (GetFirstPage) (
        HPROPSHEETPAGE *phpage
        );

    STDMETHOD (GetLastPage) (
        HPROPSHEETPAGE *phpage
        );

private:
    HRESULT FinalConstruct();
    void FinalRelease();
    static BOOL CALLBACK GetKeyDlgProc (
        HWND hwndDlg,  
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam 
        );

        
    static BOOL CALLBACK HelpDlgProc (
        HWND hwndDlg,  
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam 
        );

    static BOOL CALLBACK LastPageDlgProc (
        HWND hwndDlg,  
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam 
        );

    HRESULT GetClsidPropertyName (
        _In_ const CLSID* pCLSID, 
        _In_opt_ PCWSTR pwszPropertyName,
        _Out_writes_(maxResultLen) PWSTR pResultStr,
        _In_ UINT  maxResultLen);

private:
    IHV_UI_REQUEST*         m_pUIRequest;
    IHV_UI_RESPONSE         m_UIResponse;
    
    IUnknown*				m_pUnkSite;
    
    HPROPSHEETPAGE			m_hFirstPagePsp;
    HPROPSHEETPAGE			m_hLastPagePsp;
};


#endif _IHVSAMPLEEXTUI_H_
