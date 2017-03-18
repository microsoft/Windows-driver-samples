AvsCamera: AVStream Camera Sample Driver 
========================================
The AvsCamera sample provides a pin-centric AVStream capture driver for a simulated front and back camera. The driver performs simulated captures at 320x240 or 640x480 in RGB24, RGB32, YUY2 and NV12 formats at various frame rates. The purpose of the sample is to demonstrate how to write a fully functional AVStream camera driver. 

This sample features strong parameter validation and overflow detection.  It provides validation and simulation logic for all advanced camera controls in the CCaptureFilter class.  A real camera driver would replace the filter automation table and CSensor and CSynthesizer class hierarchies to produce a new camera driver.

The sample comes with its own MFT0 called AvsCameraMft0.dll.  This MFT0 is used to parse metadata supplied in the AvsCamera driver samples.  The metadata communications from the driver is primarily a private channel to its MFT0.  The MFT0 is responsible for reformatting that information for the capture pipeline.

## Universal Windows Driver Compliant
This sample builds a Universal Windows Driver. It uses only APIs and DDIs that are included in OneCoreUAP.  The sample works on 32-bit and 64-bit x86, amd64 and arm platforms.  Once installed, the simulated camera should show in the Windows Inbox camera app.

## Building the sample
The AvsCamera sample can be built by opening the AvsCamera.sln solution file. A successful build produces AvsCamera.sys, AvsCameraMft0.dll, AvsCamera.inf and AvsCamera.cat. 
