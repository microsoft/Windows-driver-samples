---
page_type: sample
description: "The Microsoft SDCA Virtual Audio Device Driver (SdcaVad) shows how to develop ACX/SDCA audio drivers that expose support for SDCA audio devices."
languages:
- cpp
products:
- windows
- windows-wdk
urlFragment: sdcavad
---

# SDCA Virtual Audio Device Driver Sample

## Introduction

The Microsoft SDCA Virtual Audio Device Driver (SdcaVad) shows how to develop ACX/SDCA audio drivers that expose support for SDCA audio devices.

The following table shows the features that are implemented in the various subdirectories of this sample.

| Directory | Description |
| --- | --- |
| SdcaVCodec | SDCA Virual Codec Driver. |
| SdcaVDsp | SDCA Virual DSP Driver. |
| SdcaVXu | SDCA Virtual XU Driver. |
| Apo\kws | Sample APO that uses KSPROPERTY_INTERLEAVEDAUDIO_FORMATINFORMATION to determine if the keyword spotter pin is interleaving loopback audio with the microphone audio and identify which channels contain loopback audio. If it is interleaved the APO will strip out the loopback audio and deliver only the microphone audio upstream. Because channel data is removed, the APO negotiates an output format which is different than the input format. |
| EventDetectorAdapter | Sample Event Detector Adapter. |

## Build the sample

If you simply want to build this sample driver and don't intend to run or test it, then you do not need a target computer (also called a test computer). If, however, you would like to deploy, run and test this sample driver, then you need a second computer that will serve as your target computer. Instructions are provided in the **Run the sample** section to show you how to set up the target computer - also referred to as *provisioning* a target computer.

Perform the following steps to build this sample driver.

### 1. Open the driver solution in Visual Studio

In Microsoft Visual Studio, Click **File** \> **Open** \> **Project/Solution...** and navigate to the folder that contains the sample files (for example, *C:\Windows-driver-samples\audio\sdcavad*). Double-click the *sdcavad* solution file.

In Visual Studio locate the Solution Explorer. (If this is not already open, choose **Solution Explorer** from the **View** menu.) In Solution Explorer, you can see one solution that has six projects.

### 2. Set the sample's configuration and platform

In Solution Explorer, right-click **Solution 'sdcavad' (6 of 6 projects)**, and choose **Configuration Manager**. Make sure that the configuration and platform settings are the same for the six projects. Set the configuration to **Debug**, and the platform to **x64** for all the projects. If you make any configuration and/or platform changes for one project, you must make the same changes for all the remaining projects.

### 3. Build the sample using Visual Studio

In Visual Studio, click **Build** \> **Build Solution**.

### 4. Locate the built driver package

In File Explorer, navigate to the folder that contains the sample files. For example, you would navigate to *C:\\Windows-driver-samples\\audio\\sdcavad*, if that's the folder you specified in the preceding Step 1.

In the folder, the location of the driver package varies depending on the configuration and platform settings that you selected in the **Configuration Manager**. For example, if you set **Debug** and **x64**, then the built driver package will be saved to a folder named *Debug* inside a folder named *x64*. Double-click the folder for the built driver package, and then double-click the folder named *package*.

The package should contain these files:

| File | Description |
| --- | --- |
| SdcaVCodec.sys | The SDCA Virtual Codec Driver file. |
| SdcaVCodec.inf | A information(INF) file that contians information needed to install the SDCA Virtual Codec Driver. |
| SdcaVDsp.sys | The SDCA Virtual DSP Driver file. |
| SdcaVDsp.inf | A information(INF) file that contians information needed to install the SDCA Virtual DSP Driver. |
| SdcaVXu.sys | The SDCA Virtual XU Driver file. |
| SdcaVXu.inf | A information(INF) file that contians information needed to install the SDCA Virtual XU Driver. |
| EventDetectorContosoAdapter.dll | Sample Event detector adapter. |
| SdcaVKwsApo.dll | The KWS APO. |
| SdcaVApo.inf | A information (INF) file that installs an APO device. |
| sdcavad.cat | A signed catalog file, which serves as the signature for the entire package. |

## Run the sample

The computer where you install the driver is called the *target computer* or the *test computer*. Typically this is a separate computer from the computer on which you develop and build the driver package. The computer where you develop and build the driver is called the *host computer*.

The process of moving the driver package to the target computer and installing the driver is called *deploying* the driver. You can deploy the SDCA sample driver automatically or manually.

### Prepare the target computer

First of all, install the latest [Windows Driver Kit](https://docs.microsoft.com/windows-hardware/drivers/download-the-wdk) (WDK) on the target computer, minimum version required for the WDK is 25926, which corresponds to the canary channel.

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

Create a folder on the target for the built driver package (for example, *C:\\SdcaVad*). Copy all the files from the built driver package on the host computer to the folder that you created on the target computer.

Create a folder on the target computer for the certificate created by the build process. For example, you could create a folder named *C:\\Certificates* on the target computer, and then copy *package.cer* to it from the host computer. You can find this certificate in the same folder on the host computer, as the *package* folder that contains the built driver files. On the target computer, right-click the certificate file, and click **Install**, then follow the prompts to install the test certificate.

If you need more detailed instructions for setting up the target computer, see [Preparing a Computer for Manual Driver Deployment](https://docs.microsoft.com/windows-hardware/drivers/develop/preparing-a-computer-for-manual-driver-deployment).

#### A note on signatures

Since most of these binary files are executed in kernel mode, it is important that they are signed and, optionally, to have a kernel debugger attached.

Without any signature or kernel debugger, the driver will not be installed in the target computer. With a kernel debugger attached, the driver can be installed and the driver files (.sys extension) would be loaded, but any user mode files (.dll files) will not be loaded.

The only way of installing and executing the whole driver sample is to have all the files (.sys, .dll and .cat) signed with a trusted certificate. This will allow the entire driver to be loaded even without a kernel debugger attached.

For more information on the subject, see [Driver signing](https://docs.microsoft.com/windows-hardware/drivers/install/driver-signing).

### Install the driver

#### Single INF files

Each sample driver contains an INF file, which will install the sample driver.

On the target computer, open a Command Prompt window as Administrator. Navigate to your driver package folder, and enter the following command:

`devcon install SdcaVDsp.inf SOUNDWIRETEST\DSP`
`devcon install SdcaVCodec.inf Root\SDCAVCodec`

Then, the XU INF (*SdcaVXu.inf*) and the APO INF (*SdcaVApo.inf*) can be installed - right-click the INF file and select **Install** to install it.

After successfully installing the sample drivers, you're now ready to test it.

### Test the driver

On the target computer, in a Command Prompt window, enter **devmgmt.msc** to open Device Manager. In Device Manager, on the **View** menu, choose **Devices by type**. In the device tree, locate *SDCA Virtual Dsp Audio Driver*. This is typically under the **Sound, video and game controllers** node.

On the target computer, open Control Panel and navigate to **Hardware and Sound** \> **Manage audio devices**. In the Sound dialog box, select the speaker icon labeled as *SDCA Virtual Codec Audio Driver*, then click **Set Default**, but do not click **OK**. This will keep the Sound dialog box open.

Locate an MP3 or other audio file on the target computer and double-click to play it. Then in the Sound dialog box, verify that there is activity in the volume level indicator associated with the *SDCA Virtual Codec Audio Driver* driver.

## HLK testing

The sample uploaded here is tested using the latest HLK version available to make sure it passes all audio tests in the current playlist. However, since it is a virtual audio driver it does not implement audio mixing and simulates capture and loopback by generating a tone. Given these limitations, there are some HLK tests that are expected to fail because they rely on the described functionality.

In the case of audio tests, one of these exceptions is the Hardware Offload of Audio Processing Test. This test is aimed at devices that support offload capabilities and performs checks to make sure that the device complies with the appropiate requirements. In the particular case of SdcaVad, this test will fail for endpoints with offload and loopback.

For endpoints with offload, the test will fail because the driver includes offload pins but it does not implement a mixer with volume, mute and peak meter nodes, etc. For the case of endpoints with loopback, the test will fail because the driver simulates loopback by returning a sine tone instead of performing real mixing of streams in host and/or offload pins.

Besides, the current version of SdcaVad also failed the General Audio Test and the Device Power State Transition Test and we're investigating the failures.
