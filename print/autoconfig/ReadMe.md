Print Auto Configuration Sample
===============================

The auto configuration sample demonstrates how to implement Unidrv-based and PScript5-based drivers to leverage the inbox support for auto-configuration. The sample works only when used with the standard TCP/IP port monitor or the Network-Connected Device (NCD) port monitor.

**Auto-Configuration Basics**

Prior to Microsoft® Windows® Vista, the settings of a print queue were set initially to the driver's default settings, rather than to the appropriate settings based on the device. For Unidrv or Pscript5, this means that static default values specified in the GPD or PPD file are used for the initial print queue setup. This static set of defaults must, necessarily, represent the minimum configuration that a printer can ship with. For instance, if a stapling unit is optional for a device, then by default, such a device cannot have stapling capability enabled, otherwise the user interface for devices without the stapling unit would show stapling as an option. A customer who selected the stapling option but found that it did not work would likely be confused.

For any device that comes with features not present in the basic model, a user or administrator must manually configure these features on the print queue after installation. This can at times be a confusing and non-intuitive experience. The configuration process is easy to get wrong, particularly with regard to internal parameters such as memory and hard disk size, which significantly can affect printing speed and quality.

Auto-configuration solves this problem by automatically configuring the print queue according to the installable features on the device, rather than simply using the driver's static default settings. The main target for auto-configuration is network printers. They are the most likely ones to have multiple optional features and require more manual configuration. For more information, see [Printer Autoconfiguration](http://msdn.microsoft.com/en-us/library/windows/hardware/ff560774(v=vs.85).aspx).


Build the sample
----------------

The auto configuration sample doesn’t have any binaries to be built. It may be installed by using **Add Printer Wizard** and supplying the AutoCnfg.INF as the INF file.

But to build a signed driver package using Windows 8.1 driver kit (Windows Driver Kit (WDK)) and Visual Studio 2013, for the project file (csproj) that ships with the auto configuration sample, perform the following steps.

1. Open the solution file in Visual Studio 2013

2. Add all non-binary files (usually located in the \\install directory of the sample) to the Package project

a. In the **Solution Explorer**, right click **Driver Files**

b. Select **Add**, then click **Existing Item**

c. Navigate to the location to which you downloaded the sample, and select all the files in the install directory, or the equivalent set of non-binary files such as INFs, INIs, GPD, PPD files, etc.

d. Click **Add**

3. Configure these files to be added into the driver package

a. In the **Solution Explorer**, right click the Package project and select **Properties**

b. In the left pane, click **Configuration Properties** \> **Driver Install** \> **Package Files**.

c. In the right pane, use the ellipsis button (...) to browse to the set of files that needs to be added to the driver package. All the data files that you added in **Step 2-c**, except the INF file, should be added.

**Note**  This configuration is per-architecture, so this configuration must be repeated for each architecture that will be built.

d. Click **OK**

4. Open the INF file and edit it to match the built output

a. Open the INF file

b. In the Version section, add a reference to a catalog file like this: CatalogFile=XpsDrvSmpl.cat

c. In the SourceDisksFiles section, change the location of the DLL files you are building, to =1. This indicates that there is no architecture specific directory in this driver. If you ship multiple architectures simultaneously, you will need to collate the driver INF manually.

At this point, Visual Studio 2013 will be able to build a driver package and output the files to disk. In order to configure driver signing and deployment, see [Developing, Testing, and Deploying Drivers](http://msdn.microsoft.com/en-us/library/windows/hardware/ff554651(v=vs.85).aspx).

For more information about how to build a driver solution using Microsoft Visual Studio, see [Building a Driver](http://msdn.microsoft.com/en-us/library/windows/hardware/ff554644).

Run the sample
--------------

To test the auto-configuration feature you need to install the driver to a Standard TCP/IP port. You need a printer, which understands and responds to the bidi SNMP queries, connected to the port. The tcpbidi.xml file located in system32 directory has information about the SNMP OIDs used for each query. The installable options in the device settings will reflect the information obtained by querying the printer.

