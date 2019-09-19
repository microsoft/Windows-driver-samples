---
page_type: sample
description: "Illustrates using remote I/O target interfaces to open a HID collection in kernel-mode and send IOCTL requests to set and get feature reports."
languages:
- cpp
products:
- windows
- windows-wdk
---

# KMDF filter driver for a HID device

Firefly is a KMDF-based filter driver for a HID device. Along with illustrating how to write a filter driver, this sample shows how to use remote I/O target interfaces to open a HID collection in kernel-mode and send IOCTL requests to set and get feature reports, as well as how an application can use WMI interfaces to send commands to a filter driver.

## Related topics

[Human Input Devices Design Guide](https://docs.microsoft.com/windows-hardware/drivers/hid)

[Human Input Devices Reference](https://docs.microsoft.com/windows-hardware/drivers/ddi/content/_hid)

## Related technologies

[Creating Framework-based HID Minidrivers](https://docs.microsoft.com/previous-versions//ff540774(v=vs.85))

## Build the sample

For information on how to build a driver using Microsoft Visual Studio, see [Building a Driver with Visual Studio and the WDK](https://docs.microsoft.com/windows-hardware/drivers/develop/building-a-driver). When you build the sample, MSBuild.exe creates luminous.lib, firefly.sys, flicker.exe, and sauron.dll. Copy these files as well as the KMDF coinstaller (wdfcoinstallerMMmmm.dll) and the INF file (firefly.inf) to a floppy disk or a temporary directory on the target system.

> [!NOTE]
> You can obtain redistributable framework updates by downloading the **wdfcoinstaller.msi** package from [WDK 8 Redistributable Components](https://go.microsoft.com/fwlink/p/?LinkID=253170). This package performs a silent install into the directory of your Windows Driver Kit (WDK) installation. You will see no confirmation that the installation has completed. You can verify that the redistributables have been installed on top of the WDK by ensuring there is a redist\\wdf directory under the root directory of the WDK, %ProgramFiles(x86)%\\Windows Kits\\8.0.

## Installation

To install the driver:

1. Plug the Microsoft USB Optical mouse into your target machine and verify that the mouse works. The drivers for this mouse come with the operating system so the device will start working automatically when you plug in.

1. You may need to make Group Policy changes in order to replace the existing mouse driver. If you are unable to perform steps 3-9, do the following:

   1. Open **gpedit.msd**.

   1. In the Group Policy Object Editor navigation pane, open the Computer Configuration folder. Then open Administrative Templates, open System, open Device Installation, and then open Device Installation Restrictions.

   1. Enable *Prevent installation of devices not described by other policy settings*. This will prevent Windows from automatically installing the default mouse driver so that you can then install Firefly.

   1. Enable *Allow administrators to override device installation policy*. This will allow you to bypass the ""The installation of this device is forbidden by system policy" error that you may otherwise receive when you attempt to install Firefly.

   1. You may need to reboot.

1. Bring up the Device Manager (type **devmgmt.msc** in the Start/Run window and press enter).

1. Find the Microsoft Optical mouse under "Mice and other pointing devices"

1. Right click on the device and choose "Update Driver Software."

1. Select "Browse my computer for driver software."

1. Browse to the temporary folder you created earlier. Click Next. Click through the warning.

1. You will see "Windows has successfully updated your driver software" for the "Shiny Things Firefly Mouse" device.

1. The system will copy all the files and restart the mouse device to install the upper filter. Click Close and you are ready to run the test app.

## Testing the Sample

Copy the flicker.exe to the target machine and run it from an elevated command prompt. The usage is:

Usage: Flicker \<-0 | -1 | -2\>

-0 turns off light

-1 turns on light

-2 flashes light

### Testing the DLL

The following description applies to Windows Media Player 12 running on Windows 7:

1. Copy the sauron.dll to the Windows Media player Visualization directory (C:\\Program Files\\Windows Media Player\\Visualizations).

1. Register the DLL with COM by calling "regsvr32 sauron.dll" in command shell.

1. Run Windows Media Player *as administrator*.

1. Click on the "Switch to Now Playing" button in the lower right of the application.

1. Right click, select Visualizations, and you will see a menu item called "Sauron."

1. Choose either Firefly Bars or Firefly Flash and play some music.

1. You will see the mouse light dancing to the tune of the music.

1. You can unregister the DLL by calling "regsvr32 -u sauron.dll".

## Programming Tour

The Firefly sample is installed as an upper filter driver for the Microsoft USB Intellimouse Optical. An application provided with the sample can cause the light of the optical mouse to blink by sending commands to the filter driver using the WMI interface.

The sample consists of:

- Driver (firefly.sys): The Firefly driver is an upper device filter driver for the mouse driver (mouhid.sys). Firefly is a generic filter driver based on the toaster filter driver sample available in the WDK. During start device, the driver registers a WMI class (FireflyDeviceInformation). The user mode application connects to the WMI namespace (root\\wmi) and opens this class using COM interfaces. Then the application can make requests to read ("get") or change ("set") the current value of the TailLit data value from this class. In response to a set WMI request, the driver opens the HID collection using IoTarget and sends [**IOCTL\_HID\_GET\_COLLECTION\_INFORMATION**](https://docs.microsoft.com/windows-hardware/drivers/ddi/content/hidclass/ni-hidclass-ioctl_hid_get_collection_information) and [**IOCTL\_HID\_GET\_COLLECTION\_DESCRIPTOR**](https://docs.microsoft.com/windows-hardware/drivers/ddi/content/hidclass/ni-hidclass-ioctl_hid_get_collection_descriptor) requests to get the preparsed data. The driver then calls [**HidP\_GetCaps**](https://docs.microsoft.com/windows-hardware/drivers/ddi/content/hidpi/nf-hidpi-hidp_getcaps) using the preparsed data to retrieve the capabilities of the device. After getting the capabilities of the device, the driver creates a feature report to set or clear the feature that causes the light to toggle.

- Library (luminous.lib): The sources for this file are located in the \\hid\\firefly\\lib folder. You will need to build the library before using it. This library is shared by the WDM and WDF samples. All the interfaces required to access the WMI is defined in this library and exposed as CLuminous class.

- Application (flicker.exe): The sources for this file are located in the \\hid\\firefly\\app folder. You will need to build the application before using it. This application is shared by the WDM and WDF samples. The application links to luminous.lib to open the WMI interfaces and send set requests to toggle the light.

- Sauron (sauron.dll): The sources for this file are located in the \\hid\\firefly\\sauron folder. You will need to build this dll before using it. The library is shared by the WDM and WDF samples. Sauron is a Windows Media Player visualization DLL, and is based on a sample from the Windows Media Player SDK kit. By using this DLL, you can cause the mouse lights to blink to the beats of the music.
