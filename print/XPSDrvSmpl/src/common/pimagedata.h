/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   pimagedata.h

Abstract:

   PageImageableSize data structure definition. This provides a convenient
   description of the PrintSchema PageImageableSize feature.

--*/

#pragma once

#include "pimageschema.h"

namespace XDPrintSchema
{
    namespace PageImageableSize
    {
        struct PageImageableData
        {
            PageImageableData() :
                imageableSizeWidth(0),
                imageableSizeHeight(0),
                originWidth(0),
                originHeight(0),
                extentWidth(0),
                extentHeight(0)
            {
            }

            INT imageableSizeWidth;
            INT imageableSizeHeight;
            INT originWidth;
            INT originHeight;
            INT extentWidth;
            INT extentHeight;
        };
    }
}

