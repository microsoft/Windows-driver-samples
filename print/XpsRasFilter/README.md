XPS Rasterization Filter Service Sample
=======================================

This sample implements an XPSDrv filter that rasterizes fixed pages in an XPS document. Hardware vendors can modify this sample to build an XPSDrv filter that produces bitmap images for their printers or other display devices. The sample uses the XPS Rasterization Service that creates rasterizer objects for use by XPSDrv filters. A rasterizer object takes an XPS Object Model (XPS OM) page object and creates a bitmap of a specified region of the page. The sample implements an XPSDrv filter (xpsrasfilter.dll) that can be inserted into the XPS Filter Pipeline. For each fixed page in an XPS document, the sample filter does the following:

-   Uses the XPS rasterization service to create a rasterizer object for the fixed page.
-   Partitions the fixed page into several horizontal bands.
-   Uses the rasterizer object to render each horizontal band as a bitmap image.

The Print Filter Pipeline is part of the XPS Print Path [Windows Print Path Overview](print.windows_print_path_overview). Fixed pages are sent as an XPS data stream from the XPS Spooler to the print filter pipeline. The print filter pipeline manager takes the XPS fixed page, calls each filter in the order defined in the pipeline configuration file, and then sends either Fixed Page OM objects or a data stream to each filter as required. The filters process the data and return either Fixed Page OM objects or a data stream back to the print filter pipeline manager. (See MSDN entry for Filter Pipeline Interfaces items IXpsDocumentProvider, IXpsDocumentConsumer, IPrintWriteStream, and IPrintReadStream.)

As a print filter pipeline service, the XPS Rasterization Service can be loaded into the filter pipeline when the pipeline is initialized by adding a filter service provider tag to the configuration XML file (for example, \<FilterServiceProvider dll="XpsRasterService.dll"/\>). The service is then available to be called by the filters when they are initialized and called by the print filter pipeline manager.

The XPS Rasterization Service operates as follows:

-   The calling filter initializes an instance of the rasterizer by passing in the XPS OM for the fixed page.
-   The calling filter calls the RasterizeRect method of the rasterizer to render a specified rectangle area of the fixed page.
-   RasterizeRect writes the WIC (Windows Imaging Component) bitmap data to memory. (The address is specified as a parameter to RasterizeRect.)

The default parameters in this sample are as follows:

-   Letter-sized physical page (can override in print ticket).
-   0.25-inch margins (creating an 8-inch by 10.5-inch imageable area).
-   Scaling is set to FitApplicationBleedSizeToImageableSize.
-   Destination resolution set to 96 dpi (can override in print ticket).

