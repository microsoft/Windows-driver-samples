/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   psizedata.h

Abstract:

   PageMediaSize data structure definition. This provides a convenient
   description of the PrintSchema PageMediaSize feature.

--*/

#pragma once

#include "psizeschema.h"

namespace XDPrintSchema
{
    namespace PageMediaSize
    {
        struct PageMediaSizeData
        {
            PageMediaSizeData() :
                pageWidth(215900),
                pageHeight(279400)
            {
            }

            INT             pageWidth;
            INT             pageHeight;
        };
    }
}

