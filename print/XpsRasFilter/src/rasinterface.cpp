// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
//
// File Name:
//
//    rasinterface.cpp
//
// Abstract:
//
//    Class to wrap rasterization-related calculations, interactions with
//    the Xps Rasterization Service, Xps Rasterization Service Callback,
//    and eventual raster output (i.e. TIFF encoding)
//

#include "precomp.h"
#include "WppTrace.h"
#include "Exception.h"
#include "filtertypes.h"
#include "UnknownBase.h"
#include "xpsrasfilter.h"
#include "OMConvertor.h"
#include "rasinterface.h"
#include "BitmapHandler.h"

#include "rasinterface.tmh"

namespace xpsrasfilter
{

//
//Routine Name:
//
//    RasterizationInterface::CreateRasterizationInterface
//
//Routine Description:
//
//    Static factory method that creates an instance of
//    RasterizationInterface.
//
//Arguments:
//
//    pPropertyBag    - Property Bag
//    pStream         - Filter output stream (IPrintWriteStream)
//
//Return Value:
//
//    RasterizationInterface_t (smart ptr)
//    The new RasterizationInterface.
//
RasterizationInterface_t
RasterizationInterface::CreateRasterizationInterface(
    const IPrintPipelinePropertyBag_t &pPropertyBag,
    const IPrintWriteStream_t &pStream
    )
{
    Variant_t                   varRasFactory;
    IXpsRasterizationFactory_t  pXPSRasFactory;

    //
    // Get the Xps Rasterization Service factory. Since XpsRasterService.dll
    // is specified as an OptionalFilterServiceProvider in the filter pipeline 
    // configuration file, the factory may not be available in the property bag 
    // (e.g. when running on Windows Vista, Windows XP, etc). In this case,
    // this call will fail and the filter could fail, as we do here, or could 
    // default to some other behavior.
    //
    THROW_ON_FAILED_HRESULT(
        pPropertyBag->GetProperty(
            L"MS_IXpsRasterizationFactory", 
            &varRasFactory
            )
        );

    IUnknown_t pUnk(varRasFactory.punkVal);

    THROW_ON_FAILED_HRESULT(
        pUnk.QueryInterface(&pXPSRasFactory)
        );

    //
    // Prepare the rasterization/encode/stream interface
    //
    TiffStreamBitmapHandler_t pBitmapHandler(
        TiffStreamBitmapHandler::CreateTiffStreamBitmapHandler(pStream)
        );

    RasterizationInterface_t pReturnInterface(
                                new RasterizationInterface(
                                        pXPSRasFactory, 
                                        pBitmapHandler
                                        )
                                );

    return pReturnInterface;
}

//
//Routine Name:
//
//    RasterizationInterface::RasterizationInterface
//
//Routine Description:
//
//    Construct the Rasterization Interface with the
//    IXpsRasterizationFactory interface and bitmap
//    handler.
//
//Arguments:
//
//    pRasFactory     - Xps Rasterization Service object factory
//    pBitmapHandler  - Class to handle band bitmaps
//
RasterizationInterface::RasterizationInterface(
        const IXpsRasterizationFactory_t    &pRasFactory,
        TiffStreamBitmapHandler_t           pBitmapHandler
        ) : m_pXPSRasFactory(pRasFactory), 
            m_pBitmapHandler(pBitmapHandler)
{
}

//
//Routine Name:
//
//    RasterizationInterface::FinishRasterization
//
//Routine Description:
//
//    Tell the Rasterization Interface that the last
//    page has been rasterized.
//
//Arguments:
//
//    None
//
void
RasterizationInterface::FinishRasterization()
{
    m_pBitmapHandler->WriteFooter();
}


//
//Routine Name:
//
//    RasterizationInterface::RasterizeAndStreamPage
//
//Routine Description:
//
//    Given an IXpsOMPage and a set of Print Ticket
//    parameters, this method invokes the Xps Rasterization
//    Service for each band of the page, and outputs the
//    resultant raster data.
//
//Arguments:
//
//    pPage               - page to rasterize
//    printTicketparams   - raw parameters from the print ticket(s)
//
void
RasterizationInterface::RasterizePage(
    const IXpsOMPage_t                 &pPage,
    const ParametersFromPrintTicket    &printTicketParams,
    const FilterLiveness_t             &pLiveness
    )
{
    //
    // Calculate rasterization parameters
    //
    ParametersFromFixedPage fixedPageParams;
    THROW_ON_FAILED_HRESULT(
        pPage->GetPageDimensions(&fixedPageParams.fixedPageSize)
        );
    THROW_ON_FAILED_HRESULT(
        pPage->GetBleedBox(&fixedPageParams.bleedBoxRect)
        );
    THROW_ON_FAILED_HRESULT(
        pPage->GetContentBox(&fixedPageParams.contentBoxRect)
        );

    RasterizationParameters rastParams(
        printTicketParams,
        fixedPageParams
        );

    //
    // Create the Rasterizer
    //
    IXpsRasterizer_t rasterizer;
    THROW_ON_FAILED_HRESULT(
        m_pXPSRasFactory->CreateRasterizer(
                            pPage,
                            rastParams.rasterizationDPI, 
                            XPSRAS_RENDERING_MODE_ANTIALIASED, 
                            XPSRAS_RENDERING_MODE_ANTIALIASED, 
                            &rasterizer
                            )
        );

    //
    // Set the minimal line width to 1 pixel
    //
    THROW_ON_FAILED_HRESULT(
        rasterizer->SetMinimalLineWidth(1)
        );

    //
    // Loop over bands
    //
    INT bandOriginY = 0;

    while (pLiveness->IsAlive() && 
           bandOriginY < rastParams.rasterHeight)
    {
        DoTraceMessage(XPSRASFILTER_TRACE_VERBOSE, L"Rasterizing Band");

        IWICBitmap_t bitmap;

        //
        // Calculate the height of this band
        //
        INT bandHeight = rastParams.bandHeight;
        if (bandOriginY + rastParams.bandHeight >= rastParams.rasterHeight)
        {
            bandHeight = rastParams.rasterHeight - bandOriginY;
        }

        //
        // Rasterize this band
        //
        {
            HRESULT hr = rasterizer->RasterizeRect(
                            rastParams.originX,
                            bandOriginY + rastParams.originY,
                            rastParams.rasterWidth,
                            bandHeight, 
                            static_cast<IXpsRasterizerNotificationCallback *>(pLiveness), 
                            &bitmap
                            );

            //
            // Do not throw if we have cancelled rasterization
            //
            if (hr == HRESULT_FROM_WIN32(ERROR_PRINT_CANCELLED))
            {
                DoTraceMessage(XPSRASFILTER_TRACE_VERBOSE, L"Rasterization Cancelled");
                return;
            }

            THROW_ON_FAILED_HRESULT(hr);
        }

        bandOriginY += bandHeight;

        //
        // The resolution of the bitmap defaults to the
        // rasterization DPI, which includes the scaling factor.
        // We want the output bitmap to reflect the destination DPI.
        //
        THROW_ON_FAILED_HRESULT(
            bitmap->SetResolution(
                printTicketParams.destDPI,
                printTicketParams.destDPI
                )
            );

        //
        // Encode the raster data as TIFF and stream out
        //
        m_pBitmapHandler->ProcessBitmap(bitmap);
    }
}

//
//Routine Name:
//
//    RasterizationParameters::RasterizationParameters
//
//Routine Description:
//
//    Given a set of fixed page parameters and a set of Print Ticket
//    parameters, this constructor calculates the parameters necessary to
//    invoke the Xps Rasterization Service.
//
//Arguments:
//
//    printTicketparams    - raw parameters from the print ticket(s)
//    fixedPageParams     - raw parameters from the fixed page
//
RasterizationParameters::RasterizationParameters(
    const ParametersFromPrintTicket &printTicketParams,
    const ParametersFromFixedPage   &fixedPageParams
    )
{
    //
    // Rasterize the entire physical page at the desired resolution. The fixed page
    // is scaled and traslated to place it correctly witin the physical page.
    //
    rasterHeight = static_cast<INT>(
                        printTicketParams.physicalPageSize.height 
                            * (printTicketParams.destDPI / xpsDPI) 
                        + 0.5
                        );
    rasterWidth  = static_cast<INT>(
                        printTicketParams.physicalPageSize.width 
                            * (printTicketParams.destDPI / xpsDPI) 
                        + 0.5
                        );

    //
    // Determine the source rectangle for the scale operation
    //
    XPS_RECT srcRect = {0,0,0,0};

    switch(printTicketParams.scaling)
    {
        case SCALE_BLEEDTOIMAGEABLE:

            srcRect = fixedPageParams.bleedBoxRect;

            break;
        default:
            DoTraceMessage(XPSRASFILTER_TRACE_ERROR, L"Unknown scaling type");
            THROW_ON_FAILED_HRESULT(E_UNEXPECTED);
            break;
    }

    //
    // Determine the destination rectangle for the scale operation
    //
    XPS_RECT destRect = {0,0,0,0};

    switch(printTicketParams.scaling)
    {
        case SCALE_BLEEDTOIMAGEABLE:

            destRect = printTicketParams.imageableArea;

            break;
        default:
            DoTraceMessage(XPSRASFILTER_TRACE_ERROR, L"Unknown scaling type");
            THROW_ON_FAILED_HRESULT(E_UNEXPECTED);
            break;
    }


    //
    // Because we want to fit the fixed page into a physical page of
    // potentially different aspect ratio, it is necessary to calculate
    // the scaling factor assuming either the height or width constrains
    // the operation; the rasterization dpi is then the smaller of the two.
    //
    //
    // The basic calculation for scaling factor is:
    //
    //      Scaling Factor = (Destination Dimension / Source Dimension)
    //
    // Multiplying by desired Destination DPI we get the DPI at which to 
    //    rasterize the source content in order to get the desired size
    //
    //      Rasterization DPI = Scaling Factor * Desired Destination DPI
    //
    {
        FLOAT heightScalingFactor = destRect.height / srcRect.height;
        FLOAT widthScalingFactor  = destRect.width / srcRect.width;

        FLOAT heightRastDPI   = heightScalingFactor * printTicketParams.destDPI;
        FLOAT widthRastDPI    = widthScalingFactor * printTicketParams.destDPI;

        rasterizationDPI = min(heightRastDPI, widthRastDPI);
    }

    //
    // To determine the rasterization origin, subtract the translation
    // due to the destination rectangle (e.g. imageable area) from the 
    // translation due to the choice of scale (e.g. bleed box origin)
    //
    {
        FLOAT srcOffsetX = srcRect.x * rasterizationDPI / xpsDPI;
        FLOAT srcOffsetY = srcRect.y * rasterizationDPI / xpsDPI;

        FLOAT destOffsetX = destRect.x * printTicketParams.destDPI / xpsDPI;
        FLOAT destOffsetY = destRect.y * printTicketParams.destDPI / xpsDPI;

        originX = static_cast<INT>(srcOffsetX - destOffsetX - 0.5);
        originY = static_cast<INT>(srcOffsetY - destOffsetY - 0.5);
    }

    //
    // The height of each band is determined by the maximum band size (in bytes) 
    // divided by 4 bytes per pixel to get the total pixels, and then divided by
    // the width of the raster data. This results in a band bitmap that is close 
    // to the target band size.
    //
    bandHeight = (RasterizationInterface::ms_targetBandSize / 4) / rasterWidth;

    if (bandHeight == 0)
    {
        //
        // The physical page is too wide to rasterize at this 
        // maximum band size and dpi. Throw.
        //
        THROW_ON_FAILED_HRESULT(
            E_OUTOFMEMORY
            );
    }
}

} // namespace xpsrasfilter
