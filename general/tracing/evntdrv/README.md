---
page_type: sample
description: "Demonstrates the use of the Event Tracing for Windows (ETW) API in a driver."
languages:
- cpp
products:
- windows
- windows-wdk
---

# Eventdrv

Eventdrv is a sample kernel-mode trace provider and driver. The driver does not control any hardware; it simply generates trace events. It is designed to demonstrate the use of the [Event Tracing for Windows (ETW)](https://docs.microsoft.com/windows-hardware/drivers/devtest/event-tracing-for-windows--etw-) API in a driver.

Evntdrv registers as a provider by calling the [**EtwRegister**](https://docs.microsoft.com/windows-hardware/drivers/ddi/content/wdm/nf-wdm-etwregister) API. If the registration is successful, it logs a StartEvent with the device's name, the length of the name, and the status code. Then, when the sample receives a DeviceIOControl call, it logs a SampleEventA event. Finally, when the driver gets unloaded, it logs an UnloadEvent event with a pointer to the device object

> [!NOTE]
> The Windows Pre-Processor (WPP) Tracing tools such as TraceView.exe cannot be used to start, stop, or view traces.

## Run the sample

1. Install the manifest (Evntdrv.xml), which is located in the Evntdrv\\Eventdrv folder. Open a Visual Studio Command window (Run as administrator) and use the following command:

    `wevtutil im evntdrv.xml`

    Installing the manifest creates registry keys that enable tools to find the resource and message files that contain event provider information. For further details, see [wevtutil](https://docs.microsoft.com/windows-server/administration/windows-commands/wevtutil).

    > [!NOTE]
    > Using a Visual Studio Command windows sets up the environment variables you need to run the tracing tools for this sample.

1. Make a folder in the system directory called ETWDriverSample (for example, C:\\ETWDriverSample).

    Copy Eventdrv.sys and Evntctrl.exe to the ETWDriverSample folder.

    The ETWDriverSample directory must be created because the path to the resource file that is specified in the evntdrv.xml manifest points to the %SystemRoot%\\ETWDriverSample folder. If this folder is not created and the Eventdrv.sys binary is not copied, decoding tools cannot find the event information to decode the trace file.

1. Use Tracelog to start a trace session that is called "TestEventdrv." The following command starts the trace session and creates a trace log file, Eventdrv.etl, in the local directory.

    `tracelog -start TestEventdrv -guid #b5a0bda9-50fe-4d0e-a83d-bae3f58c94d6 -f Eventdrv.etl`

1. To generate trace messages, run Evntctrl.exe. Each time you type a character other than **Q** or **q**, Evntctrl sends an IOCTL to the driver that signals it to generate trace messages. To stop Evntctrl, type **Q** or **q**.

1. To stop the trace session, run the following command:

    `tracelog -stop TestEventdrv`

1. To display the traces collected in the Tracedrv.etl file, run the following command:

    `tracerpt Eventdrv.etl`

    This command creates two files: Summary.txt and Dumpfile.xml. Dumpfile.xml will contain the event information in an XML format.

1. To uninstall the manifest, run the following command:

    `wevtutil um evntdrv.xml`

## Notes

If you are building the Eventdrv sample to test on a 64-bit version of Windows, you need to sign the driver. All 64-bit versions of Windows require driver code to have a digital signature for the driver to load. See [Signing a Driver](https://docs.microsoft.com/windows-hardware/drivers/develop/signing-a-driver) and [Signing a Driver During Development and Testing](https://docs.microsoft.com/windows-hardware/drivers/install/signing-drivers-during-development-and-test). You might also need to configure the test computer so that it can load test-signed kernel mode code, see [The TESTSIGNING Boot Configuration Option](https://docs.microsoft.com/windows-hardware/drivers/install/the-testsigning-boot-configuration-option) and [**BCDEdit /set**](https://docs.microsoft.com/windows-hardware/drivers/devtest/bcdedit--set).
