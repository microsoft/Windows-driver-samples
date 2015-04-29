/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   wmdata.h

Abstract:

   PageWatermark data structure definition. This provides a convenient
   description of the PrintSchema PageWatermark feature.

--*/

#pragma once

#include "wmschema.h"

namespace XDPrintSchema
{
    namespace PageWatermark
    {
        struct WMTextData
        {
            WMTextData() :
                bstrFontColor(L"#FFFFFF"),
                fontSize(12),
                bstrText(L"Undefined")
            {
            }

            CComBSTR bstrFontColor;
            INT      fontSize;
            CComBSTR bstrText;
        };

        struct WatermarkData
        {
            WatermarkData() :
                type(TextWatermark),
                widthOrigin(0),
                heightOrigin(0),
                widthExtent(215900),
                heightExtent(279400),
                transparency(0),
                angle(0),
                layering(Layering::Overlay)
            {
            }

            EWatermarkOption          type;
            INT                       widthOrigin;
            INT                       heightOrigin;
            INT                       widthExtent;
            INT                       heightExtent;
            INT                       transparency;
            INT                       angle;
            WMTextData                txtData;
            Layering::ELayeringOption layering;
        };
    }
}

