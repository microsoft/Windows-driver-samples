////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_SID.cpp
//
//   Abstract:
//      This module contains functions which assist in actions pertaining to security identifiers
//         (SIDs).
//
//   Naming Convention:
//
//      <Scope><Module><Object><Action><Modifier>
//  
//      i.e.
//
//       <Scope>
//          {
//                           - Function is likely visible to other modules
//            Prv            - Function is private to this module.
//          }
//       <Module>
//          {
//            Hlpr           - Function is from HelperFunctions_* Modules.
//          }
//       <Object>
//          {
//            SID            - Function pertains to SID objects.
//          }
//       <Action>
//          {
//            Create         - Function allocates and fills memory.
//            Destroy        - Function cleans up and frees memory.
//            Get            - Function retrieves requested data.
//          }
//       <Modifier>
//          {
//            Current        - Function acts on current object.
//            ForCurrentUser - Function acts on behalf of current user.
//            WellKnown      - Function acts on Windows defined entities.
//          }
//
//   Private Functions:
//      PrvHlprUserNameGetCurrent
//
//   Public Functions:
//      HlprSIDDestroy(),
//      HlprSIDCreate(),
//      HlprSIDGetForCurrentUser()
//      HlprSIDGetWellKnown()
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "HelperFunctions_Include.h" /// .

/**
 @private_function="PrvHlprUserNameGetCurrent"
 
   Purpose:  Allocate memory and return a string representation of the current user name.       <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing any allocated memory using 
             HLPR_DELETE_ARRAY()                                                                <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS724432.aspx              <br>
*/
_Success_(return != 0)
PWSTR PrvHlprUserNameGetCurrent()
{
   UINT32 status    = ERROR_GEN_FAILURE;
   PWSTR  pUserName = 0;

   for(UINT32 nameLength = 0;
       status != NO_ERROR;
      )
   {
      if(nameLength)
      {
         HLPR_NEW_ARRAY(pUserName,
                        WCHAR,
                        nameLength);
         HLPR_BAIL_ON_ALLOC_FAILURE(pUserName,
                                    status);
      }

      if(!GetUserName(pUserName,
                      (DWORD*)&nameLength))
      {
         status = GetLastError();

         if(status != ERROR_INSUFFICIENT_BUFFER ||
            nameLength == 0)
         {
            HlprLogError(L"PrvHlprUserNameGetCurrent : GetUserName() [status: %#x]",
                         status);

            HLPR_BAIL;
         }

         HLPR_DELETE_ARRAY(pUserName);
      }

      if(pUserName &&
         nameLength)
      {
         pUserName[nameLength - 1] = '\0'; /// Ensure NULL Termination

         status = NO_ERROR;

         break;
      }
   }

   HLPR_BAIL_LABEL:

   if(status != NO_ERROR)
   {
      HLPR_DELETE_ARRAY(pUserName);
   }

   return pUserName;
}

/**
 @helper_function="HlprSIDDestroy"
 
   Purpose:  Free an allocated SID.                                                   <br>
                                                                                      <br>
   Notes:                                                                             <br>
                                                                                      <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/aa379594.aspx                    <br>
*/
_At_(*ppSID, _Pre_ _Maybenull_)
_At_(*ppSID, _Post_ _Null_)
_Success_(*ppSID == 0)
VOID HlprSIDDestroy(_Inout_ SID** ppSID)
{
   if(ppSID)
   {
      HLPR_DELETE_ARRAY(*ppSID);
   }

   return;
}

/**
 @helper_function="HlprSIDCreate"

   Purpose:  Allocate memory and populate with a SID for either the specified account name and 
             type (SID_NAME_USE), or a well-known SID based on the type (WELL_KNOWN_SID_TYPE).  <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing any allocated memory using HlprSIDDestroy(). <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/AA379594.aspx                              <br> 
*/
_At_(*ppSID, _Pre_ _Null_)
_When_(return != NO_ERROR, _At_(*ppSID, _Post_ _Null_))
_When_(return == NO_ERROR, _At_(*ppSID, _Post_ _Notnull_))
_Success_(return == NO_ERROR)
UINT32 HlprSIDCreate(_Outptr_result_bytebuffer_(*pSIDSize) SID** ppSID,
                     _Out_ SIZE_T* pSIDSize,
                     _In_opt_ PCWSTR pAccountName,                      /* 0 */
                     _In_ UINT32 type)                                  /* WinNullSid */
{
   UINT32 status = NO_ERROR;

   if(ppSID &&
      pSIDSize)
   {
      PWSTR        pDomainName    = 0;
      SIZE_T       accountSIDSize = 0;
      SIZE_T       domainNameSize = 0;
      const UINT32 NUM_TRIES      = 2;

      for(UINT32 index = 0;
          index < NUM_TRIES;
          index++)
      {
         BYTE* pByteBuffer = 0;

         if(pAccountName)
         {
            if(!LookupAccountName(0,
                                  pAccountName,
                                  *ppSID,
                                  (DWORD*)&accountSIDSize,
                                  pDomainName,
                                  (DWORD*)&domainNameSize,
                                  (PSID_NAME_USE)(&type)))
            {
               status = GetLastError();

               if(status != ERROR_INSUFFICIENT_BUFFER ||
                  accountSIDSize == 0 ||
                  domainNameSize == 0)
               {
                  HlprLogError(L"HlprSIDCreate : LookupAccountName [status: %#x][accountSIDSize: %d][domainNameSize: %d]",
                               status,
                               accountSIDSize,
                               domainNameSize);

                  HLPR_BAIL;
               }
               else
                  status = NO_ERROR;

               HLPR_NEW_ARRAY(pDomainName,
                              WCHAR,
                              domainNameSize);
               HLPR_BAIL_ON_ALLOC_FAILURE(pDomainName,
                                          status);
            }
            else
               break;
         }
         else
         {
            if(!CreateWellKnownSid((WELL_KNOWN_SID_TYPE)type,
                                   0,
                                   *ppSID,
                                   (DWORD*)&accountSIDSize))
            {
               status = GetLastError();

               if(status != ERROR_INSUFFICIENT_BUFFER ||
                  accountSIDSize == 0)
               {
                  HlprLogError(L"HlprSIDCreate : CreateWellKnownSid() [status: %#x][accountSIDSize: %d]",
                               status,
                               accountSIDSize);

                  HLPR_BAIL;
               }
               else
                  status = NO_ERROR;
            }
            else
               break;
         }

         if(index)
         {
            HLPR_DELETE_ARRAY(pDomainName);
         }
         else
         {
            HLPR_NEW_ARRAY(pByteBuffer,
                           BYTE,
                           accountSIDSize);
            HLPR_BAIL_ON_ALLOC_FAILURE(pByteBuffer,
                                       status);

            *ppSID = (SID*)pByteBuffer;

            *pSIDSize = accountSIDSize;
         }
      }

      if(*ppSID == 0 ||
         *pSIDSize == 0 ||
         !IsValidSid(*ppSID))
      {
         status = ERROR_INVALID_SID;
      
         HlprLogError(L"HlprSIDCreate : IsValidSid() [status: %#x]",
                      status);

         HLPR_BAIL;
      }

      status = NO_ERROR;

      HLPR_BAIL_LABEL:

      if(status != NO_ERROR)
      {
#pragma warning(push)
#pragma warning(disable: 26000) /// no possible overflow

         HlprSIDDestroy(ppSID);

#pragma warning(pop)

         *pSIDSize = 0;
      }
   }
   else
   {
      status = ERROR_INVALID_PARAMETER;

      HlprLogError(L"HlprSIDCreate() [status: %#x][ppSID: %#p][pSIDSize: %#p]",
                   status,
                   ppSID,
                   pSIDSize);
   }

   return status;
}

/**
 @helper_function="HlprSIDGetForCurrentUser"

   Purpose:  Allocate memory and populate with a SID for the current running user.              <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing any allocated memory using HlprSIDDestroy(). <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/AA379594.aspx                              <br> 
*/
_At_(*ppSID, _Pre_ _Null_)
_When_(return != NO_ERROR, _At_(*ppSID, _Post_ _Null_))
_When_(return == NO_ERROR, _At_(*ppSID, _Post_ _Notnull_))
_Success_(return == NO_ERROR)
UINT32 HlprSIDGetForCurrentUser(_Outptr_result_bytebuffer_(*pSIDSize) SID** ppSID,
                                _Inout_ SIZE_T* pSIDSize)
{
   UINT32 status = NO_ERROR;

   if(ppSID &&
      pSIDSize)
   {
      PWSTR pUserName = 0;

      pUserName = PrvHlprUserNameGetCurrent();
      HLPR_BAIL_ON_NULL_POINTER_WITH_STATUS(pUserName,
                                            status);

      status = HlprSIDCreate(ppSID,
                             pSIDSize,
                             pUserName,
                             SidTypeUser);
      HLPR_BAIL_ON_FAILURE(status);

      HLPR_BAIL_LABEL:

      if(status != NO_ERROR)
      {
         HlprSIDDestroy(ppSID);

         *pSIDSize = 0;
      }

      HLPR_DELETE_ARRAY(pUserName);
   }
   else
   {
      status = ERROR_INVALID_PARAMETER;

      HlprLogError(L"HlprSIDGetForCurrentUser() [status: %#x][ppSID: %#p][pSIDSize: %#p]",
                   status,
                   ppSID,
                   pSIDSize);
   }

   return status;
}

/**
 @helper_function="HlprSIDGetWellKnown"

   Purpose:  Allocate memory and populate with a SID of the Well Known type provided.           <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing any allocated memory using HlprSIDDestroy(). <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/AA446585.aspx                              <br> 
             HTTP://MSDN.Microsoft.com/En-US/Library/AA379151.aspx                              <br> 
*/
_At_(*ppSID, _Pre_ _Null_)
_When_(return != NO_ERROR, _At_(*ppSID, _Post_ _Null_))
_When_(return == NO_ERROR, _At_(*ppSID, _Post_ _Notnull_))
_Success_(return == NO_ERROR)
UINT32 HlprSIDGetWellKnown(_In_ WELL_KNOWN_SID_TYPE sidType,
                           _Outptr_result_bytebuffer_(*pSIDSize) SID** ppSID,
                           _Inout_ UINT32* pSIDSize)
{
   UINT32 status = NO_ERROR;

   if(ppSID &&
      pSIDSize)
   {
      UINT32       sidSize   = 0;
      const UINT32 NUM_TRIES = 2;

      for(UINT32 index = 0;
          index < NUM_TRIES;
          index++)
      {
         BYTE* pBuffer = 0;

         if(!CreateWellKnownSid(sidType,
                                0,
                                *ppSID,
                                (DWORD*)&sidSize))
         {
            status = GetLastError();

            if(status != ERROR_INSUFFICIENT_BUFFER ||
               sidSize == 0)
            {
               HlprLogError(L"HlprSIDGetWellKnown : CreateWellKnownSid() [status: %#x][sidSize: %d]",
                            status,
                            sidSize);

               HLPR_BAIL;
            }
            else
               status = NO_ERROR;

            if(index == 0)
            {
               HLPR_NEW_ARRAY(pBuffer,
                              BYTE,
                              sidSize);
               HLPR_BAIL_ON_ALLOC_FAILURE(pBuffer,
                                          status);

               *ppSID = (SID*)pBuffer;

               *pSIDSize = sidSize;
            }
         }
         else
            break;
      }

      if(*ppSID == 0 ||
         *pSIDSize == 0 ||
         !IsValidSid(*ppSID))
      {
         status = ERROR_INVALID_SID;

         HlprLogError(L"HlprSIDGetWellKnown : IsValidSid() [status: %#x]",
              status);

         HLPR_BAIL;
      }

      status = NO_ERROR;

      HLPR_BAIL_LABEL:

      if(status != NO_ERROR)
      {
#pragma warning(push)
#pragma warning(disable: 26000) /// no possible overflow

         HlprSIDDestroy(ppSID);

#pragma warning(pop)

         *pSIDSize = 0;
      }
   }
   else
   {
      status = ERROR_INVALID_PARAMETER;

      HlprLogError(L"HlprSIDGetWellKnown() [status: %#x][ppSID: %#p][pSIDSize: %#p]",
                   status,
                   ppSID,
                   pSIDSize);
   }

   return status;
}

