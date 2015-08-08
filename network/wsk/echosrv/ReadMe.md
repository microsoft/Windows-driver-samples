WSK TCP Echo Server
===================

This sample driver is a minimal driver meant to demonstrate the usage of the Winsock Kernel (WSK) programming interface.

The sample implements a simple kernel-mode application by using the Winsock Kernel (WSK) programming interface. The application accepts incoming TCP connection requests on port 40007 over both IPv4 and IPv6 and, on each connection, it echoes all received data back to the peer until the connection is closed by the peer. The application is designed to use a single worker thread to perform all of its processing. For better performance on a multi-processor computer, the sample can be enhanced to use more worker threads. This sample is designed such that operations on a given connection should always be processed by the same worker thread. This provides a simple form of synchronization that ensures proper socket closure in a setting where multiple operations might be outstanding and completed asynchronously on a given connection. For the sake of simplicity, this sample does not enforce any limit on the number of connections accepted (other than the natural limit imposed by the available system memory) or on the amount of time that a connection stays alive. A production server application should be designed with these security points in mind.

This sample is not intended for use in a production environment.


WPP SOFTWARE TRACING
--------------------

This sample driver uses WPP Software Tracing in order to log its actions. You can find detailed information on WPP Software Tracing in the WDK documentation. Here is a quick overview of one way to collect trace logs from the sample driver by using the tracing tools that are available in the \\tools\\tracing directory in the WDK. All code for this sample is located in \\network\\WSK\\echosrv directory.

1.  In a Command Prompt window, copy Echosrv.ctl and Echosrv.pdb into a directory and change to that directory (cd).
2.  Start software tracing for the sample driver by typing the following command:

    **tracelog -start echosrvtrace -guid echosrv.ctl -f logfile.etl -flags 0x3**

    The value that is provided for the -flags option determines which events will be logged by the sample driver. The sample currently has two event types denoted by the TRCERROR and TRCINFO macros where TRCERROR is 0x1 and TRCINFO is 0x2. Thus, a flag value of 0x3 (0x1 combined in a bitwise OR with 0x2) in the previous tracelog command tells the sample driver to log both TRCERROR and TRCINFO events.

3.  In order to stop tracing, type the following command:

    tracelog -stop echosrvtrace

4.  Convert the trace logs in Logfile.etl into a human-readable format by typing the following command:

    **tracefmt -o logfile.txt -f logfile.etl -r . -i \\***full-path***\\ echosrv.sys**

5.  Open Logfile.txt to view the trace logs.

Be aware that tracing for the sample driver can be started at any time before the driver is started or while the driver is already running.

To run the sample
-----------------

Install and run this sample driver by using the following steps:

1.  Copy the Echosrv.sys file to a directory on the test machine.
2.  In a Command Prompt window, type the following command:

    **sc create echosrv type= kernel binpath= \\***full-path***\\ echosrv.sys**

    where \\*full-path*\\ is the directory that contains the Echosrv.sys file.

3.  To start the driver, type:

    **sc start echosrv**

4.  To stop the driver, type:

    **sc stop echosrv**

After the driver is installed and started, it will listen for incoming TCP connection requests on port 40007 over both IPv4 and IPv6 protocols until the driver is stopped. On each connection, the driver will echo all the received data back to the peer until the connection is closed by the peer.

For more information on the usage of the Winsock Kernel (WSK) programming interface, see [Winsock Kernel](http://msdn.microsoft.com/en-us/library/windows/hardware/ff571084).

