/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   bkdata.h

Abstract:

   Booklet data structure definition. This provides a convenient description
   of the PrintSchema JobBindAllDocuments and DcoumentBinding features.

--*/

#pragma once

#include "bkschema.h"

namespace XDPrintSchema
{
    namespace Binding
    {
        struct BindingData
        {
            BindingData() :
                bindFeature(JobBindAllDocuments),
                bindOption(None),
                bindGutter(0)
            {
            }

            EBinding       bindFeature;
            EBindingOption bindOption;
            INT            bindGutter;
        };
    }
}

