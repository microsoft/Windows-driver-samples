// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
//
// File Name:
//
//    rasinterface.h
//
// Abstract:
//
//    Class to wrap rasterization-related calculations, interactions with
//    the Xps Rasterization Service, Xps Rasterization Service Callback,
//    and eventual raster output (i.e. TIFF encoding)
//

#pragma once

namespace xpsrasfilter
{

class RasterizationInterface
{
public:
    static
    RasterizationInterface_t
    CreateRasterizationInterface(
        const IPrintPipelinePropertyBag_t &pPropertyBag,
        const IPrintWriteStream_t &pStream
        );

    RasterizationInterface();

    void
    RasterizePage(
        const IXpsOMPage_t              &pPage,
        const ParametersFromPrintTicket &printTicketParams,
        const FilterLiveness_t          &pLiveness
        );

    void
    CancelRasterization();

    void
    FinishRasterization();

    //
    // Target band size; 16MB
    //
    const static LONG ms_targetBandSize = 1024 * 1024 * 16;

private:

    //
    // Constructor is private; use CreateRasterizationInterface
    // to create instances
    //
    RasterizationInterface(
        const IXpsRasterizationFactory_t    &pRasFactory,
        TiffStreamBitmapHandler_t           pBitmapHandler
        );

    //
    // prevent copy semantics
    //
    RasterizationInterface(const RasterizationInterface&);
    RasterizationInterface& operator=(const RasterizationInterface&);

    //
    // Internal data members
    //

    //
    // Xps Rasterization Service Factory
    //
    IXpsRasterizationFactory_t m_pXPSRasFactory;

    //
    // Bitmap Handler
    //
    TiffStreamBitmapHandler_t m_pBitmapHandler;
};

//
// Parameters that determine how a page is rasterized, in the
// units that the Rasterization Service expects
//
struct RasterizationParameters
{
    FLOAT       rasterizationDPI;       // dpi (scaling factor)
    INT         rasterHeight;           // total size of raster
    INT         rasterWidth;
    INT         originX;                // origin of the rasterization (translation)
    INT         originY;
    INT         bandHeight;             // height of individual bands

    RasterizationParameters(
        const ParametersFromPrintTicket &printTicketParams,
        const ParametersFromFixedPage   &fixedPageParams
        );
};

const FLOAT xpsDPI = 96.0f;

} // namespace xpsrasfilter
