SYSVAD Audio Device Driver Sample
========================================

The SYSVAD audio device driver sample shows how to develop a WDM audio driver that exposes support for multiple audio devices.

Some of these audio devices are embedded in the system (for example, speakers, microphone arrays) while others are pluggable (like headphones, speakers, microphones, Bluetooth headsets etc.). The driver uses WaveRT and audio offloading for rendering devices. The driver uses a "virtual audio device" instead of an actual hardware-based adapter, and highlights the different aspects of the audio offloading WDM audio driver architecture.

Driver developers can use the framework in this sample to provide support for various audio devices without concern for hardware dependencies. The framework includes implementations of the following interfaces:

-   The *CAdapterCommon* interface gives the miniports access to virtual mixer hardware. It also implements the **IAdapterPowerManagement** interface.

-   The *CMiniportTopologySYSVAD* interface is the base class for all sample topologies. It has very basic common functions. In addition, this class contains common topology property handlers.

For more information about the Windows audio engine, see [Exposing Hardware-Offloaded Audio Processing in Windows](http://msdn.microsoft.com/en-us/windows/hardware/br259116), and note that audio hardware that is offload-capable replicates the architecture that is presented in the diagram shown in the topic.


Build the sample
----------------

If you simply want to Build this sample driver and don't intend to run or test it, then you do not need a target computer (also called a test computer). If, however, you would like to deploy, run and test this sample driver, then you need a second computer that will server as your target computer. Instructions are provided in the **Run the sample** section to show you how to set up the target computer.

Perform the following steps to build this sample driver.

**1. Open the driver solution in Visual Studio**

In Microsoft Visual Studio, Click **File** \> **Open** \> **Project/Solution...** and navigate to the folder that contains the sample files (for example, *C:\Windows-driver-samples\audio\sysvad*). Double-click the *sysvad* solution file.

In Visual Studio locate the Solution Explorer. (If this is not already open, choose **Solution Explorer** from the **View** menu.) In Solution Explorer, you can see one solution that has four projects. Note that the project titled SwapAPO is actually a folder that contains two projects - APO and PropPageExtensions.

**2. Set the sample's configuration and platform**

In Solution Explorer, right-click **Solution 'sysvad' (7 projects)**, and choose **Configuration Manager**. Make sure that the configuration and platform settings are the same for the four projects. By default, the configuration is set to **Debug**, and the platform is set to **Win32** for all the projects. If you make any configuration and/or platform changes for one project, you must make the same changes for all of the other projects.

**3. Build the sample using Visual Studio**

In Visual Studio, click **Build** \> **Build Solution**.

**4. Locate the built driver package**

In File Explorer, navigate to the folder that contains the sample files. For example, you would navigate to *C:\\Documents\\Windows-driver-samples\\audio\\sysvad*, if that's the folder you specified in the preceding Step 1.

In the folder, the location of the driver package varies depending on the configuration and platform settings that you selected in the **Configuration Manager**. For example, if you left the default settings unchanged, then the built driver package will be saved to a folder named *Debug* inside the same folder as the sample files. Double-click the folder for the built driver package, and then double-click the folder named *package*.

The package should contain these files:

File | Description
-----|------------
PropPageExt.dll | A sample driver extension for a property page.
TabletAudioSample.sys | The tablet (PC) sample driver file.
PhoneAudioSample.sys | The tablet (PC) sample driver file.
SwapAPO.dll | A sample driver extension for a UI to manage APOs.
KeywordDetectorContosoAdapter.dll| A sample keyword detector that is used with voice activation.

Locate the inf file for the platform that you want to work with. For example tabletaudiosample.inf and copy it to the same *package* folder.

Run the sample
--------------

The computer where you install the driver is called the *target computer* or the *test computer*. Typically this is a separate computer from the computer on which you develop and build the driver package. The computer where you develop and build the driver is called the *host computer*.

The process of moving the driver package to the target computer and installing the driver is called *deploying* the driver. You can deploy the sample driver, SysvadAudioSample, automatically or manually.


### Manual deployment

Before you manually deploy a driver, you must prepare the target computer by turning on test signing. You also need to locate the DevCon tool in your WDK installation. After that you're ready to install and test driver sample.

**1. Prepare the target computer**

Open a Command Prompt window as Administrator. Then enter the following command:

**bcdedit /set TESTSIGNING ON**

Reboot the target computer. Then navigate to the Tools folder in your WDK installation and locate the DevCon tool. For example, look in the following folder:

*C:\\Program Files (x86)\\Windows Kits\\10\\Tools\\x64\\devcon.exe*

Copy *devcon.exe* to a folder on the target computer where it is easier to find. For example, create a *C:\\Tools* folder and copy *devcon.exe* to that folder.

Create a folder on the target for the built driver package (for example, *C:\\SysvadDriver*). Copy all the files from the built driver package on the host computer and save them to the folder that you created on the target computer.

If you need more detailed instructions for setting up the target computer, see [Preparing a Computer for Manual Driver Deployment](http://msdn.microsoft.com/library/windows/hardware/dn265571.aspx).

**2. Install the driver**

The package folder contains the sample drivers and the three driver extension DLL samples. The following instructions show you how to install and test the sample driver. Here's the general syntax for the devcon tool that you will use to install the driver:

**devcon install \<*INF file*>\<*hardware ID*\>**

The INF file required for installing this driver is *tabletaudiosample.inf*. Here's how to find the hardware ID for installing the *TabletAudioSample.sys* sample: On the target computer, navigate to the folder that contains the files for your driver (for example, *C:\\SysvadDriver*). Then right-click the INF file (*tabletaudiosample.inf*) and open it with Notepad. Use Ctrl+F to find the [HardwareIds] section. Note that there is a comma-separated element at the end of the row. The element after the comma shows the hardware ID. So for this sample, the hardware ID is *Root\sysvad_TabletAudioSample*.

On the target computer, open a Command Prompt window as Administrator. Navigate to your driver package folder, and enter the following command:

**devcon install tabletaudiosample.inf Root\sysvad_TabletAudioSample**

If the driver doesn't install, view this log file for additional information using notepad:
**%windir%\inf\setupapi.dev.log**

For more detailed instructions, see [Configuring a Computer for Driver Deployment, Testing, and Debugging](http://msdn.microsoft.com/en-us/library/windows/hardware/hh698272(v=vs.85).aspx).

After successfully installing the sample driver, you're now ready to test it.

### Automatic deployment

For information on automatic deployment, see [Provision a computer for driver deployment and testing](https://msdn.microsoft.com/library/windows/hardware/dn745909.aspx).

### Test the driver

On the target computer, in a Command Prompt window, enter **devmgmt** to open Device Manager. In Device Manager, on the **View** menu, choose **Devices by type**. In the device tree, locate *Microsoft Virtual Audio Device (WDM) - Tablet Sample*. This is typically under the **Sound, video and game controllers** node.

On the target computer, open Control Panel and navigate to **Hardware and Sound** \> **Manage audio devices**. In the Sound dialog box, select the speaker icon labeled as *Microsoft Virtual Audio Device (WDM) - Tablet Sample*, then click **Set Default**, but do not click **OK**. This will keep the Sound dialog box open.

Locate an MP3 or other audio file on the target computer and double-click to play it. Then in the Sound dialog box, verify that there is activity in the volume level indicator associated with the *Microsoft Virtual Audio Device (WDM) - Tablet Sample* driver.

You can attach a debugger and set breakpoints to step through the driver code as it runs. For more information, see [Getting Started with Windows Debugging](https://msdn.microsoft.com/library/windows/hardware/mt219729.aspx).  
