//
// Copyright (C) Microsoft Corporation 2005
// IHV UI Extension sample
//


typedef enum _IHV_SECURITY_TYPE IHV_SECURITY_TYPE, *PIHV_SECURITY_TYPE;
typedef enum _IHV_AUTH_TYPE IHV_AUTH_TYPE, *PIHV_AUTH_TYPE;
typedef enum _IHV_CIPHER_TYPE IHV_CIPHER_TYPE, *PIHV_CIPHER_TYPE;


HRESULT
Wstr2Bstr
(
    _In_        LPCWSTR     pszSrc,
    _Outptr_ BSTR*       pbstrDest
);

HRESULT
Wstr2Wstr
(
   _In_        LPCWSTR     pszSrc,
   _Outptr_ LPWSTR*     ppszDest
);


HRESULT
Wstr2Dword
(
    IN  LPCWSTR     pszSrc,
    OUT DWORD*      pdwDest
);


HRESULT
Dword2Bstr
(
    IN  DWORD       dwSrc,
    OUT BSTR*       pbstrDest
);


HRESULT
Wstr2Bool
(
    IN  LPCWSTR     pszSrc,
    OUT BOOL*       pbDest
);

HRESULT
Bool2Bstr
(
    IN  BOOL        bSrc,
    OUT BSTR*       pbstrDest
);


HRESULT
Wstr2AuthType
(
    IN  LPCWSTR             pszSrc,
    OUT PIHV_AUTH_TYPE      pAuthType
);

HRESULT
AuthType2Bstr
(
    IN  IHV_AUTH_TYPE       AuthType,
    OUT BSTR*               pbstrDest
);

HRESULT
Wstr2SecurityType
(
    IN  LPCWSTR             pszSrc,
    OUT PIHV_SECURITY_TYPE  pSecurityType
);

HRESULT
SecurityType2Bstr
(
    IN  IHV_SECURITY_TYPE   SecurityType,
    OUT BSTR*               pbstrDest
);

HRESULT
Wstr2CipherType
(
    IN  LPCWSTR             pszSrc,
    OUT PIHV_CIPHER_TYPE    pCipherType
);

HRESULT
CipherType2Bstr
(
    IN  IHV_CIPHER_TYPE     CipherType,
    OUT BSTR*               pbstrDest
);
