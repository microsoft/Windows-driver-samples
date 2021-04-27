---
page_type: sample
description: "Demonstrates how to use event tracing control APIs to collect events from the system trace provider."
languages:
- cpp
products:
- windows
- windows-wdk
---

# SystemTraceProvider

This sample application demonstrates how to use event tracing control APIs to collect events from the system trace provider.

The sample code provided shows how to start an [Event Tracing](https://docs.microsoft.com/windows/win32/etw/event-tracing-portal) for Windows trace session and how to enable system events with stacks. When you build and run the application, it collects the trace data for 30 seconds and then stops. The sample application writes the results to a file, Systemtrace.etl. For more information, see [Tools for Software Tracing](https://docs.microsoft.com/windows-hardware/drivers/devtest/tools-for-software-tracing).

You can process the Systemtrace.etl file using Tracerpt.exe, a command-line trace tool included in Windows that formats trace events. It also analyzes the events and generates summary reports. For more information about how to use this tool, see [Tracerpt](https://docs.microsoft.com/windows-server/administration/windows-commands/tracerpt_1).

You can also process the file using the [Windows Performance Toolkit](https://docs.microsoft.com/windows-hardware/test/wpt/) (WPT), which is available in the SDK.
