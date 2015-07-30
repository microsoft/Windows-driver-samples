CDROM Storage Class Driver
==========================

The CD ROM driver is used to provide access to CD, DVD and Blu-ray drives. It supports Plug and Play, Power Management, and AutoRun (media change notification).

Build the sample
----------------

You can build the sample in two ways: using Microsoft Visual Studio or the command line (*MSBuild*).
**Note:** When building in Visual Studio, INFVerifer will throw errors. This is intended. Fix those errors with your custom values to build successfully.

Building a Driver Using Visual Studio
-------------------------------------

You build a driver the same way you build any project or solution in Visual Studio. When you create a new driver project using a Windows driver template, the template defines a default (active) project configuration and a default (active) solution build configuration. When you create a project from existing driver sources or convert existing driver code that was built with previous versions of the WDK, the conversion process preserves the target version information (operating systems and platform).

The default Solution build configuration is Visual Studio Debug and Win32.

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

The in-box CD ROM driver is protected by the system, and thus a normal device driver update attempt through the Device Manager will fail. Users are not encouraged to replace the in-box CD ROM driver. The following work-around is provided in case there is a need, but the users are warned that this may harm the system.

1.  Locate the "cdrom.inf" file in the binary output directory, and update the file by replacing all "cdrom.sys" occurrences with "mycdrom.sys".
2.  Rename the "cdrom.inf" file to "mycdrom.inf".
3.  Copy "mycdrom.sys" and "mycdrom.inf" from the binary output directory to the test machine, if applicable.
4.  Launch the Device Manager
5.  Select the appropriate device under the "DVD/CD-ROM drives" category.
6.  On the right-click menu, select "Update Driver Software...".
7.  Select "Browse my computer for driver software".
8.  Select "Let me pick from a list of device drivers on my computer".
9.  Click "Have Disk...", and point to the directory that contains "mycdrom.inf" and "mycdrom.sys".
10. Click "Next". If you get a warning dialog about installing unsigned driver, click "Yes".
11. Click "Next" to complete the driver upgrade.
12. After installation completes successfully, "mycdrom.sys" will be the effective driver for the device, "cdrom.sys" will no longer be used.

For more information, see [CD-ROM Drivers](http://msdn.microsoft.com/en-us/library/windows/hardware/ff551391) in the storage technologies design guide.

