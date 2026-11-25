---
page_type: sample
description: "A driver device transform which loads in a process streaming an Avstream based camera device using Media Foundation."
languages:
- cpp
products:
- windows
- windows-wdk
---

# Driver Device Transform Sample

Illustrative example for a *Driver Device Transform*, (abbreviation: DMFT) which loads in a process streaming an Avstream based camera device using Media Foundation.

A *Driver Device Transform* is a new kind of a transform that's used with a specific camera when capturing video. The *Driver Device Transform* is also known as DeviceMFT because it is the first Device Transform applied to the video source. This *Driver Device Transform* is an alternative to the Driver MFT, for example, MFT0  in that it caters to the source rather than the streams. An N stream source supporting DeviceMFT will have a single instance of the *Device Driver Transform* loaded, while MFT0 will have *N* instances for each pipeline process. The DeviceMFT can advertise multiple streams at the output, which can differ from the number of streams advertised by the source. This is analogous to having a user mode driver in the MF pipeline which intercepts and processes commands before they enter/ leave the Kernel mode driver loaded for the streaming source.

This sample is designed to be used with a specific camera. To run the sample, you need the your camera's device ID and device metadata package.

DeviceMFT will get loaded in Frameserver, and not in the context of the streaming application. Frameserver is a on demand service, which is loaded only when the application tries to stream the camera. Frameserver makes it possible for multiple clients to stream the same camera device.  
**Note:** Debugging the Frameserver is outside the scope of this document.

This sample has a dependency on Windows Implementation library, which can be installed in Visual studio or VSCode using Nuget or vcpkg.

# Installation
Please build the sample according to the architecture of the platform on test. Most cameras are registered under the categories 
  - KSCATEGORY_VIDEO_CAMERA
  - KSCATEGORY_CAPTURE
  - KSCATEGORY_VIDEO

The GUID under which the sample needs to be registered is defined in `Common.h` as<br>
    *{0E313280-3169-4F41-A329-9E854169634F}*<br>
This value needs to be copied as a REG_SZ under the above mentioned device interfaces of the selected device<br>
E.g. please add the following key value pair under a registry key which will look like, below

_HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\DeviceClasses\{e5323777-f976-4f5b-9b55-b94699c46e44}\*.*#{e5323777-f976-4f5b-9b55-b94699c46e44}\#GLOBAL\Device Parameters_

**key:CameraDeviceMftClsid value:{0E313280-3169-4F41-A329-9E854169634F}**


## Related topics

### Concepts

[UWP device apps for cameras](https://docs.microsoft.com/windows-hardware/drivers/devapps/uwp-device-apps-for-webcams)

[Media Foundation Transforms](https://docs.microsoft.com/windows/win32/medfound/media-foundation-transforms)

[Streaming media device driver design guide](https://docs.microsoft.com/windows-hardware/drivers/stream)

[Universal camera driver design guide for Windows 10](https://docs.microsoft.com/windows-hardware/drivers/stream/windows-10-technical-preview-camera-drivers-design-guide)

[Windows Implementation Library](https://github.com/microsoft/wil)

### Samples

[Device app for camera sample](https://go.microsoft.com/fwlink/p/?linkid=249442)

[Camera Capture UI sample](https://go.microsoft.com/fwlink/p/?linkid=249441%20)
