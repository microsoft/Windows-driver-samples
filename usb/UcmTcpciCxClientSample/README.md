---
page_type: sample
description: "Demonstrates how to create a Windows USB Type-C port controller driver using the USB Connector Manager Type-C Port Controller Interface class extension driver (UcmTcpciCx)."
languages:
- cpp
products:
- windows
- windows-wdk
---

# UcmTcpciCx Port Controller Client Driver

This is a skeleton sample driver that shows how to create a Windows USB Type-C port controller driver using the USB Connector Manager Type-C Port Controller Interface class extension driver (UcmTcpciCx). UcmTcpciCx is currently only available using the Windows Insider program - documentation for UcmTcpciCx will be available at the next release of Windows.

This sample demonstrates the following:

- Registration with the UcmTcpci class extension driver (UcmTcpciCx)

- Initializing the port controller's Type-C and Power Delivery capabilities

- Initializing the I2C communications channel to the port controller hardware

- Performing reads/writes over I2C

- Handling hardware requests from UcmTcpciCx

- Handling alerts from the port controller hardware and notifying UcmTcpciCx of the alert

- Power management

- Platform-level device reset in the case of an unresponsive I2C controller

## Customizing the sample for your port controller

The sample contains a number of comments prefaced with `// TODO` - review them and modify the code as necessary as you are writing your driver.

## Note regarding Type-C port controller hardware

This sample assumes a device that complies with the USB Type-C Port Controller Interface specification, Revision 1.0 (part of the [USB 3.1 specification download](https://usb.org/documents)). Such a device uses a predefined register layout and an I2C communications channel.
If your port controller hardware is not exactly compliant with the specification, you will need to make additional modifications to the sample.

## Performing read/writes over I2C

The USB Type-C Port Controller Interface specification defines I2C to be the channel by which software communicates with the port controller hardware.
If your port controller hardware is not compliant with the specification and does not use I2C as the communications channel, you will need to make additional modifications to the sample.

## When to write a UcmTcpciCx client driver

UcmTcpciCx is intended for system port controller drivers. If you are bringing up a USB Type-C peripheral, you do not need to write a USB Type-C specific driver; a regular USB client driver will suffice. Refer to [Developing Windows client drivers for USB devices](https://docs.microsoft.com/windows-hardware/drivers/usbcon/usb-driver-development-guide) to determine what type of driver, if any, you need to write to make your USB device work with Windows.

## Testing Your Type-C Port Controller (TCPC) Implementation on Windows 10 with Raspberry Pi

### Pre-requisites

This section outlines a procedure for installing and testing a TCPCI implementation for Windows 10 on a Raspberry Pi computer running Windows 10 IoT Core. You will need a Raspberry Pi 2 or 3, your TCPC device, a USB-to-Serial converter for debugging, and 7 jumper wires to connect your TCPC to the Raspberry Pi (3 for the debug board, 4 for the TCPC).

You will also need the Windows 10 IoT Core Insider Preview builds, the IoT tool set, the [Windows Driver Kit (WDK)](https://docs.microsoft.com/windows-hardware/drivers/download-the-wdk), and the UcmTcpciCxClientSample source code.

### Setup

#### Operating System

Download the latest available [Windows 10 IoT Core Insider Preview](https://www.microsoft.com/software-download/windowsiot) and install on your Raspberry Pi 2 or 3 per the instructions at [Windows for Internet of Things](https://developer.microsoft.com/windows/iot).

Note, you will only need to complete the first two steps, “1 Get the tools” and “2 Setup your device” to proceed with the TCPC validation described here.

#### PowerShell Connection

Connect to your Raspberry Pi via a [PowerShell](https://docs.microsoft.com/windows/iot-core/connect-your-device/powershell) remote session. You will use this connection to setup the debugger and install your TCPCI driver package.

#### Windows Device Portal

Open a connection to the Raspberry Pi via the Windows [Device Portal](https://docs.microsoft.com/windows/iot-core/manage-your-device/DevicePortal). The portal provides a Device Manager view in which you will be able to inspect your TCPC device, as well as remote shutdown and reboot controls.

#### Kernel Debugger

Setup the kernel debugger for your Raspberry Pi according to the instructions at the [Windows Debugger (WinDbg)](https://docs.microsoft.com/windows/iot-core/manage-your-device/windowsdebugger) for IoT.

#### Connect TCPC Hardware

The following assumes an I2C implementation of your TCPC; if your device uses a different transport, ignore the I2C specific references and connect the transport as appropriate.

First, shutdown and remove power from both the Raspberry Pi and your TCPC hardware. Once both boards are powered down, connect the four pins outlined below. Please refer to the GPIO pinout specification for the version of the Raspberry Pi platform you are using for the specific locations.

Connect the following pins between your TCPC and the Raspberry Pi:

- GND: many options

- SDA: pin 3

- SCL: pin 5

- ALERT: can be any GPIO but the sample ACPI is setup to use GPIO 5 (pull-up) which is pin 29 on the RPi2

Note the ACPI sample assumes GPIO5 (pin 29). If you use a different bus or interrupt you will need to update the sample ASL file to reflect the selected configuration.

Once these pins are connected, both boards may be powered on. You will not see any changes to the device manager view of the Device Portal yet.

### UcmTcpciCx Client Driver

#### Customize and Build the Driver

Edit the driver source files to add any customizations that your hardware may require. For example, the sample is designed against TCPCI Revision 1.0 Version 1.0, but if your hardware conforms to a later version that repurposes register fields that were reserved in version 1.0 you will need to update the alert handler to recognize and clear those bits accordingly.

Edit the INF to set the device path and strings to desired custom values. Build your driver in Visual Studio and copy the INF and the SYS file from the build output.

#### Install Driver Package

[Create a driver package for IoT](https://docs.microsoft.com/windows-hardware/manufacture/iot/create-install-package) in the form of a CAB file for your custom UcmTcpciCx client driver.

Once you have generated the CAB file, see step 3 for instructions on installing the driver package on your Raspberry Pi via the PowerShell connection established earlier. After the package has been installed and the system has rebooted, there is still one more step to getting your driver to load: updating the ACPI tables to describe the device to the platform.

### Update ACPI

Open sample.asl provided in the UcmTcpciCxClientSample driver sample and modify the following sections as necessary:

- Ensure that the device name matches the device path specified in your INF

- Update the resources section to reflect the actual I2C and Interrupt resources on which your TCPC is connected, default is I2C1 and GPIO5. Specify the I2C address your TCPC uses.

- Update the connector capabilities section to reflect the capabilities of the connector on your TCPC.

On a PC with the Windows 10 WDK installed run the following command on your updated ASL file:

`asl.exe sample.asl`

This will produce output file ACPITABL.dat. Copy this file to *C:\\Windows\\System32* on your Raspberry Pi device, and reboot. See the [Verification](https://docs.microsoft.com/windows/uwp/devices-sensors/enable-usermode-access#verification) section in *Enable usermode access to GPIO, I2C, and SPI* for more information on merging ACPI updates into an IoT image without a full UEFI build.

Once the Raspberry Pi has rebooted, your driver will load on the newly enumerated ACPI device node.

### Testing

#### USB Type-C Connection Exerciser

The [MCCI Model 3101 Type-C Connection Exerciser](http://www.mcci.com/mcci-v5/devtools/exerciser-type-c.html) is a tool designed by Microsoft and manufactured by MCCI.

The tools and instructions for deploying testing with this device are available at [Testing USB hardware, drivers, and apps in Windows](https://docs.microsoft.com/windows-hardware/drivers/usbcon/usb-driver-testing-guide).

Suggested testing with the Connection Exerciser device is randomized connect/disconnect for long periods of time.

### Debugging

The sample driver includes standard WDF logging which can be extracted via the kernel debugger with the following debugger command:

`!wdfkd.wdflogdump <driver name> -d`

Other useful driver traces are available from *UcmTcpciCx* and *UcmCx*.

Driver logs may also be captured via tracelog which is included in the IoT image. Sample trace capture commands using the GUID included by default in the client sample driver is as follows:

```cmd
Tracelog -start ClientSampleTrace -guid #1dc982f3-068f-4577-bcdf-1bc844e457b2 -flags 0x7fffffff -level 0xff -f clientsampletrace.etl -ft 3

<reproduce issue of interest>

Tracelog -stop ClientSampleTrace

Tracelog -remove ClientSampleTrace
```

Other useful driver trace guids:

UcmCx: C5964C90-1824-4835-857A-5E95F8AA33B2

UcmTcpciCx: 8DEAEA72-4C63-49A4-9B8B-25DA24DAE056

### Resources

[USB Type-C Port Controller Interface Specification (TCPCI)](https://www.usb.org/documents) (download as part of USB 3.1 Specification)

[Raspberry Pi](https://www.raspberrypi.org)

[Windows 10 IoT Core Insider Preview](https://www.microsoft.com/software-download/windowsiot)

[Windows for Internet of Things](https://developer.microsoft.com/windows/iot)

[Windows Insider Preview WDK](https://www.microsoft.com/software-download/windowsinsiderpreviewWDK)

[Tracelog Command Syntax](https://docs.microsoft.com/windows-hardware/drivers/devtest/tracelog-command-syntax)
