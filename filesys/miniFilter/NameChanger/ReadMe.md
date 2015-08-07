NameChanger File System Minifilter Driver
=========================================

The *NameChanger* minifilter grafts a directory from one part of a volume's namespace to another part using a mapping. The minifilter maintains this illusion by acting as a name provider, injecting entries into directory enumerations and forwarding directory change notifications.


Build the sample
----------------

You can build the sample in two ways: using Microsoft Visual Studio or the command line (*MSBuild*).

Building a Driver Using Visual Studio
-------------------------------------

You build a driver the same way you build any project or solution in Visual Studio. When you create a new driver project using a Windows driver template, the template defines a default (active) project configuration and a default (active) solution build configuration. When you create a project from existing driver sources or convert existing driver code that was built with previous versions of the WDK, the conversion process preserves the target version information (operating systems and platform).

The default Solution build configuration is Debug and Win32.

### To select a configuration and build a driver

1.  Open the driver project or solution in Visual Studio (find *filtername*.sln or *filtername*.vcxproj).
2.  Right-click the solution in the **Solutions Explorer** and select **Configuration Manager**.
3.  From the **Configuration Manager**, select the **Active Solution Configuration** (for example, Debug or Release) and the **Active Solution Platform** (for example, Win32) that correspond to the type of build you are interested in.
4.  From the Build menu, click **Build Solution** (Ctrl+Shift+B).

Building a Driver Using the Command Line (MSBuild)
--------------------------------------------------

You can build a driver from the command line using the Visual Studio Command Prompt window and the Microsoft Build Engine (MSBuild.exe) Previous versions of the WDK used the Windows Build utility (Build.exe) and provided separate build environment windows for each of the supported build configurations. You can now use the Visual Studio Command Prompt window for all build configurations.

### To select a configuration and build a driver

1.  Open a Visual Studio Command Prompt window at the **Start** screen. From this window you can use MsBuild.exe to build any Visual Studio project by specifying the project (.VcxProj) or solutions (.Sln) file.
2.  Navigate to the project directory and enter the **MSbuild** command for your target. For example, to perform a clean build of a Visual Studio driver project called *filtername*.vcxproj, navigate to the project directory and enter the following MSBuild command: **msbuild /t:clean /t:build .\\***filtername***.vcxproj**.

Run the sample
--------------

Installation
------------

The minifilter samples come with an INF file that will install the minifilter. To install the minifilter, do the following:

1.  Make sure that *filtername*.sys and *filtername*.inf are in the same directory.

    **Note** This installation will make the necessary registry updates to register the minifilter service and place *filtername*.sys in the %SystemRoot%\\system32\\drivers directory.

2.  In Windows Explorer, right-click *filtername*.inf, and click **Install**.

3.  To load the minifilter, run **fltmc load** *filtername* or **net start** *filtername*.

Design and Operation
--------------------

The *NameChanger* minifilter illustrates how to make one part of a volume's namespace appear as though it belongs to part of another namespace. It accomplishes this by altering the names of files that reside beneath a particular path (called the "real mapping") to appear as though they actually reside beneath a different path (called the "user mapping"). The .inf file supplied with the sample defines the real and user mappings in the *[Strings]* section. The three strings used for the mappings are:

String | Description
-------|-------------
UserMapping | The location where files will appear to be in when the filter is attached
UserMappingFinalComponentShort | The "short" (DOS-compliant 8.3-format) name for the final component of the UserMapping path.
RealMapping | The actual location where the files reside.

Before attaching the minifilter to a volume, you must set up the user and real paths. By default the .inf defines the mapping paths like in the following manner:

String | Mapping
-------|--------
UserMapping | "\X\Y"
UserMappingFinalComponentShort | "Y"
RealMapping | "\A\B"

To successfully attach the filter to a volume you must first create a couple of directories. For example, to attach the *NameChanger* minifilter to the F: volume, first create the RealMapping directory (the F:\\A\\B directory). Next, create the parent of the UserMapping path (the F:\\X directory). The following directories are be created:

F:\\A\\B

F:\\X

Once this is done the *NameChanger* filter should successfully attach to F:. It will change the directories you created to appear like the following:

F:\\A

F:\\X\\Y

After the minifilter attaches, the "B" subdirectory of F:\\A is no longer visible. Its contents now appear under the "Y" subdirectory of F:\\X.

For more information on file system minifilter design, start with the [File System Minifilter Drivers](http://msdn.microsoft.com/en-us/library/windows/hardware/ff540402) section in the Installable File Systems Design Guide.

