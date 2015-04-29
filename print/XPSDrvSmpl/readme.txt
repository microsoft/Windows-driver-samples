Print Features Filter Sample Driver
===================================

Sample History:
===============

Windows 7:
The sample has been changed such that the DllGetClassObject() function for each filter
returns an instance of IClassFactory instead of an instance of IPrintPipelineFilter. This
is in agreement with the updated MSDN documentation surrounding XpsDrv filters.

Note: As reflected in the MSDN documentation, DllGetClassObject() functions that return
        IPrintPipelineFilter will still be compatible with the Print Pipeline in order to
        maintain compatibility with existing XpsDrv filters.
        
Also, the sample now uses the PrintSchema keyword PageSourceColorProfile to store the
color matching profile in the PrintTicket.


Contents:
=========

install
    Install directory. Contains driver gpd and install files and is where driver DLLs are published.

src\common
    Code common to both the filter and config modules. This is mostly common print ticket handling code.

src\debug
    Common debug code.

src\filters
    Precompiled header and common sources information.

src\filters\booklet
    Booklet filter specific code.

src\filters\color
    Color filter specific code.

src\filters\common
    Code common to all filters.

src\filters\nup
    NUp filter specific code.

src\filters\scaling
    Scaling filter specific code.

src\filters\watermark
    Watermark filter specific code.

src\filters\xdcont
    Sources for XPS container handling. This code is used with the stream interface to process the XPS container.

src\inc
    Common include files.

src\ui
    Config plug-in code. This provides the custom UI and PrintTicket provider support.



