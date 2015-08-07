Toaster Sample Driver
=====================
The Toaster collection is an iterative series of samples that demonstrate fundamental aspects of Windows driver development for both Kernel-Mode Driver Framework (KMDF) and User-Mode Driver Framework (UMDF) version 1.

All the samples work with a hypothetical toaster bus, over which toaster devices can be connected to a PC.

The Toaster sample collection comprises driver projects (.vcxproj files) that are contained in the toaster.sln solution file (in general\\toaster\\toastdrv).

Related technologies
--------------------
[Windows Driver Frameworks](http://msdn.microsoft.com/en-us/library/windows/hardware/ff557565)

For detailed descriptions and code walkthroughs of each project, see [Sample Toaster Driver Programming Tour](http://msdn.microsoft.com/en-us/library/windows/hardware/dn569312). To learn how to build and run the samples, read on.


Run the sample
--------------
The computer where you install the driver is called the *target computer* or the *test computer*. Typically this is a separate computer from where you develop and build the driver package. The computer where you develop and build the driver is called the *host computer*.

The process of moving the driver package to the target computer and installing the driver is called *deploying the driver*. You can deploy components of the Toaster Sample automatically or manually. Here, we install the wdfsimple driver on the target computer.

### Specifying which projects to deploy

Before doing this, you should back up your package.vcxproj file, located in your sample directory, for example C:\\Toaster\\C++\\Package.

1.  In the Properties for the package project, navigate to **Common Properties \> References**.
2.  Remove all references except WdfSimple. (Use the **Remove Reference** button at the bottom.)

### Automatic deployment (root enumerated)

Before you automatically deploy a driver, you must provision the target computer. For instructions, see [Configuring a Computer for Driver Deployment, Testing, and Debugging](http://msdn.microsoft.com/en-us/library/windows/hardware/).

1.  On the host computer, in Visual Studio, in Solution Explorer, right click the **package** project (within the package folder), and choose **Properties**. Navigate to **Configuration Properties \> Driver Install \> Deployment**.
2.  Check **Enable deployment**, and check **Remove previous driver versions before deployment**. For **Target Computer Name**, use the drop down to select the name of a target computer that you provisioned previously. Select **Hardware ID Driver Update**, and enter **{b85b7c50-6a01-11d2-b841-00c04fad5171}\\MsToaster** for the hardware ID. (You can find this value in the WdfSimple.inx file.) Click **Apply** and **OK**.
3.  Because this solution contains many projects, you may find it easier to remove some of them before you build and deploy a driver package. To do so, right click **package** (lower case), and choose **Properties**. Navigate to **Common Properties-\>References** and click **Remove Reference** to remove projects you don't want. (You can add them back later by using **Add New Reference**.) Click **OK**.
4.  On the **Build** menu, choose **Build Solution** or **Rebuild Solution** (if you removed references).
5.  If you removed references and deployment does not succeed, try deleting the contents of the c:\\DriverTest\\Drivers folder on the target machine, and then retry deployment.

### Manual deployment (root enumerated)

Before you manually deploy a driver, you must turn on test signing and install a certificate on the target computer. You also need to copy the [DevCon](http://msdn.microsoft.com/en-us/library/windows/hardware/ff544707) tool to the target computer. For instructions, see [Preparing a Computer for Manual Driver Deployment](http://msdn.microsoft.com/en-us/library/windows/hardware/dn265571).

1.  Copy all of the files in your driver package to a folder on the target computer (for example, c:\\WdfSimplePackage).
2.  On the target computer, open a Command Prompt window as Administrator. Navigate to your driver package folder, and enter the following command:

    **devcon install WdfSimple.inf {b85b7c50-6a01-11d2-b841-00c04fad5171}\\MsToaster**

### View the root enumerated driver in Device Manager

On the target computer, in a Command Prompt window, enter **devmgmt** to open Device Manager. In Device Manager, on the **View** menu, choose **Devices by type**. In the device tree, locate **Microsoft WDF Simple Toaster (No Class Installer)**.

In Device Manager, on the **View** menu, choose **Devices by connection**. Locate **Microsoft WDF Simple Toaster (No Class Installer)** as a child of the root node of the device tree.

Build the sample using MSBuild
------------------------------

As an alternative to building the Toaster sample in Visual Studio, you can build it in a Visual Studio Command Prompt window. In Visual Studio, on the **Tools** menu, choose **Visual Studio Command Prompt**. In the Visual Studio Command Prompt window, navigate to the folder that has the solution file, Toaster.sln. Use the MSBuild command to build the solution. Here are some examples:

**msbuild /p:configuration="Debug" /p:platform="x64" Toaster.sln**

**msbuild /p:configuration="Release" /p:platform="Win32" Toaster.sln**

For more information about using MSBuild to build a driver package, see [Building a Driver](http://msdn.microsoft.com/en-us/library/windows/hardware/ff554644).

UMDF Toaster File Manifest
--------------------------
#### WUDFToaster.idl
Component Interface file

#### WUDFToaster.cpp
DLL Support code - provides the DLL's entry point as well as the DllGetClassObject export.

#### WUDFToaster.def
This file lists the functions that the driver DLL exports.

#### stdafx.h
This is the main header file for the sample driver.

#### driver.cpp & driver.h
Definition and implementation of the IDriverEntry callbacks in CDriver class.

#### device.cpp & device.h
Definition and implementation of various interfaces and their callbacks in CDevice class. Add your PnP and Power interfaces specific for your hardware.

#### queue.cpp & queue.h
Definition and implementation of the base queue callback class (CQueue). IQueueCallbackDevicekIoControl, IQueueCallbackRead and IQueueCallBackWrite callbacks are implemented to handle I/O control requests.

#### WUDFToaster.rc
This file defines resource information for the WUDF Toaster sample driver.

#### WUDFToaster.inf
Sample INF for installing the sample WUDF Toaster driver under the Toaster class of devices.

#### WUDFtoaster.ctl, internal.h
This file lists the WPP trace control GUID(s) for the sample driver. This file can be used with the tracelog command's -guid flag to enable the collection of these trace events within an established trace session.
These GUIDs must remain in sync with the trace control guids defined in internal.h.

Toastmon File Manifest
----------------------
#### comsup.cpp & comsup.h
Boilerplate COM Support code - specifically base classes which provide implementations for the standard COM interfaces IUnknown and IClassFactory which are used throughout the sample.
The implementation of IClassFactory is designed to create instances of the CMyDriver class. If you should change the name of your base driver class, you would also need to modify this file.

#### dllsup.cpp
Boilerplate DLL Support code - provides the DLL's entry point as well as the single required export (DllGetClassObject).
These depend on comsup.cpp to perform the necessary class creation.

#### exports.def
This file lists the functions that the driver DLL exports.

#### makefile
This file redirects to the real makefile, which is shared by all the driver components of the Windows Driver Kit.

#### internal.h
This is the main header file for the ToastMon driver

#### driver.cpp & driver.h
Definition and implementation of the driver callback class for the ToastMon sample.

#### device.cpp & device.h
Definition and implementation of the device callback class for the ToastMon sample. This is mostly boilerplate, but also registers for RemoteInterface Arrival notifications. When a RemoteInterface arrival callback occurs, it calls CreateRemoteInterface and creates a CMyRemoteTarget callback object to handle I/O on that RemoteInterface.

#### RemoteTarget.cpp & RemoteTarget.h
Definition and implementation of the remote target callback class for the ToastMon sample.

#### list.h
Doubly-linked-list code

#### ToastMon.rc
This file defines resource information for the ToastMon sample driver.

#### UMDFToastMon.inf
Sample INF for installing the Skeleton driver to control a root enumerated device with a hardware ID of UMDFSamples\\ToastMon

