XPSDrv Driver and Filter Sample
===============================

This sample is intended to provide a starting point for developing XPSDrv printer drivers and to illustrate the facility and potential of an XPSDrv print driver. This goal is accomplished by implementing a number of real-world features within a set of XPS print pipeline filters that are configured through a configuration plug-in that supports custom UI content and PrintTicket handling.

Windows includes a print architecture and a document format known as XPS (XML Paper Specification). Part of the new architecture is the XPSDrv print driver, which is designed to provide a flexible, extensible path to manipulate and print an XPS spool file through a series of filters.

This sample is intended to provide a starting point for developing XPSDrv printer drivers and to illustrate the facility and potential of an XPSDrv print driver. This goal is accomplished by implementing a number of real-world features within a set of XPS print pipeline filters that are configured through a configuration plug-in that supports custom UI content and PrintTicket handling.

The sample broadly consists of three components: a set of filters, a configuration plug-in for handling custom UI content, and a configuration plug-in for handling more advanced PrintTicket features. For more information, see [XPS Printing Features](http://msdn.microsoft.com/en-us/library/windows/hardware/ff564299(v=vs.85).aspx).


Build the sample
----------------

To build a driver solution using Windows Driver Kit (WDK) 10 and Visual Studio 2015, perform the following steps.

1. Open the solution file in Visual Studio 2015.
2. Add all non-binary files (usually located in the \\install directory of the sample) to the Package project:
  a. In the **Solution Explorer**, right click **Driver Files**
  b. Select **Add**, then click **Existing Item**
  c. Navigate to the location to which you downloaded the sample, and select all the files in the install directory, or the equivalent set of non-binary files such as INFs, INIs, GPD, PPD files, etc.
  d. Click **Add**
3. Configure these files to be added into the driver package:
  a. In the **Solution Explorer**, right click on the solution and choose **Add** > **New Project**. Choose **Driver Install Package** under Visual C++/Windows Driver/Package.
  b. In the **Solution Explorer**, right click the Package project and select **Properties**.
  c. In the left pane, click **Configuration Properties** \> **Driver Install** \> **Package Files**.
  d. In the right pane, use the ellipsis button (...) to browse to the set of files that needs to be added to the driver package. All the data files that you added in **Step 2-c**, except the INF file, should be added. This configuration is per-architecture, so this configuration must be repeated for each architecture that will be built.
  e. Click **OK**.
4. Open the INF file and edit it to match the built output.
  a. Open the INF file.
  b. In the Version section, add a reference to a catalog file like this: CatalogFile=XpsDrvSmpl.cat.
  c. In the SourceDisksFiles section, change the location of the DLL files you are building, to =1. This indicates that there is no architecture specific directory in this driver. If you ship multiple architectures simultaneously, you will need to collate the driver INF manually.

At this point, Visual Studio 2015 will be able to build a driver package and output the files to disk. In order to configure driver signing and deployment, see [Developing, Testing, and Deploying Drivers](http://msdn.microsoft.com/en-us/library/windows/hardware/ff554651(v=vs.85).aspx).

**Note** If you compile your sample driver with Microsoft Visual Studio version 10, or 11 with the \_DEBUG flag set, then you should not use CComVariant on the following two XPS Print Filter Pipeline properties:

-   XPS\_FP\_USER\_TOKEN
-   XPS\_FP\_PRINTER\_HANDLE

There is a known issue with the current implementation of the Print Filter Pipeline, where the variant type for these two properties is set to VT\_BYREF. And as a result of this known issue, any filter binary that is compiled with the \_DEBUG flag set will experience the ATLASSERT() failure. This is because when you use the CComVariant, its destructor checks the returned value from the Clear() function, as shown:


```c_cpp
~CComVariant() throw()
{
   HRESULT hr = Clear();
   ATLASSERT(SUCCEEDED(hr));
   (hr);
}
```

When you compile this sample driver with Visual Studio version 9, you don't experience this problem because the destructor for CComVariant doesn't perform this check on the returned value from the Clear() function.

Installation
------------

The sample has the following prerequisites:

-   Microsoft XPS Document Writer print driver and the XPS filter-pipeline infrastructure.
-   Microsoft MSXML 6.0

Install the driver through the **Add Printer Wizard** by selecting \<driver root\>\\install as the source for the driver install.

Design and Operation
--------------------

### Overview

This sample can be used as a basis for implementing a driver based on the new XPS pipeline infrastructure.

The Page Scaling filter is written by using the stream interface to attempt to demonstrate how to use the stream interface. It thus depends on a ZIP library to handle the PK archive structure. The sample does not include the code for the PK archive handling.

Two interfaces are defined in *ipkarch.h* and *ipkfile.h* that need support from an additional PK archive handling module called pkarch.dll. Pkarch.dll is a file that is included in the [PKWare SDK](http://www.pkware.com/software/developer-tools/sdk/pkzip-standard-toolkit). If this module is not present, the page scaling filter sample will revert to merely copying the data from the read stream to the write stream. Developers who are using this sample can choose one of the following options:

-   Simplify the scaling filter to use the XPS interfaces (like the other four filters)
-   License the third-party zip library that is used in the sample
-   Modify the sample to use another ZIP library. For example, you can modify the sample to use the [Packaging API Reference](http://msdn.microsoft.com/en-us/library/windows/desktop/dd371643(v=vs.85).aspx)

IPKArch defines an interface for initializing, controlling, and accessing the PK archive. IPKFile defines an interface that abstracts the details of a PK archive file header record from the XPS container handling. Access to the files within the archive is provided through a map between the file name and file objects that support the IPKFile interface. This allows the XPS processing code to retrieve file data by name (a convenience as the interaction between parts and relationships between parts is defined using the part name).

### Print Pipeline Filters

There are five filters that are split into two types: four use the XPS filter interface and one uses the stream filter interface. The XPS interface provides the filter writer with a logical view on an XPS document by presenting logical XPS document parts (XPS Document, Fixed Document Sequences, Fixed Documents, and Fixed Pages) to the filter in a known and defined order. The stream interface simply provides a stream that contains the XPS document--a zip archive. The filter writer is in this case required to handle the PK archive structure, decompression of the parts within the container, and the XPS Open Packaging conventions.

The four filters that use the XPS interface provide support for the following:

-   The Watermark Filter is responsible for adding mark-up to Fixed Page content to express textual, bitmap, and vector-based watermarks.
-   The Booklet Filter is responsible for page re-ordering and padding page insertion to create booklets from the XPS document. Note that this filter re-uses the NUp filter to provide appropriate page transformation.
-   The NUp Filter is responsible for transforming and combining logical pages onto physical pages to provide multiple page per sheet support.
-   The Color Management Filter is responsible for constructing and applying color transforms to Fixed Page content.

The stream interface filter provides Page Scaling support (that is, wrapping content with the appropriate transforms to scale from a source Fixed Page to the destination).

### UI Plug-in

The UI plug-in provides support for controlling features that are not supported by the Unidrv core UI. Three additional printer property pages are implemented that provide color management, watermarking, and general features. The UI plug-in also provides custom Devmode support for options that are not easily expressed in a GPD file (for example, the numeric values required to define custom page scaling options).

### Print Ticket Provider Plug-in

Unidrv provides support for mapping simple features and options from a GPD file to a Print Ticket by a "PrintSchemaKeywordMap" keyword in the GPD file. Many of the features that are supported by the sample filters require a more sophisticated mapping between the GPD description and the Print Schema definition. These features include features that are nested within features and control of numeric values. The Print Ticket provider plug-in provides this support by converting a number of GPD feature options to a single more complicated Print Ticket feature. For example, the Print Schema definition of PageScaling requires numeric values for custom page scaling options (offset and scale values) as well as a sub-feature defining the scaling offset option. This plug-in can take settings from a "flat" description of the scaling and offset alignment features in the GPD combined with custom DevMode values configured by the UI plug-in to generate an appropriate PrintTicket construct that conforms to the PrintSchema definition of PageScaling.

The components of the XPSDrv sample enable a user or application to configure the filter pipeline to process an XPS container according to the supported filters. The XPS spool file that is passed to the filter pipeline broadly comprises two components: the document structure and page content as well as one or more Print Tickets that configure the print job at various levels. Job content will be defined either as output directly from a Windows Presentation Foundation (WPF) application or through a conversion from legacy GDI to XPS through the Microsoft XPS Document Writer (MXDW). PrintTicket settings can be controlled directly through the UI or through application settings in either DevMode or PrintTicket.

To enable configuration of the driver, the sample uses a set of GPD files and a configuration plug-in. The configuration plug-in provides custom DevMode and custom UI support. This support enables the driver to store and configure driver specific settings that can be used as a source for configuring PrintTicket features. In order to provide the appropriate PrintTicket support, the driver makes use of a combination of the core Unidrv PrintTicket support and a PrintTicket provider plug-in to enable the generation of more sophisticated PrintTicket settings. In combination these allow an application or user to setup a PrintTicket ready for inclusion in an XPS document.

With the PrintTicket in place and the document content supplied by either an application using WPF or a legacy application through MXDW, the filters that make up the filter pipeline for the sample driver act on the XPS document according to the settings specified in the PrintTicket. Each filter checks whether its functionality has been enabled and extracts any settings relevant to its function before processing the container appropriately. The order in which filters are applied is important to the eventual output; for example, a filter adding page content (like the watermark filter) can be placed anywhere in the pipeline, but its placement determines where the content is added. Configured to run before an NUp filter ensures page content is confined to the logical page; whereas, if it is run after an NUp filter, the watermark will be applied to the transformed pages. The filter ordering is controlled by the filter configuration file; this file is an XML document that details the order and interface type for each filter.

### Common Filter Functionality

Regardless of the type of interface that a filter uses to process the XPS document content, there are common requirements for all filters. These include filter initialization and shutdown and are provided as a base class from which either an XPS or stream interface filter can derive.

### XPS Interface

Four of the five filters make use of the XPS interface that is provided by the filter pipeline manager as XPS provider and consumer interfaces. The XPS provider interface supplies XPS parts to the filter on demand. When a part is requested from the provider, a generic interface pointer is returned. This interface is queried to identify its type: an XPS document, a Fixed Document Sequence, a Fixed Document or a Fixed Page. This functionality is common between filters that use the XPS interface and a base class XPS interface implementation (that derives from the common filter class described earlier) is provided that retrieves, identifies, and dispatches the part to the appropriate handler. When all parts have been processed, the base class calls a method to finalize the filter. To supply the filter-specific functionality, a filter derives from the base class and implements handlers for any of the part types it requires and/or the finalize method to complete its function. Additionally, the base class provides default implementations for the XPS part handlers (using these exclusively merely passes the document on through the XPS consumer interface) and a method for initializing the XPS interface.

### Stream Interface

One of the filters (page scaling) makes use of the stream interface that is provided by the filter pipeline manager as a read and write stream. This interface is a very basic interface that merely supplies the filter with a stream that contains the XPS document as a PK archive. To be able to use the stream interface to modify document content, the filter needs to be able to process the PK archive to retrieve all of the files contained within, decompress the data associated with these files, process the open packaging conventions (taking into account potentially interleaved data), and compress and send the data in a structure appropriate to both the open package and PK archive conventions. A class is provided that uses an XPS document processor for accessing the logical structure of the package and reporting the Fixed Page content to the filter. This class supports the necessary interfaces to initialize the IO streams, start the process of handling the data, and a default handler for processing Fixed Page content. The page scaling filter derives from this class and implements its own Fixed Page handler that is called by the XPS processor.

### Watermark Filter

The Watermark filter is intended to demonstrate adding presentation content to the Fixed Pages within an existing XPS container by using the XPS interface as the source of the XPS document. This added content acts as a watermark and is configured by the PrintTicket. The filter takes as input the PageWatermark public Print Schema feature with custom support for vector and bitmap watermark types and the Fixed Pages in an XPS document. This custom content is expressed as Private options to the existing PageWatermark definition. The output is the sequence of Fixed Pages and accompanying resources required for the watermark.

The filter is configured by parsing an input PrintTicket by using DOM to extract the relevant features and options. If the functionality is enabled, the filter processes the document as required. The PrintTicket is constructed through the following algorithm to provide settings at the appropriate scope (Fixed Document Sequence, Fixed Document and Fixed Page):

1.  Validate and merge the print ticket from the FDS with the default PrintTicket converted from the default DevMode in the property bag. The resultant ticket will be the Job level ticket.
2.  Validate and merge the print ticket from the current FD with the Job level ticket from step 1. The resultant ticket will be the document level ticket.
3.  Validate and merge the print ticket from the current FP with the Doc level ticket from step 2. The resultant ticket will be the page level ticket.

When a watermark is enabled in the PrintTicket, the filter creates a watermark of the appropriate type (text, raster, or vector). This is returned as a generic watermark interface that abstracts the watermark type from the filter. The filter then calls the watermark object to send any resources that it may require to the filter pipeline (the font for text, the bitmap for raster, and none for vector). All resources are added through a resource cache that enables the watermark object to ignore any problems with sending repeated resources (the cache checks if the resource is present and only sends it if it has not seen the resource before). With the resource in place, the filter instantiates a SAX handler passing the watermark object. The SAX handler is used to parse the Fixed Page, allowing the filter to control when it inserts the watermark into the Fixed Page; when underlay is required, the mark-up is inserted when the FixedPage start element is encountered and when overlay is required the mark-up is inserted when FixedPage end element is encountered. By passing the abstracted watermark object, the SAX handler can be re-used for any watermark as it merely requests appropriate mark-up from the watermark object and inserts it into the Fixed Page mark-up as appropriate.

### Vector Mark-up

XPS documents can contain a range of vector elements that contain color data that describes how the elements should be rendered on a specific device. The following elements support color data:

-   Color
-   Fill
-   Stroke

When any of these elements are found in a fixed page, the SAX handler passes the associated color data to a color conversion object. This uses the following algorithm to convert the color:

1.  The XML mark-up is broken down into color channel values, color channel value types, color format, and any other color details contained in the color mark-up string.
2.  The resulting values are color transformed in conjunction with preferences defined in the PrintTicket. The result is a new set of color values.
3.  These new values are used to reconstructs a color reference containing the new color values.
4.  The newly constructed color reference is used in place of the original color reference, resulting in the transformed color output.

### Bitmap Resources

When a bitmap is referred to in a fixed page, the SAX handler passes the bitmap URI to a bitmap conversion method in the color converter object.

The conversion process starts by creating a new color managed bitmap object that represents the bitmap along with a color profile manager. The color profile manager is initialized by the PrintTicket settings and is used to supply a suitable color transform based on those settings.

After the color managed bitmap object is created, the object is passed to a resource caching class that manages which bitmaps are written out and ensures bitmaps are only written out once. A single bitmap can be referred to many times in an XPS container but the bitmap itself should have a color transformation applied only once and should be written out only once to avoid unnecessary processing overhead. The caching manager ensures this by creating a unique key generated from the bitmap URI and color profile that is then recorded against the URI. If the caching manager receives bitmap objects with an existing key, those bitmaps do not need re-processing and the stored URI is used merely to identify the color transformed bitmap.

If a bitmap has not been handled yet, the caching manager calls a write data method in the color bitmap object to indicate that the bitmap should write itself out to the filter pipeline. The write process also includes the application of a color transform to the bitmap. The following steps occur to apply the transform:

1.  A stream is created to the bitmap itself and the bitmap is loaded into memory.
2.  A bitmap codec object is created that takes the bitmap and uses an appropriate codec to decompress the bitmap and present the bitmap data and values.
3.  The bitmap data is then converted by using a color transform supplied by the color profile management class.
4.  The bitmap codec object re-encodes the bitmap by using a matching codec to that used to decode the bitmap.
5.  The encoded bitmap is streamed back out to the container.

### Booklet Filter

The Booklet filter is intended to demonstrate how a filter can re-order the pages and add additional pages to a document to enable booklet binding using the XPS interface as the source of the XPS document. Page transformation is not applied by the filter but deferred to the NUp filter demonstrating filter re-use. The filter takes as input the JobBinding and DocumentBinding public Print Schema features (defined in the PrintTicket), and the sequence of pages in the fixed documents or fixed document sequence within the XPS document and outputs the fixed pages in an appropriate order to be printed as a booklet. Page content is not modified; however, an additional fixed page might be required to ensure the appropriate fixed page flow.

The filter is configured by parsing an input PrintTicket by using DOM to extract the relevant features and options. If the functionality is enabled, the filter processes the document as required. Again the PrintTicket is constructed according to the algorithm that is documented in the watermark filter notes.

The booklet filter maintains a list of references to the fixed page parts within an XPS document as they are presented to the filter. This list is used to output the correct page order and is reset and repopulated according to whether JobBinding or DocumentBinding is set. If JobBinding is enabled, all pages within the Fixed Documents that make up the Fixed Document Sequence are cached and the list is not flushed until the document has completed. If DocumentBinding is enabled, the filter caches all pages in a Fixed Document and flushes the list at the end of the Fixed Document. When the list is flushed, the pages are re-ordered and a padding page is added if the total page count is odd before being sent on to the filter pipeline.

### NUp Filter

The NUp filter is intended to demonstrate how a filter can transform vector mark-up within an XPS container by using the XPS interface as the source of the XPS document. The PrintTicket might contain preferences that the filter uses to apply a multi-up transformation to the containers fixed pages, resulting in a new set of pages that contain the original pages as child canvases. The filter takes as input the JobNUp, DocumentNUp, JobBinding, DocumentBinding, PageMediaSize, and PageOrientation public Print Schema features (defined in the PrintTicket) and the Fixed Pages that define the Fixed Document and Fixed Document Sequence. The output is a new sequence of Fixed Pages that contain the render-transformed source pages sent through the XPS interface to the filter pipeline.

The filter is configured by parsing an input PrintTicket using DOM to extract the relevant features and options. If the functionality is enabled, the filter processes the document as required. Again the PrintTicket is constructed according to the algorithm that is documented in the watermark filter notes.

Within the filter, SAX is used to parse the XPS container with each fixed page being read, amended, and inserted into a new fixed page as a canvas. The amendment of the source fixed page involves removing any wrapping mark-up that defines a fixed page and in place adding suitable mark-up to define a new canvas. The new canvas mark-up includes a transformation matrix that positions, rotates, and scales the canvas on the new fixed page. The transformation is provided by a supporting object that generates the transform according to the PrintTicket settings. Settings in the print ticket that are used include how many source pages should be stored as canvases in a single new fixed page (NUp count), the ordering of canvases on the new fixed page, and the target page size and orientation.

New canvases are added to the new fixed page mark-up until the document contains the number of source fixed pages that are required in a single new fixed page or until there are no source pages left. When the new fixed page is full, it is written out to the pipeline and another fixed page is started ready for population with source pages.

In addition to the NUp feature processing, the NUp filter is re-used to apply the 2-Up processing required for booklet printing. This is achieved by looking for a valid binding option in the PrintTicket and applying 2-Up as appropriate.

Page Scaling Filter
-------------------

The page scaling filter is intended to demonstrate how a filter can transform vector mark-up within a XPS container by using the stream interface as the source of the XPS document. The PrintTicket can contain page scaling preferences that the filter uses to amend page dimension values in each fixed page processed.

Within the filter, SAX is used to parse the XPS container with each fixed page being read, amended, and written back out. The modification of the fixed page mark-up begins by modifying the fixed page dimensions to match the target page size specified in the print ticket for the current fixed page. Subsequently, the fixed page content is scaled to correctly fit the target scale by applying a canvas around the source content that includes a transformation matrix. The SAX handler is supported by a page scale class which manages the creation of a transformation matrix and the presentation of the matrix correctly formatted for use in the fixed page.

XPS Container Handling
----------------------

The stream filter is required to handle the Open Packaging conventions that are used by an XPS document. These conventions can be thought of as the document structure above that of the PK archive itself. Sources for a set of classes are provided that support processing of the XPS document in terms of the constituent files in the PK archive. This includes:

-   Initiating the document processing based on the root relationships part defined in the package.
-   Validation of the parts within the package against their content type and usage.
-   Processing of Fixed Document Sequence and all resources associated with it.
-   Processing of the Fixed Documents within the Fixed Document Sequence and all resources associated with them.
-   Processing of the Fixed Page parts with the Fixed Documents and all resources associated with them.
-   Passing Fixed Page content processing to a registered FP handler for modification.
-   Passing all parts on to a PK archive handling module with valid ordering.

The XPS processor is not responsible for extracting or decompressing part data from the PK archive. This task is the responsibility of an additional module that implements an interface known to the XPS container handling code.

UI Plug-in
----------

The UI Plug-in is intended to demonstrate how to extend the standard Unidrv UI to add additional property sheets and controls and to provide support for custom features that are not supported by the core Unidrv UI. Three new pages have been added that allow control of the sample filters:

-   Color--enables configuration of the color conversion filter.
-   Watermarks--enables a watermark to be selected and enable configuration of its properties for use in the watermark filter.
-   Features--includes controls to configure the page scaling, booklet, and NUp filters. In addition, the standard driver settings--duplex, intent, and page borders--can also be modified.

Various UI control types have been implemented on these property pages to enable a user to modify settings in the driver. The UI supports the following control types:

-   Check-box
-   Combo-box
-   List-Box
-   Edit-Box
-   Edit number (with buddy up/down control)

The settings that associated with each of these controls are stored as one the following types:

-   GPD options. These settings are defined in the sample driver GPD configuration file and are handled by the Unidrv core via the CPSUI interface. All GPD options that are managed from the UI plug-in are hidden from the standard Unidrv Treeview dialog box.
-   OEM private DevMode. These settings are settings that cannot be represented in the GPD file format (for example, numerical values and strings). They are instead added to the DevMode at a private offset.

The Unidrv UI Plug-in interface includes methods that allow additional property pages to be inserted into the drivers property sheet. In the sample, only additional document property pages are implemented because there are no device property pages required. A collection of property page objects are stored at the plug-in interface level, each of which is responsible for the three property pages (Color, Watermarks, and Features). Each property page object creates a Microsoft Windows property page from a dialog resource and handles all Windows Messages that are received through a common dialog procedure. A collection of UI control objects are stored in the property page, one for each Windows control on the page.

When a property page receives a message for a control, it looks up the relevant control in the collection (by using the resource identifier) and calls the appropriate method in the control interface, given the Windows message. Each control object is responsible for handling its own initialization and any user input through the appropriate get and set functions. The sample get and set functions provide an interface to read and update the GPD options and OEM private DevMode.

PrintTicket Provider Plug-in
----------------------------

The sample Print Ticket Provider plug-in is intended to demonstrate how to validate and map driver settings from a DevMode description to a Print Ticket description and back again. The driver allows configuration of custom settings through a custom user interface. These custom settings are represented as either GPD options or in the OEM private DevMode and are mapped to and from the Print Ticket using an XML schema.

To support Print Ticket handling, the Unidrv UI Plug-in Print Ticket Provider Interface is utilized. The Unidrv UI Plug-in interface includes methods that allow mapping of DevMode to and from a Print Ticket Schema. A collection of feature conversion objects are stored at the plug-in interface level, each of which is responsible for the conversion of a specific feature (Color, Watermark, Booklet, Scaling, and NUp). Whenever the Unidrv core calls through the external interface of the plug-in to convert from DevMode to Print Ticket or Print Ticket to DevMode, the collection is iterated through calling each conversion object in turn; each then perform its own DevMode/Print Ticket mapping via the appropriate get and set functions. The sample get and set functions provide an interface to read and write the GPD options, OEM private DevMode, and Print Ticket keyword value pairs.

