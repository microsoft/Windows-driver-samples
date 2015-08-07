USBView sample application
==========================

Usbview.exe is a Windows GUI application that allows you to browse all USB controllers and connected USB devices on your system. The left pane in the main application window displays a connection-oriented tree view, and the right pane displays the USB data structures pertaining to the selected USB device, such as the Device, Configuration, Interface, and Endpoint Descriptors, as well as the current device configuration.

**Important** If you need UsbView as a tool, do not download this sample. Instead get UsbView.exe from the [Windows Driver Kit (WDK)](http://go.microsoft.com/fwlink/p?linkid=391063) in the Windows Kits\\*\<version\>*\\Tools\\*\<arch\>* folder. If you need to see the source code for UsbView, open the **Browse code** tab.

This functional application sample demonstrates how a user-mode application can enumerate USB host controllers, USB hubs, and attached USB devices, and query information about the devices from the registry and through USB requests to the devices.

The IOCTL calls (see the system include file USBIOCTL.H) demonstrated by this sample include:

-   [**IOCTL\_GET\_HCD\_DRIVERKEY\_NAME**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff537236)
-   [**IOCTL\_USB\_GET\_DESCRIPTOR\_FROM\_NODE\_CONNECTION**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff537310)
-   [**IOCTL\_USB\_GET\_NODE\_CONNECTION\_DRIVERKEY\_NAME**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff537317)
-   [**IOCTL\_USB\_GET\_NODE\_CONNECTION\_INFORMATION**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff537319)
-   [**IOCTL\_USB\_GET\_NODE\_CONNECTION\_NAME**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff537323)
-   [**IOCTL\_USB\_GET\_NODE\_INFORMATION**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff537324)
-   [**IOCTL\_USB\_GET\_ROOT\_HUB\_NAME**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff537326)

For information about USB, see [Universal Serial Bus (USB) Drivers](http://msdn.microsoft.com/en-us/library/windows/hardware/ff538930).

Run the sample
--------------

### Local debugging

1.  Change **Debugger** to launch to **Local Windows Debugger**.
2.  On the **Debug** menu, select **Start debugging** or hit **F5**.

### Manual deployment to a remote target computer

If you want to debug the sample app on a remote computer,

1.  Copy the executable to a folder on the remote computer.
2.  Specify project properties as per the instructions given in [Set Up Remote Debugging for a Visual Studio Project](http://msdn.microsoft.com/en-us/library/8x6by8d2.aspx).
3.  Change **Debugger** to launch to **Remote Windows Debugger**.
4.  On the **Debug** menu, select **Start debugging** or hit **F5**.

### View a USB device in Usbview

1.  Attach a USB device to one of USB ports on the computer that has Usbview running.
2.  In the device tree, locate the device. For example the device might be under the Intel(R) ICH10 Family USB Universal Host Controller - 3A34 \> Root Hub node.
3.  View host controller and port properties on the right pane.

Code tour
---------

File manifest | Description 
--------------|------------
Resource.h | ID definitions for GUI controls 
Usbdesc.h | USB descriptor type definitions 
Usbview.h | Main header file for this sample 
Vndrlist.h | List of USB Vendor IDs and vendor names 
Debug.c | Assertion routines for the checked build 
Devnode.c | Routines for accessing DevNode information 
Dispaud.c | Routines for displaying USB audio class device information 
Enum.c | Routines for displaying USB device information 
Usbview.c | Entry point and GUI handling routines 

The major topics covered in this tour are:

-   GUI handling routines
-   Device enumeration routines
-   Device information display routines

The file Usbview.c contains the sample application entry point and GUI handling routines. On entry, the main application window is created, which is actually a dialog box as defined in Usbview.rc. The dialog box consists of a split window with a tree view control on the left side and an edit control on the right side.

The routine RefreshTree() is called to enumerate USB host controller, hubs, and attached devices and to populate the device tree view control. RefreshTree() calls the routine EnumerateHostControllers() in Enum.c to enumerate USB host controller, hubs, and attached devices. After the device tree view control has been populated, USBView\_OnNotify() is called when an item is selected in the device tree view control. This calls UpdateEditControl() in Display.c to display information about the selected item in the edit control.

The file Enum.c contains the routines that enumerate the USB bus and populate the tree view control. The USB device enumeration and information collection process is the main point of this sample application. The enumeration process starts at EnumerateHostControllers() and goes like this:

1.  Enumerate Host Controllers and Root Hubs. Host controllers have symbolic link names of the form HCDx, where x starts at 0. Use CreateFile() to open each host controller symbolic link. Create a node in the tree view to represent each host controller. After a host controller has been opened, send the host controller an IOCTL\_USB\_GET\_ROOT\_HUB\_NAME request to get the symbolic link name of the root hub that is part of the host controller.
2.  Enumerate Hubs (Root Hubs and External Hubs). Given the name of a hub, use CreateFile() to open the hub. Send the hub an IOCTL\_USB\_GET\_NODE\_INFORMATION request to get info about the hub, such as the number of downstream ports. Create a node in the tree view to represent each hub.
3.  Enumerate Downstream Ports. Given a handle to an open hub and the number of downstream ports on the hub, send the hub an IOCTL\_USB\_GET\_NODE\_CONNECTION\_INFORMATION request for each downstream port of the hub to get info about the device (if any) attached to each port. If there is a device attached to a port, send the hub an IOCTL\_USB\_GET\_NODE\_CONNECTION\_NAME request to get the symbolic link name of the hub attached to the downstream port. If there is a hub attached to the downstream port, recurse to step (2). Create a node in the tree view to represent each hub port and attached device. USB configuration and string descriptors are retrieved from attached devices in GetConfigDescriptor() and GetStringDescriptor() by sending an IOCTL\_USB\_GET\_DESCRIPTOR\_FROM\_NODE\_CONNECTION() to the hub to which the device is attached.

The file Display.c contains routines that display information about selected devices in the application edit control. Information about the device was collected during the enumeration of the device tree. This information includes USB device, configuration, and string descriptors and connection and configuration information that is maintained by the USB stack. The routines in this file simply parse and print the data structures for the device that were collected when it was enumerated. The file Dispaud.c parses and prints data structures that are specific to USB audio class devices.

