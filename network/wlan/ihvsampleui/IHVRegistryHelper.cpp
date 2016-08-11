//
// Copyright (C) Microsoft Corporation 2005
// IHV UI Extension sample
//

#include "precomp.h"

#define CLSIDSTR_CALLBACK            L"{4A01f9f9-6012-4343-A8C4-10B5DF32672A}" // IHV Ext UI CLSID
#define CLSID_CALLBACK_FRIENDLY_NAME L"Wireless 802.11 IHV Sample Config UI"

#define REGCLSID        L"CLSID"
#define INPROCSERVER32  L"InprocServer32"
#define THREADINGMODEL  L"ThreadingModel"
#define FREETHREADING   L"Both"

#define ARRAY_SIZE(s) (sizeof(s) / sizeof(s[0]))

extern HINSTANCE g_hInst;


typedef HRESULT (APIENTRY *RegisterPageWithPageProc) (
    const GUID *pguidParentPage,
    const GUID *pguidChildPage,
    const LPWSTR pszChildModuleFileName,
    const LPWSTR pszFriendlyName,
    const DWORD dwBehaviorFlags,
    const DWORD dwUserFlags,
    const LPWSTR pszCommandLine);

typedef HRESULT (APIENTRY *UnregisterPageProc) (
    const GUID *pguidPage,
    const BOOL fUnregisterFromCOM);



//
// RegisterServer - Register the COM Server by creating required keys
//

#pragma warning (push)
#pragma warning (disable:6262)

HRESULT 
CRegHelper::RegisterServer ()
{
   HRESULT hr = S_OK;
   wchar_t  wszModule[_MAX_PATH] = {0};  
   DWORD result = 0;
   
   result = GetModuleFileName(g_hInst, wszModule, ARRAY_SIZE(wszModule));

    if (result == 0)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

   wchar_t wszCLSIDKey[MAX_LENGTH] = {0};       // CLSID\\wszCLSID.
   wchar_t wszInprocKey[MAX_LENGTH + 2] = {0};  // CLSID\\InprocServer32

   // get the class ID strings.
   StringCchCopyW(wszCLSIDKey, MAX_LENGTH, REGCLSID);
   StringCchCatW(wszCLSIDKey, MAX_LENGTH, L"\\");
   StringCchCatW(wszCLSIDKey, MAX_LENGTH, CLSIDSTR_CALLBACK);

   // create entries under CLSID.
   // Description
   FAILHR(SetKeyAndValue(wszCLSIDKey, NULL, CLSID_CALLBACK_FRIENDLY_NAME));
   // set the server path.
   FAILHR(SetKeyAndValue(wszCLSIDKey, INPROCSERVER32, wszModule));
   
   // add the threading model information.
   hr = StringCchPrintfW(wszInprocKey, MAX_LENGTH + 2,  L"%s\\%s", wszCLSIDKey, INPROCSERVER32);
   if(FAILED(hr))
   {
   	 hr = S_FALSE;
	 return hr;
   }
   
   FAILHR(SetRegValue(wszInprocKey, THREADINGMODEL, FREETHREADING));

   // register the extension UI wizard page
   HINSTANCE hinstLib = LoadLibrary(TEXT("connect.dll")); 
   if (hinstLib != NULL) 
   {
      
       // get the export function used for registering
       RegisterPageWithPageProc registerPageWithPageProc = 
           (RegisterPageWithPageProc) GetProcAddress(hinstLib, (LPCSTR)("RegisterPageWithPage"));

       if (NULL != registerPageWithPageProc) 
       {
            hr = (registerPageWithPageProc) (NULL,                      // stand alone page (no parent)
                                             &GUID_SAMPLE_IHVUI_CLSID,  // clsid of the extension UI wizard page
                                             NULL,                      // filename already registered through COM
                                             CLSID_CALLBACK_FRIENDLY_NAME,  // friendly name
                                             0x2,                       // allow duplicate instances
                                             0,                         // no user flags
                                             NULL);                     // no command line 
       }

       FreeLibrary(hinstLib); 
   }

   return hr;
}
#pragma warning (pop)

//
// UnRegisterServer - Register the COM Server by creating required keys
//
HRESULT 
CRegHelper::UnregisterServer()
{
    HRESULT hr = S_OK;
    wchar_t wszCLSIDKey[MAX_LENGTH] = {0}; // CLSID\\wszCLSID.


    // get the class ID strings.
    StringCchCopyW(wszCLSIDKey,MAX_LENGTH, REGCLSID);
    StringCchCatW(wszCLSIDKey, MAX_LENGTH,  L"\\");
    StringCchCatW(wszCLSIDKey, MAX_LENGTH,  CLSIDSTR_CALLBACK);
    StringCchCatW(wszCLSIDKey, MAX_LENGTH,  L"\\");
    StringCchCatW(wszCLSIDKey, MAX_LENGTH,  INPROCSERVER32);  


    // delete the sub key of the Class ID key
    FAILHR(DeleteKey(wszCLSIDKey));

    StringCchCopyW(wszCLSIDKey,MAX_LENGTH, REGCLSID);
    StringCchCatW(wszCLSIDKey, MAX_LENGTH,  L"\\");
    StringCchCatW(wszCLSIDKey, MAX_LENGTH,  CLSIDSTR_CALLBACK);

    // delete Class ID key
    FAILHR(DeleteKey(wszCLSIDKey));

    // unregister the extension UI wizard page
    HINSTANCE hinstLib = LoadLibrary(TEXT("connect.dll")); 
    if (hinstLib != NULL) 
    {
    
        // get the export function used for unregistering
        UnregisterPageProc unregisterPageProc = 
            (UnregisterPageProc) GetProcAddress(hinstLib, (LPCSTR)("UnregisterPage"));

        if (NULL != unregisterPageProc) 
        {
             hr = (unregisterPageProc) (&GUID_SAMPLE_IHVUI_CLSID,  // clsid of the extension UI wizard page
                                        FALSE);                    // already unregistered from COM
        }

        FreeLibrary(hinstLib); 
    }

    return hr;   

}

//
//	Set an entry in the registry of the form:
//			HKEY_CLASSES_ROOT\wszKey\wszSubkey = wszValue
//
BOOL 
CRegHelper::SetKeyAndValue(
    const wchar_t* pwszKey,
    const wchar_t* pwszSubkey,
    const wchar_t* pwszValue
    )
{
   HKEY hKey;	 	 		// handle to the new reg key.
   wchar_t wszRegKey[MAX_LENGTH] = {0}; // buffer for the full key name.


   // init the key with the base key name.
   StringCchCopyW(wszRegKey, MAX_LENGTH, pwszKey);
   // append the subkey name (if there is one).
   if (pwszSubkey != NULL)
   {
	  StringCchCatW(wszRegKey, MAX_LENGTH, L"\\");
	  StringCchCatW(wszRegKey, MAX_LENGTH, pwszSubkey);
   }

   // create the registry key.
   if (RegCreateKeyEx(
           HKEY_CLASSES_ROOT, 
           wszRegKey, 
           0, 
           NULL,
           REG_OPTION_NON_VOLATILE, 
           KEY_ALL_ACCESS, 
           NULL,
           &hKey, 
           NULL) == ERROR_SUCCESS)
   {
      // set the value (if there is one).
      if (pwszValue != NULL)
      {
         RegSetValueEx(
            hKey, 
            NULL, 
            0, 
            REG_SZ, 
            (BYTE *)pwszValue,
            (DWORD) ((wcslen(pwszValue) + 1) * sizeof (wchar_t))
            );
   	}
		
      RegCloseKey(hKey);	
		
      return TRUE;
   }	

   return FALSE;	
}

//
// SetRegValue - Open the key, create a new keyword and value pair under it.
//
BOOL 
CRegHelper::SetRegValue(
    const wchar_t* pwszKeyName,
    const wchar_t* pwszKeyword,
    const wchar_t* pwszValue
    )
{
   HKEY hKey; // handle to the new reg key.

   // create the registration key.
   if (RegCreateKeyEx(
           HKEY_CLASSES_ROOT, 
           pwszKeyName, 
           0, 
           NULL,
           REG_OPTION_NON_VOLATILE, 
           KEY_ALL_ACCESS, 
           NULL,
           &hKey, 
           NULL) == ERROR_SUCCESS)
   {
      // set the value (if there is one).
      if (pwszValue != NULL)
      {
         RegSetValueEx(
             hKey, 
             pwszKeyword, 
             0, 
             REG_SZ, 
             (BYTE *)pwszValue, 
             (DWORD) ((wcslen(pwszValue) + 1) * sizeof (wchar_t))
             );
      }

      RegCloseKey(hKey);


      return TRUE;
   }

   return FALSE;
}


//
//	Delete an entry in the registry of the form:
//			HKEY_CLASSES_ROOT\wszKey\wszSubkey = wszValue
//
BOOL 
CRegHelper::DeleteKey(const wchar_t* pwszSubkey)
{
   DWORD result = 0;
   
   if (pwszSubkey != NULL)
   {
      // delete the registry key.
      result = RegDeleteKey(HKEY_CLASSES_ROOT, pwszSubkey);
   }
   else
   {
      return FALSE;
   }

   return ((ERROR_SUCCESS == result)?TRUE:FALSE);
}


