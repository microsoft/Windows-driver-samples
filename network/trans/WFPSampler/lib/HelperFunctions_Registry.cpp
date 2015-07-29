////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation. All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_Registry.cpp
//
//   Abstract:
//      This module contains functions which simplify registry operations.
//
//   Private Functions:
//
//   Public Functions:
//      HlprRegistryDeleteKey(),
//      HlprRegistryDeleteValue(),
//      HlprRegistryGetValue(),
//      HlprRegistrySetValue(),
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2012  -     1.0   -  Creation
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "HelperFunctions_Include.h" /// .

/**
 @helper_function="HlprRegistryDeleteValue"
 
   Purpose: Delete the Value from the specified Registry key.                                   <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-Us/Library/Windows/Desktop/MS724851.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprRegistryDeleteValue(_In_ HKEY hKey,
                               _In_opt_ PWSTR pSubKey,
                               _In_ PWSTR pValueName)
{
   UINT32 status    = NO_ERROR;
   HKEY   keyHandle = 0;

   if(pSubKey)
   {
      UINT32 dispositionValue = 0;

      status = RegCreateKeyEx(hKey,
                              pSubKey,
                              0,
                              0,
                              REG_OPTION_NON_VOLATILE,
                              KEY_WRITE,
                              0,
                              &keyHandle,
                              (LPDWORD)&dispositionValue);
      if(status != NO_ERROR)
      {
         HlprLogError(L"HlprRegistryDeleteValue : RegCreateKeyEx() [status: %#x][subKey: %s]",
                      status,
                      pSubKey);

         HLPR_BAIL;
      }
   }
   else
      keyHandle = hKey;

   status = RegDeleteValue(keyHandle,
                           pValueName);
   if(status != NO_ERROR)
   {
      if(status != ERROR_FILE_NOT_FOUND)
      {
         HlprLogError(L"HlprRegistryDeleteValue : RegDeleteValue() [status: %#x][Value: %s%s%s]",
                      status,
                      pSubKey ? pSubKey : L"",
                      pSubKey ? L"\\" : L"",
                      pValueName);

         HLPR_BAIL;
      }

      status = NO_ERROR;
   }

   HLPR_BAIL_LABEL:

   return status;
}


/**
 @helper_function="HlprRegistrySetValue"
 
   Purpose: Opens a handle to the key, creates the appropriate value, and closes the key.       <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-Us/Library/Windows/Desktop/MS724844.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-Us/Library/Windows/Desktop/MS724923.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprRegistrySetValue(_In_ HKEY hKey,
                            _In_ PWSTR pSubKey,
                            _In_ UINT32 type,
                            _In_opt_ PWSTR pValueName,
                            _In_reads_bytes_(valueSize) BYTE* pValue,
                            _In_ UINT32 valueSize)
{
   UINT32 status           = NO_ERROR;
   HKEY   keyHandle        = 0;
   UINT32 dispositionValue = 0;

   status = RegCreateKeyEx(hKey,
                           pSubKey,
                           0,
                           0,
                           REG_OPTION_NON_VOLATILE,
                           KEY_WRITE,
                           0,
                           &keyHandle,
                           (LPDWORD)&dispositionValue);
   if(status != NO_ERROR)
   {
      HlprLogError(L"HlprRegistrySetValue : RegCreateKeyEx() [status: %#x][subKey: %s]",
                   status,
                   pSubKey);

      HLPR_BAIL;
   }

   status = RegSetValueEx(keyHandle,
                          pValueName,
                          0,
                          type,
                          pValue,
                          valueSize);
   if(status != NO_ERROR)
   {
      HlprLogError(L"HlprRegistrySetValue : RegSetValueEx() [status: %#x][valueName: %s]",
                   status,
                   pValueName ? pValueName : L"(Default)");

      HLPR_BAIL;
   }

   HLPR_BAIL_LABEL:

   RegCloseKey(keyHandle);

   return status;
};

/**
 @helper_function="HlprRegistryGetValue"
 
   Purpose: Retrieves a value from the registry.                                                <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-Us/Library/Windows/Desktop/MS724868.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprRegistryGetValue(_In_ HKEY hKey,
                            _In_ PWSTR pSubKey,
                            _In_opt_ PWSTR pValueName,
                            _Inout_ REGISTRY_VALUE* pRegValue)
{
   UINT32 status = NO_ERROR;

   status = RegGetValue(hKey,
                        pSubKey,
                        pValueName,
                        RRF_RT_ANY,
                        (LPDWORD)&(pRegValue->type),
                        (PVOID)pRegValue->pBuffer,
                        (LPDWORD)&(pRegValue->size));
   if(status != NO_ERROR)
   {
      HlprLogError(L"HlprRegistryGetValue : RegGetValue() [status: %#x][Value: %s\\%s]",
                   status,
                   pSubKey,
                   pValueName ? pValueName : L"(Default)");

      HLPR_BAIL_ON_FAILURE(status);
   }

   HLPR_BAIL_LABEL:

   return status;
} 

/**
 @helper_function="HlprRegistryDeleteKey"
 
   Purpose: Delete the SubKey from the Registry.                                                <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-Us/Library/Windows/Desktop/MS724845.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprRegistryDeleteKey(_In_ HKEY hKey,
                             _In_ PWSTR pSubKey)
{
   UINT32 status = NO_ERROR;

   status = RegDeleteKey(hKey,
                         pSubKey);
   if(status != NO_ERROR)
   {
      if(status != ERROR_FILE_NOT_FOUND)
      {
         HlprLogError(L"HlprRegistryDeleteKey : RegDeleteKey() [status: %#x][subKey: %s]",
                      status,
                      pSubKey);

         HLPR_BAIL;
      }

      status = NO_ERROR;
   }

   HLPR_BAIL_LABEL:

   return status;
}
