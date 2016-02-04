
//
// Structure to marshal information for
// postassociation thread.
//
typedef
struct _POST_ASSOC_DATA
{
    HANDLE      hIhvExtAdapter;
    HANDLE      hDot11SvcHandle;
    HANDLE      hSecuritySessionId;
}
POST_ASSOC_DATA, *PPOST_ASSOC_DATA;


// Pre-association when the profile does have the key.
DWORD
WINAPI
DoWepPreAssociate
(
    PADAPTER_DETAILS    pAdapterDetails,
    DWORD*              pdwReasonCode
);


// Pre-association when the profile does not have the key.
DWORD
WINAPI
DoMissingKeyWepPreAssociate
(
    PADAPTER_DETAILS    pAdapterDetails,
    DWORD*              pdwReasonCode
);


// no op function for preassociation when
// ihv is used only for connectivity. in a
// realistic implementation there would probably
// be calls to Dot11ExtNicSpecificExtension
// in this function to prepare the driver for
// additional connectivity settings.
DWORD
WINAPI
DoIhvConnPreAssociate
(
    PADAPTER_DETAILS    pAdapterDetails,
    DWORD*              pdwReasonCode
);


// This function is responsible for the post association
// operations and completing the post association call
// for WEP scenario.
DWORD
WINAPI
DoWepPostAssociate
(
   LPVOID pvPostAssociate
);



