NDIS 6.0 Filter Driver
======================

The Ndislwf sample is a do-nothing pass-through NDIS 6 filter driver that demonstrates the basic principles underlying an NDIS 6.0 Filter driver. The sample replaces the NDIS 5 Sample Intermediate Driver (Passthru driver).

Although this sample filter driver is installed as a modifying filter driver, it doesn't modify any packets; it only repackages and sends down all OID requests. You can modify this filter driver to change packets before passing them along. Or you can use the filter to originate new packets to send or receive. For example, the filter could encrypt/compress outgoing and decrypt/decompress incoming data.


For more information, see [NDIS Filter Drivers](http://msdn.microsoft.com/en-us/library/windows/hardware/ff565492) in the network devices design guide.


Automatic deployment
--------------------

Before you automatically deploy a driver, you must provision the target computer. For instructions, see [Configuring a Computer for Driver Deployment, Testing, and Debugging](http://msdn.microsoft.com/en-us/library/windows/hardware/). After you have provisioned the target computer, continue with these steps:

1.  On the host computer, in **Visual Studio**, in **Solution Explorer**, right click **package** (lower case), and choose **Properties**. Navigate to **Configuration Properties \> Driver Install \> Deployment**.
2.  Check **Enable deployment**, and check **Remove previous driver versions before deployment**. For **Target Computer Name**, select the name of a target computer that you provisioned previously. Select **Do not install**. Click **OK**.
3.  On the **Build** menu, choose **Build Solution**.
4.  On the target computer, open **Control Panel**. Click **Network and Internet** and then open **Network and Sharing Center**.
5.  Under **View your active networks**, click the connection listed under **Connections:** and click **Properties**. If you have previously installed this sample, highlight it in the list.
6.  Click **Install**, then **Service**, then **Add**.
    **Note** You may see multiple instances of the **NDIS Sample LightWeight Filter** service. If so, highlight the newest one.
7.  Click **Have Disk**.
8.  In the **Install from Disk** dialog, browse to the DriverTest\\Drivers directory. Highlight the netlwf.inf file and click **Open**, then click OK. This should show **NDIS Sample LightWeight Filter** in a list of **Network Services**. Highlight **NDIS Sample LightWeight Filter** and click **OK**. Click **OK**. Click **Close**. Click **Close**. This installs the Ndislwf filter driver service.

**Note** If you've installed the Ndislwf sample on the target computer before, you can use the [PnPUtil](http://msdn.microsoft.com/en-us/library/windows/hardware/ff550419) tool to delete the older versions from the driver store.

Manual deployment
-----------------

Before you manually deploy a driver, you must turn on test signing and install a certificate on the target computer. You also need to copy the [DevCon](http://msdn.microsoft.com/en-us/library/windows/hardware/ff544707) tool to the target computer. For instructions, see [Preparing a Computer for Manual Driver Deployment](http://msdn.microsoft.com/en-us/library/windows/hardware/dn265571).

Ndislwf is installed as a service (called **NDIS Sample LightWeight Filter** in the supplied INF). To install it, do the following:

1.  Prepare an installation directory on the target computer and copy these files from the host computer into the directory:
```
    netlwf.cat
    netlwf.inf
    ndislwf.sys
```
2.  Open **Control Panel**.
3.  Click **Network and Internet** and then open **Network and Sharing Center**. Under **View your active networks**, click the connection listed under **Connections**: and click **Properties**.
4.  If you have previously installed this sample, highlight it in the list.
5.  Click **Install**, then **Service**, then **Add**, then **Have Disk**.
6.  Browse to the installation directory. Highlight the netlwf.inf file and click **Open**, then click OK. This should show **NDIS Sample LightWeight Filter** in a list of Network Services. Highlight this and click OK. Click OK. This installs the Ndislwf filter driver.

**Note** If you've installed the Ndislwf sample on the target computer before, you can use the [PnPUtil](http://msdn.microsoft.com/en-us/library/windows/hardware/ff550419) tool to delete the older versions from the driver store.

Viewing sample output in the debugger
-------------------------------------

### Setting up kernel-mode debugging automatically

If you chose to deploy your driver automatically, then kernel debugging is already set up for you.

On the host computer, in **Visual Studio**, in the **Debug** menu, choose **Attach to Process**. For **Transport**, choose **Windows Kernel Mode Debugger**. For **Qualifier**, choose the name of your target computer. Click **Attach**.

**Note** If you see a dialog box that asks you to allow the debugger to communicate through the firewall, click the boxes for all types of networks. Click **Allow Access**.

For more information, see [Setting Up Kernel-Mode Debugging in Visual Studio](http://msdn.microsoft.com/en-us/library/windows/hardware/hh439376).

### Setting up kernel-mode debugging manually

If you chose to deploy your driver manually, then you need to set up kernel debugging manually. For instructions, see [Setting Up Kernel-Mode Debugging Manually](http://msdn.microsoft.com/en-us/library/windows/hardware/hh439378).

The kernel-mode debuggers (WinDbg.exe and Kd.exe) are included in the WDK.

On the host computer, locate and open a kernel-mode debugger (example: c:\\Program Files (x86)\\Windows Kits\\10\\Debuggers\\x64\\windbg.exe). Establish a kernel-mode debugging session between the host and target computers. The details of how to do this depend on the type of debug cable you are using. For information about how to start a debugging session, see [Setting Up Kernel-Mode Debugging Manually](http://msdn.microsoft.com/en-us/library/windows/hardware/hh439378).

Setting kd\_default\_mask
-------------------------

This sample calls [**DbgPrint**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff543632) to send trace messages to the kernel-mode debugger. To see the trace messages, you must set the value of the **kd\_default\_mask** variable.

On the host computer, break in to the debugger if you are not already broken in. (In the **Debug** menu, choose **Break** or **Break All**, or press **CTRL-Break**). At the debugger command line, enter this command: **ed kd\_default\_mask 0x8**.

To resume execution of the target computer, enter the **g** command in the debugger. (In the **Debug** menu, choose **Continue**.)

Viewing trace messages
----------------------

On the host computer, in the kernel-mode debugger, verify that you see trace messages similar to these:
```
NDISLWF: ===>DriverEntry...
NDISLWF: ===>FilterRegisterOptions
NDISLWF: <===FilterRegisterOptions
NDISLWF: ==>FilterRegisterDevice
NDISLWF: <==FilterRegisterDevice: 0
NDISLWF: <===DriverEntry, Status =        0
NDISLWF: ===>FilterAttach: NdisFilterHandle FFFFE00000F73650
NDISLWF: <===FilterAttach:    Status 0
```
What the Ndislwf sample driver does:
------------------------------------

1.  During [*DriverEntry*](http://msdn.microsoft.com/en-us/library/windows/hardware/ff544113), the ndislwf driver registers as an NDIS 6 filter driver.
2.  Later on, NDIS calls Ndislwf's [*FilterAttach*](http://msdn.microsoft.com/en-us/library/windows/hardware/ff549905) handler, for each underlying NDIS adapter on which it is configured to attach.
3.  In the context of [*FilterAttach*](http://msdn.microsoft.com/en-us/library/windows/hardware/ff549905) Handler, the filter driver calls [**NdisFSetAttributes**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff562619) to register its filter module context with NDIS. After that, the filter driver can read its own setting in registry by calling [**NdisOpenConfigurationEx**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff563717), and call other `NdisXxx` functions.
4.  After [*FilterAttach*](http://msdn.microsoft.com/en-us/library/windows/hardware/ff549905) successfully returns, NDIS restarts the filter later by calling its [*FilterRestart*](http://msdn.microsoft.com/en-us/library/windows/hardware/ff549962) handler. *FilterRestart* should prepare to handle send/receive data. After restart return successfully, filter driver should be able to process send/receive.
5.  All requests and sends coming from overlying drivers for the Ndislwf filter driver are repackaged if necessary and sent down to NDIS, to be passed to the underlying NDIS driver.
6.  All indications arriving from an underlying NDIS driver are forwarded up by Ndislwf filter driver.
7.  NDIS calls the filter's [*FilterPause*](http://msdn.microsoft.com/en-us/library/windows/hardware/ff549957) handler when NDIS needs to detach the filter from the stack or there is some configuration changes in the stack. In processing the pause request from NDIS, the Ndislwf driver waits for all its own outstanding requests to be completed before it completes the pause request.
8.  NDIS calls the Ndislwf driver's [*FilterDetach*](http://msdn.microsoft.com/en-us/library/windows/hardware/ff549918) entry point when NDIS needs to detach a filter module from NDIS stack. The *FilterDetach* handler should free all the memory allocation done in [*FilterAttach*](http://msdn.microsoft.com/en-us/library/windows/hardware/ff549905), and undo the operations it did in *FilterAttach* Handler.


