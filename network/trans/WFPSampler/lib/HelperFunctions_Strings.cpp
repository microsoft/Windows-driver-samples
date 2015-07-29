////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_Strings.cpp
//
//   Abstract:
//      This module contains functions which assist in actions pertaining to strings.
//
//   Naming Convention:
//
//      <Scope><Module><Object><Action><Modifier>
//  
//      i.e.
//
//       <Scope>
//          {
//                    - Function is likely visible to other modules
//          }
//       <Module>
//          {
//            Hlpr    - Function is from HelperFunctions_* Modules.
//          }
//       <Object>
//          {
//            Strings - Function pertains to null terminated wide character strings.
//          }
//       <Action>
//          {
//            Are     - Function compares values.
//          }
//       <Modifier>
//          {
//            Equal   - Function determines equality between values.
//          }
//
//   Private Functions:
//
//   Public Functions:
//      HlprStringsAreEqual(),
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
  @helper_function="HlprStringsAreEqual"
 
   Purpose:  Determine if two strings are identical.                                            <br>
                                                                                                <br>
   Notes:    Function is overloaded.                                                            <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/E0Z9K731.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/CHD90W8E.aspx              <br>
*/
BOOLEAN HlprStringsAreEqual(_In_reads_(stringSizeAlpha) PCWSTR pStringAlpha,
                            _In_ SIZE_T stringSizeAlpha,
                            _In_reads_(stringSizeOmega) PCWSTR pStringOmega,
                            _In_ SIZE_T stringSizeOmega,
                            _In_ BOOLEAN isCaseSensitive)                    /* FALSE */
{
   BOOLEAN areEqual = FALSE;

   if(pStringAlpha &&
      pStringOmega)
   {
      if(stringSizeAlpha != stringSizeOmega)
         HLPR_BAIL;

      if(pStringAlpha == pStringOmega)
         areEqual = TRUE;
      else
      {
         if(stringSizeAlpha != stringSizeOmega)
            HLPR_BAIL;

         if(isCaseSensitive)
         {
            if(wcscmp(pStringAlpha,
                      pStringOmega))
               HLPR_BAIL;
         }
         else
         {
            if(_wcsnicmp(pStringAlpha,
                         pStringOmega,
                         stringSizeAlpha))
               HLPR_BAIL;
         }

         areEqual = TRUE;
      }
   }

   HLPR_BAIL_LABEL:

   return areEqual;
}

/**
 @helper_function="HlprStringsAreEqual"
 
   Purpose:  Determine if two strings are identical.                                            <br>
                                                                                                <br>
   Notes:    Function is overloaded.                                                            <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS647539.aspx              <br>
*/
BOOLEAN HlprStringsAreEqual(_In_opt_ PCWSTR pStringAlpha,
                            _In_opt_ PCWSTR pStringOmega,
                            _In_ BOOLEAN isCaseSensitive) /* FALSE */
{
   BOOLEAN areEqual = FALSE;

   if(pStringAlpha &&
      pStringOmega)
   {
      UINT32 status    = NO_ERROR;
      size_t alphaSize = 0;
      size_t omegaSize = 0;

      status = StringCchLength(pStringAlpha,
                               STRSAFE_MAX_CCH,
                               &alphaSize);
      if(FAILED(status))
      {
         HlprLogError(L"HlprStringsAreEqual : StringCchLength() [status = %#x]",
                      status);

         HLPR_BAIL;
      }
      
      status = StringCchLength(pStringOmega,
                               STRSAFE_MAX_CCH,
                               &omegaSize);
      if(FAILED(status))
      {
         HlprLogError(L"HlprStringsAreEqual : StringCchLength() [status = %#x]",
                      status);

         HLPR_BAIL;
      }

      areEqual = HlprStringsAreEqual(pStringAlpha,
                                     alphaSize,
                                     pStringOmega,
                                     omegaSize,
                                     isCaseSensitive);
   }

   HLPR_BAIL_LABEL:

   return areEqual;
}

/**
 @helper_function="HlprStringsAreEqual"
 
   Purpose:  Determine if two strings are identical.                                            <br>
                                                                                                <br>
   Notes:    Function is overloaded.                                                            <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA380518.aspx              <br>
*/
BOOLEAN HlprStringsAreEqual(_In_ const UNICODE_STRING* pUnicodeStringAlpha,
                            _In_ const UNICODE_STRING* pUnicodeStringOmega,
                            _In_ BOOLEAN isCaseSensitive)                   /* FALSE */
{
   BOOLEAN areEqual = FALSE;

#pragma warning(push)
#pragma warning(disable: 26018) /// constrined by UNICODE_STRING::Length

   if(pUnicodeStringAlpha &&
      pUnicodeStringAlpha->Length &&
      pUnicodeStringOmega &&
      pUnicodeStringOmega->Length)
      areEqual = HlprStringsAreEqual(pUnicodeStringAlpha->Buffer,
                                     pUnicodeStringAlpha->Length,
                                     pUnicodeStringOmega->Buffer,
                                     pUnicodeStringOmega->Length,
                                     isCaseSensitive);

#pragma warning(pop)

   return areEqual;
}
