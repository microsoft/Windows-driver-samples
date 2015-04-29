/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   bkschema.h

Abstract:

   Binding (booklet) PrintSchema definition. This defines the features,
   options and enumerations that describe the PrintSchema JobBindAllDocuments and
   DocumentBinding features within a XDPrintSchema::Binding namespace.

--*/

#pragma once

#include "schema.h"

namespace XDPrintSchema
{
    //
    // Binding elements described as Printschema keywords
    //
    namespace Binding
    {
        //
        // Job and Document share identical options so we define two
        // features within the Binding namespace.
        //
        enum EBinding
        {
            JobBindAllDocuments = 0, EBindingMin = 0,
            DocumentBinding,
            EBindingMax
        };

        extern LPCWSTR BIND_FEATURES[EBindingMax];

        //
        // Option names
        //
        enum EBindingOption
        {
            Bale = 0, EBindingOptionMin = 0,
            BindBottom,
            BindLeft,
            BindRight,
            BindTop,
            Booklet,
            EdgeStitchBottom,
            EdgeStitchLeft,
            EdgeStitchRight,
            EdgeStitchTop,
            Fold,
            JogOffset,
            Trim,
            None,
            EBindingOptionMax
        };

        extern LPCWSTR BIND_OPTIONS[EBindingOptionMax];

        extern LPCWSTR BIND_PROP;

        extern LPCWSTR BIND_PROP_REF_SUFFIX;
    }
}

