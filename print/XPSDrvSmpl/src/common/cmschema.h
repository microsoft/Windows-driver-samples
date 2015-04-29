/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   cmschema.h

Abstract:

   PageColorManagement PrintSchema definition. This defines the features, options
   and enumerations that describe the PrintSchema PageColorManagement feature
   within a XDPrintSchema::PageColorManagement namespace.

--*/

#pragma once

#include "schema.h"

namespace XDPrintSchema
{
    //
    // PageColorManagement elements described as Printschema keywords
    //
    namespace PageColorManagement
    {
        //
        // Feature name
        //
        extern LPCWSTR PCM_FEATURE;

        //
        // Option names
        //
        enum EPCMOption
        {
            None = 0, EPCMOptionMin = 0,
            Device,
            Driver,
            System,
            EPCMOptionMax
        };

        extern LPCWSTR PCM_OPTIONS[EPCMOptionMax];
    }
}

