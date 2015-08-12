Tracedrv
========

Tracedrv is a sample driver instrumented for software tracing. The driver does not control any hardware; it simply generates trace messages. It is designed to show how to use WPP software tracing macros in a driver.

Tracedrv initializes tracing (by using WPP\_INIT\_TRACING) and, when it receives a DeviceIOControl call, it starts a thread that logs 100 trace messages. The WPP software tracing directives, calls, and macros in the code are accompanied by comments that explain their purpose

While examining Tracedrv, read the [WPP Software Tracing](http://msdn.microsoft.com/en-us/library/windows/hardware/ff556204) in the Windows Driver Kit (WDK). This section includes a reference section that describes the directives, macros, and calls required for WPP software tracing.

Run the sample
--------------

To test the Tracedrv event tracing provider, use the following procedure.

1.  Copy the Tracectl.exe file that was created when you built the Tracedrv solution from the Tracectl directory (for example, \\Documents\\Visual Studio 2015\\Projects\\tracedrv\\tracectl\\*platform*) to the Tracedrv directory (for example, \\Documents\\Visual Studio 2015\\Projects\\tracedrv\\tracedrv\\*platform*).
2.  Use Tracepdb to create a trace message format (TMF) file and a trace message control (TMC) file from the Tracedrv.pdb file. Tracepdb is located in the C:\\Program Files (x86)\\Windows Kits\\10\\bin\\*platform* directory. The PDB file that is used in this command is created when you the build the solution. Open a Visual Studio Command prompt window and navigate to the target build platform and configuration directory. Type the following command:

    **tracepdb -f tracedrv.pdb**

3.  In the same Tracedrv target build directory, create a control GUID file for Tracedrv by opening a text file, adding the following content, and saving the file as Tracedrv.ctl.

  ```txt
  d58c126f-b309-11d1-969e-0000f875a5bc
  ```

4.  Use Tracelog to start a trace session that is called *TestTracedrv*. Tracelog is located in the C:\\Program Files (x86)\\Windows Kits\\10\\bin\\*platform* directory. The Tracedrv.ctl file that is used in this command was created in the previous step. The following command starts a trace session and creates a trace log file, tracedrv.etl, in the local directory.

  ``` 
  tracelog -start TestTracedrv -guid tracedrv.ctl -f tracedrv.etl -flag 1
  ```

  **Note** Without the -flag parameter, Tracedrv will not generate any trace messages.

5.  To generate trace messages, run Tracectl.exe. This executable file is built when you build the solution. Each time you type a character, other than **Q** or **q**, Tracectl sends an IOCTL to the driver that signals it to generate trace messages. To stop Tracectl, type **Q** or **q**.
6.  To stop the trace session, use the following Tracelog command.

  ``` 
  tracelog -stop TestTracedrv
  ```

7.  To display the trace messages in the Tracedrv.etl file, use Tracefmt.exe. Tracefmt.exe is located in the C:\\Program Files (x86)\\Windows Kits\\10\\bin\\*platform*. The TMF file used in this command was created by Tracepdb.exe in step 2. The **-p** option specifies the directory of the TMF file. In this case, the TMF file is in the current directory. Type the following command:

  ```
  tracefmt tracedrv.etl -p . -o Tracedrv.out
  ```

The resulting Tracedrv.out file is a human-readable text file of the Tracedrv trace messages. To interpret the trace messages, in the Tracedrv.c file, search for the [**DoTraceMessage**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff544918) macros.

Notes
-----

This sample driver should not be used in a production environment.

Also, because it is not a Plug and Play driver, Tracedrv does not demonstrate tracing in a Plug and Play environment.

Tracedrv demonstrates the basic elements required for software tracing. It does not demonstrate more advanced tracing techniques, such as writing customized tracing calls (variations of [**DoTraceMessage**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff544918)), or the use of WMI calls for software tracing.

If you are building the Tracedrv sample to test on a 64-bit version of Windows, you need to sign the driver. All 64-bit versions of Windows require driver code to have a digital signature for the driver to load. See [Signing a Driver](http://msdn.microsoft.com/en-us/library/windows/hardware/ff554809) and [Signing a Driver During Development and Testing](http://msdn.microsoft.com/en-us/library/windows/hardware/hh967733). You might also need to configure the test computer so that it can load test-signed kernel mode code, see [The TESTSIGNING Boot Configuration Option](http://msdn.microsoft.com/en-us/library/windows/hardware/ff553484) and [**BCDEdit /set**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff542202).

