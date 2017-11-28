//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1998 - 2005  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:   printschema.cxx
//    
//  PURPOSE:  Constant definitions for print schema names.
//
//  

#include "precomp.hxx"
#include "printschema.hxx"
 
// This indicates to Prefast that this is a usermode driver file.
_Analysis_mode_(_Analysis_code_type_user_driver_);

LPCWSTR printschema::FRAMEWORK_URI = 
    L"http://schemas.microsoft.com/windows/2003/08/printing/printschemaframework";

LPCWSTR printschema::KEYWORDS_URI = 
    L"http://schemas.microsoft.com/windows/2003/08/printing/printschemakeywords";

//
// Element types defined in Printschema framework
//

LPCWSTR printschema::FEATURE_ELEMENT_NAME = L"Feature";
LPCWSTR printschema::OPTION_ELEMENT_NAME = L"Option";
LPCWSTR printschema::NAME_ATTRIBUTE_NAME = L"name";

//
// Elements described as Printschema keywords
//

LPCWSTR printschema::photoprintingintent::FEATURE = L"PagePhotoPrintingIntent";
LPCWSTR printschema::photoprintingintent::OPTIONS[] = { L"None", L"PhotoDraft", L"PhotoStandard", L"PhotoBest", };

namespace printschema
{
    namespace photoprintingintent
    {
        //
        // operator prefix increment
        //
        const EIntentValue & operator++(EIntentValue &e)

        {
            e = (EIntentValue)(e + 1);
            return e;
        }
    }
}

LPCWSTR printschema::pagemediatype::FEATURE = L"PageMediaType";
LPCWSTR printschema::pagemediatype::PLAIN = L"Plain";
LPCWSTR printschema::pagemediatype::PHOTOGRAPHIC = L"Photographic";

LPCWSTR printschema::pageoutputquality::FEATURE = L"PageOutputQuality";
LPCWSTR printschema::pageoutputquality::DRAFT = L"Draft";
LPCWSTR printschema::pageoutputquality::HIGH = L"High";
LPCWSTR printschema::pageoutputquality::PHOTOGRAPHIC = L"Photographic";
