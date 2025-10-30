---
page_type: sample
description: "The Microsoft Simple Audio Sample Device Driver shows how to develop a simple WDM audio driver that exposes support for two basic audio devices (a speaker and microphone array)."
languages:
- cpp
products:
- windows
- windows-wdk
---

# Simple Audio Sample Device Driver

## Introduction

The Microsoft Simple Audio Sample Device Driver shows how to develop a simple WDM audio driver that exposes support for two basic audio devices (a speaker and microphone array). These audio devices are embedded in the system (not pluggable) and the driver uses WaveRT for rendering these devices. The driver also uses a "virtual audio device" instead of an actual hardware-based adapter.

Driver developers can use the framework in this sample to provide support for various audio devices without concern for hardware dependencies. The framework includes implementations of the following interfaces:

- The CAdapterCommon interface gives the miniports access to virtual mixer hardware. It also implements the **IAdapterPowerManagement** interface.

- The CMiniportTopologySimpleAudioSample interface is the base class for all sample topologies. It has very basic common functions. In addition, this class contains common topology property handlers.

The following table shows the contents of the various subdirectories of this sample.

| Directory | Description |
| --- | --- |
| Source | Contains all of the source code for the Simple Audio Sample driver. |
| Filters | Contains the definitions of the wave and topology filters of the endpoints that are present in the Simple Audio Sample driver (speaker and microphone array). |
| Main | Contains the core functionality of the driver. |
| Utilities | Contains helper files that provide additional functionality, such as a test tone generator and an audio file reader. |

## Build the sample

If you simply want to build this sample driver and don't intend to run or test it, then you do not need a target computer (also called a test computer). If, however, you would like to deploy, run and test this sample driver, then you need a second computer that will serve as your target computer. Instructions are provided in the **Run the sample** section to show you how to set up the target computer - also referred to as *provisioning* a target computer.

Perform the following steps to build this sample driver.

### 1. Open the driver solution in Visual Studio

In Microsoft Visual Studio, Click **File** \> **Open** \> **Project/Solution...** and navigate to the folder that contains the sample files (for example, *C:\Windows-driver-samples\audio\SimpleAudioSample*). Double-click the *SimpleAudioSample* solution file.

In Visual Studio locate the Solution Explorer. (If this is not already open, choose **Solution Explorer** from the **View** menu.) In Solution Explorer, you can see one solution that has five projects.

### 2. Set the sample's configuration and platform

In Solution Explorer, right-click **Solution ‘SimpleAudioSample’ (5 of 5 projects)**, and choose **Configuration Manager**. Make sure that the configuration and platform settings are the same for the five projects. By default, the configuration is set to **Debug**, and the platform is set to **Win32** for all the projects. If you make any configuration and/or platform changes for one project, you must make the same changes for all the remaining projects.

### 3. Build the sample using Visual Studio

In Visual Studio, click **Build** \> **Build Solution**.

### 4. Locate the built driver package

In File Explorer, navigate to the folder that contains the sample files. For example, you would navigate to *C:\\Windows-driver-samples\\audio\\SimpleAudioSample*, if that's the folder you specified in the preceding Step 1.

In the folder, the location of the driver package varies depending on the configuration and platform settings that you selected in the **Configuration Manager**. For example, if you left the default settings unchanged, then the built driver package will be saved to a folder named *Debug* inside the same folder as the sample files. Double-click the folder for the built driver package, and then double-click the folder named *package*.

The package should contain these files:

| File | Description |
| --- | --- |
| SimpleAudioSample.sys | The driver file. |
| SimpleAudioSample.cat | A signed catalog file, which serves as the signature for the entire package. |
| SimpleAudioSample.inf | A non-componentized information (INF) file that contains information needed to install the driver. |

## Run the sample

The computer where you install the driver is called the *target computer* or the *test computer*. Typically this is a separate computer from the computer on which you develop and build the driver package. The computer where you develop and build the driver is called the *host computer*.

The process of moving the driver package to the target computer and installing the driver is called *deploying* the driver. You can deploy the SimpleAudioSample driver automatically or manually.

### Prepare the target computer

First of all, install the latest [Windows Driver Kit](https://docs.microsoft.com/windows-hardware/drivers/download-the-wdk) (WDK) on the target computer.

Before you manually deploy a driver, you must prepare the target computer by turning on test signing and by installing a certificate. You also need to locate the DevCon tool in your WDK installation. After that you're ready to run the built driver sample.

Open a Command Prompt window as Administrator. Then enter the following command:

`bcdedit /set TESTSIGNING ON`

and reboot the target computer.

> [!IMPORTANT]
> Before using BCDEdit to change boot information you may need to temporarily suspend Windows security features such as BitLocker and Secure Boot on the test PC.

Re-enable these security features when testing is complete and appropriately manage the test PC, when the security features are disabled.

After rebooting, navigate to the Tools folder in your WDK installation and locate the DevCon tool. For example, look in the following folder:

C:\\Program Files (x86)\\Windows Kits\\10\\Tools\\x64\\devcon.exe

Copy *devcon.exe* to a folder on the target computer where it is easier to find. For example, create a *C:\\Tools* folder and copy *devcon.exe* to that folder.

Create a folder on the target for the built driver package (for example, *C:\\SimpleAudioSampleDriver*). Copy all the files from the built driver package on the host computer and save them to the folder that you created on the target computer.

Create a folder on the target computer for the certificate created by the build process. For example, you could create a folder named *C:\\Certificates* on the target computer, and then copy *package.cer* to it from the host computer. You can find this certificate in the same folder on the host computer, as the *package* folder that contains the built driver files. On the target computer, right-click the certificate file, and click **Install**, then follow the prompts to install the test certificate.

If you need more detailed instructions for setting up the target computer, see [Preparing a Computer for Manual Driver Deployment](https://docs.microsoft.com/windows-hardware/drivers/develop/preparing-a-computer-for-manual-driver-deployment).

#### A note on signatures

Since most of these binary files are executed in kernel mode, it is important that they are signed and, optionally, to have a kernel debugger attached.

Without any signature or kernel debugger, the driver will not be installed in the target computer. With a kernel debugger attached, the driver can be installed and the driver files (.sys extension) would be loaded. The only way of installing and executing the driver sample without a kernel debugger attached is to have both the .sys and .cat files signed with a trusted certificate.

For more information on the subject, see [Driver signing](https://docs.microsoft.com/windows-hardware/drivers/install/driver-signing).

### Install the driver

To install SimpleAudioSample.inf, open a Command Prompt window as administrator on the target computer, then navigate to your driver package folder and enter the following command:

`devcon install SimpleAudioSample.inf Root\SimpleAudioSample` 

If you get an error message about *devcon* not being recognized, try adding the path to the *devcon* tool. For example, if you copied it to a folder called *C:\\Tools*, then try using the following command:

`C:\\tools\\devcon install SimpleAudioSample.inf Root\SimpleAudioSample`

This installs the Simple Audio Sample driver with a hardware ID of "Root\SimpleAudioSample".

Once the INF file is installed, a device should appear in Device Manager named *Microsoft Virtual Audio Device (WDM) - Simple Audio Sample*.

After successfully installing the sample driver, you're now ready to test it.

For more detailed instructions, see [Provision a computer for driver deployment and testing](https://docs.microsoft.com/windows-hardware/drivers/gettingstarted/provision-a-target-computer-wdk-8-1).

### Test the driver

On the target computer, in a Command Prompt window, enter **devmgmt.msc** to open Device Manager. In Device Manager, on the **View** menu, choose **Devices by type**. In the device tree, locate *Virtual Audio Device (WDM) – Simple Audio Sample*. This is typically under the **Sound, video and game controllers** node.

On the target computer, open Control Panel and navigate to **Hardware and Sound** \> **Manage audio devices**. In the Sound dialog box, select the speaker icon labeled as *Virtual Audio Device (WDM) – Simple Audio Sample*, then click **Set Default**, but do not click **OK**. This will keep the Sound dialog box open.

Locate an MP3 or other audio file on the target computer and double-click to play it. Then in the Sound dialog box, verify that there is activity in the volume level indicator associated with the *Virtual Audio Device (WDM) – Simple Audio Sample* driver.
