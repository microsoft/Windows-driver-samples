Eventdrv
========

Eventdrv is a sample kernel-mode trace provider and driver. The driver does not control any hardware; it simply generates trace events. It is designed to demonstrate the use of the [Event Tracing for Windows (ETW)](http://msdn.microsoft.com/en-us/library/windows/hardware/ff545699) API in a driver.

Evntdrv registers as a provider by calling the [**EtwRegister**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff545603) API. If the registration is successful, it logs a StartEvent with the device's name, the length of the name, and the status code. Then, when the sample receives a DeviceIOControl call, it logs a SampleEventA event. Finally, when the driver gets unloaded, it logs an UnloadEvent event with a pointer to the device object

**Note** The Windows Pre-Processor (WPP) Tracing tools such as TraceView.exe cannot be used to start, stop, or view traces.


Run the sample
--------------

1.  Install the manifest (Evntdrv.xml), which is located in the Evntdrv\\Eventdrv folder. Open a Visual Studio Command window (Run as administrator) and use the following command:

    ```
    wevtutil im evntdrv.xml
    ```

    Installing the manifest creates registry keys that enable tools to find the resource and message files that contain event provider information. For further details about the WevtUtil.exe tool, see the MSDN Library.

    **Note** Using a Visual Studio Command windows sets up the environment variables you need to run the tracing tools for this sample.

2.  Make a folder in the system directory called ETWDriverSample (for example, C:\\ETWDriverSample).

    Copy Eventdrv.sys and Evntctrl.exe to the ETWDriverSample folder.

    The ETWDriverSample directory must be created because the path to the resource file that is specified in the evntdrv.xml manifest points to the %SystemRoot%\\ETWDriverSample folder. If this folder is not created and the Eventdrv.sys binary is not copied, decoding tools cannot find the event information to decode the trace file.

3.  Use Tracelog to start a trace session that is called "TestEventdrv." The following command starts the trace session and creates a trace log file, Eventdrv.etl, in the local directory.

    ```
    Tracelog -start TestEventdrv -guid #b5a0bda9-50fe-4d0e-a83d-bae3f58c94d6 -f Eventdrv.etl
    ```

4.  To generate trace messages, run Evntctrl.exe. Each time you type a character other than **Q** or **q**, Evntctrl sends an IOCTL to the driver that signals it to generate trace messages. To stop Evntctrl, type **Q** or **q**.

5.  To stop the trace session, run the following command:

    ```
    tracelog -stop TestEventdrv
    ```

6.  To display the traces collected in the Tracedrv.etl file, run the following command:

    ```
    tracerpt Eventdrv.etl
    ```

    This command creates two files: Summary.txt and Dumpfile.xml. Dumpfile.xml will contain the event information in an XML format.

7.  To uninstall the manifest, run the following command:

    ```
    wevtutil um evntdrv.xml
    ```

Notes
-----

If you are building the Eventdrv sample to test on a 64-bit version of Windows, you need to sign the driver. All 64-bit versions of Windows require driver code to have a digital signature for the driver to load. See [Signing a Driver](http://msdn.microsoft.com/en-us/library/windows/hardware/ff554809) and [Signing a Driver During Development and Testing](http://msdn.microsoft.com/en-us/library/windows/hardware/hh967733). You might also need to configure the test computer so that it can load test-signed kernel mode code, see [The TESTSIGNING Boot Configuration Option](http://msdn.microsoft.com/en-us/library/windows/hardware/ff553484) and [**BCDEdit /set**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff542202).

