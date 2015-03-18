// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
//
// File Name:
//
//    omconvertor.cpp
//
// Abstract:
//
//    Object model conversion routines. This class provides routines
//      to convert from filter pipeline objects into Xps Object Model 
//      objects.
//

#include "precomp.h"
#include "WppTrace.h"
#include "Exception.h"
#include "filtertypes.h"
#include "UnknownBase.h"
#include "OMConvertor.h"

#include "OMConvertor.tmh"

namespace xpsrasfilter
{

//
//Routine Name:
//
//    CreateXpsOMPageFromIFixedPage
//
//Routine Description:
//
//    This is the main method called by the filter. It
//    proceeds to call the remaining Create* methods
//    to convert from the print pipeline Object Model 
//    to the Xps Object Model.
//    
//    Takes an IFixedPage (print pipeline Object Model) 
//    and converts it to an IXpsOMPage (Xps Object Model).
//
//Arguments:
//
//    pPageIn     - IFixedPage to convert
//    pOMFactory  - Xps Object Model Object Factory
//    pOpcFactory - Opc Object Factory
//
//Return Value:
//
//    IXpsOMPage_t (smart pointer)
//    Result IXpsOMPage
//
IXpsOMPage_t
CreateXpsOMPageFromIFixedPage(
   const IFixedPage_t          &pPageIn,
   const IXpsOMObjectFactory_t &pOMFactory,
   const IOpcFactory_t         &pOpcFactory
   )
{
    //
    // Get additional page parameters (uri and stream)
    //
    IOpcPartUri_t pPartUri;
    IStream_t pPartStream;

    pPartStream = GetStreamFromPart(
        static_cast<IPartBase_t>(pPageIn)
        );
    pPartUri = CreateOpcPartUriFromPart(
        static_cast<IPartBase_t>(pPageIn), 
        pOpcFactory
        );

    //
    // Call Xps Object Model to create the page resource
    //
    IXpsOMPage_t pPageOut;

    THROW_ON_FAILED_HRESULT(
        pOMFactory->CreatePageFromStream(
            pPartStream,
            pPartUri,
            CollectPageResources(pPageIn, pOMFactory, pOpcFactory),
            FALSE, // Do not reuse objects
            &pPageOut
            )
        );

    return pPageOut;
}

//
//Routine Name:
//
//    CreateImageFromIPartImage
//
//Routine Description:
//
//    Takes an IPartImage (print pipeline Object Model) 
//    and converts it to an IXpsOMImageResource (Xps Object Model).
//
//Arguments:
//
//    pImageIn    - IPartImage to convert
//    pOMFactory  - Xps Object Model Object Factory
//    pOpcFactory - Opc Object Factory
//
//Return Value:
//
//    IXpsOMImageResource_t (smart pointer)
//    Result IXpsOMImageResource
//
IXpsOMImageResource_t
CreateImageFromIPartImage(
    const IPartImage_t             &pImageIn,
    const IXpsOMObjectFactory_t    &pOMFactory,
    const IOpcFactory_t            &pOpcFactory
    )
{
    //
    // Get IPartBase parameters (stream and uri)
    //
    IStream_t            pPartStream;
    IOpcPartUri_t        pPartUri;

    pPartStream = GetStreamFromPart(
        static_cast<IPartBase_t>(pImageIn)
        );
    pPartUri = CreateOpcPartUriFromPart(
        static_cast<IPartBase_t>(pImageIn),
        pOpcFactory
        );

    //
    // Get the image type and convert it to the corresponding enum
    //
    BSTR_t strContentType;
    XPS_IMAGE_TYPE type = XPS_IMAGE_TYPE_WDP;
    
    THROW_ON_FAILED_HRESULT(
        pImageIn->GetImageProperties(&strContentType)
        );

    if (0 == _wcsicmp(strContentType, L"image/jpeg"))
    {
        type = XPS_IMAGE_TYPE_JPEG;
    }
    else if (0 == _wcsicmp(strContentType, L"image/png"))
    {
        type = XPS_IMAGE_TYPE_PNG;
    }
    else if (0 == _wcsicmp(strContentType, L"image/tiff"))
    {
        type = XPS_IMAGE_TYPE_TIFF;
    }
    else if (0 == _wcsicmp(strContentType, L"image/vnd.ms-photo"))
    {
        type = XPS_IMAGE_TYPE_WDP;
    }
    else
    {
        //
        // unknown content type
        //
        THROW_ON_FAILED_HRESULT(E_INVALIDARG);
    }

    //
    // Call Xps Object Model to create the image resource
    //
    IXpsOMImageResource_t   pImageOut;

    THROW_ON_FAILED_HRESULT(
        pOMFactory->CreateImageResource(
            pPartStream, 
            type, 
            pPartUri, 
            &pImageOut
            )
        );

    return pImageOut;
}

//
//Routine Name:
//
//    CreateProfileFromIPartColorProfile
//
//Routine Description:
//
//    Takes an IPartColorProfile (print pipeline Object Model) 
//    and converts it to an IXpsOMColorProfileResource (Xps Object Model).
//
//Arguments:
//
//    pProfileIn      - IPartColorProfile to convert
//    pOMFactory      - Xps Object Model Object Factory
//    pOpcFactory     - Opc Object Factory
//
//Return Value:
//
//    IXpsOMColorProfileResource_t (smart pointer)
//    Result IXpsOMColorProfileResource
//
IXpsOMColorProfileResource_t
CreateProfileFromIPartColorProfile(
    const IPartColorProfile_t      &pProfileIn,
    const IXpsOMObjectFactory_t    &pOMFactory,
    const IOpcFactory_t            &pOpcFactory
    )
{
    //
    // Get IPartBase parameters (stream and uri)
    //
    IStream_t            pPartStream;
    IOpcPartUri_t        pPartUri;

    pPartStream = GetStreamFromPart(
        static_cast<IPartBase_t>(pProfileIn)
        );
    pPartUri = CreateOpcPartUriFromPart(
        static_cast<IPartBase_t>(pProfileIn),
        pOpcFactory
        );

    //
    // Call Xps Object Model to create the color profile resource
    //
    IXpsOMColorProfileResource_t pProfileOut;

    THROW_ON_FAILED_HRESULT(
        pOMFactory->CreateColorProfileResource(
            pPartStream, 
            pPartUri, 
            &pProfileOut
            )
        );

    return pProfileOut;
}

//
//Routine Name:
//
//    CreateDictionaryFromIPartResourceDictionary
//
//Routine Description:
//
//    Takes an IPartResourceDictionary (print pipeline Object Model) 
//    and converts it to an IXpsOMRemoteDictionaryResource (Xps Object Model).
//
//Arguments:
//
//    pDictionaryIn     - IPartResourceDictionary to convert
//    pOMFactory        - Xps Object Model Object Factory
//    pOpcFactory       - Opc Object Factory
//    pResources        - The resources of the fixed page
//
//Return Value:
//
//    IXpsOMRemoteDictionaryResource_t (smart pointer)
//    Result IXpsOMRemoteDictionaryResource
//
IXpsOMRemoteDictionaryResource_t
CreateDictionaryFromIPartResourceDictionary(
    const IPartResourceDictionary_t    &pDictionaryIn,
    const IXpsOMObjectFactory_t        &pOMFactory,
    const IOpcFactory_t                &pOpcFactory,
    const IXpsOMPartResources_t        &pResources
    )
{
    //
    // Get IPartBase parameters (stream and uri)
    //
    IStream_t            pPartStream;
    IOpcPartUri_t        pPartUri;
    
    pPartStream = GetStreamFromPart(
        static_cast<IPartBase_t>(pDictionaryIn)
        );
    pPartUri = CreateOpcPartUriFromPart(
        static_cast<IPartBase_t>(pDictionaryIn),
        pOpcFactory
        );

    //
    // Call Xps Object Model to create the remote dictionary resource
    //
    IXpsOMRemoteDictionaryResource_t  pDictionaryOut;

    THROW_ON_FAILED_HRESULT(
        pOMFactory->CreateRemoteDictionaryResourceFromStream(
            pPartStream,
            pPartUri,
            pResources,
            &pDictionaryOut)
        );
    
    return pDictionaryOut;
}

//
//Routine Name:
//
//    CreateFontFromIPartFont
//
//Routine Description:
//
//    Takes an IPartFont (print pipeline Object Model) 
//    and converts it to an IXpsOMFontResource (Xps Object Model).
//
//Arguments:
//
//    pFontIn     - IPartFont to convert
//    pFactory    - Xps Object Model Object Factory
//    pOpcFactory - Opc Object Factory
//
//Return Value:
//
//    IXpsOMFontResource_t (smart pointer)
//    Result IXpsOMFontResource
//
IXpsOMFontResource_t
CreateFontFromIPartFont(
    const IPartFont_t              &pFontIn,
    const IXpsOMObjectFactory_t    &pOMFactory,
    const IOpcFactory_t            &pOpcFactory
    )
{
    //
    // Get IPartBase parameters (stream and uri)
    //
    IStream_t            pPartStream;
    IOpcPartUri_t        pPartUri;

    pPartStream = GetStreamFromPart(
        static_cast<IPartBase_t>(pFontIn)
        );
    pPartUri = CreateOpcPartUriFromPart(
        static_cast<IPartBase_t>(pFontIn),
        pOpcFactory
        );

    //
    // Get the font restriction
    //
    EXpsFontRestriction eFontRestriction = Xps_Restricted_Font_Installable;

    {
        IPartFont2_t            pFont2In;

        if (SUCCEEDED(pFontIn->QueryInterface(__uuidof(IPartFont2), reinterpret_cast<void **>(&pFont2In))))
        {
            pFont2In->GetFontRestriction(&eFontRestriction);
        }
    }

    //
    // Get the font obfuscation
    //
    EXpsFontOptions eFontOptions;

    {
        BSTR_t                  contentType;

        THROW_ON_FAILED_HRESULT(
            pFontIn->GetFontProperties(&contentType, &eFontOptions)
            );
    }

    //
    // It is necessary to combine the obfuscation and restriction 
    // attributes from the print pipeline into the one parameter that
    // the Xps Object Model consumes.
    //
    
    XPS_FONT_EMBEDDING omEmbedding;

    if (eFontOptions == Font_Normal)
    {
        omEmbedding = XPS_FONT_EMBEDDING_NORMAL;
    }
    else if (eFontOptions == Font_Obfusticate &&
             (eFontRestriction &
                (Xps_Restricted_Font_PreviewPrint | 
                 Xps_Restricted_Font_NoEmbedding)))
    {
        //
        // If the font is obfuscated, and either the PreviewPrint or 
        // NoEmbedding restriction flags are set, then create a
        // Restricted font
        //
        omEmbedding = XPS_FONT_EMBEDDING_RESTRICTED;
    }
    else
    {
        omEmbedding = XPS_FONT_EMBEDDING_OBFUSCATED;
    }

    //
    // Call Xps Object Model to create the font resource
    //
    IXpsOMFontResource_t pFontOut;

    THROW_ON_FAILED_HRESULT(
        pOMFactory->CreateFontResource(
            pPartStream, 
            omEmbedding, 
            pPartUri,
            FALSE,  // fonts received from the pipeline are already de-obfuscated
            &pFontOut
            )
        );

    return pFontOut;
}

//
//Routine Name:
//
//    CollectPageResources
//
//Routine Description:
//
//    Iterates over all of the resources related to
//    a fixed page and adds them to a resource 
//    collection.
//
//Arguments:
//
//    pPage       - The page to query for resources
//    pOMFactory  - Xps Object Model Object Factory
//    pOpcFactory - Opc Object Factory
//
//Return Value:
//
//    IXpsOMPartResources_t (smart pointer)
//    The resource collection of all of the resources of the page
//
IXpsOMPartResources_t
CollectPageResources(
    const IFixedPage_t             &pPage,
    const IXpsOMObjectFactory_t    &pOMFactory,
    const IOpcFactory_t            &pOpcFactory
    )
{
    IXpsOMPartResources_t pResources;

    IXpsOMFontResourceCollection_t              pFonts;
    IXpsOMImageResourceCollection_t             pImages;
    IXpsOMColorProfileResourceCollection_t      pProfiles;
    IXpsOMRemoteDictionaryResourceCollection_t  pDictionaries;

    //
    // collection of resource dictionaries saved for later processing.
    //
    ResourceDictionaryList_t dictionaryList;

    //
    // Create the resource collection and get all of the
    // resource-specific sub-collections.
    //
    THROW_ON_FAILED_HRESULT(
        pOMFactory->CreatePartResources(&pResources)
        );
    THROW_ON_FAILED_HRESULT(
        pResources->GetFontResources(&pFonts)
        );
    THROW_ON_FAILED_HRESULT(
        pResources->GetImageResources(&pImages)
        );
    THROW_ON_FAILED_HRESULT(
        pResources->GetColorProfileResources(&pProfiles)
        );
    THROW_ON_FAILED_HRESULT(
        pResources->GetRemoteDictionaryResources(&pDictionaries)
        );
    //
    // Get the XpsPartIterator and iterate through all of the parts
    // related to this fixed page.
    //
    IXpsPartIterator_t itPart;

    THROW_ON_FAILED_HRESULT(
        pPage->GetXpsPartIterator(&itPart)
        );

    for (;  !itPart->IsDone(); itPart->Next())
    {
        BSTR_t uri;
        IUnknown_t pUnkPart;

        THROW_ON_FAILED_HRESULT(
            itPart->Current(&uri, &pUnkPart)
            );

        IPartFont_t               pFontPart;
        IPartImage_t              pImagePart;
        IPartColorProfile_t       pProfilePart;
        IPartResourceDictionary_t pDictionaryPart;

        if (SUCCEEDED(pUnkPart.QueryInterface(&pFontPart)))
        {
            //
            // Convert the font part to Xps Object Model and add it to the 
            // font resource collection
            //
            THROW_ON_FAILED_HRESULT(
                pFonts->Append(
                    CreateFontFromIPartFont(
                        pFontPart,
                        pOMFactory,
                        pOpcFactory
                        )
                    )
                );
        }
        else if (SUCCEEDED(pUnkPart.QueryInterface(&pImagePart)))
        {
            //
            // Convert the image part to Xps Object Model and add it to the 
            // image resource collection
            //
            THROW_ON_FAILED_HRESULT(
                pImages->Append(
                    CreateImageFromIPartImage(
                        pImagePart, 
                        pOMFactory,
                        pOpcFactory
                        )
                    )
                );
        }
        else if (SUCCEEDED(pUnkPart.QueryInterface(&pProfilePart)))
        {
            //
            // Convert the color profile part to Xps Object Model and add it 
            // to the color profile resource collection
            //
            THROW_ON_FAILED_HRESULT(
                pProfiles->Append(
                    CreateProfileFromIPartColorProfile(
                        pProfilePart, 
                        pOMFactory,
                        pOpcFactory
                        )
                    )
                );
        }
        else if (SUCCEEDED(pUnkPart.QueryInterface(&pDictionaryPart)))
        {
            //
            // In order to process the remote resource dictionary, all of 
            // its linked resources must be present in pResources. To ensure 
            // this, we delay the conversion of the remote resource
            // dictionaries until all of the other resources have been converted.
            //
            dictionaryList.push_back(pDictionaryPart);
        }
        else
        {
            //
            // Any other page resources are ignored
            //
        }
    }

    for (ResourceDictionaryList_t::const_iterator it = dictionaryList.begin();
            it != dictionaryList.end();
            ++it)
    {
        //
        // Convert the remote dictionary to Xps Object Model and add it 
        // to the remote dictionary collection
        //
        THROW_ON_FAILED_HRESULT(
                pDictionaries->Append(
                    CreateDictionaryFromIPartResourceDictionary(
                        *it, 
                        pOMFactory,
                        pOpcFactory,
                        pResources
                        )
                    )
                );
    }
    
    return pResources;
}

//
//Routine Name:
//
//    GetStreamFromPart
//
//Routine Description:
//
//    Gets the IStream from this part.
//
//Arguments:
//
//    pPart           - An Xps Part
//
//Return Value:
//    
//    IStream_t (smart pointer)
//    The stream of the part's content
//
IStream_t
GetStreamFromPart(
    const IPartBase_t  &pPart
    )
{
    //
    // Get the IPrintReadStream for the part from the pipeline Object Model
    //
    IPrintReadStream_t   pStream;
    THROW_ON_FAILED_HRESULT(
        pPart->GetStream(&pStream)
        );

    return CreateIStreamFromIPrintReadStream(pStream);
}

//
//Routine Name:
//
//    CreateIStreamFromIPrintReadStream
//
//Routine Description:
//
//    Creates an IStream from an IPrintReadStream.
//
//Arguments:
//
//    pReadStream - A Print Pipeline IPrintReadStream
//
//Return Value:
//    
//    IStream_t (smart pointer)
//    A stream with the same content as the argument stream.
//
IStream_t
CreateIStreamFromIPrintReadStream(
    const IPrintReadStream_t   &pReadStream
    )
{
    //
    // Get the size of the stream
    //
    ULONGLONG tmpPos;
    size_t partSize;

    THROW_ON_FAILED_HRESULT(
        pReadStream->Seek(0, SEEK_END, &tmpPos)
        );

    //
    // GlobalAlloc can only allocate size_t bytes, so
    // throw if the part is larger than that
    //
    THROW_ON_FAILED_HRESULT(
        ULongLongToSizeT(tmpPos, &partSize)
        );

    THROW_ON_FAILED_HRESULT(
        pReadStream->Seek(0, SEEK_SET, &tmpPos)
        );

    //
    // Allocate an HGLOBAL for the part cache
    //
    SafeHGlobal_t pHBuf(
                    new SafeHGlobal(GMEM_FIXED, partSize)
                    );

    //
    // Read the part into the cache
    //
    {
        //
        // Lock the HGLOBAL and get the address of the buffer
        // from the RAII lock object
        //
        HGlobalLock_t lock = pHBuf->Lock();
        BYTE *pBuffer = lock->GetAddress();

        //
        // Allow the number of bytes to read to be clipped to max ULONG
        // and then spin on fEOF until the stream is exhausted
        //
        ULONG numToRead;

        if (FAILED(SizeTToULong(partSize, &numToRead)))
        {
            numToRead = MAXUINT;
        }

        BOOL fEOF;
        ULONG numRead;
        size_t pos = 0;

        //
        // Iterate until all bytes from the stream 
        // have been read into the buffer
        //
        do
        {
            THROW_ON_FAILED_HRESULT(
                pReadStream->ReadBytes(pBuffer + pos, numToRead, &numRead, &fEOF)
                );

            pos += numRead;
        } while (!fEOF && numRead);
    }

    //
    // Create an IStream from the part cache
    //
    IStream_t pIStream = pHBuf->ConvertToIStream();

    LARGE_INTEGER zero = {0};
    THROW_ON_FAILED_HRESULT(
        pIStream->Seek(zero, SEEK_SET, NULL)
        );

    return pIStream;
}

//
//Routine Name:
//
//    CreateOpcPartUriFromPart
//
//Routine Description:
//
//    Gets the Opc Uri from the Xps Part.
//
//Arguments:
//
//    pPart       - An Xps Part
//    pFactory    - Opc Factory
//
//Return Value:
//
//    IOpcPartUri_t (smart pointer)
//    The Uri of the part
//
IOpcPartUri_t
CreateOpcPartUriFromPart(
    const IPartBase_t      &pPart,
    const IOpcFactory_t    &pFactory
    )
{
    BSTR_t strPartUri;
    
    THROW_ON_FAILED_HRESULT(
        pPart->GetUri(&strPartUri)
        );
   
    IOpcPartUri_t   pPartUri;
    THROW_ON_FAILED_HRESULT(
        pFactory->CreatePartUri(strPartUri, &pPartUri)
        );
    return pPartUri;
}

} // namespace xpsrasfilter

