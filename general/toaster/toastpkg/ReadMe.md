Toaster Package Sample
======================

The Toastpkg sample simulates hardware-first and software-first installation of the toaster sample driver.

The Toaster Installation Package comprises driver projects (.vcxproj files) that are contained in the toastpkg.sln solution file (in general/toaster/toastpkg).

This document discusses the different approaches that end users take when adding new hardware to their computer, and describes an approach that addresses these scenarios in a consistent, robust manner. It also outlines the mechanisms provided to facilitate additional vendor requirements such as the installation of value-added software.

**Introduction**

The installation of software to support an instance of a given device (known as "device installation" or "driver installation") is done in a device-centric fashion in Windows operating systems. A device INF that matches up with one of the device's hardware or compatible IDs is used to identify the required driver file(s), registry modifications, etc., that are needed to make the device fully operational. This INF, along with the files copied thereby and a catalog that contains the digital signatures of the INF and these other files, constitute what is known as a "driver package".

Because device installation is done for a specific instance of a device, the "natural" method of adding devices to a computer running a Plug and Play operating system is by plugging in the device first, letting Plug and Play find the device and automatically initiate an installation for that device. The device installation may then proceed using a driver package supplied with the OS, or a "3rd-party" driver package (supplied via CD-ROM, the Internet, or some other distribution mechanism). When the device installation is initiated by the addition of hardware, this is termed a "hardware-first" device installation.

Users may, however, take an alternate approach to adding hardware to their computer. In this scenario, they first run a setup program (perhaps launched as an autorun application when the vendor-supplied CD-ROM is inserted). This setup program may perform installation activities, and then prompt the user to insert their hardware. Upon the hardware's insertion, the vendor-supplied driver package (which was "pre-installed" by the setup program) is then found by Plug and Play, and the installation proceeds as in the hardware-first scenario. When the device installation is initiated by running a setup program, this is termed a "software-first" device installation. This approach to adding new hardware is just as valid as the hardware-first scenario, and some vendors may even instruct their users (via documentation that ships with the hardware) that this is the preferred method.

Vendors must support the hardware-first scenario (by providing a driver package that may be supplied to the "Found New Hardware" wizard with no "pre-configuration" performed by a setup program or other mechanism). Vendors may optionally support the software-first scenario as well, but the actual installation of the device instance is done by Plug and Play upon the device's arrival, as described above.

Vendors may also wish to perform additional activities as part of the device installation. For example, the vendor may want to allow the user to optionally install one or more applications that ship with the device (e.g., a scanner that ships with an image processing application). Such software is termed "value-added software". Value-added software is distinct from the files that comprise the driver package because, unlike the core driver files, the device does not require value-added software to function properly. In the previous example of a scanner, for instance, perhaps the user already has an image processing application that they prefer. The user should be given the option of whether or not they want to install any value-added software. Additional activities (such as allowing the user to select value-added software offerings) may be accomplished by using a vendor-supplied device-specific co-installer.

