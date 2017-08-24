//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1998 - 2005  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:   printschema.hxx
//    
//  PURPOSE:  Constant declarations for print schema names.
//  

#pragma once

typedef 
enum tagPrintschemaVersion
{
    PRINTSCHEMA_VERSION_NUMBER =  1
} EPrintschemaVersion;


namespace printschema
{
 
    extern LPCWSTR FRAMEWORK_URI;
  
    extern LPCWSTR KEYWORDS_URI;

    //
    // Element types defined in Printschema framework
    //

    extern LPCWSTR FEATURE_ELEMENT_NAME;
    extern LPCWSTR OPTION_ELEMENT_NAME;
    extern LPCWSTR NAME_ATTRIBUTE_NAME;

    //
    // Elements described as Printschema keywords
    //

    namespace photoprintingintent
    {
        extern LPCWSTR FEATURE;
        
        enum EIntentValue
        {
            None = 0,
            Draft,
            Standard,
            Best,
            EIntentValueMax, // size of enum list
        };

        extern LPCWSTR OPTIONS[EIntentValueMax];

        const EIntentValue & operator++(EIntentValue &e);
    }

    namespace pagemediatype
    {
        extern LPCWSTR FEATURE;
        extern LPCWSTR PLAIN;
        extern LPCWSTR PHOTOGRAPHIC;
    }

    namespace pageoutputquality
    {
        extern LPCWSTR FEATURE;
        extern LPCWSTR DRAFT;
        extern LPCWSTR HIGH;
        extern LPCWSTR PHOTOGRAPHIC;
    }

}
