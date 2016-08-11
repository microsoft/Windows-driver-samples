//
// Copyright (C) Microsoft Corporation 2005
// IHV UI Extension sample
//

#ifndef _IHVSAMPLEEXTUICON_H_
#define _IHVSAMPLEEXTUICON_H_

class CDot11SampleExtUIConProperty: public IDot11SampleExtUIConProperty
{
public:
    CDot11SampleExtUIConProperty();

    ~CDot11SampleExtUIConProperty();
    
    // IUnknown Implementation
    BEGIN_INTERFACE_TABLE()
        IMPLEMENTS_INTERFACE(IDot11SampleExtUIConProperty)
    END_INTERFACE_TABLE();

    STDMETHODIMP 
    GetDot11ExtUIPropertyFriendlyName(
        BSTR* bstrPropertyName  // IHV friendly name
        );

    //Used to extend property
    STDMETHODIMP 
    DisplayDot11ExtUIProperty(
        HWND hParent, // Parent Window Handle 
        BSTR bstrIHVProfile, // IHV data from the profile 
        PDOT11EXT_IHV_PARAMS pIHVParams, // Select profile MS security settings
        BSTR* bstrModifiedIHVProfile, // modified IHV data to be stored in the profile
        BOOL* pbIsModified // flag to denote if profile was modified
        );

	//Used to get the currently chosen entry to display as selected in the dropdown list
    STDMETHODIMP 
    Dot11ExtUIPropertyGetSelected(
        BSTR bstrIHVProfile, // IHV data from the profile 
        PDOT11EXT_IHV_PARAMS pIHVParams, // Select profile MS security settings
        BOOL* pfIsSelected // flag denoting if this is the selected profile
        );

	//Used to set the current entry as chosen from the dropdown list
    STDMETHODIMP 
    Dot11ExtUIPropertySetSelected(
        BSTR bstrIHVProfile, // IHV data from the profile 
        PDOT11EXT_IHV_PARAMS pIHVParams, // Select profile MS security settings
        BSTR* bstrModifiedIHVProfile, // modified IHV data to be stored in the profile
        BOOL* pbIsModified // flag to denote if profile was modified
        );

    STDMETHODIMP 
    Dot11ExtUIPropertyHasConfigurationUI(BOOL *fHasConfigurationUI);

    //Used to get additional display data (ciphers for auth types)
    STDMETHODIMP 
    Dot11ExtUIPropertyGetDisplayInfo(
        DOT11_EXT_UI_DISPLAY_INFO_TYPE dot11ExtUIDisplayInfoType, // the diapaly type to be described
        BSTR bstrIHVProfile, // IHV data from the profile 
        PDOT11EXT_IHV_PARAMS pIHVParams, // Select profile MS security settings
        ULONG *pcEntries, // number of dependent strings
        ULONG *puDefaultSelection, // the entry in the array to be selected by default
        DOT11_EXT_UI_PROPERTY_DISPLAY_INFO **ppDot11ExtUIProperty // array of returned info structure
        );

    STDMETHODIMP 
    Dot11ExtUIPropertySetDisplayInfo(
        DOT11_EXT_UI_DISPLAY_INFO_TYPE dot11ExtUIDisplayInfoType, // the diapaly type to be modified
        BSTR bstrIHVProfile, // IHV data from the profile 
        PDOT11EXT_IHV_PARAMS pIHVParams, // Select profile MS security settings
        DOT11_EXT_UI_PROPERTY_DISPLAY_INFO *pDot11ExtUIProperty, // selected info structure
        BSTR* bstrModifiedIHVProfile, // modified IHV data to be stored in the profile
        BOOL* pbIsModified // flag to denote if profile was modified
        );

    STDMETHODIMP
    Dot11ExtUIPropertyIsStandardSecurity(
        BOOL *fIsStandardSecurity, // if this interface is a standard auth method
        DOT11_EXT_UI_SECURITY_TYPE *dot11ExtUISecurityType  // which of the standard auth methods it is
        );
    
    // initialize the connection page
    STDMETHODIMP
    Initialize(BSTR bstrPropertyName);

private:
    bool                        m_fInitialized;
    BSTR                        m_bstrFN;
    DOT11_EXT_UI_PROPERTY_TYPE  m_ExtType;
    BOOL                        m_fModified;
};


INT_PTR CALLBACK 
SimpleDialogProcCon(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
);

#endif _IHVSAMPLEEXTUICON_H_
