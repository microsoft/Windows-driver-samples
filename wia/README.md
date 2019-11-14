---
page_type: sample
description: "Contains samples and test tools for Windows Image Acquisition (WIA)."
languages:
- cpp
products:
- windows
- windows-wdk
---

# Windows Image Acquisition (WIA) Driver Samples

The Windows Image Acquisition driver sample set contains samples and test tools for Windows Image Acquisition (WIA), a driver architecture and user interface for acquiring images from still image devices such as scanners.

## PRODUCTION SCANNING WIA 2.0 DRIVER

The ProdScan directory contains a sample WIA 2.0 mini-driver. This sample shows how to add Production Scanning features to a WIA 2.0 mini-driver.

## EXTENDED WIA 2.0 MONSTER DRIVER

The Wiadriverex directory contains a sample WIA 2.0 mini-driver. This sample shows how to write a WIA 2.0 mini-driver that uses the stream-based WIA 2.0 transfer model. It also shows an implementation of a very simple segmentation filter, image processing filter, and error handling extension for the WIA 2.0 mini-driver.

For more information, see [Introduction to WIA](https://docs.microsoft.com/windows-hardware/drivers/image/introduction-to-wia).

## Build the sample

You can build the sample in two ways: using the Visual Studio Integrated Development Environment (IDE) or from the command line using the Visual Studio Command Prompt window and the Microsoft Build Engine (MSBuild.exe).

### Building the sample using Visual Studio

1. Open Visual Studio. From the **File** menu, select **Open Project/Solution**. Within your WDK installation, navigate to src\\wia and open the wia.sln project file.

1. Right-click the solution in the **Solution Explorer** and select **Configuration Manager**.

1. From the **Configuration Manager**, select the **Active Solution Configuration** (for example, Windows 8.1 Debug or Windows 8.1 Release) and the **Active Solution Platform** (for example, Win32) that correspond to the type of build you are interested in.

1. From the **Build** menu, click **Build Solution** (Ctrl+Shift+B).

Previous versions of the WDK used the Windows Build utility (Build.exe) and provided separate build environment windows for each of the supported build configurations. You can use the Visual Studio Command Prompt window for all build configurations.

### Building the sample using the command line (MSBuild)

1. Open a Visual Studio Command Prompt window. Click **Start** and search for **Developer Command Prompt**. If your project is under %PROGRAMFILES%, you need to open the command prompt window using elevated permissions (**Run as administrator**). From this window you can use MsBuild.exe to build any Visual Studio project by specifying the project (.VcxProj) or solutions (.Sln) file.

1. Navigate to the project directory and enter the **MSbuild** command for your target. For example, to perform a clean build of a Visual Studio driver project called extend.vcxproj, navigate to the project directory and enter the following MSBuild command: **msbuild /t:clean /t:build .\\extend.vcxproj**.

1. If the build succeeds, you will find the driver (extend.dll) in the binary output directory corresponding to the target platform, for example src\\wia\\extend\\Windows 8.1 Debug.

## Run the sample

Run the "copywia.cmd” batch file to gather all of the binaries into a subdirectory named “wiabins”. The WIA driver sample can be installed by using the Add Device icon in the Scanners and Cameras control panel. Use the Have Disk button to point to the wiabins\\drivers or wiabins\\drivers folder. Wiatest.exe (from the WDK Tools\\Wia directory), MS Paint, the Scanner and Camera Wizard, or any TWAIN application (through the WIA TWAIN compatibility layer) can be used to test the samples.
