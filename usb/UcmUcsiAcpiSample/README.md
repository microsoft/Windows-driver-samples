---
page_type: sample
description: "Demonstrates how to create a Windows USB Type-C port controller driver using the USB Connector Manager class extension driver (UcmCx)."
languages:
- cpp
products:
- windows
- windows-wdk
---

# UcmUcsiCx ACPI Client Driver

This is a sample driver that shows how to create a Windows USB Type-C port controller driver using the USB Connector Manager class extension driver (UcmCx). The sample is a driver for an embedded controller which is compliant with the [USB Type-C Connector System Software Interface (UCSI)](https://www.intel.com/content/www/us/en/io/universal-serial-bus/usb-type-c-ucsi-spec.html).

## Note on UCM UCSI

Microsoft already provides an inbox UCSI Class extension (UcmUcsiCx.sys) and ACPI client driver (UcmUcsiAcpiClient.sys) . This UcmUcsi ACPI sample driver is not to a very large extent identical to and functionally replaceable with UcmUcsiAcpiClient.sys. Microsoft recommends that you use this sample to bring up your UcmUcsiCx Client driver that is interfaced over a non ACPI transport.

This sample demonstrates the following:

- Preparing connector collection with UcmUcsiCx

- Initializing Platform Policy Manager (or PPM) object with UcmUcsiCx

- Handling IOCTLs from UcmUcsiCx to send or get UCSI block

- Delivering PPM notifications to UcmUcsiCx

## Customizing the sample for your UCSI transport

This sample is specific to ACPI based UCSI systems. You may choose to structure your driver in a similar way for your own hardware. Understand the requirements and specification of your own system prior to writing a new UcmUcsiCx client driver. In addition to making the logic and functionality suit your specific UCSI transport, you will need to modify the .inf such that it matches your device's information.

### UcmUcsiCx structure

In this sample, UCMUCSI-specific interactions are split apart from most of the UCSI-specific operations.

#### UcmUcsiCx Interactions

The following files contain methods that interface with UcmUcsiCx.

- Ppm.cpp

  - Contains bulk of interactions with UcmUcsiCx. Ppm::CreateAndInitialize which is called in the context of PrepareHardware PnP callback, creates connector collection with UcmUcsiCx by calling DDI UcmUcsiConnectorCollectionCreate and  UcmUcsiConnectorCollectionAddConnector. The collection serves as a parameter to initialize PPM object with the class extension when the DDI UcmUcsiPpmCreate is called. Ppm::PpmNotificationCallback function calls DDI on the event of a PPM notification. Ppm::EvtIoDeviceControl contains the logic of handling IOCTLs from UcmUcsiCx.

- Fdo.cpp

  - FDO callbacks, functions, and types, most of which do not interface with UcmUcsiCx. However, the method Fdo::CreateAndInitialize calls to UcmUcsiCx DDIs UcmUcsiDeviceInitInitialize and UcmUcsiDeviceInitialize.

#### UCSI and WDF Interactions

The remainder of the files perform operations for UCSI and WDF, non-specific to UCM.

- Acpi.cpp

  - ACPI method evaluation helper routines.

- Driver.cpp

  - Entry point to the driver. Initializes the driver with WDF.

- Ppm.cpp

  - Type-C Platform Policy Manager. Main interface to talk to the UCSI-compliant hardware (via ACPI transport in this case).

## When to write a UcmUcsiCx client driver

UcmUcsiCx is intended to abstract UCSI protocol from UCSI based driver support. If you have a system that implements PPM logic in an Embedded controller which is connected to the system over a non ACPI transport - e.g. USB, PCI, I2C or UART, it is advised that you write a UcmUcsiCx client driver that implements the necessary transport. Ideally, you would just need to replace Acpi class and implement your way to talk to the PPM. Please note, UcmUcsiCx client is the power policy owner. The class extension does handle S0 idling if you chose to enable it in the client driver.
