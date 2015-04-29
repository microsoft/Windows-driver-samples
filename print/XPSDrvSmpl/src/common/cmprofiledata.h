/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   cmprofiledata.h

Abstract:

   PageSourceColorProfileData data structure definition. This provides
   a convenient description of the PrintSchema PageSourceColorProfileData
   feature.

--*/

#pragma once

#include "cmprofileschema.h"

namespace XDPrintSchema
{
    namespace PageSourceColorProfile 
    {
        struct PageSourceColorProfileData 
        {
            PageSourceColorProfileData() :
                cmProfile(RGB)
            {
            }

            EProfileOption cmProfile;
            CComBSTR       cmProfileName;
        };
    }
}

