---
page_type: sample
description: "Demonstrates how Driver Verifier (DV) can find errors in a WDM driver."
languages:
- cpp
products:
- windows
- windows-wdk
---

# DV-FailDriver-WDM

The DV-FailDriver-WDM sample driver contains intentional code errors that are designed to show the capabilities and features of [Driver Verifier](https://docs.microsoft.com/windows-hardware/drivers/devtest/driver-verifier) (DV) and the [Device Fundamentals tests](https://docs.microsoft.com/windows-hardware/drivers/devtest/device-fundamentals-tests).  Driver Verifier is a component of the Windows kernel designed to detect drivers that are behaving poorly and stop their execution via a bugcheck; the Device Fundamentals tests are a series of tests provided in the Windows Driver Kit (WDK) designed to provide a good basic set of tests against a driver.

> [!CAUTION]
> This sample driver contains intentional code errors that are designed to show the capabilities and features of DV and SDV. This sample driver is not intended as an example for real driver development projects.

## Build the sample for desktop

1. In the **Solutions Explorer** window, select the driver solution (DV-FailDriver-WDM).

1. From the **Build** menu, select **Build Solution**.

## Deploy the sample

See [Deploying a Driver to a Test Computer](https://docs.microsoft.com/windows-hardware/drivers/develop/deploying-a-driver-to-a-test-computer) for details on how to deploy the sample.

#### New

It is highly encouraged when testing a driver to use [DevGen](https://learn.microsoft.com/windows-hardware/drivers/devtest/devgen) to create a [SwDevice](https://learn.microsoft.com/windows/win32/api/_swdevice/).

__DevGen Usage Example__

1. From a terminal (running as admin) on test computer go to the WDK tools path and run `.\devgen /add /bus SWD /hardwareid root\defect_toastmon`. ***If WDK is not available on test computer, simply copy over `devgen.exe`***

2. There is now a `Generic software device` available in the system. ***Use Device Manager as a simple means to inspect or from a terminal run `pnputil /enum-devices /deviceid "root\defect_toastmon"`.***

3. Now that there is a device with the appropriate hardware ID, deploy/install driver sample accordingly.

> Note: Use `.\devgen /remove "<Device instance path>"` to remove SW device (the path can be copied from output of `.\devgen /add` or from Device Manager).
>
> Device instance path example `"SWD\DEVGEN\{D0299946-2EC2-C146-B00B-E01144166F8B}"`

## Test the sample

See [How to test a driver at runtime](https://docs.microsoft.com/windows-hardware/drivers/develop/how-to-test-a-driver-at-runtime-from-a-command-prompt) for details on how to run tests on the Toastmon driver.

To observe the injected defect being caught by DV, you should enable DV on the Toastmon driver, and then run the "DF - PNP Surprise Remove (Development and Integration)" test on the Toastmon sample driver and device.
