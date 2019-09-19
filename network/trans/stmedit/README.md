---
page_type: sample
description: "Demonstrates replacing a string pattern for a Transmission Control Protocol (TCP) connection using the Windows Filtering Platform (WFP)."
languages:
- cpp
products:
- windows
- windows-wdk
---

# Windows Filtering Platform Stream Edit Sample

This sample driver demonstrates replacing a string pattern for a Transmission Control Protocol (TCP) connection using the Windows Filtering Platform (WFP).

## Universal Windows Driver Compliant

This sample builds a Universal Windows Driver. It uses only APIs and DDIs that are included in OneCoreUAP.

The sample consists of a kernel-mode Windows Filtering Platform (WFP) callout driver (Stmedit.sys) that can operate in one of the following modes:

- Inline editing where all modification is done within the `ClassifyFn` callout function.

- Out-of-band editing where all modification is done by a worker thread (the default).

The sample performs inspection for both Internet Protocol version 4 (IPv4) and Internet Protocol version 6 (IPv6) traffic.

Before experimenting with the sample, add an exception for the InspectionPort to your host firewall.

## Automatic deployment

Before you automatically deploy a driver, you must provision the target computer. For instructions, see [Provision a computer for driver deployment and testing](https://docs.microsoft.com/windows-hardware/drivers/gettingstarted/provision-a-target-computer-wdk-8-1).

After you have provisioned the target computer, continue with these steps:

1. On the host computer, in Visual Studio, in Solution Explorer, right click **package** (lower case), and choose **Properties**. Navigate to **Configuration Properties \> Driver Install \> Deployment**.

1. Check **Enable deployment**, and check **Remove previous driver versions before deployment**. For **Target Computer Name**, select the name of a target computer that you provisioned previously. Select **Do not install**. Click **OK**.

1. On the **Build** menu, choose **Build Solution**.

1. On the target computer, navigate to DriverTest\\Drivers, and locate the file stmedit.inf. Right click stmedit.inf, and choose **Install**.

## Manual deployment

Before you manually deploy a driver, you must turn on test signing and install a certificate on the target computer. You also need to copy the [DevCon](https://docs.microsoft.com/windows-hardware/drivers/devtest/devcon) tool to the target computer. For instructions, see [Preparing a Computer for Manual Driver Deployment](https://docs.microsoft.com/windows-hardware/drivers/develop/preparing-a-computer-for-manual-driver-deployment).

After you have prepared the target computer for manual deployment, continue with these steps:

1. Copy all of the files in your driver package to a folder on the target computer (for example, c:\\WfpStreamEditSamplePackage).

1. On the target computer, navigate to your driver package folder. Right click stmedit.inf, and choose **Install**

## Create Registry values

- On the target computer, open Regedit, and navigate to this key:

  **HKLM**\\**System**\\**CurrentControlSet**\\**Services**\\**strmedit**\\**Parameters**

You can create and set values for the following registry entries.

- **StringToFind** (REG_SZ, default = "rainy")

- **StringX** (REG_SZ, default = "cloudy")

- **StringToReplace** (REG_SZ, default = "sunny")

- **InspectionLocalPort** (REG_DWORD, default = 8888)

- **InspectionRemotePort** (REG_DWORD, default = 0)

    > [!NOTE]
    > For this sample, a local or remote port is mandatory. Both cannot be zero.

- **InspectioDirection** (REG_DWORD, default = 2) Possible values : 2 (inbound + outbound), 0 (FWP_DIRECTION_OUTBOUND), 1 (FWP_DIRECTION_INBOUND)

- **MultipleCallouts** (REG_DWORD, default = true/1) Controls registration of multiple callouts. Set 0 for false, other for TRUE

- **BusyThreshold** (REG_DWORD, default = 16KB) BusyThreshold value is in KBs (e.g. a value of 5 means 5KB)

## Start the stmedit service

On the target computer, open a Command Prompt window as Administrator, and enter **net start stmedit**. (To stop the driver, enter **net stop stmedit**.)

## Remarks

For more information on creating a Windows Filtering Platform Callout Driver, see [Windows Filtering Platform Callout Drivers](https://docs.microsoft.com/windows-hardware/drivers/network/windows-filtering-platform-callout-drivers2).
