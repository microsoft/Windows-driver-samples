SystemTraceProvider
===================

This sample application demonstrates how to use event tracing control APIs to collect events from the system trace provider.

The sample code provided shows how to start an [Event Tracing](http://msdn.microsoft.com/en-us/library/windows/hardware/bb968803) for Windows trace session and how to enable system events with stacks. When you build and run the application, it collects the trace data for 30 seconds and then stops. The sample application writes the results to a file, Systemtrace.etl. For more information, see [Tools for Software Tracing](http://msdn.microsoft.com/en-us/library/windows/hardware/ff552961).

You can process the Systemtrace.etl file using Tracerpt.exe, a command-line trace tool included in Windows that formats trace events. It also analyzes the events and generates summary reports. For more information about how to use this tool, see [Tracerpt](http://go.microsoft.com/fwlink/p/?linkid=179389) topic on the TechNet website.

You can also process the file using the [Windows Performance Toolkit](http://go.microsoft.com/fwlink/p/?linkid=250774) (WPT), which is available in the SDK.

