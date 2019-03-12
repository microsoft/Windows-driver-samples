---
topic: sample
name: Device Console (DevCon) Tool
description: DevCon enables, disables, installs, configures, and removes devices on the local computer and displays detailed information about devices on local and remote computers.
languages:
  - cpp
products:
  - windows
---

<!---
    name: Device Console (DevCon) Tool
    platform: Tool
    language: cpp
    category: Setup
    description: DevCon enables, disables, installs, configures, and removes devices on the local computer and displays detailed information about devices on local and remote computers.
    samplefwlink: https://go.microsoft.com/fwlink/p/?linkid=856741
--->

# Device Console (DevCon) Tool

[DevCon](http://msdn.microsoft.com/en-us/library/windows/hardware/ff544707) is a command-line tool that displays detailed information about devices, and lets you search for and manipulate devices from the command line. DevCon enables, disables, installs, configures, and removes devices on the local computer and displays detailed information about devices on local and remote computers. DevCon is included in the WDK.

This document explains the DevCon design, and how to use the SetupAPI and device installation functions to enumerate devices and perform device operations in a console application. For a complete description of DevCon features and instructions for using them, see the DevCon help file included with the WDK documentation in Driver Development Tools/Tools for Testing Drivers/DevCon.

DevCon is provided in ready-to-run form in tools\\devcon. For usage, refer to the document provided with devcon.exe. DevCon is a command line utility with built-in documentation available by typing "devcon help".

## Build the sample

You can build the sample in two ways: using the Visual Studio Integrated Development Environment (IDE) or from the command line using the Visual Studio Command Prompt window and the Microsoft Build Engine (MSBuild.exe).

### Building the sample using Visual Studio

1. Open Visual Studio. From the **File** menu, select **Open Project/Solution**. Navigate to the DevCon sample folder and open the devcon.sln project file.
1. Right-click the solution in the **Solution Explorer** and select **Configuration Manager**.
1. From the **Configuration Manager**, select the **Active Solution Configuration** (for example, Debug or Release) and the **Active Solution Platform** (for example, Win32) that correspond to the type of build you are interested in.
1. From the **Build** menu, click **Build Solution** (Ctrl+Shift+B).

Previous versions of the WDK used the Windows Build utility (Build.exe) and provided separate build environment windows for each of the supported build configurations. You can use the Visual Studio Command Prompt window for all build configurations.

### Building the sample using the command line (MSBuild)

1. Open a Visual Studio Command Prompt window. Click **Start** and search for **Developer Command Prompt**. If your project is under %PROGRAMFILES%, you need to open the command prompt window using elevated permissions (**Run as administrator**). From this window you can use MsBuild.exe to build any Visual Studio project by specifying the project (.VcxProj) or solutions (.Sln) file.
1. Navigate to the project directory and enter the **MSbuild** command for your target. For example, to perform a clean build of a Visual Studio driver project called devcon.vcxproj, navigate to the project directory and enter the following MSBuild command: **msbuild /t:clean /t:build .\\devcon.vcxproj**.
1. If the build succeeds, you will find the tools (devcon.exe) in the binary output directory corresponding to the target platform, for example samples\\setup\\devcon\\Debug.

## Run the sample

Type "devcon find \*" to list device instances of all present devices on the local machine.

Type "devcon status @root\\rdp\_mou\\0000" to list status of the terminal server mouse driver.

Type "devcon status \*PNP05\*" to list status of all COM ports.

### How DevCon works

Running "devcon help" will provide a list of commands along with short descriptions of what each command does. "devcon help \<command\>" will give more detailed help on that command. The interpretation of each command is done via a dispatch table "DispatchTable" that is at the bottom of "cmds.cpp". Some of the commands make use of a generic device enumerator "EnumerateDevices". A few of these commands will work when given a remote target computer, and will also work if using the 32-bit devcon on Wow64. 

A description of some of the more interesting functions and the APIs they use follows:

cmdClasses  

- This command demonstrates the use of [**SetupDiBuildClassInfoListEx**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff550911) to enumerate all device class GUID's. The function [**SetupDiClassNameFromGuidEx**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff550950) and [**SetupDiGetClassDescriptionEx**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff551058) are used to obtain more information about each device class.

cmdListClass  

- This command demonstrates the use of [**SetupDiClassGuidsFromNameEx**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff550941) to enumerate one or more class GUID's that match the class name. This command also demonstrates the use of [**SetupDiGetClassDevsEx**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff551072) to list all the devices for each class GUID.

cmdFind cmdFindAll cmdStatus  

- A simple use of *EnumerateDevices* (explained below) to list devices and display different levels of information about each device. Note that all but *cmdFindAll* use DIGCF\_PRESENT to only list information about devices that are currently present. The main functionality for these and related devices is done inside *FindCallback.*

cmdEnable cmdDisable cmdRestart

- These commands show how to issue DIF\_PROPERTYCHANGE to enable a device, disable a device, or restart a device. The main functionality for each of these commands is done inside *ControlCallback*. These operations cannot be done on a remote machine or in the context of Wow64. CFGMGR32 API's should not be used as they skip class and co-installers.

cmdUpdate  

- This command shows how to use [**UpdateDriverForPlugAndPlayDevices**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff553534) to update the driver for all devices to a specific driver. Normally INSTALLFLAG\_FORCE would not be specified allowing **UpdateDriverForPlugAndPlayDevices** to determine if there is a better match already known. It's specified in DevCon to allow DevCon to be used more effectively as a debugging/testing tool. This cannot be done on a remote machine or in the context of Wow64.

cmdInstall  

- A variation of *cmdUpdate* to install a driver when there is no associated hardware. It creates a new root-enumerated device instance and associates it with a made up hardware ID specified on the command line (which should correspond to a hardware ID in the INF). This cannot be done on a remote machine or in the context of Wow64.

cmdRemove  

- A command to remove devices. Plug & Play devices that are removed will reappear in response to *cmdRescan*. The main functionality of this command is in *RemoveCallback* that demonstrates the use of DIF\_REMOVE. This cannot be done on a remote machine or in the context of Wow64. CFGMGR32 API's should not be used as they skip class and co-installers.

cmdRescan  

- This command shows the correct way to rescan for all Plug & Play devices that may have previously been removed, or that otherwise require a rescan to detect them.

cmdDPAdd  

- This command allows you to add a Driver Package to the machine. The main functionality of this command demonstrates the use of [**SetupCopyOEMInf**](http://msdn.microsoft.com/en-us/library/windows/hardware/). Adding a Driver Package to the machine doesn't mean the drivers are installed on devices, it simply means the drivers are available automatically when a new device is plugged in or a existing device is updated.

cmdDPDelete  

- This command allows you to uninstall a Driver Package from the machine. The main functionality of this command demonstrates the use of [**SetupUninstallOEMInf**](http://msdn.microsoft.com/en-us/library/windows/hardware/). Removing a Driver Package from the machine does not uninstall the drivers associated with a device. If you want to accomplish both then use *cmdRemove* on all the devices using a given Driver Package and then *cmdDPDelete* to remove the Driver Package itself from the machine.

cmdDPEnum  

- This command allows you to enumerate all of the 3rd party Driver Packages currently installed on the machine and also shows you how to get some common properties from a Driver Package (Provider, Class description, DriverVer date and version).

Reboot  

- This function shows how to correctly reboot the machine from a hardware install program. In particular it passes flags to **ExitWindowsEx** that cause the reboot to be associated with hardware installation. You should never reboot the machine unnecessarily.

EnumerateDevices  

- Demonstrates the use of [**SetupDiGetClassDevsEx**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff551072) to enumerate all devices or all present devices, either globally or limited to a specific setup class. Demonstrates the use of [**SetupDiCreateDeviceInfoListEx**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff550958) to create a blank list associated with a class or not (for most cases, a blank list need not be associated with a class). Demonstrates the use of [**SetupDiOpenDeviceInfo**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff552071) to add a device instance into a device info list. These last two API's are ideal to obtain a [SP\_DEVINFO\_DATA](http://msdn.microsoft.com/en-us/library/windows/hardware/ff552344) structure from a device instance and machine name when mixing CFGMGR32 API's with SETUPAPI API's. [**SetupDiGetDeviceInfoListDetail**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff551103) is called to obtain a remote machine handle that may be passed into CFGMGR32 API's. [**SetupDiEnumDeviceInfo**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff551010) is called to enumerate each and every device that is in the device info list (either explicitly added, or determined by the call to **SetupDiGetClassDevsEx**). The instance ID is obtained by calling [**CM\_Get\_Device\_ID\_Ex**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff538411), using information in devInfo (obtained from **SetupDiEnumerateDeviceInfo**) and devInfoListDetail (obtained from **SetupDiGetDeviceInfoListDetail**). *GetHwIds* is called to obtain a list of hardware and compatible ID's (explained below). Once an interesting device has been determined (typically by checking hardware ID's) then the callback is called to operate on that individual device.

GetHwIds  

- Shows how to get the complete list of hardware ID's or compatible ID's for a device using [**SetupDiGetDeviceRegistryProperty**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff551967).

GetDeviceDescription  

- Shows how to obtain descriptive information about a device. The friendly name is used if it exists, otherwise the device description is used.

DumpDeviceWithInfo  

- Shows how to obtain an instance ID (or use any CFGMGR32 API) given HDEVINFO (device info list) and [PSP\_DEVINFO\_DATA](http://msdn.microsoft.com/en-us/library/windows/hardware/ff552344) (device info data).

DumpDeviceStatus  

- Shows how to interpret the information returned by [**CM\_Get\_DevNode\_Status\_Ex**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff538517). Refer to *cfg.h* for information returned by this API.

DumpDeviceResources  

- Shows how to obtain information about resources used by a device.

DumpDeviceDriverFiles  

- Provided as a debugging aid, obtains information about the files apparently being used for a device. It uses [**SetupDiBuildDriverInfoList**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff550917) to obtain information about the driver being used for the specified device. The driver list associated with a device may be enumerated by calling [**SetupDiEnumDriverInfo**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff551018). In this case, there will be no more than one driver listed. This function proceeds to obtain a list of files that would normally be copied for this driver using DIF\_INSTALLDEVICEFILES. [**SetupScanFileQueue**](http://msdn.microsoft.com/en-us/library/windows/hardware/) is used to enumerate the file queue to display the list of files that are associated with the driver.

DumpDeviceDriverNodes  

- Provided as a debugging aid, this function determines the list of compatible drivers for a device. It uses [**SetupDiBuildDriverInfoList**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff550917) to obtain the list of compatible drivers. In this case, all drivers are enumerated, however typically DIF\_SELECTBESTCOMPATDRV and [**SetupDiGetSelectedDriver**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff552013) would be used together to find which driver the OS would consider to be the best.

DumpDeviceStack  

- This function determines class and device upper and lower filters.
