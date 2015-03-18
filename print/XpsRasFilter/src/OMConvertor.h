// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
//
// File Name:
//
//    omconvertor.h
//
// Abstract:
//
//    Object model conversion routines.
//

#pragma once

namespace xpsrasfilter
{

//
// Top-level Create Page routine
//
IXpsOMPage_t
CreateXpsOMPageFromIFixedPage(
   const IFixedPage_t          &pPageIn,
   const IXpsOMObjectFactory_t &pFactory,
   const IOpcFactory_t         &pOpcFactory
   );

//
// Individual Part conversion routines
//
IXpsOMImageResource_t
CreateImageFromIPartImage(
    const IPartImage_t             &pImageIn,
    const IXpsOMObjectFactory_t    &pFactory,
    const IOpcFactory_t            &pOpcFactory
    );

IXpsOMColorProfileResource_t
CreateProfileFromIPartColorProfile(
    const IPartColorProfile_t      &pProfileIn,
    const IXpsOMObjectFactory_t    &pFactory,
    const IOpcFactory_t            &pOpcFactory
    );

IXpsOMRemoteDictionaryResource_t
CreateDictionaryFromIPartResourceDictionary(
    const IPartResourceDictionary_t    &pDictionaryIn,
    const IXpsOMObjectFactory_t        &pFactory,
    const IOpcFactory_t                &pOpcFactory,
    const IXpsOMPartResources_t        &pResources
    );

IXpsOMFontResource_t
CreateFontFromIPartFont(
    const IPartFont_t              &pFontIn,
    const IXpsOMObjectFactory_t    &pFactory,
    const IOpcFactory_t            &pOpcFactory
    );

// 
// Utility Routines
//
IStream_t
GetStreamFromPart(
    const IPartBase_t  &pPart
    );

IOpcPartUri_t
CreateOpcPartUriFromPart(
    const IPartBase_t      &pPart,
    const IOpcFactory_t    &pOpcFactory
    );

IXpsOMPartResources_t
CollectPageResources(
    const IFixedPage_t             &pPage,
    const IXpsOMObjectFactory_t    &pFactory,
    const IOpcFactory_t            &pOpcFactory
    );

IStream_t
CreateIStreamFromIPrintReadStream(
    const IPrintReadStream_t   &pReadStream
    );

} // namespace xpsrasfilter
