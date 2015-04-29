/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   porientdata.h

Abstract:

   PageOrientation data structure definition. This provides a convenient
   description of the PrintSchema PageOrientation feature.

--*/

#pragma once

#include "porientschema.h"

namespace XDPrintSchema
{
    namespace PageOrientation
    {
        struct PageOrientationData
        {
            PageOrientationData() :
                orientation(Landscape)
            {
            }

            EOrientationOption orientation;
        };
    }
}

