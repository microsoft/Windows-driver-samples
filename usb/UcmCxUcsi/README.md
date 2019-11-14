---
page_type: sample
description: "Demonstrates how to create a Windows USB Type-C port controller driver using the USB Connector Manager class extension driver (UcmCx)."
languages:
- cpp
products:
- windows
- windows-wdk
urlFragment: ucmtcpcicx-port-controller-client-driver-v2
---

# UcmCxUcsi Port Controller Client Driver

This is a sample driver that shows how to create a Windows USB Type-C port controller driver using the USB Connector Manager class extension driver (UcmCx). The sample is a driver for an embedded controller which is compliant with the [USB Type-C Connector System Software Interface (UCSI)](http://www.intel.com/content/www/us/en/io/universal-serial-bus/usb-type-c-ucsi-spec.html).

## Background reading - UcmCx documentation

Start at the UcmCx documentation at [USB Type-C connector driver programming reference](https://docs.microsoft.com/windows-hardware/drivers/ddi/content/_usbref/#type-c-driver-reference).

## Note on UCSI

Microsoft already provides an inbox UCSI driver, UcmUcsi.sys. This UcmCxUcsi sample driver is not identical to the inbox UCSI driver. Microsoft recommends that you use the inbox UcmUcsi.sys for your UCSI-compliant system rather than writing your own. This sample driver is meant for developers bringing up a UcmCx driver for their own non-UCSI platforms. For more information about the UCSI driver, reference [USB Type-C Connector System Software Interface (UCSI) driver](https://docs.microsoft.com/windows-hardware/drivers/usbcon/ucsi).

This sample demonstrates the following:

- Registration with the USB Connector Manager (UCM) class extension driver.

- Initializing the port controller's Type-C and Power Delivery capabilities.

- Performing data and power role swaps requested by UCM

- Notifying UCM of Type-C and Power Delivery events on the connector.

## Customizing the sample for your port controller

This sample is specific to UCSI systems. You may choose to structure your driver in a similar way for your own hardware. Understand the requirements and specification of your own system prior to writing a new UcmCx client driver. In addition to making the logic and functionality suit your specific port controller hardware, you will need to modify the .inf such that it matches your device's information.

## UcmCxUcsi structure

In this sample, UCM-specific interactions are split apart from most of the UCSI-specific operations.

### UcmCx interactions

The following files contain methods that interface with UcmCx.

- UcmCallbacks.cpp

  - Contains the implementations [EVT_UCM_CONNECTOR_SET_DATA_ROLE](https://docs.microsoft.com/windows-hardware/drivers/ddi/content/ucmmanager/nc-ucmmanager-evt_ucm_connector_set_data_role) and [EVT_UCM_CONNECTOR_SET_POWER_ROLE](https://docs.microsoft.com/windows-hardware/drivers/ddi/content/ucmmanager/nc-ucmmanager-evt_ucm_connector_set_power_role). These are callbacks from UCM which ask the client driver to perform role swaps.

- UcmNotifications.cpp

  - Contains methods that communicate with UcmCx using the client driver support methods described in the [USB Type-C connector driver programming reference](https://docs.microsoft.com/windows-hardware/drivers/ddi/content/_usbref/#type-c-driver-reference).

- Fdo.cpp

  - FDO callbacks, functions, and types, most of which do not interface with UCM. However, the method `Fdo_EvtDeviceSelfManagedIoInit` contains the code segment which initializes the device with UCM.

### UCSI and WDF Interactions

The remainder of the files perform operations for UCSI and WDF, non-specific to UCM.

- Acpi.cpp

  - ACPI method evaluation helper routines.

- Driver.cpp

  - Entry point to the driver. Initializes the driver with WDF.

- Ppm.cpp

  - Type-C Platform Policy Manager. Main interface to talk to the UCSI-compliant hardware.

## When to write a UcmCx client driver

UcmCx is intended for system port controller drivers. If you are bringing up a USB Type-C peripheral, you do not need to write a USB Type-C specific driver; a regular USB client driver will suffice. Refer to [Developing Windows client drivers for USB devices](https://docs.microsoft.com/windows-hardware/drivers/usbcon/usb-driver-development-guide) to determine what type of driver, if any, you need to write to make your USB device work with Windows. You may look at [Do I need to write a driver for my USB Type-C hardware?](https://techcommunity.microsoft.com/t5/Microsoft-USB-Blog/Do-I-need-to-write-a-driver-for-my-USB-Type-C-hardware/ba-p/270876) for a more detailed overview.
