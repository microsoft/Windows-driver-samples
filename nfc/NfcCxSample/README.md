---
page_type: sample
description: "Sample code for creating a NFC driver using the NFC Class Extension."
languages:
- cpp
products:
- windows
- windows-wdk
---

# NFC CX client driver sample

A template for creating an NFC functional driver using the NFC Class Extension (CX) driver.

This is a complete version of the code created in the [NFC CX quick start guide](https://docs.microsoft.com/windows-hardware/drivers/nfc/nfc-class-extension-quickstart).

## Prerequisites

To use the NFC CX, your NFC Controller must implement NFC Forum's [NFC Controller Interface (NCI)](https://nfc-forum.org/our-work/specifications-and-application-documents/specifications/nfc-controller-interface-nci-specification/) protocol.

In addition, the NFC CX requires the driver to use the [UMDF 2](https://docs.microsoft.com/windows-hardware/drivers/wdf/overview-of-the-umdf) framework.

## Universal windows driver compliant

This sample builds a Universal Windows Driver. It uses only APIs and DDIs that are included in OneCoreUAP.

## File manifest

### WDK

- [NfcCx.h](https://docs.microsoft.com/windows-hardware/drivers/ddi/content/nfccx/) - The NFC CX's client driver API. This file is included in the WDK.

### Project

- [Driver.cpp](windows-drivertemplate-nfc/Driver.cpp)
  - `DriverEntry` - Driver initialization.

- [Device.cpp](windows-drivertemplate-nfc/Device.cpp)
  - `DeviceContext::AddDevice` - Device initialization, including initializing the NFC CX driver.
  - `DeviceContext::WriteNciPacket` - The implementation of the [`EvtNfcCxWriteNciPacket`](https://docs.microsoft.com/windows-hardware/drivers/ddi/content/nfccx/nc-nfccx-evt_nfc_cx_write_nci_packet) callback function.
  - `DeviceContext::ReadNciPacket` - Demonstrates calling [`NfcCxNciReadNotification`](https://docs.microsoft.com/windows-hardware/drivers/ddi/content/nfccx/nf-nfccx-nfccxncireadnotification).

- [Device.h](windows-drivertemplate-nfc/Device.h)
  - `DeviceContext` - The class that stores the device instance's state.
