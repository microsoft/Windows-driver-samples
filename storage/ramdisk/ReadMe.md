RAMDisk Storage Driver Sample
=============================

The RAMDisk storage driver sample demonstrates how to write a software only function driver using the Kernel Mode Driver Framework (KMDF). This driver creates a RAM disk drive.The RAM disk can be used like any other disk, but the contents of the disk will be lost when the computer is shut down.

Build the sample
----------------

### Open the driver solution in Visual Studio ###

In Visual Studio, open the solution file, ramdisk.sln, and locate Solution Explorer (if this is not already open, choose **Solution Explorer** from the **View** menu). In Solution Explorer, you can see one solution that has two projects. There is a driver project named **WdfRamdisk** and a package project named **package** (lower case).

### Set the configuration and platform in Visual Studio

In Visual Studio, in Solution Explorer, right click **Solution 'ramdisk'(2 projects)**, and choose **Configuration Manager**. Set the configuration and the platform. Make sure that the configuration and platform are the same for both the driver project and the package project. Do not check the **Deploy** boxes. Here are some examples of configuration and platform settings.

### Build the sample using Visual Studio ###

In Visual Studio, on the **Build** menu, choose **Build Solution**.

For more information about using Visual Studio to build a driver package, see [Building a Driver](http://msdn.microsoft.com/en-us/library/windows/hardware/ff554644).

### Locate the built driver package ###

In File Explorer, navigate to the folder that contains your built driver package. The location of this folder varies depending on what you set for configuration and platform. The package contains these files:

File | Description
-----| -----------
Kmdfsamples.cat | A signed catalog file, which serves as the signature for the entire package.
Ramdisk.inf | An information (INF) file that contains information needed to install the driver.
WdfCoinstaller010xx.dll | The coinstaller for version 1.xx of KMDF.
WdfRamdisk.sys | The driver file. 

Run the sample
--------------

The computer where you install the driver is called the *target computer* or the *test computer*. Typically this is a separate computer from where you develop and build the driver package. The computer where you develop and build the driver is called the *host computer*.

The process of moving the driver package to the target computer and installing the driver is called *deploying the driver*. You can deploy RAMDisk sample driver automatically or manually.

###Automatic deployment ###

Before you automatically deploy a driver, you must provision the target computer. For instructions, see [Configuring a Computer for Driver Deployment, Testing, and Debugging](http://msdn.microsoft.com/en-us/library/windows/hardware/).

1.  On the host computer, in Visual Studio, in Solution Explorer, right click **package** (lower case), and choose **Properties**. Navigate to **Configuration Properties \> Driver Install \> Deployment**.
2.  Check **Enable deployment**, and check **Remove previous driver versions before deployment**. For **Target Computer Name**, select the name of a target computer that you provisioned previously. Select **Hardware ID Driver Update**, and enter **Ramdisk** for the hardware ID. Click **OK**.
3.  On the **Build** menu, choose **Deploy Package** or **Build Solution**.

### Manual deployment ###

Before you manually deploy a driver, you must turn on test signing and install a certificate on the target computer. You also need to copy the [DevCon](http://msdn.microsoft.com/en-us/library/windows/hardware/ff544707) tool to the target computer. For instructions, see [Preparing a Computer for Manual Driver Deployment](http://msdn.microsoft.com/en-us/library/windows/hardware/dn265571).

1.  Copy all of the files in your driver package to a folder on the target computer (for example, c:\\RamdiskStorageDriverPackage).
2.  On the target computer, open a Command Prompt window as Administrator. Navigate to your driver package folder, and enter the following command:

    **Devcon install ramdisk.inf Ramdisk**

### View the installed driver in Device Manager ###

On the target computer, in a Command Prompt window, enter **devmgmt** to open Device Manager. In Device Manager, on the **View** menu, choose **Devices by type**. In the device tree, locate **WDF Sample RAM disk Driver** (for example, this might be under the **Sample Device** node).

The RAM disk sample is a root enumerated software driver. To see this in Device Manager, choose **Devices by connection** from the **View** menu. Locate **WDF Sample RAM disk Driver** as a child of the root node of the device tree.

### Save a file on the RAM disk ###

On the target computer, open a Command Prompt window as Administrator. Enter **R:** to switch to the RAM disk drive. In your Command Prompt window, enter **notepad** to open Notepad. Type some text in your notepad document, and then save the document on the R drive. In your Command Prompt window, enter **dir** to verify that the file was saved.

### View Ramdisk entries in the Registry ###


The INF file in the RAM disk driver package specifies parameters that get saved in the registry. On the target computer, open the registry editor (Regedit.exe). In the registry editor, locate the Parameters key for the Ramdisk service. For example,

**HKLM**\\**SYSTEM**\\**CurrentControlSet**\\**Services**\\**Ramdisk**\\**Parameters**

The registry key has these entries:

Parameter       | Value   | Description
----------------|---------|------------
DiskSize        |0x100000 |The size, in bytes, of the RAM disk drive.
DriveLetter     |R:       |The driver letter associated with the RAM disk drive.
RootDirEntries  |0x200    |The number of entries in the root directory.</td>

Using MSBuild
-------------

As an alternative to building the RAMDisk Storage Driver sample in Visual Studio, you can build it in a Visual Studio Command Prompt window. In Visual Studio, on the **Tools** menu, choose **Visual Studio Command Prompt**. In the Visual Studio Command Prompt window, navigate to the folder that has the solution file, ramdisk.sln. Use the [MSBuild](http://go.microsoft.com/fwlink/p/?linkID=262804) command to build the solution. Here are some examples:

**msbuild /p:configuration="Debug" /p:platform="x64" ramdisk.sln**

**msbuild /p:configuration="Release" /p:platform="Win32" ramdisk.sln**

For more information about using [MSBuild](http://go.microsoft.com/fwlink/p/?linkID=262804) to build a driver package, see [Building a Driver](http://msdn.microsoft.com/en-us/library/windows/hardware/ff554644).

