Sample Function Driver for OSR USB-FX2 (UMDF Version 2)
=======================================================

The umdf2\_fx2 sample is a User-Mode Driver Framework (UMDF) version 2 driver for the OSR USB-FX2 device.

The specification for the device can be found at <http://www.osronline.com/hardware/OSRFX2_32.pdf>. The driver and sample device metadata also work with the [Custom driver access](http://go.microsoft.com/fwlink/p/?LinkID=248288) sample.


Related technologies
--------------------

[User-Mode Driver Framework](http://msdn.microsoft.com/en-us/library/windows/hardware/ff560456)


Run the sample
--------------

The computer where you install the driver is called the *target computer* or the *test computer*. Typically this is a separate computer from where you develop and build the driver package. The computer where you develop and build the driver is called the *host computer*.

The process of moving the driver package to the target computer and installing the driver is called *deploying the driver*. You can deploy a driver sample automatically or manually.

### Automatic deployment (FX2 board)

Before you automatically deploy a driver, you must provision the target computer. For instructions, see [Configuring a Computer for Driver Deployment, Testing, and Debugging](http://msdn.microsoft.com/en-us/library/windows/hardware/).

1.  Plug in the OSR USB-FX2 board to the target computer.
2.  On the host computer, in Visual Studio, in Solution Explorer, right click **package** (lower case), and choose **Properties**. Navigate to **Configuration Properties \> Driver Install \> Deployment**.
3.  Check **Enable deployment**, and check **Remove previous driver versions before deployment**. For **Target Computer Name**, select the name of a target computer that you provisioned previously. Select **Install and Verify**. Click **OK**.
4.  On the **Build** menu, choose **Deploy Package** or **Build Solution**.

### Manual deployment (FX2 board)

Before you manually deploy a driver, you must turn on test signing and install a certificate on the target computer. You also need to copy the [DevCon](http://msdn.microsoft.com/en-us/library/windows/hardware/ff544707) tool to the target computer. For instructions, see [Preparing a Computer for Manual Driver Deployment](http://msdn.microsoft.com/en-us/library/windows/hardware/dn265571).

1.  Copy all of the files in your driver package to a folder on the target computer (for example, c:\\umdf2\_fx2).
2.  Plug in the OSR USB-FX2 board to the target computer. Open a Command Prompt window and enter **dvmgmt** to open Device Manager. In Device Manager, locate the node for the OSR USB-FX2 board. Right click the node, and choose **Properties**. In the **Details** tab, under **Properties**, choose **Hardware Ids**. Note the hardware IDs listed for your FX2 board. One of these IDs should match one of the IDs in the osrusbfx2um.inf file. For example, Device Manager might show an ID of USB\\VID\_0547&PID\_1002, which matches one of the IDs in the [Microsoft.*xxx*] section of osrusbfx2um.inf.

3.  On the target computer, open a Command Prompt window as Administrator. Navigate to your driver package folder, and enter this command:

    **devcon update osrusbfx2um.inf"***HardwareID***"**

    where *HardwareID* is the hardware ID of your FX2 board. Here is an example:

    **devcon update osrusbfx2um.inf "USB\\VID\_0547&PID\_1002"**

View the driver for the OSR USB-FX2 board in Device Manager
-----------------------------------------------------------

On the target computer, in your Command Prompt window, enter **devmgmt** to open Device Manager. In Device Manager, on the **View** menu, choose **Devices by type**. In the device tree, locate **UMDF 2.0 Sample Driver for OSR Fx2 Learning Kit** (for example, this might be under the **Sample Device** node).

In Device Manager, on the **View** menu, choose **Devices by connection**. Locate **UMDF 2.0 Sample Driver for OSR Fx2 Learning Kit** as a child of a USB hub node, which may be contained with the **ACPI x64-based PC** node.

Build the sample using MSBuild
------------------------------

As an alternative to building the driver sample in Visual Studio, you can build it in a Visual Studio Command Prompt window. In Visual Studio, on the **Tools** menu, choose **Visual Studio Command Prompt**. In the Visual Studio Command Prompt window, navigate to the folder that has the solution file, umdf2echo.sln. Use the MSBuild command to build the solution. Here is an example:

**msbuild /p:configuration=”Win8 Release” /p:platform=”Win32” umdf2\_fx2.sln**

For more information about using MSBuild to build a driver package, see [Building a Driver](http://msdn.microsoft.com/en-us/library/windows/hardware/ff554644).

