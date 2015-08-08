PCIDRV - WDF Driver for PCI Device
==================================

This sample demonstrates how to write a KMDF driver for a PCI device. The sample works with the Intel 82557/82558 based PCI Ethernet Adapter (10/100) and Intel compatibles.

This adapter supports scatter-gather DMA, wake on external event (Wait-Wake), and idle power down. The hardware specification is publicly available, and the source code to interface with the hardware is included in the WDK.


Overview
--------

The following is a list of key KMDF interfaces demonstrated in this sample:

-   Handling PnP & Power Events

-   Registering Device Interface

-   Hardware resource mapping: Port, Memory & Interrupt

-   DMA Interfaces

-   Parallel default queue for write requests. If the write cannot be satisfied immediately, the request is put into a manual parallel queue.

-   Parallel manual queue for Read requests

-   Parallelc default queue for IOCTL requests. If the ioctl cannot be satisfied immediately, the request is put into a manual parallel queue.

-   Request cancelation

-   Handling Interrupt & DPC

-   Watchdog Timer DPC to monitor the device state.

-   Event Tracing & HEXDUMP

-   Reading & Writing to the registry

Note: This sample provides an example of a minimal driver intended for educational purposes. Neither the driver nor its sample test programs are intended for use in a production environment.

As stated earlier, this sample is meant to demonstrate how to write a KMDF driver for a generic PCI device and not for PCI network controllers. For network controllers, you should write a monolithic NDIS miniport driver based on the samples given under the \\network\\ndis directory.

Note that it is still possible to use a subset of KMDF APIs when writing a NDIS miniport (see \\network\\ndis\\usbnwifi directory for a sample on how to use KMDF interfaces to talk to USB device in an NDIS miniport).

The sample driver has been tested on the following Intel Ethernet controllers:

Device Description | Hardware ID
-------------------|------------
IBM Netfinity 10/100 Ethernet Adapter |  PCIVEN_8086&DEV_1229&SUBSYS_005C1014&REV_05
Intel(R) PRO/100+ Management Adapter with Alert On LAN | PCI\VEN_8086&DEV_1229&SUBSYS_000E8086&REV_08
Intel 8255x-based PCI Ethernet Adapter (10/100) | PCI\VEN_8086&DEV_1229&SUBSYS_00000000&REV_01
Intel Pro/100 S Server Adapter | PCI\VEN_8086&DEV_1229&SUBSYS_00508086&REV_0D
Intel 8255x-based PCI Ethernet Adapter (10/100) | PCI\VEN_8086&DEV_1229&SUBSYS_00031179&REV_08
Intel(R) PRO/100 VE Network Connection | PCI\VEN_8086&DEV_103D&SUBSYS_00011179&REV_83
Intel(R) PRO/100 VM Network Connection | PCI\VEN_8086&DEV_1031&REV_42
Intel(R) PRO/100 VE Network Connection | PCI\VEN_8086&DEV_1038&REV_41
Intel(R) PRO/100 SR Mobile Adapter | PCI\VEN_8086&DEV_1229

Using this sample as a standalone driver
----------------------------------------

```
          ---------------------
         |                     |
         |        MYPING       | <-- Usermode test application
         |                     |
          ---------------------
                   ^
                   |                                     UserMode
-------------------------------------------------------------------
                   |                                     KernelMode
                   V
          ---------------------
         |                     |
         |        PCIDRV       | <-- Installed as a function driver
         |                     |
          ---------------------
                   ^
                   | <-----Talk to the hardware using I/O resources
                   V
             ---------------
            |    H/W NIC    |
             ---------------
                    |||||||
                    -------
```

You can install the driver as a standalone driver of a custom setup class, called Sample Class using GENPCI.INF. The PCI device is not seen as a network controller and as a result no protocol driver is bound to the device. In order to test the read & write path of the driver, you can use the specially developed ping application, called MYPING. This test application crafts the entire Ethernet frame in usermode and sends it to the driver to be transferred on the wire. In this configuration, you can only ping another machine on the same subnet. The application does all the ARP and AARP resolution in the usermode to get the MAC address of the target machine and sends ICMP ECHO requests.

The PCIDRV sample acts as a power policy owner of the device and implements all the wait-wake and idle detection logic.

INSTALLATION
------------

The driver can be installed as a Net class driver or as a standalone driver (user defined class). The KMDF versions of the INF files are dynamically generated from .INX file. In addition to the driver files, you have to include the WDF coinstaller DLL from the \\redist\\wdf folder of the WDK.

You can obtain redistributable framework updates by downloading the *wdfcoinstaller.msi* package from [WDK 8 Redistributable Components](http://go.microsoft.com/fwlink/p/?LinkID=226396). This package performs a silent install into the directory of your Windows Driver Kit (WDK) installation. You will see no confirmation that the installation has completed. You can verify that the redistributables have been installed on top of the WDK by ensuring there is a redist\\wdf directory under the root directory of the WDK, %ProgramFiles(x86)%\\Windows Kits\\8.0.

### TESTING

To test standalone driver configuration: You should use the specially developed ping application, called MYPING that comes with the sample. The Ping.exe provided in the system will not work because in this configuration, the test card is not bound to any network protocol - it's not seen as Net device by the system. Currently the test application doesn't have ability to get an IP address from a network DHCP server. As a result, it is better to connect the network device to a private hub and ping another machine connected to that hub. For example, let us say you have a test machine A and another machine B (development box).

-   Connect machine A and Machine B to a local hub.

-   Assign a static IP address, say 128.0.0.1 to the NIC on machine B.

-   Clear the ARP table on machine B by running **Arp -d** on the command line

-   Now run Myping.exe. This application enumerates GUID\_DEVINTERFACE\_PCIDRV and displays the name of the devices with an index number. This number will be used in identifying the interface when you invoke ping dialog.

-   In the ping dialog specify the following and click okay:

-   Device Index: 1 \<- number displayed in the list window

-   Source Ip Address: 128.0.0.4 \<- You can make up any valid IP address for test Machine A

-   Destination IP Address: 128.0.0.1 \<- IP address of machine B

-   Packet Size: 1428 \<- Default max size of ping payload. Minimum value is 32 bytes.

If the machine B has more than one adapter and if the second adapter is connected to the internet (Corporate Network), instead of assigning static IP address to the adapter that's connected to the test machine, you can install Internet Connection Sharing (ICS) on it and get an IP address for ICS. This would let you use the test machine to browse the internet when the sample is installed in the miniport configuration and also in the standalone mode without making up or stealing somebody's IP address. For example, let us say the machine B has two adapters NIC1 and NIC2. NIC1 is connected to the CorpNet and NIC2 is connected to the private hub. Install ICS on NIC2 as described below:

-   Select the NIC2 in the Network Connections Applet.

-   Click the **Properties** button.

-   Go to the Advanced Tab and Check the box "Allow Other network users to connect through this computers internet connection" in the Internet Connection Sharing choice.

-   This will assign 192.168.0.1 IP address to NIC2.

-   Now on machine B, you can assume 192.168.0.2 as the local IP address and run Myping.exe . Or, you can install the sample in the miniport configuration and browse the internet.

Other menu options of myping applications are:

-   Reenumerate All Device: This command lets you terminate active ping threads and close handle to all the device and reenumerate the devices again and display their names with index numbers. This might cause the devices to have new index numbers.

-   Cleanup: This command terminates ping threads and closes handles to all the devices.

-   Clear Display: Clears the window.

-   Verbose: Let you get more debug messages.

-   Exit: Terminate the application.

**Note** You can use this application only on a device installed in the standalone configuration. If you run it on a device that's installed as a miniport, you will get an error message. For such devices, you can use the system provided ping.exe.

RESOURCES
---------

For the latest release of the Windows Driver Kit, see http://www.microsoft.com/whdc/.

If you have questions on using or adapting this sample for your project, you can either contact Microsoft Technical Support or post your questions in the Microsoft driver development newsgroup.

FILE MANIFEST
-------------

File | Description
-----|------------
KMDF | Contains the driver.
KMDF\HW | Contains hardware specific code.
TEST | Contains source of test application (MYPING).



