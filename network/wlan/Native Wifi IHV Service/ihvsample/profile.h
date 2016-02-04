//
// Copyright (C) Microsoft Corporation 2005
// IHV UI Extension sample
//

#pragma once

#ifndef _IHVSAMPLEPROFILE_H
#define _IHVSAMPLEPROFILE_H



#define MAX_AUTH_TYPES 3


// IHV Auth types
typedef enum _IHV_AUTH_TYPE
{
    IHVAuthV1, 
    IHVAuthV2,
    IHVAuthV3,
    IHVAuthInvalid
}
IHV_AUTH_TYPE, *PIHV_AUTH_TYPE;


#define MAX_CIPHER_TYPES 4


// IHV cipher types
typedef enum _IHV_CIPHER_TYPE
{
    None,
    IHVCipher1,
    IHVCipher2,
    IHVCipher3,
    IHVCipherInvalid
}
IHV_CIPHER_TYPE, *PIHV_CIPHER_TYPE;





// Ihv connectivity profile data type.
typedef
struct _IHV_CONNECTIVITY_PROFILE
{
    DWORD       dwParam1;
    LPWSTR      pszParam2;
}
IHV_CONNECTIVITY_PROFILE, *PIHV_CONNECTIVITY_PROFILE;






// Ihv security profile data type.
typedef struct _IHV_SECURITY_PROFILE
{
    BOOL                bUseIhvConnectivityOnly;
    BOOL                bUseFullSecurity;
    IHV_AUTH_TYPE       AuthType;
    IHV_CIPHER_TYPE     CipherType;
    DWORD               dwParam1;
    LPWSTR              pszParam2;
}
IHV_SECURITY_PROFILE, *PIHV_SECURITY_PROFILE;



// Converts string to connectivity profile.
DWORD
GetIhvConnectivityProfile
(
    PDOT11EXT_IHV_CONNECTIVITY_PROFILE  pDot11ExtIhvConnProfile,
    PIHV_CONNECTIVITY_PROFILE*          ppConnectivityProfile
);


// free connectivity profile.
VOID
FreeIhvConnectivityProfile
(
    PIHV_CONNECTIVITY_PROFILE*  ppConnectivityProfile
);


// Converts string to security profile.
DWORD
GetIhvSecurityProfile
(
    PDOT11EXT_IHV_SECURITY_PROFILE      pDot11ExtIhvSecProfile,
    PIHV_SECURITY_PROFILE*              ppSecurityProfile
);


// free security profile.
VOID
FreeIhvSecurityProfile
(
    PIHV_SECURITY_PROFILE*      ppSecurityProfile
);


#endif _IHVSAMPLEPROFILE_H

