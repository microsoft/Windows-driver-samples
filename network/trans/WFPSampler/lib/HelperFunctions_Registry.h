////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation. All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_Registry.h
//
//   Abstract:
//      This module contains prototypes for functions which simplif registry operations
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

#ifndef HELPERFUNCTIONS_REGISTRY_H
#define HELPERFUNCTIONS_REGISTRY_H

typedef struct REGISTRY_VALUE_
{
   UINT32 type;
   UINT32 size;
   union
   {
      UINT32 dword;
      UINT64 qword;
      BYTE   pBuffer[1024];
   };
}REGISTRY_VALUE,*PREGISTRY_VALUE;

_Success_(return == NO_ERROR)
UINT32 HlprRegistryDeleteValue(_In_ HKEY hKey,
                               _In_opt_ PWSTR pSubKey,
                               _In_ PWSTR pValueName);

_Success_(return == NO_ERROR)
UINT32 HlprRegistrySetValue(_In_ HKEY hKey,
                            _In_ PWSTR pSubKey,
                            _In_ UINT32 type,
                            _In_opt_ PWSTR pValueName,
                            _In_reads_bytes_(valueSize) BYTE* pValue,
                            _In_ UINT32 valueSize);

_Success_(return == NO_ERROR)
UINT32 HlprRegistryGetValue(_In_ HKEY hKey,
                            _In_ PWSTR pSubKey,
                            _In_opt_ PWSTR pValueName,
                            _Inout_ REGISTRY_VALUE* pRegValue);

_Success_(return == NO_ERROR)
UINT32 HlprRegistryDeleteKey(_In_ HKEY hKey,
                             _In_ PWSTR pSubKey);

#endif /// HELPERFUNCTIONS_REGISTRY_H