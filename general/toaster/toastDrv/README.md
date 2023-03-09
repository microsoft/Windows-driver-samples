---
page_type: sample
description: "An iterative series of samples that demonstrate KDMF and UDMF1 driver development."
languages:
- cpp
products:
- windows
- windows-wdk
---

# Toaster Sample Driver

The Toaster collection is an iterative series of samples that demonstrate fundamental aspects of Windows driver development for both Kernel-Mode Driver Framework (KMDF).

All the samples work with a hypothetical toaster bus, over which toaster devices can be connected to a PC.

The Toaster sample collection comprises driver projects (.vcxproj files) that are contained in the toaster.sln solution file (in general\\toaster\\toastdrv).

## Related technologies

[Windows Driver Frameworks](https://docs.microsoft.com/windows-hardware/drivers/wdf/)

For detailed descriptions and code walkthroughs of each project, see [Sample Toaster Driver Programming Tour](https://docs.microsoft.com/windows-hardware/drivers/wdf/sample-toaster-driver-programming-tour). To learn how to build and run the samples, read on.

## Run the sample

The computer where you install the driver is called the *target computer* or the *test computer*. Typically this is a separate computer from where you develop and build the driver package. The computer where you develop and build the driver is called the *host computer*.

The process of moving the driver package to the target computer and installing the driver is called *deploying the driver*. You can deploy components of the Toaster Sample automatically or manually. Here, we install the wdfsimple driver on the target computer.

### Specifying which projects to deploy

Before doing this, you should back up your package.vcxproj file, located in your sample directory, for example C:\\Toaster\\C++\\Package.

1. In the Properties for the package project, navigate to **Common Properties \> References**.

1. Remove all references except WdfSimple. (Use the **Remove Reference** button at the bottom.)

### Automatic deployment (root enumerated)

Before you automatically deploy a driver, you must provision the target computer. For instructions, see [Provision a computer for driver deployment and testing](https://docs.microsoft.com/windows-hardware/drivers/gettingstarted/provision-a-target-computer-wdk-8-1).

1. On the host computer, in Visual Studio, in Solution Explorer, right click the **package** project (within the package folder), and choose **Properties**. Navigate to **Configuration Properties \> Driver Install \> Deployment**.

1. Check **Enable deployment**, and check **Remove previous driver versions before deployment**. For **Target Computer Name**, use the drop down to select the name of a target computer that you provisioned previously. Select **Hardware ID Driver Update**, and enter **{b85b7c50-6a01-11d2-b841-00c04fad5171}\\MsToaster** for the hardware ID. (You can find this value in the WdfSimple.inx file.) Click **Apply** and **OK**.

1. Because this solution contains many projects, you may find it easier to remove some of them before you build and deploy a driver package. To do so, right click **package** (lower case), and choose **Properties**. Navigate to **Common Properties-\>References** and click **Remove Reference** to remove projects you don't want. (You can add them back later by using **Add New Reference**.) Click **OK**.

1. On the **Build** menu, choose **Build Solution** or **Rebuild Solution** (if you removed references).

1. If you removed references and deployment does not succeed, try deleting the contents of the c:\\DriverTest\\Drivers folder on the target machine, and then retry deployment.

### Manual deployment (root enumerated)

Before you manually deploy a driver, you must turn on test signing and install a certificate on the target computer. You also need to copy the [DevCon](https://docs.microsoft.com/windows-hardware/drivers/devtest/devcon) tool to the target computer. For instructions, see [Preparing a Computer for Manual Driver Deployment](https://docs.microsoft.com/windows-hardware/drivers/develop/preparing-a-computer-for-manual-driver-deployment).

1. Copy all of the files in your driver package to a folder on the target computer (for example, c:\\WdfSimplePackage).

1. On the target computer, open a Command Prompt window as Administrator. Navigate to your driver package folder, and enter the following command:

   `devcon install WdfSimple.inf {b85b7c50-6a01-11d2-b841-00c04fad5171}\\MsToaster`

### View the root enumerated driver in Device Manager

On the target computer, in a Command Prompt window, enter **devmgmt** to open Device Manager. In Device Manager, on the **View** menu, choose **Devices by type**. In the device tree, locate **Microsoft WDF Simple Toaster (No Class Installer)**.

In Device Manager, on the **View** menu, choose **Devices by connection**. Locate **Microsoft WDF Simple Toaster (No Class Installer)** as a child of the root node of the device tree.

## Build the sample using MSBuild

As an alternative to building the Toaster sample in Visual Studio, you can build it in a Visual Studio Command Prompt window. In Visual Studio, on the **Tools** menu, choose **Visual Studio Command Prompt**. In the Visual Studio Command Prompt window, navigate to the folder that has the solution file, Toaster.sln. Use the MSBuild command to build the solution. Here are some examples:

`msbuild /p:configuration="Debug" /p:platform="x64" Toaster.sln`

`msbuild /p:configuration="Release" /p:platform="Win32" Toaster.sln`

For more information about using MSBuild to build a driver package, see [Building a Driver with Visual Studio and the WDK](https://docs.microsoft.com/windows-hardware/drivers/develop/building-a-driver).

