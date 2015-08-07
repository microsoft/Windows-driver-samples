Sample KMDF Driver Implementing a WMI Data Provider
===================================================

WmiSamp WMI Provider is a sample KMDF driver that implements a WMI data provider.

## Universal Windows Driver Compliant
This sample builds a Universal Windows Driver. It uses only APIs and DDIs that are included in OneCoreUAP.

The sample demonstrates how to register the WMI providers and create provider instances for the Framework device object. It also illustrates how to handle the WMI queries sent to the device.

The **Firefly**, **PCIDRV**, and **Toaster** sample drivers also implement WMI data providers.

Installation
------------

In Visual Studio, you can press F5 to build the sample and then deploy it to a target machine. For more information, see [Deploying a Driver to a Test Computer](http://msdn.microsoft.com/en-us/library/windows/hardware/hh454834).

**Note** You can obtain redistributable framework updates by downloading the *wdfcoinstaller.msi* package from [WDK 8 Redistributable Components](http://go.microsoft.com/fwlink/p/?LinkID=226396). This package performs a silent install into the directory of your Windows Driver Kit (WDK) installation. You will see no confirmation that the installation has completed. You can verify that the redistributables have been installed on top of the WDK by ensuring there is a redist\\wdf directory under the root directory of the WDK, %ProgramFiles(x86)%\\Windows Kits\\8.0.

Testing
-------

To test the WmiSamp driver, run the generated WmiSamp.vbs script file. This will cause WMI to query all data blocks and properties, and put the result in a .log file. For more sophisticated testing, the VBScript can be extended by hand. The WBEMTest tool (located in %windir%\\system32\\wbem\\) can also be used.


WMI Mof Check Tool
------------------

WmiMofCk validates that the classes, properties, methods and events specified in a binary mof file (.bmf) are valid for use with WMI. It also generates useful output files needed to build and test the WMI data provider.

-   If the -h parameter is specified, a C language header file is created that defines the GUIDs, data structures, and method indices specified in the MOF file.

-   If the -t parameter is specified, a VBScript applet is created that will query all data blocks and properties specified in the .mof file. This can be useful for testing WMI data providers.

-   If the -x parameter is specified, a text file is created that contains the text representation of the binary .mof data. This can be included in the source of the driver if the driver supports reporting the binary .mof via a WMI query rather than a resource on the driver image file.

-   Usage: wmimofck -h\<C Header output file\> -x\<Hexdump output file\> -t\<VBScript test output file\> \<binary mof input file\>

**Note** A byproduct of compiling the .mof file is a .vbs file. This is a VBScript file that is run from the command line on the target machine running the new device driver. It will cause WMI to query all data blocks and properties, and put the results into a .log file. This can be very useful for testing WMI support in your driver. For more sophisticated testing, the VBScript can be extended by hand.

