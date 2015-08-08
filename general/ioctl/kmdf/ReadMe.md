Non-PnP Driver Sample
====================

This sample is primarily meant to demonstrate how to write a NON-PNP driver using the Kernel Mode Driver Framework.

It also illustrates several other important framework interfaces. Following table gives a typical usage scenario and summarizes all the features used in this sample.

This sample would be useful for writing a driver that does not interact with any hardware. Typically, such drivers are written to provide some kernel-level services to a user application. These drivers are dynamically loaded by the application when it is run and unloaded when it exits.

(Examples: FileMon, Regmon, DeviceTree are examples of tools that use this type of driver.)

-   How to Write a NON PNP driver

-   How to register EvtPreProcessCallback to handle requests in the context of the calling thread

-   Show how to probe and lock buffers in the preprocess callback for METHOD\_NEITHER IOCTL requests

-   Also show how to handle other 3 types of IOCTLs (METHOD\_BUFFERED, METHOD\_IN\_DIRECT & METHOD\_OUT\_DIRECT)

-   How to open a file in Kernel-mode and Read & Write to it

-   Finally show event tracing and dumping variable length data in the tracelog using HEXDUMP format.

The sample is accompanied by a simple multithreaded Win32 console application to test the driver.

*Disclaimer*: This is a minimal driver meant to demonstrate an OS feature. Neither it nor its sample programs are intended for use in a production environment. Rather, they are intended for educational purposes and as a skeleton driver.


Build the sample
----------------

For information on how to build a driver solution using Microsoft Visual Studio, see [Building a Driver](http://msdn.microsoft.com/en-us/library/windows/hardware/ff554644).

If the build succeeds, you will find the driver, nonpnp.sys, and the test application, nonpnpapp.exe, in the binary output directory specified for the build environment.

To test this driver, copy the nonpnp.inf into the same folder as the nonpnpapp.exe and the wdfcoinstaller\<version\>.dll .

**Note** You can obtain redistributable framework updates by downloading the *wdfcoinstaller.msi* package from [WDK 8 Redistributable Components](http://go.microsoft.com/fwlink/p/?LinkID=226396). This package performs a silent install into the directory of your Windows Driver Kit (WDK) installation. You will see no confirmation that the installation has completed. You can verify that the redistributables have been installed on top of the WDK by ensuring there is a redist\\wdf directory under the root directory of the WDK, %ProgramFiles(x86)%\\Windows Kits\\8.0.

Next, run nonpnpapp.exe, a simple Win32 multithreaded console mode application. The driver will be automatically loaded and started. When you exit the app, the driver will be stopped and removed.

Usage: nonpnpapp.exe (-l) (-v version)

**Note** This application first tries to open the device (\\Device\\FileIo). If the device doesn't exist, it takes that as a hint that the driver is not loaded and tries to load the driver using service control manager API. If the service is loaded successfully, it tries to open the device again. If successful, it makes all four different types of DeviceControl calls to the driver. After that it makes a WriteFile call with an arbitrary size buffer. The driver, in response, writes that buffer to a file opened in the Create request. The name of the file was provided by the application as part of the device name and the directory path is hardcoded to %WINDIR%\\temp. When the WriteFile returns, the application makes a ReadFile call to read the file through the driver, and then compares the data returned by the driver with the one it originally wrote. If you specify -l option in command line, the application does this Write and Read operation in an infinite loop. The -v command line option is used to specify the version of the KMDF coinstaller (wdfcoinstaller\<version\>.dll) to load. If none is specified then it loads the coinstaller for v1.0 (wdfcoinstaller01000.dll)

### WDF SECTION

Nonpnp drivers typically don't need an INF file to install. Since we are using framework interfaces, we have to use the Kmdf coinstaller to install the framework binaries on the target machine. The Kmdf coinstaller needs a WDF specific section in the INF to get the driver service name and the version of the Kmdf library the driver is bound to. The syntax and description of the section is given below. Any non inf based driver using Kmdf library will need to have a dummy inf file with the wdf section in it. The format of the Wdf section is given below:

[Version]

Signature="\$WINDOWS NT\$"

[\<driver name\>.NT.Wdf]

KmdfService = \<your driver service name here\>, \<driver service install subsection\>

[\<driver service install subsection\>]

KmdfLibraryVersion = \<version bound to here, in Major.minor format\>

For example, for V1.0 KmdfLibraryVersion is "1.0"

