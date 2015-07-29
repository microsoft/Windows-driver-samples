Windows Filtering Platform Sample
=================================

The WFPSampler sample driver is a sample firewall. It has a command-line interface which allows adding filters at various WFP layers with a wide variety of conditions. Additionally it exposes callout functions for injection, basic action, proxying, and stream inspection.

WFPSampler.Exe is the command-line interface used by the user to define the policy.

WFPSamplerService.Exe is the service which instructs BFE to add or remove policies.

WFPSamplerCalloutDriver.Sys is the driver which houses the various callout functions.

WFPSamplerProxyService.Exe is the service which listens for connections to proxy.

WFPSampler.Lib is a library of user mode helper functions used throughout the project.

WFPSamplerSys.Lib is a library of kernel mode helper functions used throughout the project.

"WFPSamplerInstall.cmd" will copy the necessary binaries to their appropriate location, and install each component.

"WFPSamplerInstall.cmd -r" will uninstall each component and remove the binaries from the appropriate location.

Once you have downloaded the sample, the .mht files in the sample's docs directory describe the various WFP filtering scenarios that you can try.

For more information about WFP callout drivers, see [Windows Filtering Platform Callout Drivers](http://msdn.microsoft.com/en-us/library/windows/hardware/ff571068).

Related topics
--------------

[Windows Filtering Platform Callout Drivers](http://msdn.microsoft.com/en-us/library/windows/hardware/ff571068)

Open the driver solution in Visual Studio
-----------------------------------------

Navigate to the folder that has the extracted sample. Double click the solution file, WFPSampler.sln. In Visual Studio, locate Solution Explorer. (If this is not already open, choose **Solution Explorer** from the **View** menu.) In Solution Explorer, you can see one solution that has 6 projects:

-   a user-mode application project named **WFPSampler** (under the **Exe** node)
-   a user-mode library project named **WFPSampler** (under the **Lib** node)
-   a package project named **package** (lower case) (under the **Package** node)
-   a user-mode service project named **WFPSamplerService** (under the **Svc** node)
-   a driver project named **WFPSamplerCalloutDriver** (under the **Sys** node)
-   a kernel-mode library project named **WFPSampler** (under the **Syslib** node)

Set the configuration and platform in Visual Studio
---------------------------------------------------

In Visual Studio, in Solution Explorer, right click **Solution ‘WFPSampler’ (6 projects)**, and choose **Configuration Manager**. Set the configuration and the platform. Make sure that the configuration and platform are the same for all three projects. Do not check the **Deploy** boxes. SHere are some examples of configuration and platform settings.

<table>
<colgroup>
<col width="33%" />
<col width="33%" />
<col width="33%" />
</colgroup>
<thead>
<tr class="header">
<th align="left">Configuration
Platform
Description</th>
</tr>
</thead>
<tbody>
<tr class="odd">
<td align="left">Win8.1 Debug
x64
The driver will run on an x64 hardware platform that is running Windows 8.1. The driver will not run on any earlier versions of Windows.</td>
<td align="left">Win7 Debug
x64
The driver will run on an x64 hardware platform that is running Windows 7 or a later version of Windows.</td>
</tr>
</tbody>
</table>

Set the KMDF version for the driver and kernel-mode library
-----------------------------------------------------------

The operating system that you specified in your configuration is called the *target operating system*. For example, if you specified Win7 Debug in your configuration, your target operating system is Windows 7. In Solution Explorer, right-click **WFPSamplerCalloutDriver** (under the **Sys** node), and choose **Properties**. Navigate to **Configuration Properties \> Driver Model Settings**. Set **KMDF Version Major** to 1. Set **KMDF Version Minor** according to your target operating system.

<table>
<colgroup>
<col width="50%" />
<col width="50%" />
</colgroup>
<thead>
<tr class="header">
<th align="left">Target operating system
KMDF minor version</th>
</tr>
</thead>
<tbody>
<tr class="odd">
<td align="left">Windows 7
9</td>
<td align="left">Windows 8
11</td>
</tr>
</tbody>
</table>

Repeat this process for the **WFPSampler** kernel-mode library (under the **Syslib** node).

Set the runtime library for the user-mode application, library, and service
---------------------------------------------------------------------------

In Solution Explorer, right-click the **WFPSampler** user-mode application project (under the **Exe** node), and choose **Properties.** Navigate to **Configuration Properties \> C/C++ \> Code Generation**. For **Runtime Library**, select **Multi-threaded Debug (/MTd)**. Click **OK**.

Repeat this process for the **WFPSampler** user-mode library (under the **Lib** node) and the **WFPSampler** user-mode service (under the **Svc** node).

Edit the restart setting in the sample installation script
----------------------------------------------------------

Open the WfpSamplerInstall.cmd file (in the scripts folder) in Visual Studio.

Change this line:

<table>
<colgroup>
<col width="100%" />
</colgroup>
<tbody>
<tr class="odd">
<td align="left"><pre><code>RunDLL32.Exe syssetup,SetupInfObjectInstallAction DefaultInstall 131 %WinDir%\System32\Drivers\WFPSamplerCalloutDriver.Inf</code></pre></td>
</tr>
</tbody>
</table>

to this:

<table>
<colgroup>
<col width="100%" />
</colgroup>
<tbody>
<tr class="odd">
<td align="left"><pre><code>RunDLL32.Exe syssetup,SetupInfObjectInstallAction DefaultInstall 132 %WinDir%\System32\Drivers\WFPSamplerCalloutDriver.Inf</code></pre></td>
</tr>
</tbody>
</table>

For more information about this setting, see the Remarks section for the [**InstallHinfSection**](http://msdn.microsoft.com/en-us/library/windows/hardware/aa376957) function.

Build the sample using Visual Studio
------------------------------------

In Visual Studio, on the **Build** menu, choose **Build Solution**.

For more information about using Microsoft Visual Studio to build a driver package, see [Building a Driver](http://msdn.microsoft.com/en-us/library/windows/hardware/ff554644).

Locate the built driver package
-------------------------------

In File Explorer, navigate to the folder that contains your built driver package. The location of this folder varies depending on what you set for configuration and platform. For example, if your settings are Win7 Debug and x64, the package is in your sample folder under x64\\Win7Debug\\package.

The package folder contains these 4 files:

<table>
<colgroup>
<col width="50%" />
<col width="50%" />
</colgroup>
<thead>
<tr class="header">
<th align="left">File
Description</th>
</tr>
</thead>
<tbody>
<tr class="odd">
<td align="left">wfpsamplercalloutdriver.cat
A signed catalog file, which serves as the signature for the entire package.</td>
<td align="left">WFPSamplerCalloutDriver.inf
An information (INF) file that contains information needed to install the driver.</td>
</tr>
</tbody>
</table>

**Note**  

The build process might also put WdfCoinstaller010*xx*.dll in the package folder, but this file is not really part of the driver package. The INF file does not reference any coinstallers. The driver package, which is test-signed, actually contains only three files:

-   wfpsamplercalloutdriver.cat
-   WFPSamplerCalloutDriver.inf
-   WFPSamplerCalloutDriver.sys

Because the package does not contain a KMDF coinstaller, it is important that you set the KMDF minor version (as described previously) according to your target operating system when you built the driver.

Locate the symbol file (PDB) for the driver
-------------------------------------------

In **File Explorer**, locate the symbol file, WFPSamplerCalloutDriver.pdb. The location of this file varies depending on what you set for configuration and platform. For example, if your settings are Win7 Debug and x64, the PDB file is in your sample folder under sys\\x64\\Win7Debug.

Locate the user-mode application and its symbol file (PDB)
----------------------------------------------------------

In **File Explorer**, locate the user-mode application (WFPSampler.exe) and its symbol file (WFPSampler.pdb). The location of these files varies depending on what you set for configuration and platform. For example, if your settings are Win7 Debug and x64, WFPSampler.exe and WFPSampler.pdb are in your sample folder under exe\\x64\\Win7Debug.

Locate the kernel-mode service and its symbol file (PDB)
--------------------------------------------------------

In **File Explorer**, locate the kernel-mode library, WFPSamplerService.exe. The location of this file varies depending on what you set for configuration and platform. For example, if your settings are Win7 Debug and x64, WFPSamplerService.exe and WFPSamplerService.pdb are in your sample folder under svc\\x64\\Win7Debug.

Run the sample
--------------

The computer where you install the driver is called the *target computer* or the *test computer*. Typically this is a separate computer from where you develop and build the driver package. The computer where you develop and build the driver is called the *host computer*.

The process of moving the driver package to the target computer and installing the driver is called *deploying the driver*. You can deploy the Windows Filtering Platform Sample driver automatically or manually.

Automatic deployment
--------------------

Before you automatically deploy a driver, you must provision the target computer. For instructions, see [Configuring a Computer for Driver Deployment, Testing, and Debugging](http://msdn.microsoft.com/en-us/library/windows/hardware/). After you have provisioned the target computer, continue with these steps:

1.  On the host computer, in Visual Studio, in Solution Explorer, right-click **package** (lower case), and choose **Properties**. Navigate to **Configuration Properties \> Driver Install \> Deployment**.
2.  Check **Enable deployment**, and check **Remove previous driver versions before deployment**. For **Target Computer Name**, select the name of a target computer that you provisioned previously. Select **Do not install**. Click **OK**.
3.  In the **Build** menu, choose **Build Solution**.
4.  Copy the following files to the DriverTest\\Drivers folder on the target computer:
    -   The user-mode application (WFPSampler.exe) file
    -   The kernel-mode service (WFPSamplerService.exe) file

Manual deployment
-----------------

Before you manually deploy a driver, you must turn on test signing and install a certificate on the target computer. You also need to copy the [DevCon](http://msdn.microsoft.com/en-us/library/windows/hardware/ff544707) tool to the target computer. For instructions, see [Preparing a Computer for Manual Driver Deployment](http://msdn.microsoft.com/en-us/library/windows/hardware/dn265571). After you have prepared the target computer for manual deployment, copy the following files to a folder on the target computer (for example, c:\\WFPSamplerSamplePackage):

-   The 4 files in your driver package folder
-   The user-mode application (WFPSampler.exe) file
-   The kernel-mode service (WFPSamplerService.exe) file

Copy additional files to the target computer
--------------------------------------------

Copy the driver's PDB file (WFPSamplerCalloutDriver.pdb), the user-mode service's PDB file (WFPSamplerService.pdb) and the user-mode application's PDB file (WFPSampler.pdb) to a folder on the target computer (for example, c:\\Symbols).

Copy the [TraceView](http://msdn.microsoft.com/en-us/library/windows/hardware/ff553872) and [**SignTool**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff551778) tools to a folder on the target computer (for example c:\\Tools).

-   [TraceView](http://msdn.microsoft.com/en-us/library/windows/hardware/ff553872) comes with the WDK. You can find it in your WDK installation folder under Tools (for example, c:\\Program Files (x86)\\Windows Kits\\8.1\\Tools\\x64\\TraceView.exe).
-   [**SignTool**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff551778) also comes with the WDK. You can find it in your WDK installation folder under bin (for example, c:\\Program Files (x86)\\Windows Kits\\8.1\\bin\\x64\\SignTool.exe).

Installing the driver
---------------------

1.  On the target computer, open a Command Prompt window as Administrator. Navigate to the folder that contains the installation script:
    -   For manual deployment, this will be the folder that you copied the driver page files into (for example, c:\\WFPSamplerSamplePackage).
    -   For automatic deployment, this will be DriverTest\\Drivers.

2.  Enter **WFPSamplerInstall.cmd** to run the installation script.
    **Note**  If you need to uninstall a previous version of the driver, enter **WFPSamplerInstall.cmd -r**.

Running the user-mode application
---------------------------------

On the target computer, open a Command Prompt window as Administrator.

If you just want to see whether you can run the application, enter **WFPSampler.exe -?**.

The .mht files in the docs directory describe the various WFP filtering scenarios that you can try.

For example, you can test the basic packet examination scenario by using the following command line:

**WFPSampler.exe -s BASIC\_PACKET\_EXAMINATION -l FWPM\_LAYER\_INBOUND\_IPPACKET\_V4 -v**

This command line adds a dynamic filter (-v) at the FWPM\_LAYER\_INBOUND\_IPPACKET\_V4 layer (-l) which references the appropriate callout driver function. This filter will have no conditions, so it will act on all traffic seen at this layer.

Start a logging session in TraceView
------------------------------------

On the target computer, open TraceView.exe as Administrator. On the **File** menu, choose **Create New Log Session**. Click **Add Provider**. Select **PDB (Debug Information File)**, and enter the path to your PDB file, WFPSamplerCalloutDriver.pdb. Click **OK** and click **Next**. Click the **\>\>** button next to **Set Flags and Level**, double-click the **L** button next to **Level**, and set the **Level** to **Information**. Click **OK** and click **Finish**.

If you want to test whether your TraceView.exe session is working, you can enter the following commands and see what the trace output looks like:

-   **net stop WFPSamplerCallouts**
-   **net start WFPSamplerCallouts**

For more information, see [Creating a Trace Session with a PDB File](http://msdn.microsoft.com/en-us/library/windows/hardware/ff543582).

Tracing for the sample driver can be started at any time before the driver is started or while the driver is already running.

