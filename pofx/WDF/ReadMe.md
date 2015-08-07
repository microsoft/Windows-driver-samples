KMDF Power Framework (PoFx) Sample
==================================

This solution consists of two samples that demonstrate how a KMDF driver can implement F-state-based power management. The SingleComp sample demonstrates how a KMDF driver can implement F-state-based power management for a device that has only a single component. The MultiComp sample demonstrates how a KMDF driver can implement F-state-based power management for a device that has an arbitrary number of components that can be individually power-managed.

## Universal Windows Driver Compliant
This sample builds a Universal Windows Driver. It uses only APIs and DDIs that are included in OneCoreUAP.

Related technologies
--------------------
[Supporting Functional Power States](http://msdn.microsoft.com/en-us/library/windows/hardware/hh451017)

SingleComp Overview
-------------------

This sample demonstrates how a KMDF driver can implement F-state-based power management for a device that has only a single component.

The sample illustrates the use of the [**WdfDeviceWdmAssignPowerFrameworkSettings**](http://msdn.microsoft.com/en-us/library/windows/hardware/hh451097) method to specify power framework settings for the single component that represents the entire device. The power framework settings that can be specified include the F-states for the component and the power framework callbacks that are invoked when the component's active/idle condition or its F-state changes.

The sample also illustrates the use of the [**WdfDeviceAssignS0IdleSettings**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff545903) method to instruct KMDF to begin power-management of the device (and the component that represents the entire device).

Installation
------------

The driver can be installed on a root-enumerated device using the devcon.exe tool.

1.  Obtain the devcon.exe tool from the WDK
2.  Copy the driver binary, INF file and the KMDF coinstaller to a directory on your test machine.

    **Note** You can obtain redistributable framework updates by downloading the Wdfcoinstaller.msi package from [WDK 8 Redistributable Components](http://go.microsoft.com/fwlink/p/?LinkID=226396). This package performs a silent install into the directory of your Windows Driver Kit (WDK) installation. You will see no confirmation that the installation has completed. You can verify that the redistributables have been installed on top of the WDK by ensuring there is a redist\\wdf directory under the root directory of the WDK, %ProgramFiles(x86)%\\Windows Kits\\8.0.

3.  Run the command "devcon.exe install SingleComponentFStateSample.inf root\\SingleComponentFStateDevice".

Use the PowerFxApp.exe application to send I/O requests to the driver. Running the command "PowerFxApp.exe /?" displays detailed usage information.

For detailed information about implementing F-state-based power management for a single component device, see [Supporting Multiple Functional Power States for Single-Component Devices](http://msdn.microsoft.com/en-us/library/windows/hardware/hh451032).

MultiComp Overview
------------------

This sample demonstrates how a KMDF driver can implement F-state-based power management for a device that has an arbitrary number of components that can be individually power-managed.

The sample driver statically links to a helper library (WdfPoFx.lib) that encapsulates all of the generic code to interact with the power framework. The device-specific code is implemented in the driver itself, outside of the helper library. The idea behind this organizing the code in this manner is make the helper library reusable by other drivers. The directory structure for the sample is as follows:

-   The helper library is implemented in the 'lib' subdirectory.
-   The interface between the helper library and the rest of the driver code is defined in the 'inc' subdirectory.
-   The driver is implemented in the 'driver' subdirectory.

Installation
------------

The driver can be installed on a root-enumerated device using the devcon.exe tool.

1.  Obtain the devcon.exe tool from the WDK
2.  Copy the driver binary, INF file and the KMDF coinstaller to a directory on your test machine.
    **Note** You can obtain the co-installers by downloading theWdfcoinstaller.msi package from [WDK 8 Redistributable Components](http://go.microsoft.com/fwlink/p/?LinkID=226396).
3.  Run the command "devcon.exe install WdfMultiComp.inf WDF\\WdfMultiComp".

Testing
-------

Use the PowerFxApp.exe application to send I/O requests to the driver. Running the command "PowerFxApp.exe /?" displays detailed usage information.

Design overview
---------------

The driver controls a device that has more than one component. It needs to access one of those components for processing each I/O request that it receives. The specific component that it needs to access depends on the I/O request that it receives.

In order to support this, the driver creates one top-level, power-managed queue to receive all its requests. It also creates one secondary, power-managed queue for each of its components. These secondary queues are called component queues.

When the driver's dispatch routine for the top-level queue is invoked, it examines the request to determine which component it needs to access in order to process the request. Then, it forwards the request to the component queue for the component that it needs to access for that request. When the driver's dispatch routine for the component queue is invoked, it accesses the component hardware to process the request.

The driver's top-level queue and component queues are all power-managed so KMDF ensures that the device is in D0 while the queues are in a dispatching state. The key point to note is that the driver is designed to maintain a component queue in a dispatching state only when the component is active. In order to achieve this, the driver stops the component queue when the component becomes idle and starts the component queue when the component becomes active. (To be precise, this mechanism of stopping and starting queues is encapsulated in the power framework helper library used by the driver). Thus, the driver is able to ensure that when the component queue is in a dispatching state, not only is the device in D0 but the component corresponding to that queue is also active. Thus it is safe to access the component hardware when the component queue's dispatch routine is invoked.

Implementation notes
--------------------

The driver uses the power framework helper library to manage most of its interactions with the power framework. In order to achieve this, during device initialization, the driver performs the following tasks in its [*EvtDriverDeviceAdd*](http://msdn.microsoft.com/en-us/library/windows/hardware/ff541693) callback.

-   Enables the helper library to register its own KMDF callbacks for PNP and power-management of the device.
-   Provides the helper library with power-framework-related information about the device.
-   Provides the helper library with information about the component queues.

During I/O request processing, the driver uses routines provided by helper library to forward requests to component queues and also to complete requests.

The main tasks performed by the power framework helper library on behalf of the driver are:

-   Registration and unregistration with the power framework.
-   Stopping component queues when the corresponding components become idle and starting them when the corresponding components become active.
-   Notifying the power framework when the device returns to its working state (D0) in response to the system returning from a low-power state to the working state (S0).

The power framework helper library does not have any hardware-specific information, so any tasks that are specific to the device's hardware are performed by the driver. In this sample, the device hardware is represented by a very simple simulation. The notable hardware-specific tasks in this sample are:

-   Accessing component hardware to process I/O requests.
-   Accessing component hardware to change the component's F-state.

As mentioned earlier, the hardware access shown is this sample is entirely simulated in software. This sample does not work with a real device, it installs on a root-enumerated software device.

S0-idle power management support
--------------------------------

The power framework helper library implements support for S0-idle power management for the device. Note that this is different from the component power management support that is enabled by the power framework. Component power management enables individual components of the device to be power-managed by putting them in different F-states while the device is in a working state (D0). S0-idle power management for the device enables the device as a whole to be power-managed by putting it into different D-states while the system is in a working state (S0).

The code to implement S0-idle power management support for the device is conditionally compiled based on the value of the PFH\_S0IDLE\_SUPPORTED compiler switch. If the switch is set to a nonzero value, the code to implement S0-idle power management support is included. If set to zero, the code is omitted thereby resulting in a smaller binary size. Thus, a driver that requires S0-idle power management for the device can use the power framework helper library's support for it but a driver that does not require it can reduce its binary size by omitting the code that is specific to S0-idle power management.

Additional Information
----------------------

For detailed information about implementing F-state-based power management for a multiple component device, see [Supporting Multiple Functional Power States for Multiple-Component Devices](http://msdn.microsoft.com/en-us/library/windows/hardware/hh451028).

