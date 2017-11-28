<!---
    name: SysVAD Virtual Audio Device Driver Sample
    platform: WDM
    language: cpp
    category: Audio
    description: The Microsoft SysVAD Virtual Audio Device Driver (SYSVAD) shows how to develop a WDM audio driver that exposes support for multiple audio devices.
    samplefwlink: http://go.microsoft.com/fwlink/p/?LinkId=620183
--->

SysVAD Virtual Audio Device Driver Sample
========================================

The Microsoft SysVAD Virtual Audio Device Driver (SYSVAD) shows how to develop a WDM audio driver that exposes support for multiple audio devices.

Some of these audio devices are embedded in the system (for example, speakers, microphone arrays) while others are pluggable (like headphones, speakers, microphones, Bluetooth headsets etc.). The driver uses WaveRT and audio offloading for rendering devices. The driver uses a "virtual audio device" instead of an actual hardware-based adapter, and highlights the different aspects of the audio offloading WDM audio driver architecture.

Driver developers can use the framework in this sample to provide support for various audio devices without concern for hardware dependencies. The framework includes implementations of the following interfaces:

-   The CAdapterCommon interface gives the miniports access to virtual mixer hardware. It also implements the **IAdapterPowerManagement** interface.

-   The CMiniportTopologyMSVAD interface is the base class for all sample topologies. It has very basic common functions. In addition, this class contains common topology property handlers.

The following table shows the features that are implemented in the various subdirectories of this sample.


For more information about the Windows audio engine, see [Exposing Hardware-Offloaded Audio Processing in Windows](http://msdn.microsoft.com/en-us/windows/hardware/br259116), and note that audio hardware that is offload-capable replicates the architecture that is presented in the diagram shown in the topic.


Build the sample
----------------

If you simply want to Build this sample driver and don't intend to run or test it, then you do not need a target computer (also called a test computer). If, however, you would like to deploy, run and test this sample driver, then you need a second computer that will server as your target computer. Instructions are provided in the **Run the sample** section to show you how to set up the target computer - also referred to as *provisioning* a target computer.

Perform the following steps to build this sample driver.

**1. Open the driver solution in Visual Studio**

In Microsoft Visual Studio, Click **File** \> **Open** \> **Project/Solution...** and navigate to the folder that contains the sample files (for example, *C:\Windows-driver-samples\audio\sysvad*). Double-click the *sysvad* solution file.

In Visual Studio locate the Solution Explorer. (If this is not already open, choose **Solution Explorer** from the **View** menu.) In Solution Explorer, you can see one solution that has four projects.

**2. Set the sample's configuration and platform**

In Solution Explorer, right-click **Solution 'sysvad' (4 projects)**, and choose **Configuration Manager**. Make sure that the configuration and platform settings are the same for the four projects. By default, the configuration is set to **Debug**, and the platform is set to **Win32** for all the projects. If you make any configuration and/or platform changes for one project, you must make the same changes for the remaining three projects.

**3. Build the sample using Visual Studio**

In Visual Studio, click **Build** \> **Build Solution**.

**4. Locate the built driver package**

In File Explorer, navigate to the folder that contains the sample files. For example, you would navigate to *C:\\Documents\\Windows-driver-samples\\audio\\sysvad*, if that's the folder you specified in the preceding Step 1.

In the folder, the location of the driver package varies depending on the configuration and platform settings that you selected in the **Configuration Manager**. For example, if you left the default settings unchanged, then the built driver package will be saved to a folder named *Debug* inside the same folder as the sample files. Double-click the folder for the built driver package, and then double-click the folder named *package*.

The package should contain these files:

File | Description 
-----|------------
TabletAudioSample.sys OR PhoneAudioSample.sys| The driver file.
SwapAPO.dll | A sample APO. 
DelayAPO.dll | A sample APO. 
sysvad.cat | A signed catalog file, which serves as the signature for the entire package. 
TabletAudioSample.inf | An information (INF) file that contains information needed to install the driver. 
PhoneAudioSample.inf | An information (INF) file that contains information needed to install the driver.

Run the sample
--------------

The computer where you install the driver is called the *target computer* or the *test computer*. Typically this is a separate computer from the computer on which you develop and build the driver package. The computer where you develop and build the driver is called the *host computer*.

The process of moving the driver package to the target computer and installing the driver is called *deploying* the driver. You can deploy the sample driver, TabletAudioSample or PhoneAudioSample, automatically or manually.

### Automatic deployment

Before you automatically deploy a driver, you must provision the target computer. Verify that the target computer has an ethernet cable connecting it to your local network, and that your host and target computers can ping each other. Then perform the following steps to prepare your host and target computers.

**1. Provision the target computer**

On the target computer install the latest [Windows Driver Kit](http://msdn.microsoft.com/en-us/windows/hardware/gg454513.aspx) (WDK), and then when the installation is completed, navigate to the following folder:

\\Program Files (x86)\\Windows Kits\\10\\Remote\\<*architecture*>\\

For example, if your target computer is an x64 machine, you would navigate to:

\\Program Files (x86)\\Windows Kits\\10\\Remote\\x64\\

Double-click the *WDK Test Target Setup x64-x64\_en-us.msi* file to run it. This program prepares the target computer for provisioning.

On the host computer, in Visual Studio click **Driver** \> **Test** \> **Configure Computers...**, and then click **Add a new computer**.

Type the name of the target computer, select **Provision computer and choose debugger settings**, and click **Next**. In the next window, verify that the **Connection Type** is set to Network. Leave the other (default) settings as they are, and click **Next**. For more information about the settings in this window, see [Getting Set Up for Debugging](http://msdn.microsoft.com/en-us/library/windows/hardware/hh450944(v=vs.85).aspx).

**2. Prepare the host computer**

If you haven't already done so, then preform the steps in the **Build the sample** section, to build the sample driver.

In Visual Studio, in Solution Explorer, right click **package** (lower case), and choose **Properties**. Navigate to **Configuration Properties** \> **Driver Install** \> **Deployment**.

Check , **Enable deployment** and check **Remove previous driver versions before deployment**. For **Target Computer Name**, select the name of a target computer that you provisioned previously. Select **Hardware ID Driver Update**, and enter *\*Root\sysvad_TabletAudioSample* for the hardware ID. Click **OK**.

On the **Build** menu, choose **Deploy Package** or **Build Solution**. This will deploy the sample driver to your target computer.

On the target computer, perform the steps in the **Test the sample** section to test the sample driver.

### Manual deployment

Before you manually deploy a driver, you must prepare the target computer by turning on test signing and by installing a certificate. You also need to locate the DevCon tool in your WDK installation. After that you're ready to run the built driver sample.

**1. Prepare the target computer**

Open a Command Prompt window as Administrator. Then enter the following command:

**bcdedit /set TESTSIGNING ON**

Reboot the target computer. Then navigate to the Tools folder in your WDK installation and locate the DevCon tool. For example, look in the following folder:

*C:\\Program Files (x86)\\Windows Kits\\10\\Tools\\x64\\devcon.exe*

Copy *devcon.exe* to a folder on the target computer where it is easier to find. For example, create a *C:\\Tools* folder and copy *devcon.exe* to that folder.

Create a folder on the target for the built driver package (for example, *C:\\SysvadDriver*). Copy all the files from the built driver package on the host computer and save them to the folder that you created on the target computer.

Create a folder on the target computer for the certificate created by the build process. For example, you could create a folder named *C:\\Certificates* on the target computer, and then copy *package.cer* to it from the host computer. You can find this certificate in the same folder on the host computer, as the *package* folder that contains the built driver files. On the target computer, right-click the certificate file, and click **Install**, then follow the prompts to install the test certificate.

If you need more detailed instructions for setting up the target computer, see [Preparing a Computer for Manual Driver Deployment](http://msdn.microsoft.com/en-us/library/windows/hardware/dn265571(v=vs.85).aspx).

**2. Install the driver**

The TabletAudioSample or PhoneAudioSample driver package contains a sample driver and 2 driver extension samples. The following instructions show you how to install and test the sample driver. Here's the general syntax for the devcon tool that you will use to install the driver:

**devcon install \<*INF file*>\<*hardware ID*\>**

The INF file required for installing this driver is *sysvad.inf*. Here's how to find the hardware ID for installing the *TabletAudioSample.sys or PhoneAudioSample* sample: On the target computer, navigate to the folder that contains the files for your driver (for example, *C:\\SysvadDriver*). Then right-click the INF file (*sysvad.inf*) and open it with Notepad. Use Ctrl+F to find the [MicrosoftDS] section. Note that there is a comma-separated element at the end of the row. The element after the comma shows the hardware ID. So for this sample, the hardware ID is \*ROOT\sysvad_TabletAudioSample.

On the target computer, open a Command Prompt window as Administrator. Navigate to your driver package folder, and enter the following command:

**devcon install sysvad.inf \*ROOT\sysvad_TabletAudioSample*

If you get an error message about *devcon* not being recognized, try adding the path to the *devcon* tool. For example, if you copied it to a folder called *C:\\Tools*, then try using the following command:

**c:\\tools\\devcon install sysvad.inf   \*ROOT\sysvad_TabletAudioSample**

For more detailed instructions, see [Configuring a Computer for Driver Deployment, Testing, and Debugging](http://msdn.microsoft.com/en-us/library/windows/hardware/hh698272(v=vs.85).aspx).

After successfully installing the sample driver, you're now ready to test it.

### Test the driver

On the target computer, in a Command Prompt window, enter **devmgmt** to open Device Manager. In Device Manager, on the **View** menu, choose **Devices by type**. In the device tree, locate *Microsoft Virtual Audio Device (WDM) - Tablet Audio Sample*. This is typically under the **Sound, video and game controllers** node.

On the target computer, open Control Panel and navigate to **Hardware and Sound** \> **Manage audio devices**. In the Sound dialog box, select the speaker icon labeled as *Microsoft Virtual Audio Device (WDM) - Tablet Audio Sample*, then click **Set Default**, but do not click **OK**. This will keep the Sound dialog box open.

Locate an MP3 or other audio file on the target computer and double-click to play it. Then in the Sound dialog box, verify that there is activity in the volume level indicator associated with the *Microsoft Virtual Audio Device (WDM) - Tablet Audio Sample* driver.

