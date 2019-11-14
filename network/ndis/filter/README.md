---
page_type: sample
description: "A pass-through NDIS 6 filter driver demonstrating the basic principles of an NDIS 6.0 Filter driver."
languages:
- cpp
products:
- windows
- windows-wdk
---

# NDIS 6.0 Filter Driver

The Ndislwf sample is a do-nothing pass-through NDIS 6 filter driver that demonstrates the basic principles underlying an NDIS 6.0 Filter driver. The sample replaces the NDIS 5 Sample Intermediate Driver (Passthrough driver).

Although this sample filter driver is installed as a modifying filter driver, it doesn't modify any packets; it only repackages and sends down all OID requests. You can modify this filter driver to change packets before passing them along. Or you can use the filter to originate new packets to send or receive. For example, the filter could encrypt/compress outgoing and decrypt/decompress incoming data.

For more information, see [NDIS Filter Drivers](https://docs.microsoft.com/windows-hardware/drivers/network/ndis-filter-drivers) in the network devices design guide.

## Automatic deployment

Before you automatically deploy a driver, you must provision the target computer. For instructions, see [Provision a computer for driver deployment and testing](https://docs.microsoft.com/windows-hardware/drivers/gettingstarted/provision-a-target-computer-wdk-8-1).

After you have provisioned the target computer, continue with these steps:

1. On the host computer, in **Visual Studio**, in **Solution Explorer**, right click **package** (lower case), and choose **Properties**. Navigate to **Configuration Properties \> Driver Install \> Deployment**.

1. Check **Enable deployment**, and check **Remove previous driver versions before deployment**. For **Target Computer Name**, select the name of a target computer that you provisioned previously. Select **Do not install**. Click **OK**.

1. On the **Build** menu, choose **Build Solution**.

1. On the target computer, open **Control Panel**. Click **Network and Internet** and then open **Network and Sharing Center**.

1. Under **View your active networks**, click the connection listed under **Connections:** and click **Properties**. If you have previously installed this sample, highlight it in the list.

1. Click **Install**, then **Service**, then **Add**.

    > [!NOTE]
    > You may see multiple instances of the **NDIS Sample LightWeight Filter** service. If so, highlight the newest one.

1. Click **Have Disk**.

1. In the **Install from Disk** dialog, browse to the DriverTest\\Drivers directory. Highlight the netlwf.inf file and click **Open**, then click OK. This should show **NDIS Sample LightWeight Filter** in a list of **Network Services**. Highlight **NDIS Sample LightWeight Filter** and click **OK**. Click **OK**. Click **Close**. Click **Close**. This installs the Ndislwf filter driver service.

> [!NOTE]
> If you've installed the Ndislwf sample on the target computer before, you can use the [PnPUtil](https://docs.microsoft.com/windows-hardware/drivers/devtest/pnputil) tool to delete the older versions from the driver store.

## Manual deployment

Before you manually deploy a driver, you must turn on test signing and install a certificate on the target computer. You also need to copy the [DevCon](https://docs.microsoft.com/windows-hardware/drivers/devtest/devcon) tool to the target computer. For instructions, see [Preparing a Computer for Manual Driver Deployment](https://docs.microsoft.com/windows-hardware/drivers/develop/preparing-a-computer-for-manual-driver-deployment).

Ndislwf is installed as a service (called **NDIS Sample LightWeight Filter** in the supplied INF). To install it, do the following:

1. Prepare an installation directory on the target computer and copy these files from the host computer into the directory:

    - netlwf.cat

    - netlwf.inf

    - ndislwf.sys

1. Open **Control Panel**.

1. Click **Network and Internet** and then open **Network and Sharing Center**. Under **View your active networks**, click the connection listed under **Connections**: and click **Properties**.

1. If you have previously installed this sample, highlight it in the list.

1. Click **Install**, then **Service**, then **Add**, then **Have Disk**.

1. Browse to the installation directory. Highlight the netlwf.inf file and click **Open**, then click OK. This should show **NDIS Sample LightWeight Filter** in a list of Network Services. Highlight this and click OK. Click OK. This installs the Ndislwf filter driver.

> [!NOTE]
> If you've installed the Ndislwf sample on the target computer before, you can use the [PnPUtil](https://docs.microsoft.com/windows-hardware/drivers/devtest/pnputil) tool to delete the older versions from the driver store.

## Viewing sample output in the debugger

### Setting up kernel-mode debugging automatically

If you chose to deploy your driver automatically, then kernel debugging is already set up for you.

On the host computer, in **Visual Studio**, in the **Debug** menu, choose **Attach to Process**. For **Transport**, choose **Windows Kernel Mode Debugger**. For **Qualifier**, choose the name of your target computer. Click **Attach**.

> [!NOTE]
> If you see a dialog box that asks you to allow the debugger to communicate through the firewall, click the boxes for all types of networks. Click **Allow Access**.

For more information, see [Setting Up Kernel-Mode Debugging in Visual Studio](https://docs.microsoft.com/windows-hardware/drivers/debugger/setting-up-kernel-mode-debugging-in-visual-studio).

### Setting up kernel-mode debugging manually

If you chose to deploy your driver manually, then you need to set up kernel debugging manually. For instructions, see [Setting Up Kernel-Mode Debugging Manually](https://docs.microsoft.com/windows-hardware/drivers/debugger/setting-up-kernel-mode-debugging-in-visual-studio).

The kernel-mode debuggers (WinDbg.exe and Kd.exe) are included in the WDK.

On the host computer, locate and open a kernel-mode debugger (example: c:\\Program Files (x86)\\Windows Kits\\10\\Debuggers\\x64\\windbg.exe). Establish a kernel-mode debugging session between the host and target computers. The details of how to do this depend on the type of debug cable you are using. For information about how to start a debugging session, see [Setting Up Kernel-Mode Debugging Manually](https://docs.microsoft.com/windows-hardware/drivers/debugger/setting-up-a-network-debugging-connection).

## Setting kd\_default\_mask

This sample calls [**DbgPrint**](https://docs.microsoft.com/windows-hardware/drivers/ddi/content/wdm/nf-wdm-dbgprint) to send trace messages to the kernel-mode debugger. To see the trace messages, you must set the value of the **kd\_default\_mask** variable.

On the host computer, break in to the debugger if you are not already broken in. (In the **Debug** menu, choose **Break** or **Break All**, or press **CTRL-Break**). At the debugger command line, enter this command: **ed kd\_default\_mask 0x8**.

To resume execution of the target computer, enter the **g** command in the debugger. (In the **Debug** menu, choose **Continue**.)

## Viewing trace messages

On the host computer, in the kernel-mode debugger, verify that you see trace messages similar to these:

```txt
NDISLWF: ===>DriverEntry...
NDISLWF: ===>FilterRegisterOptions
NDISLWF: <===FilterRegisterOptions
NDISLWF: ==>FilterRegisterDevice
NDISLWF: <==FilterRegisterDevice: 0
NDISLWF: <===DriverEntry, Status =        0
NDISLWF: ===>FilterAttach: NdisFilterHandle FFFFE00000F73650
NDISLWF: <===FilterAttach:    Status 0
```

## What the Ndislwf sample driver does

1. During [*DriverEntry*](https://docs.microsoft.com/windows-hardware/drivers/ddi/content/wdm/nc-wdm-driver_initialize), the ndislwf driver registers as an NDIS 6 filter driver.

1. Later on, NDIS calls Ndislwf's [*FilterAttach*](https://docs.microsoft.com/windows-hardware/drivers/ddi/content/ndis/nc-ndis-filter_attach) handler, for each underlying NDIS adapter on which it is configured to attach.

1. In the context of [*FilterAttach*](https://docs.microsoft.com/windows-hardware/drivers/ddi/content/ndis/nc-ndis-filter_attach) Handler, the filter driver calls [**NdisFSetAttributes**](https://docs.microsoft.com/windows-hardware/drivers/ddi/content/ndis/nf-ndis-ndisfsetattributes) to register its filter module context with NDIS. After that, the filter driver can read its own setting in registry by calling [**NdisOpenConfigurationEx**](https://docs.microsoft.com/windows-hardware/drivers/ddi/content/ndis/nf-ndis-ndisopenconfigurationex), and call other `NdisXxx` functions.

1. After [*FilterAttach*](https://docs.microsoft.com/windows-hardware/drivers/ddi/content/ndis/nc-ndis-filter_attach) successfully returns, NDIS restarts the filter later by calling its [*FilterRestart*](https://docs.microsoft.com/windows-hardware/drivers/ddi/content/ndis/nc-ndis-filter_restart) handler. *FilterRestart* should prepare to handle send/receive data. After restart return successfully, filter driver should be able to process send/receive.

1. All requests and sends coming from overlying drivers for the Ndislwf filter driver are repackaged if necessary and sent down to NDIS, to be passed to the underlying NDIS driver.

1. All indications arriving from an underlying NDIS driver are forwarded up by Ndislwf filter driver.

1. NDIS calls the filter's [*FilterPause*](https://docs.microsoft.com/windows-hardware/drivers/ddi/content/ndis/nc-ndis-filter_pause) handler when NDIS needs to detach the filter from the stack or there is some configuration changes in the stack. In processing the pause request from NDIS, the Ndislwf driver waits for all its own outstanding requests to be completed before it completes the pause request.

1. NDIS calls the Ndislwf driver's [*FilterDetach*](https://docs.microsoft.com/windows-hardware/drivers/ddi/content/ndis/nc-ndis-filter_detach) entry point when NDIS needs to detach a filter module from NDIS stack. The *FilterDetach* handler should free all the memory allocation done in [*FilterAttach*](https://docs.microsoft.com/windows-hardware/drivers/ddi/content/ndis/nc-ndis-filter_attach), and undo the operations it did in *FilterAttach* Handler.
