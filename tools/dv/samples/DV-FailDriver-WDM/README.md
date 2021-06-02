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

## Test the sample

See [How to test a driver at runtime](https://docs.microsoft.com/windows-hardware/drivers/develop/how-to-test-a-driver-at-runtime-from-a-command-prompt) for details on how to run tests on the Toastmon driver.

To observe the injected defect being caught by DV, you should enable DV on the Toastmon driver, and then run the "DF - PNP Surprise Remove (Development and Integration)" test on the Toastmon sample driver and device.
