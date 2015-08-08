UMDF SocketEcho Sample (UMDF Version 1)
=======================================

The UMDF SocketEcho sample demonstrates how to use the User-Mode Driver Framework (UMDF) to write a driver and demonstrates best practices.

This sample also demonstrates how to use a default parallel dispatch I/O queue, use a Microsoft Win32 dispatcher, and handle a socket handle by using a Win32 file I/O target.

Related technologies
--------------------

[User-Mode Driver Framework](http://msdn.microsoft.com/en-us/library/windows/hardware/ff560456)

Code Tour
---------

This sample driver is a minimal driver that is intended to demonstrate how to use UMDF. It is not intended for use in a production environment.

- **CMyDriver::OnInitialize** in **driver.cpp** is called by the framework when the driver loads. This method initiates use of the Winsock Library. 
- **CMyDriver::OnDeviceAdd** in **driver.cpp** is called by the framework to install the driver on a device stack. OnDeviceAdd creates a device callback object, and then calls IWDFDriver::CreateDevice to create an framework device object and to associate the device callback object with the framework device object.
- **CMyQueue::OnCreateFile** in **queue.cpp** is called by the framework to create a socket connection, create a file i/o target that is associated with the socket handle for this connection, and store the socket handle in the file object context.

Installation
------------

In Visual Studio, you can press F5 to build the sample and then deploy it to a target machine. For more information, see [Deploying a Driver to a Test Computer](http://msdn.microsoft.com/en-us/library/windows/hardware/hh454834). Alternatively, you can install the sample from the command line.

To test this sample, you must have a test computer. This test computer can be a second computer or, if necessary, your development computer.

To install the UMDF Echo sample driver from the command line, do the following:

1.  Copy the driver binary and the socketecho.inf file to a directory on your test computer (for example, C:\\ socketechoSample.)

2.  Copy the UMDF coinstaller, WUDFUpdate\_*MMmmmm*.dll, from the \\redist\\wdf\\\<architecture\> directory to the same directory (for example, C:\\socketechoSample).

    **Note** You can obtain redistributable framework updates by downloading the *wdfcoinstaller.msi* package from [WDK 8 Redistributable Components](http://go.microsoft.com/fwlink/p/?LinkID=226396). This package performs a silent install into the directory of your Windows Driver Kit (WDK) installation. You will see no confirmation that the installation has completed. You can verify that the redistributables have been installed on top of the WDK by ensuring there is a redist\\wdf directory under the root directory of the WDK, %ProgramFiles(x86)%\\Windows Kits\\8.0.

3.  Navigate to the directory that contains the INF file and binaries (for example, cd /d c:\\socketechoSample), and run DevCon.exe as follows:

    `devcon.exe install socketecho.inf WUDF\\socketecho`

  You can find DevCon.exe in the \\tools directory of the WDK (for example, \\tools\\devcon\\i386\\devcon.exe).

To update the socketecho driver after you make any changes, do the following:

1.  Increment the version number in the INF file. This change is not necessary, but it will help ensure that Plug and Play (PnP) selects your new driver as a better match for the device.

2.  Copy the updated driver binary and the socketecho.inf file to a directory on your test computer (for example, C:\\ socketechoSample.)

3.  Navigate to the directory that contains the INF file and binaries (for example, cd /d c:\\ socketechoSample), and run devcon.exe as follows:

  `devcon.exe update socketecho.inf WUDF\\socketecho`

To test this sample drivers on a checked operating system that you have installed (in contrast to the standard retail installations), you must modify the INF file to use the checked version of the UMDF co-installer. That is, you must do the following:

1.  In the INX file, replace all occurrences of WudfUpdate\_*MMmmmm*.dll with WudfUpdate\_*MMmmmm*\_chk.dll.

2.  Copy the WudfUpdate\_*MMmmmm*\_chk.dll file from the \\redist\\wdf\\\<architecture\> directory to your driver package instead of WudfUpdate\_*MMmmmm*.dll.

3.  If WdfCoinstaller*MMmmmm*.dll or WinUsbCoinstaller.dll is included in your driver package, repeat step 1 and step 2 for them.

Testing
-------

To test the SocketEcho driver, you can run socketechoserver.exe, which is built from the \\echo\\umdfSocketEcho\\Exe directory, and echoapp.exe, which is built from the Kernel-Mode Driver Framework (KMDF) samples in the \\echo\\kmdf directory.

First, you must install the device as described earlier. Then, run socketechoserver.exe from a Command Prompt window.

`D:\\\>socketechoserver -h`

Usage
------
```
socketechoserver Display Usage

socketechoserver -h Display Usage

socketechoserver -p Start the app as server listening on default port

socketechoserver -p [port\#] Start the app as server listening on this port

D:\\\>socketechoserver -p

Listening on socket...

In another Command Prompt window, run echoapp.exe.

D:\\\>echoapp

DevicePath: \\\\?\\root\#sample\#0000\#{ e5e65b0c-82c8-4689-96d4-f77837971990}

Opened device successfully

512 Pattern Bytes Written successfully

512 Pattern Bytes Read successfully

Pattern Verified successfully

D:\\\>echoapp -Async

DevicePath: \\\\?\\root\#sample\#0000\#{cdc35b6e-0be4-4936-bf5f-5537380a7c1a}

Opened device successfully

Starting AsyncIo

Number of bytes written by request number 0 is 1024

Number of bytes read by request number 0 is 1024

Number of bytes read by request number 1 is 1024

Number of bytes written by request number 2 is 1024

Number of bytes read by request number 2 is 1024

Number of bytes written by request number 3 is 1024

Number of bytes read by request number 3 is 1024

Number of bytes written by request number 4 is 1024

Number of bytes read by request number 4 is 1024

Number of bytes written by request number 5 is 1024

Number of bytes read by request number 5 is 1024

Number of bytes written by request number 6 is 1024

Number of bytes read by request number 6 is 1024

Number of bytes written by request number 7 is 1024

Number of bytes read by request number 7 is 1024

Number of bytes written by request number 8 is 1024

Number of bytes read by request number 8 is 1024

Number of bytes written by request number 9 is 1024

Number of bytes read by request number 9 is 1024

Number of bytes written by request number 10 is 1024

Number of bytes read by request number 10 is 1024

Number of bytes written by request number 11 is 1024

...
```

Note that independent threads perform the reads and writes in the echo test application. As a result, the order of the output might not exactly match what you see in the preceding output.

File Manifest
-------------

**Dllsup.cpp**: The DLL support code that provides the DLL's entry point and the single required export (DllGetClassObject).


