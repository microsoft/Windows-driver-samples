Early Launch Anti-Malware Driver
================================

This sample demonstrates how to use the [**IoRegisterBootDriverCallback**](http://msdn.microsoft.com/en-us/library/windows/hardware/hh439379) and [**IoUnRegisterBootDriverCallback**](http://msdn.microsoft.com/en-us/library/windows/hardware/hh439394) DDIs from an Early Launch Anti-Malware driver, to receive notifications about the initialization of regular boot start drivers.

This sample driver is a minimal driver meant to demonstrate the usage of the APIs mentioned above. It is not intended for use in a production environment.

**SIGNING THE SAMPLE**

Early Launch drivers are required to be signed with a code-signing certificate that also contains the Early Launch EKU "1.3.6.1.4.1.311.61.4.1". In a production environment, Early Launch drivers are signed by Microsoft for qualifying Anti-Malware vendors with a WHQL certificate that contains this EKU. The makecert.exe tool can be used to generate a self-signed test certificate that contains both the Early Launch EKU and the "1.3.6.1.5.5.7.3.3" Code Signing EKU. Once a certificate of this form has been created, signtool.exe can be used to sign elamsample.sys.


Run the sample
--------------

**INSTALLING THE SAMPLE**

1. Copy the signed elamsample.sys file to the %WINDIR%\\System32\\Drivers directory on your test machine.

2. Use the sc.exe tool present in Windows to install the driver:

  `sc create ElamSample binpath=%windir%\\system32\\drivers\\elamsample.sys type=kernel start=boot error=critical group=Early-Launch`
 
3. Enable test signing:

  `bcdedit /set testsigning on`

**CODE TOUR**

**DriverEntry:** Creates a framework driver object and calls IoRegisterBootDriverCallback to register to boot driver status callbacks.

**ElamSampleEvtDriverUnload:** Calls IoUnregisterBootDriverCallback to unregister for callbacks when elamsample.sys is about to be unloaded.

**ElamSampleBootDriverCallback**: Dispatches to other functions to process the specific callback types.

**ElamSampleProcessStatusUpdate:** Displays callback BdCbStatusUpdate information, such as when dependencies and drivers are about to be initialized, or when the ELAM driver is about to be unload.

**ElamSampleProcessInitializeImage:** Displays callback BdCbInitializeImage information, such as the driver image name and the name of the entity that signed the driver.

**ElamSamplePrintHex:** A utility function to display a buffer in hexadecimal form.

**TESTING**

After installing the driver, attach the Kernel Debugger and reboot your test machine. If ELAMSAMPLE\_TRACE\_LEVEL is set to DPFLTR\_ERROR\_LEVEL, traces will be output to the debugger automatically. For example:

```
ElamSample is being initialized.

ElamSample reports the following dependency is about to be initialized: ElamSample:

Image name "\\FileSystem\\RAW"

ElamSample: Not signed.

ElamSample reports that Boot Start driver dependencies are being initialized.

ElamSample reports the following dependency is about to be initialized:

ElamSample: Image name "\\SystemRoot\\system32\\PSHED.dll"

ElamSample: Image hash algorithm = 0x0000800c.

ElamSample: Image hash:

ElamSample: 21 29 88 ca 88 ab dc 0f c3 f1 c0 74 df e0 29 58

ElamSample: 2e cd 41 5e 56 bd 77 53 39 9b d9 d7 f4 47 65 d8

ElamSample: Image is signed by "Microsoft Windows".

ElamSample: Certificate issued by "MSIT Test CodeSign CA 3".

ElamSample: Certificate thumb print algorithm = 0x0000800c.

ElamSample: Certificate thumb print:

ElamSample: 93 29 d5 f2 e2 7a c9 79 41 b2 6d c0 78 35 2a d3

ElamSample: da 2d 7e 72 f0 05 5f 8b 63 8c 7b a2 6b 37 5c 4f

ElamSample reports that Boot Start drivers are about to be initialized.

ElamSample reports the following Boot Start driver is about to be initialized:

ElamSample: Image name "\\SystemRoot\\System32\\drivers\\rdyboost.sys"

ElamSample: Registry path "\\Registry\\Machine\\System\\CurrentControlSet\\Services\\rdyboost"

ElamSample: Image hash algorithm = 0x0000800c.

ElamSample: Image hash:

ElamSample: 9e 91 b2 e1 29 97 af e9 ac 6c 48 24 01 43 c8 b4

ElamSample: f6 81 bf 57 df 80 0b 05 4d 58 bb e6 d9 83 a9 08

ElamSample: Image is signed by "Microsoft Windows".

ElamSample: Certificate issued by "MSIT Test CodeSign CA 3".

ElamSample: Certificate thumb print algorithm = 0x0000800c.

ElamSample: Certificate thumb print:

ElamSample: 93 29 d5 f2 e2 7a c9 79 41 b2 6d c0 78 35 2a d3

ElamSample: da 2d 7e 72 f0 05 5f 8b 63 8c 7b a2 6b 37 5c 4f

ElamSample reports that all Boot Start drivers have been initialized and that ElamSample is about to be unloaded ElamSample is being unloaded.
```
