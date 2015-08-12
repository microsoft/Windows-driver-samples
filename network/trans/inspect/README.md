Windows Filtering Platform Traffic Inspection Sample
====================================================

This sample driver demonstrates the traffic inspection capabilities of the Windows Filtering Platform (WFP).

The sample driver consists of a kernel-mode Windows Filtering Platform (WFP) callout driver (Inspect.sys) that intercepts all transport layer traffic (for example, Transmission Control Protocol (TCP), User Datagram Protocol (UDP), and nonerror Internet Control Message Protocol (ICMP)) sent to or received from a configurable remote peer and queues then to a worker thread for out-of-band processing.

Inspect.sys inspects inbound and outbound connections and all packets that belong to those connections. Additionally, Inspect.sys demonstrates the special considerations that are required to be compatible with Internet Protocol security (IPsec).

Inspect.sys implements the `ClassifyFn` callout functions for the ALE Connect, Recv-Accept, and Transport callouts. In addition, the system worker thread that performs the actual packet inspection is also implemented along with the event mechanisms that are shared between the Classify function and the worker thread.

Connect/Packet inspection is done out-of-band by a system worker thread by using the reference-drop-clone-reinject mechanism as well as the ALE pend/complete mechanism. Therefore, the sample can serve as a basis for scenarios in which a filtering decision cannot be made within the `classifyFn()` callout and instead must be made, for example, by a user-mode application.

## Universal Windows Driver Compliant
This sample builds a Universal Windows Driver. It uses only APIs and DDIs that are included in OneCoreUAP.

Automatic deployment
--------------------

Before you automatically deploy a driver, you must provision the target computer. For instructions, see [Configuring a Computer for Driver Deployment, Testing, and Debugging](http://msdn.microsoft.com/en-us/library/windows/hardware/). After you have provisioned the target computer, continue with these steps:

1.  On the host computer, in Visual Studio, in Solution Explorer, right click **package** (lower case), and choose **Properties**. Navigate to **Configuration Properties \> Driver Install \> Deployment**.
2.  Check **Enable deployment**, and check **Remove previous driver versions before deployment**. For **Target Computer Name**, select the name of a target computer that you provisioned previously. Select **Do not install**. Click **OK**.
3.  On the **Build** menu, choose **Build Solution**.
4.  On the target computer, navigate to DriverTest\\Drivers, and locate the file inspect.inf. Right click inspect.inf, and choose **Install**.

Manual deployment
-----------------

Before you manually deploy a driver, you must turn on test signing and install a certificate on the target computer. You also need to copy the [DevCon](http://msdn.microsoft.com/en-us/library/windows/hardware/ff544707) tool to the target computer. For instructions, see [Preparing a Computer for Manual Driver Deployment](http://msdn.microsoft.com/en-us/library/windows/hardware/dn265571). After you have prepared the target computer for manual deployment, continue with these steps:

1.  Copy all of the files in your driver package to a folder on the target computer (for example, c:\\WfpTrafficInspectionSamplePackage).
2.  On the target computer, navigate to your driver package folder. Right click inspect.inf, and choose **Install**

Create Registry values
----------------------

1.  On the target computer, open Regedit, and navigate to this key:

    **HKLM**\\**System**\\**CurrentControlSet**\\**Services**\\**inspect**\\**Parameters**

2.  Create a REG\_DWORD entry named **BlockTraffic** and set it's value to 0 for permit or 1 to block.

3.  Create a REG\_SZ entry named **RemoteAddressToInspect**, and set it's value to an IPV4 or IPV6 address (example: 10.0.0.2).

Start the inspect service
-------------------------

On the target computer, open a Command Prompt window as Administrator, and enter **net start inspect**. (To stop the driver, enter **net stop inspect**.)

Remarks
-------

For more information on creating a Windows Filtering Platform Callout Driver, see [Windows Filtering Platform Callout Drivers](http://msdn.microsoft.com/en-us/library/windows/hardware/ff571068).

