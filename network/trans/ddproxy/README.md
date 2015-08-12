Windows Filtering Platform Packet Modification Sample
=====================================================

The sample driver demonstrates the packet modification capabilities of the Windows Filtering Platform (WFP).

## Universal Windows Driver Compliant
This sample builds a Universal Windows Driver. It uses only APIs and DDIs that are included in OneCoreUAP.

Run the sample
--------------

The computer where you install the driver is called the *target computer* or the *test computer*. Typically this is a separate computer from where you develop and build the driver package. The computer where you develop and build the driver is called the *host computer*.

The process of moving the driver package to the target computer and installing the driver is called *deploying the driver*. You can deploy the Windows Filtering Platform Packet Modification Sample driver automatically or manually.

Automatic deployment
--------------------

Before you automatically deploy a driver, you must provision the target computer. For instructions, see [Configuring a Computer for Driver Deployment, Testing, and Debugging](http://msdn.microsoft.com/en-us/library/windows/hardware/). After you have provisioned the target computer, continue with these steps:

1.  On the host computer, in Visual Studio, in Solution Explorer, right click **package** (lower case), and choose **Properties**. Navigate to **Configuration Properties \> Driver Install \> Deployment**.
2.  Check **Enable deployment**, and check **Remove previous driver versions before deployment**. For **Target Computer Name**, select the name of a target computer that you provisioned previously. Select **Do not install**. Click **OK**.
3.  On the **Build** menu, choose **Build Solution**.
4.  On the target computer, navigate to DriverTest\\Drivers, and locate the file ddproxy.inf. Right click ddproxy.inf, and choose **Install**.

Manual deployment
-----------------

Before you manually deploy a driver, you must turn on test signing and install a certificate on the target computer. You also need to copy the [DevCon](http://msdn.microsoft.com/en-us/library/windows/hardware/ff544707) tool to the target computer. For instructions, see [Preparing a Computer for Manual Driver Deployment](http://msdn.microsoft.com/en-us/library/windows/hardware/dn265571). After you have prepared the target computer for manual deployment, continue with these steps:

1.  Copy all of the files in your driver package to a folder on the target computer (for example, c:\\WfpPacketModificationSamplePackage).
2.  On the target computer, navigate to your driver package folder. Right click ddproxy.inf, and choose **Install**

Create Registry values
----------------------

1.  On the target computer, open Regedit, and navigate to this key:

    **HKLM**\\**System**\\**CurrentControlSet**\\**Services**\\**ddproxy**\\**Parameters**

2.  Create a REG\_SZ entry named **DestinationAddressToIntercept** and set it's value to an IPV4 or IPV6 address (example: 10.0.0.1).

3.  Create a REG\_SZ entry named **NewDestinationAddress**, and set it's value to an IPV4 or IPV6 address (example: 10.0.0.2).

You can also create and set values for the following registry entries.

-   **InspectUdp** (REG\_DWORD type): 0 for ICMP and 1 for UDP (default)
-   **DestinationPortToIntercept** (REG\_DWORD type): UDP port number (applicable if InspectUdp is set to 1)
-   **NewDestinationPort** (REG\_DWORD type): UDP port number (applicable if InspectUdp is set to 1)

Start the ddproxy service
-------------------------

On the target computer, open a Command Prompt window as Administrator, and enter **net start ddproxy**. (To stop the driver, enter **net stop ddproxy**.)

Remarks
-------

This sample driver consists of a kernel-mode Windows Filtering Platform (WFP) callout driver (Ddproxy.sys) that intercepts User Datagram Protocol (UDP) and nonerror Internet Control Message Protocol (ICMP) traffic of interest and acts as a redirector. For outbound traffic, Ddproxy.sys redirects the traffic to a new destination address and, for UDP, a new UDP port. For inbound traffic, Ddproxy.sys redirects the traffic back to the original address and UDP port values. This redirection is transparent to the application.

Packet modification is done out-of-band by a system worker thread by using the reference-drop-clone-modify-reinject mechanism. Therefore, the sample can serve as a basis for scenarios in which the filtering/modification decision cannot be made within the `classifyFn()` callout, but instead must be made, for example, by a user-mode application.

Ddproxy.sys acts as a redirector for both Internet Protocol version 4 (IPv4) and Internet Protocol version 6 (IPv6) traffic.

For more information on creating a Windows Filtering Platform Callout Driver, see [Windows Filtering Platform Callout Drivers](http://msdn.microsoft.com/en-us/library/windows/hardware/ff571068).

