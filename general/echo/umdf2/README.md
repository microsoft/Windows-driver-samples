Echo Sample (UMDF Version 2)
============================

The ECHO (UMDF version 2) sample demonstrates how to use a sequential queue to serialize read and write requests presented to the driver.

It also shows how to synchronize execution of these events with other asynchronous events such as request cancellation and DPC.

## Universal Windows Driver Compliant
This sample builds a Universal Windows Driver. It uses only APIs and DDIs that are included in OneCoreUAP.

Related technologies
--------------------

[User-Mode Driver Framework](http://msdn.microsoft.com/en-us/library/windows/hardware/ff560456)


Open the driver solution in Visual Studio
-----------------------------------------

In Microsoft Visual Studio, open the solution file (umdf2echo.sln). Choose **Solution Explorer** from the **View** menu. In Solution Explorer, you can see one solution that contains three projects. There is a driver project (Driver-\>AutoSync-\>echo), an application project (Exe-\>echoapp), and a package project named **package** (lower case).

Set the configuration and platform in Visual Studio
---------------------------------------------------

In Visual Studio, in Solution Explorer, right click **Solution**, and choose **Configuration Manager**. Set the configuration and the platform. Make sure that the configuration and platform are the same for both the driver project and the package project. Do not check the **Deploy** boxes. 


Locate the built driver package
-------------------------------

In File Explorer, navigate to the folder that contains your built driver package. The location of this folder varies depending on what you set for configuration and platform. 

Run the sample
--------------

The computer where you install the driver is called the *target computer* or the *test computer*. Typically this is a separate computer from where you develop and build the driver package. The computer where you develop and build the driver is called the *host computer*.

The process of moving the driver package to the target computer and installing the driver is called *deploying the driver*. You can deploy a driver sample automatically or manually.

### Automatic deployment (root enumerated)

Before you automatically deploy a driver, you must provision the target computer. For instructions, see [Configuring a Computer for Driver Deployment, Testing, and Debugging](http://msdn.microsoft.com/en-us/library/windows/hardware/).

1.  On the host computer, in Visual Studio, in Solution Explorer, right click **package** (lower case), and choose **Properties**. Navigate to **Configuration Properties \> Driver Install \> Deployment**.
2.  Check **Enable deployment**, and check **Remove previous driver versions before deployment**. For **Target Computer Name**, select the name of a target computer that you provisioned previously. Select **Hardware ID Driver Update**, and enter **root\\ECHO** for the hardware ID. Click **OK**.
3.  On the **Build** menu, choose **Build Solution**.

### Manual deployment (root enumerated)

Before you manually deploy a driver, you must turn on test signing and install a certificate on the target computer. You also need to copy the [DevCon](http://msdn.microsoft.com/en-us/library/windows/hardware/ff544707) tool to the target computer. For instructions, see [Preparing a Computer for Manual Driver Deployment](http://msdn.microsoft.com/en-us/library/windows/hardware/dn265571).

1.  Copy all of the files in your driver package to a folder on the target computer (for example, c:\\umdf2echoPkg).
2.  On the target computer, open a Command Prompt window as Administrator. Navigate to your driver package folder, and enter the following command:

    **devcon install echoum.inf root\\ECHO**

### View the root enumerated driver in Device Manager

On the target computer, in a Command Prompt window, enter **devmgmt** to open Device Manager. In Device Manager, on the **View** menu, choose **Devices by type**. In the device tree, locate **Sample WDF ECHO Driver** (for example, this might be under the **Sample Device** node).

In Device Manager, on the **View** menu, choose **Devices by connection**. Locate **Sample WDF ECHO Driver** as a child of the root node of the device tree.

Build the sample using MSBuild
------------------------------

As an alternative to building the driver sample in Visual Studio, you can build it in a Visual Studio Command Prompt window. In Visual Studio, on the **Tools** menu, choose **Visual Studio Command Prompt**. In the Visual Studio Command Prompt window, navigate to the folder that has the solution file, umdf2echo.sln. Use the MSBuild command to build the solution. Here is an example:

**msbuild /p:configuration="Release" /p:platform="Win32" umdf2echo.sln**

For more information about using MSBuild to build a driver package, see [Building a Driver](http://msdn.microsoft.com/en-us/library/windows/hardware/ff554644).

