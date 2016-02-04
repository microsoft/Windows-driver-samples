//
// Copyright (C) Microsoft Corporation 2005
// IHV UI Extension sample
//

#pragma once

#define FAILHR(result) \
                if (result == FALSE) \
                {\
                    hr = S_FALSE;\
                    return hr;\
                }


#define MAX_LENGTH 256


class CRegHelper
{
 public:
   static HRESULT STDMETHODCALLTYPE RegisterServer();
   static HRESULT STDMETHODCALLTYPE UnregisterServer();

private:
   static BOOL SetKeyAndValue(
       const wchar_t *pszKey, 
       const wchar_t *pszSubkey, 
       const wchar_t *pszValue 
       );
   
   static BOOL DeleteKey(const wchar_t *pszSubkey);
   
   static BOOL SetRegValue(
       const wchar_t *pszKeyName, 
       const wchar_t *pszKeyword,
       const wchar_t *pszValue 
       );
};

