Disk Class Driver
=================

The disk class driver sample is used for managing disk devices

Build the sample
----------------

You can build the sample in two ways: using Microsoft Visual Studio or the command line (*MSBuild*).

Building a Driver Using Visual Studio
-------------------------------------

You build a driver the same way you build any project or solution in Visual Studio. When you create a new driver project using a Windows driver template, the template defines a default (active) project configuration and a default (active) solution build configuration. When you create a project from existing driver sources or convert existing driver code that was built with previous versions of the WDK, the conversion process preserves the target version information (operating systems and platform).

The default Solution build configuration is Debug and Win32.

### To select a configuration and build a driver or an application

1.  Open the driver project or solution in Visual Studio (find *samplename*.sln or *samplename*.vcxproj).
2.  Right-click the solution in the **Solutions Explorer** and select **Configuration Manager**.
3.  From the **Configuration Manager**, select the **Active Solution Configuration** (for example, Debug or Release) and the **Active Solution Platform** (for example, Win32) that correspond to the type of build you are interested in.
4.  From the Build menu, click **Build Solution** (Ctrl+Shift+B).

Building a Driver Using the Command Line (MSBuild)
--------------------------------------------------

You can build a driver from the command line using the Visual Studio Command Prompt window and the Microsoft Build Engine (MSBuild.exe) Previous versions of the WDK used the Windows Build utility (Build.exe) and provided separate build environment windows for each of the supported build configurations. You can now use the Visual Studio Command Prompt window for all build configurations.

### To select a configuration and build a driver or an application

1.  Open a Visual Studio Command Prompt window at the **Start** screen. From this window you can use MsBuild.exe to build any Visual Studio project by specifying the project (.VcxProj) or solutions (.Sln) file.
2.  Navigate to the project directory and enter the **MSbuild** command for your target. For example, to perform a clean build of a Visual Studio driver project called *filtername*.vcxproj, navigate to the project directory and enter the following MSBuild command: **msbuild /t:clean /t:build .\\***samplename***.vcxproj**.

Run the sample
--------------

Installation and Operation
--------------------------

The disk class driver is used to interact with disk devices along with the appropriate port driver. The disk class driver is layered above the port driver and manages disk devices regardless of their bus type. This driver attaches to the disk devices that are enumerated by all of the storage port drivers. This driver exposes the required functionality to the file system drivers to access the disk devices.

For more information, see [Introduction to Storage Class Drivers](http://msdn.microsoft.com/en-us/library/windows/hardware/ff559215) in the storage technologies design guide.

