Kernel mode display-only miniport driver (KMDOD) sample
=======================================================

The kernel mode display-only miniport driver (KMDOD) sample implements most of the device driver interfaces (DDIs) that a display-only miniport driver should provide to the Windows Display Driver Model (WDDM). The code is useful to understand how to write a miniport driver for a display-only device, or how to develop a full WDDM driver.

For more info on how a KMDOD works, see [Kernel Mode Display-Only Driver (KMDOD) Interface](http://msdn.microsoft.com/en-us/library/windows/hardware/jj673962). For more info on WDDM drivers, see [Windows Display Driver Model (WDDM) Design Guide](http://msdn.microsoft.com/en-us/library/windows/hardware/ff570593).

This code can also help you to understand the use and implementation of display-related DDIs. The INF file shows how to make a display miniport driver visible to other WDDM components.

The sample can be installed on top of a VESA-capable graphics adapter, or on top of a graphics device that supports access to frame buffer memory through the Unified Extensible Firmware Interface (UEFI).

The sample driver does not support the *sleep* power state. If it is placed in the sleep state, the driver will cause a system bugcheck to occur. There is no workaround available, by design.

If the current display driver is not a WDDM 1.2 compliant driver, the sample driver might fail to install, with error code 43 displayed. The KMDOD driver is actually installed, but it cannot be started. The workaround for this issue is to switch to the Microsoft Basic Display Adapter Driver before installing the KMDOD sample driver, or simply to reboot your system after installing the KMDOD sample.


Installation
------------

In Microsoft Visual Studio, press **F5** to build the sample and then deploy it to a target machine. For more info, see [Deploying a Driver to a Test Computer](http://msdn.microsoft.com/en-us/library/windows/hardware/hh454834).

In some cases you might need to install the driver manually, as follows.

1.  Add the following files to the directory given by ...\\[x64]\\C++\\Package:
    -   SampleDriver.cat
    -   SampleDriver.inf
    -   SampleDriver.sys
    -   SampleDriver.cer

2.  Unless you've provided a production certificate, you should manually install the SampleDriver.cer digital certificate with the following command:

    `Certutil.exe -addstore root SampleDriver.cer`

3.  Then enable test signing by running the following BCDEdit command:

    `Bcdedit.exe -set TESTSIGNING ON`

    **Note** After you change the TESTSIGNING boot configuration option, restart the computer for the change to take effect.

    For more info, see [The TESTSIGNING Boot Configuration Option](http://msdn.microsoft.com/en-us/library/windows/hardware/ff553484).

4.  Manually install the driver using Device Manager, which is available from Control Panel.

### ACPI-based GPUs

To install the KMDOD sample driver on a GPU that is an Advanced Configuration and Power Interface (ACPI) device, add these lines to the `[MS]`, `[MS.NTamd64]`, and `[MS.NTarm]` sections of the Sampledisplay.inf file:

```
Text
" Kernel mode display only sample driver " = KDODSamp_Inst, ACPI\CLS_0003&SUBCLS_0000
" Kernel mode display only sample driver " = KDODSamp_Inst, ACPI\CLS_0003&SUBCLS_0001
" Kernel mode display only sample driver " = KDODSamp_Inst, ACPI\CLS_0003&SUBCLS_0003
```

This new code provides generic identifiers for ACPI hardware.

You can optionally delete the original lines of code within these sections of the INF file.

