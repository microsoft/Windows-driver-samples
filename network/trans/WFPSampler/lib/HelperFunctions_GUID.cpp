////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_GUID.cpp
//
//   Abstract:
//      This module contains functions which assist in actions pertaining to GUIDs.
//
//   Naming Convention:
//
//      <Scope><Module><Object><Action><Modifier>
//  
//      i.e.
//
//       <Scope>
//          {
//                      - Function is likely visible to other modules
//          }
//       <Module>
//          {
//            Hlpr      - Function is from HelperFunctions_* Modules.
//          }
//       <Object>
//          {
//            GUID      - Function pertains to GUID objects.
//          }
//       <Action>
//          {
//            Are       - Function compares values.
//            Create    - Function allocates and fills memory.
//            Destroy   - Function cleans up and frees memory.
//            Is        - Function compares values.
//            Populate  - Function fills memory with values.
//            Purge     - Function cleans up values.
//          }
//       <Modifier>
//          {
//            Equal     - Function determines equality between values.
//            Null      - Function determines if value is Null or empty.
//            String    - Function acts on a null terminated wide character string.
//          }
//
//   Private Functions:
//
//   Public Functions:
//      HlprGUIDsAreEqual(),
//      HlprGUIDCreate(),
//      HlprGUIDCreateString(),
//      HlprGUIDDestroy(),
//      HlprGUIDDestroyString(),
//      HlprGUIDIsNull()
//      HlprGUIDPopulate(),
//      HlprGUIDPurge(),
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

static const GUID NULL_GUID = {0};

/**
 @helper_function="HlprGUIDPurge"
 
   Purpose:  Cleanup a GUID.                                                                    <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
VOID HlprGUIDPurge(_Inout_ GUID* pGUID)
{
   if(pGUID)
      ZeroMemory(pGUID,
                 sizeof(GUID));

   return;
}

/**
 @helper_function="HlprGUIDDestroy"
 
   Purpose:  Cleanup and free a GUID.                                                           <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_At_(*ppGUID, _Post_ _Null_)
VOID HlprGUIDDestroy(_Inout_ GUID** ppGUID)
{
   if(ppGUID)
   {
      if(*ppGUID)
         HlprGUIDPurge(*ppGUID);

      HLPR_DELETE(*ppGUID);
   }

   return;
}

/**
 @helper_function="HlprGUIDPopulate"
 
   Purpose: Populate a GUID with a random value.                                                <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/AA379205.aspx                              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprGUIDPopulate(_Inout_ GUID* pGUID)
{
   UINT32 status = NO_ERROR;

   if(pGUID)
   {
      status = UuidCreate(pGUID);

      if(status != RPC_S_OK &&                 // 0
         status != RPC_S_UUID_LOCAL_ONLY)      // 1824
      {
         // RPC_S_UUID_NO_ADDRESS              // 1739
         HlprLogError(L"HlprGUIDPopulate : UuidCreate() [status: %#x]",
                      status);
      }
      else
        status = NO_ERROR;
   }
   else
   {
      status = ERROR_INVALID_PARAMETER;

      HlprLogError(L"HlprGUIDPopulate() [status: %#x][pGUID: %#p]",
                   status,
                   pGUID);
   }

   return status;
}

/**
 @helper_function="HlprGUIDCreate"
 
   Purpose:  Allocate and populate a GUID with a random value.                                  <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing the allocated memory using
             HlprGUIDDestroy().                                                                 <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/AA379205.aspx                              <br>
*/
_At_(*ppGUID, _Pre_ _Null_)
_When_(return != NO_ERROR, _At_(*ppGUID, _Post_ _Null_))
_When_(return == NO_ERROR, _At_(*ppGUID, _Post_ _Notnull_))
_Success_(return == NO_ERROR)
UINT32 HlprGUIDCreate(_Outptr_ GUID** ppGUID)
{
   UINT32 status = NO_ERROR;

   if(ppGUID)
   {
      HLPR_NEW(*ppGUID,
               GUID);
      HLPR_BAIL_ON_ALLOC_FAILURE(*ppGUID,
                                 status);

      status = HlprGUIDPopulate(*ppGUID);

      HLPR_BAIL_LABEL:

      if(status != NO_ERROR)
      {
         HLPR_DELETE(*ppGUID);
      }
   }
   else
   {
      status = ERROR_INVALID_PARAMETER;

      HlprLogError(L"HlprGUIDCreate() [status: %#x][ppGUID: %#p]",
                   status,
                   ppGUID);
   }

   return status;
}

/**
 @helper_function="HlprGUIDDestroyString"
 
   Purpose:  Cleanup and free a string representing a GUID.                                     <br>
                                                                                                <br>
   Notes:    Use if string was allocated by HlprGUIDCreateString().                             <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/AA378483.aspx                              <br>
*/
_When_(return != NO_ERROR, _At_(*ppGUIDString, _Post_ _Notnull_))
_When_(return == NO_ERROR, _At_(*ppGUIDString, _Post_ _Null_))
_Success_(return == NO_ERROR)
UINT32 HlprGUIDDestroyString(_Inout_ PWSTR* ppGUIDString)
{
   UINT32 status = NO_ERROR;

   if(ppGUIDString &&
      *ppGUIDString)
   {
      status = RpcStringFree(ppGUIDString);
      if(status != RPC_S_OK)
         HlprLogError(L"HlprGUIDDestroyString : RpcStringFree() [status: %#x]",
                      status);
      else
         *ppGUIDString = 0;
   }

   return status;
}

/**
 @helper_function="HlprGUIDCreateString"
 
   Purpose:  Allocate and populate a string representing the provided GUID.                     <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing the allocated memory using 
             HlprGUIDDestroyString().                                                           <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/AA379352.aspx                              <br>
*/
_Success_(return != 0)
PWSTR HlprGUIDCreateString(_In_ const GUID* pGUID)
{
   PWSTR  pGUIDString = 0;

   if(pGUID)
   {
      UINT32 status = NO_ERROR;

      status = UuidToString(pGUID,
                            &pGUIDString);
      if(status != NO_ERROR)
      {
         HlprLogError(L"HlprGUIDCreateString : UuidToString() [status: %#x]",
                      status);
      }
   }

   return pGUIDString;
}

/**
 @helper_function="HlprGUIDsAreEqual"

   Purpose:  Determine if two GUIDs are identical.                                              <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/AA379329.aspx                              <br>
*/
BOOLEAN HlprGUIDsAreEqual(_In_ const GUID* pGUIDAlpha,
                          _In_ const GUID* pGUIDOmega)
{
   RPC_STATUS status   = RPC_S_OK;
   UINT32     areEqual = FALSE;

   if(pGUIDAlpha == 0 ||
      pGUIDOmega == 0)
   {
      if((pGUIDAlpha == 0 &&
         pGUIDOmega) ||
         (pGUIDAlpha &&
         pGUIDOmega == 0))

      HLPR_BAIL;
   }

   if(pGUIDAlpha == 0 &&
      pGUIDOmega == 0)
   {
      areEqual = TRUE;

      HLPR_BAIL;
   }

   areEqual = UuidEqual((UUID*)pGUIDAlpha,
                        (UUID*)pGUIDOmega,
                        &status);
   if(status != RPC_S_OK)
      HlprLogError(L"HlprGUIDsAreEqual : UuidEqual() [status %#x]",
                   status);

   HLPR_BAIL_LABEL:

   return (BOOLEAN)areEqual;
}

/**
 @helper_function="HlprGUIDIsNull"

   Purpose:  Determine if a GUID is a NULL GUID.                                                <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
BOOLEAN HlprGUIDIsNull(_In_ const GUID* pGUID)
{
   return HlprGUIDsAreEqual(pGUID,
                            (GUID*)&NULL_GUID);;
}
