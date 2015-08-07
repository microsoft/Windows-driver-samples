Windows Filtering Platform MSN Messenger Monitor Sample
=======================================================

This sample application and driver demonstrate the stream inspection capabilities of the Windows Filtering Platform (WFP).

The sample consists of a user mode application (Monitor.exe) that registers traffic of interest. In this case, all Transmission Control Protocol (TCP) data segments that are sent and received by an application of your choice.

**Note** Originally this sample was written to monitor the MSN Messenger application. Now it can monitor any application that you specify.

Monitor.exe adds filters and callouts to Windows through the Windows Filtering Platform (WFP) Win32 API. A kernel-mode WFP callout driver (Msnmntr.sys) intercepts TCP traffic and parses out communication patterns. Monitor.exe controls the operations of the callout driver through I/O controls (IOCTLs).

The filters and callouts added by Monitor.exe are persistent across system restarts and removed only by Monitor.exe. Adding filters and callouts requires administrator privileges. Therefore, Monitor.exe must be run from an elevated command prompt.

Msnmntr.sys registers itself at two different WFP layers: FLOW-ESTABLISHED and STREAM. For simplicity, only Internet Protocol version 4 (IPv4) traffic is inspected. Msnmntr.sys registers at the FLOW-ESTABLISHED layer to associate a callout driver-specific data structure with application identity (that is, path) recorded such that the STREAM layer will only be invoked if traffic is sent or received from that particular application.

After the filters and callouts are in place and registered, WFP indicates TCP data segments to the Msnmntr.sys for inspection. As the data flows through Msnmntr.sys, it copies them (described by a chain of NET\_BUFFER\_LIST structures) to a flat buffer, parses out the communication patterns (such as client-to-server/client-to-client), and sends them to the Windows Software Trace Preprocessor (WPP) for tracing.

Automatic deployment
--------------------

Before you automatically deploy a driver, you must provision the target computer. For instructions, see [Configuring a Computer for Driver Deployment, Testing, and Debugging](http://msdn.microsoft.com/en-us/library/windows/hardware/). After you have provisioned the target computer, continue with these steps:

1.  On the host computer, in Visual Studio, in Solution Explorer, right click **package** (lower case), and choose **Properties**. Navigate to **Configuration Properties \> Driver Install \> Deployment**.
2.  Check **Enable deployment**, and check **Remove previous driver versions before deployment**. For **Target Computer Name**, select the name of a target computer that you provisioned previously. Select **Do not install**. Click **OK**.
3.  On the **Build** menu, choose **Build Solution**.
4.  On the target computer, navigate to DriverTest\\Drivers, and locate the file msnmntr.inf. Right click msnmntr.inf, and choose **Install**.

Manual deployment
-----------------

Before you manually deploy a driver, you must turn on test signing and install a certificate on the target computer. You also need to copy the [DevCon](http://msdn.microsoft.com/en-us/library/windows/hardware/ff544707) tool to the target computer. For instructions, see [Preparing a Computer for Manual Driver Deployment](http://msdn.microsoft.com/en-us/library/windows/hardware/dn265571). After you have prepared the target computer for manual deployment, continue with these steps:

1.  Copy all of the files in your driver package to a folder on the target computer (for example, c:\\WfpMsnMessengerMonitorSamplePackage).
2.  On the target computer, navigate to your driver package folder. Right click msnmntr.inf, and choose **Install**

Copy additional files to the target computer
--------------------------------------------

Copy the user-mode application, monitor.exe to a folder on the target computer (for example, c:\\WfpMsnMessengerMonitorSampleApp).

Copy the PDB file, msnmntr.pdb to a folder on the target computer (for example, c:\\Symbols).

Copy the tool TraceView.exe to a folder on the target computer (for example c:\\Tools). TraceView.exe comes with the WDK. You can find it in your WDK installation folder under Tools (for example, c:\\Program Files (x86)\\Windows Kits\\10\\Tools\\x64\\TraceView.exe).

Start the msnmntr service
-------------------------

On the target computer, open a Command Prompt window as Administrator, and enter **net start msnmntr**. (To stop the driver, enter **net stop msnmntr**.)

Running the user-mode application
---------------------------------

On the target computer, open a Command Prompt window as Administrator, and navigate to the folder that contains monitor.exe. Enter **monitor.exe addcallouts**. Then enter **monitor.exe monitor** *TargetAppPath*, where *TargetAppPath* is the path to the application that you want to monitor. Here is an example that initiates monitoring of Internet Explorer.

```
monitor.exe addcallouts
monitor.exe monitor "C:\Program Files (x86)\Internet Explorer\iexplore.exe"
```

Start a logging session in TraceView
------------------------------------

On the target computer, open TraceView.exe as Administrator. On the **File** menu, choose **Create New Log Session**. Click **Add Provider**. Select **PDB (Debug Information File)**, and enter the path to your PDB file, msnmntr.pdb. Click **OK**, and finish working through the setup procedure. Open Internet Explorer, and watch the communication patterns being displayed in the Traceview.exe tool.

Tracing for the sample driver can be started at any time before the driver is started or while the driver is already running.

For more information on creating a Windows Filtering Platform Callout Driver, see [Windows Filtering Platform Callout Drivers](http://msdn.microsoft.com/en-us/library/windows/hardware/ff571068).

Using MSBuild
-------------

As an alternative to building the WFP MSN Messenger Monitor Sample in Visual Studio, you can build it in a Visual Studio Command Prompt window. In Visual Studio, on the **Tools** menu, choose **Visual Studio Command Prompt**. In the Visual Studio Command Prompt window, navigate to the folder that has the solution file, msnmntr.sln. Use the [MSBuild](http://go.microsoft.com/fwlink/p/?linkID=262804) command to build the solution. Here are some examples:

**msbuild /p:configuration="Debug" /p:platform="x64" msnmntr.sln**

**msbuild /p:configuration="Release" /p:platform="Win32" msnmntr.sln**

For more information about using [MSBuild](http://go.microsoft.com/fwlink/p/?linkID=262804) to build a driver package, see [Building a Driver](http://msdn.microsoft.com/en-us/library/windows/hardware/ff554644).

