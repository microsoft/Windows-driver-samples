Bluetooth Echo L2CAP Profile Driver
===================================

This sample demonstrates developing [Bluetooth L2CAP profile drivers](http://msdn.microsoft.com/en-us/library/windows/hardware/ff536598) using [Bluetooth L2CAP DDIs](http://msdn.microsoft.com/en-us/library/windows/hardware/ff536585).The sample includes two drivers. One for a device that acts as an L2CAP server and another for a device that acts as an L2CAP client. The server simply echoes back any data that it receives from client on the same L2CA channel. These drivers can be used with devices that can be installed with bth.inf. Such devices get installed as 'Generic Bluetooth Radio'. Examples of such devices are Bluetooth USB dongles such as (but not limited to):

``` 
Generic Bluetooth Radio=\
                         BthUsb, USB\Vid_0a12&Pid_0001
CSR Nanosira=\
                         BthUsb, USB\Vid_0a12&Pid_0003
CSR Nanosira WHQL Reference Radio=\
                         BthUsb, USB\Vid_0a12&Pid_0004
CSR Nanosira-Multimedia=\
                         BthUsb, USB\Vid_0a12&Pid_0005
CSR Nanosira-Multimedia WHQL Reference Radio=\
                         BthUsb, USB\Vid_0a12&Pid_0006
```

Please refer to bth.inf for the complete list of devices. The installation steps below describe how to install echo server and client with such a device. Please note that RFCOMM based profiles must be developed and accessed using user-mode socket APIs. 

Build the sample
----------------

You can build the sample in two ways: using the Visual Studio Integrated Development Environment (IDE) or from the command line using the Visual Studio Command Prompt window and the Microsoft Build Engine (MSBuild.exe).

**Building the sample using Visual Studio**

1.  Open Visual Studio. From the **File** menu, select **Open Project/Solution** and open the bthecho.sln project file.
2.  Right-click the solution in the **Solution Explorer** and select **Configuration Manager**.
3.  From the **Configuration Manager**, select the **Active Solution Configuration** and the **Active Solution Platform** (for example, Win32) that correspond to the type of build you are interested in.
4.  From the **Build** menu, click **Build Solution** (Ctrl+Shift+B).

**Building the sample using the command line (MSBuild)**

1.  Open a Visual Studio Command Prompt window. Click **Start** and search for **Developer Command Prompt**. If your project is under %PROGRAMFILES%, you need to open the command prompt window using elevated permissions (**Run as administrator**). From this window you can use MsBuild.exe to build any Visual Studio project by specifying the project (.VcxProj) or solutions (.Sln) file.
2.  Navigate to each of the respective project directories and enter the appropriate **MSbuild** command for your target. For example, to perform a clean build of a Visual Studio driver project called BthEcho.vcxproj, navigate to the samples\\bluetooth\\bthecho\\bthcli\\sys project directory and enter the following MSBuild command: **msbuild /t:clean /t:build .\\BthEchoSampleCli.vcxproj**.
3.  If the build succeeds, you will find the driver (BthEchoSampleCli.sys) in the binary output directory corresponding to the target platform.

Run the sample
--------------

**INSTALLATION**

**Note**: Bluetooth echo server device and echo client device must be installed on two different machines, as described below.

**Server Installation**

1. Copy KMDF coinstaller (wdfcoinstallerMMmmm.dll, from redist\\wdf\\ ), BthEchoSampleSrv.Sys, BthEchoSampleSrv.inf and bthsrvinst.exe on a temporary directory on the target machine.

2. Run bthsrvinst.exe /i to install the echo server device. This enables the Bluetooth Enumerator (BthEnum.sys) to enumerate echo server device and create a PDO for the device (please refer to the device tree below).

3. Step \#2 causes BthEnum.sys to create a PDO. Consequently hardware installation wizard gets launched. Either go through the UI and point it to the temporary directory where you copied the binaries in step \#1, or using devcon.exe from the tools\\devcon folder, run the following command from the temporary directory:

  ``` 
  devcon.exe update BthEchoSampleSrv.inf BTHENUM\{c07508f2-b970-43ca-b5dd-cc4f2391bef4}
  ```

  If devcon.exe fails check the error level using:

  ``` 
  echo %errorlevel%
  ```

  If errorlevel is 1, you need to reboot the machine for KMDF update to take effect. If errorlevel is 2, please make sure that you have the driver files described in \#1 available in the current directory. For more information on installation failure please check setup logs.

4. Upon successful installation you will see 'Bluetooth Echo Sample Server' in Device Manager under Bluetooth devices.

**Device tree for Echo Server device**

(Drivers for FDOs are shown for each devnode in the tree.)

```
 --------------------
|BthEchoSampleSrv.sys|<----Function driver for PDO ejected by BthEnum.sys
 --------------------
          ^
          |     --------------------
          |    |Bluetooth Enumerator|
          -----|    (BthEnum.SYS)   |
                --------------------
                          ^
                          |          (Bth port driver loaded by bthusb.sys)
                          |     ---------------------       ----------------------
                          -----|     bthport.SYS     |<--->|      bthusb.SYS      |
                                ---------------------       ----------------------
                                                                     ^
                                                                     |
                                                                     V
                                                            ----------------------
                                                           |       USB Stack      |
                                                            ----------------------
                                                                     ^
                                                                     |
                                                                     V
                                                            ----------------------
                                                          | USB Bluetooth Dongle |
                                                             ----------------------
```

**Client Installation**

1. **Important**: This must be done on a separate machine from the one where echo server device was installed.

2. Copy KMDF coinstaller (wdfcoinstallerMMmmm.dll, from redist\\wdf\\), BthEchoSampleCli.Sys, BthEchoSampleCli.inf and bthecho.exe on a temporary directory on the target machine.

3. Run bthprops.cpl from a command line or right click on the Bluetooth icon in the system tray and select 'Show Devices' to bring up a list of installed Bluetooth devices.

4. In the **Bluetooth Devices** window, click the **Add a device** button.

5. In the Add a Device wizard select the server machine (the machine where you installed the echo server) as a Bluetooth device. If the server machine does not appear, please check the echo server installation and make sure that you have enabled 'Allow Bluetooth device to find this computer' on the server machine as explained above. When the server machine is correctly displayed, select it and pick 'Next'.

6. The wizard should default to a numeric compare ceremony for pairing the machines. When this happens, ensure the numbers match on both the client and the server, select 'Yes' on both machines to indicate they match, and click 'Next' on both machines to complete the pairing.

7. Go through Device Manager and update driver software for it. Either point the wizard to the temporary directory created in step \#2, or use devcon.exe from the tools\\devcon folder to install the client device:

  ```
  Devcon.exe update BthEchoSampleCli.inf BTHENUM\{c07508f2-b970-43ca-b5dd-cc4f2391bef4}
  ```

  Check for any error from devcon.exe as described in server installation.

  If the installation is successful, you will see 'Bluetooth Echo Sample Client' in Device Manager under Bluetooth devices.

The device tree for echo client device is similar to the one shown for the echo server device, since both client and server are enumerated by BthEnum.sys (although the installation mechanism and properties of client and server are different).

**Uninstalling Server**

1. Uninstall the device and delete driver software using the Bluetooth Devices window by running bthprops.cpl, right clicking on the device, and selecting 'Remove Device'

2. Run bthsrvinst.exe /u to uninstall the echo server. This would make bthenum.sys stop enumerating the echo server. Without this step 'Found New Hardware' wizard will be launched again when you reconnect the device.

**Uninstalling Client**


- Uninstall the device and delete driver software using the Bluetooth Devices window by running bthprops.cpl, right clicking on the device, and selecting 'Remove Device'.

**TESTING**

Run BthEcho.exe on the client machine. You should see client sending data to the server and receiving the same data echoed back. Press Ctrl+c to terminate the application. You will see output similar to below:

```
D:\bth\wdfcli>BthEcho.exe
DevicePath: \\?\bthenum#{c07508f2-b970-43ca-b5dd-cc4f2391bef4}_localmfg&000a#7&3
62d0a3&0&000c55ff727a_c00000001#{fc71b33d-d528-4763-a86c-78777c7bcd7b}
Opened device successfully
Written                 26 bytes: WDF Bluetooth Sample Echo
Reply from server       26 bytes: WDF Bluetooth Sample Echo
Written                 26 bytes: WDF Bluetooth Sample Echo
Reply from server       26 bytes: WDF Bluetooth Sample Echo
Written                 26 bytes: WDF Bluetooth Sample Echo
Reply from server       26 bytes: WDF Bluetooth Sample Echo
Written                 26 bytes: WDF Bluetooth Sample Echo
Reply from server       26 bytes: WDF Bluetooth Sample Echo
Written                 26 bytes: WDF Bluetooth Sample Echo
^C
```

You can launch multiple instances of BthEcho.exe. Each client application would cause echo client device to have an independent connection to the echo server and thereby have an independent echo session. You can also have echo client devices and apps installed on multiple machines and talking to a single echo server.

**CODE TOUR**

**Common code**

**Connection object**: The common code contains implementation of a connection object (in connection.\*) that is utilized by both client and the server. Client uses this object to maintain the information about opened connection and the server uses this object to maintain information about the accepted connection. Connection object also supports continuous reader which is utilized by server to read the data sent by client. Connection object is implemented as a WDF user object. Connection details are maintained in BTHECHO\_CONNECTION structure. This structure is used as the context for WDF user object. Passive dispose is used to make the connection object cleanup wait for disconnect completion and continuous reader rundown. Please see WDF documentation for WDF user object and passive dispose (see WdfObjectCreate).

**Server**

**Startup**: Server registers PSM and L2CA server and published SDP record on startup in BthEchoSrvEvtDeviceSelfManagedIoInit (this callback is invoked by WDF during device start). While registering the L2CA server it passes BthEchoSrvIndicationCallback as the callback for incoming connection notifications.

**Server SDP record creation**: The sample uses dynamic creation of server SDP record using Bluetooth DDIs. Please note that for your particular device you may be able to use static data for the SDP record.

**Incoming connections**: Bluetooth stack invokes BthEchoSrvIndicationCallback for the incoming connections from clients. In response server accepts the connection and passes BthEchoSrvConnectionIndicationCallback callback to Bluetooth stack for disconnect notification. It is possible to use the same indication callback for server and connection but the sample uses different callbacks for clarity. Server adds the accepted connection to the connection list it maintains. Please see below for connection rundown details.

**Connection rundown**: Server maintains a list of all the connections it has accepted. This allows server to properly close down these connections on orderly removal. On surprise removal bthport.sys itself gets removed and takes care of running down the connections, but in case of orderly removal driver has to make sure to rundown the connections.

**Connection state machine**:

```
                     ConnectFailed
                           ^
                           |
                           |
                           | (connection failure)
Uninitialized   ----> Connecting -------> Connected
                           | (disconnect)  /  ^
                           |              /  /
                           |             /  /
                           V            V  / (connection complete)
                     Disconnecting
                           | (disconnect complete)
                           |
                           |
                           V
                     Disconnected
```

One transition to note here is that if disconnect is received in the connecting state (i.e. when the connection is not completed) we wait for connection to completed (transition to connected state) and then invoke disconnect.

**Important**: Such state machine is needed only if Disconnect is initiated by something other than Bluetooth stack (for example device removal in our case). Bluetooth stack itself would not send disconnect before connect completion. If you adapt this sample for your device please evaluate whether your driver would require such state machine. For example, the echo client device does not need such state machine (although we use common connection code for client and the server).

**Shutdown**: Server removes the SDP record and unregisters L2CAP server and PSM in BthEchoSrvEvtDeviceSelfManagedIoCleanup (this callback is invoked by WDF during device removal). It also disconnects any open connections (see Connection rundown above).

**Client**

**Startup**: Client retrieves local and server Bluetooth address in BthEchoSrvEvtDeviceSelfManagedIoInit (this callback is invoked by WDF during device start). Please note that server Bluetooth address would be available regardless of presence of the server. Thus echo client device start doesn't fail even if echo server is not available.

**Connecting to server**: Client connects to echo server when application opens a handle to it (BthEchoCliEvtDeviceFileCreate) and closes the connection on file close (BthEchoCliEvtFileClose).

**Sending and receiving data from server**: When application writes data to the client, client sends this data to server on the connection opened for the given handle. Server would echo back this data. This data is retrieved by the application using a read operation. Echo Sample Client doesn't do any draining of echoed data on its own. Application read/write are handled using a parallel WDF I/O queue.

**Shutdown**: Client doesn't need any specific shutdown code as connection open/close and read/write are done within the context of application's handle open.


