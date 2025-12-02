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

Illustrative example for a *Driver Device Transform* which loads in a process streaming an Avstream based camera device using Media Foundation.

A *Driver Device Transform* is a new kind of a transform that's used with a specific camera when capturing video. The *Driver Device Transform* is also known as DeviceMFT because it is the first Device Transform applied to the video source. This *Driver Device Transform* is an alternative to the Driver MFT, for example, MFT0  in that it caters to the source rather than the streams. An N stream source supporting DeviceMFT will have a single instance of the *Device Driver Transform* loaded, while MFT0 will have *N* instances for each pipeline process. The DeviceMFT can advertise multiple streams at the output, which can differ from the number of streams advertised by the source. This is analogous to having a user mode driver in the MF pipeline which intercepts and processes commands before they enter/ leave the Kernel mode driver loaded for the streaming source.

In this sample, the Device Transform, when enabled, will replicate a photo sequence in the user mode from a one pin device. It acts as a passthrough for sources exposing more than one pins.

This sample is designed to be used with a specific camera. To run the sample, you need the your camera's device ID and device metadata package.

## Related topics

### Concepts

[UWP device apps for cameras](https://docs.microsoft.com/windows-hardware/drivers/devapps/uwp-device-apps-for-webcams)

[Media Foundation Transforms](https://docs.microsoft.com/windows/win32/medfound/media-foundation-transforms)

[Streaming media device driver design guide](https://docs.microsoft.com/windows-hardware/drivers/stream)

[Universal camera driver design guide for Windows 10](https://docs.microsoft.com/windows-hardware/drivers/stream/windows-10-technical-preview-camera-drivers-design-guide)

### Samples

[Device app for camera sample](https://go.microsoft.com/fwlink/p/?linkid=249442)

[Camera Capture UI sample](https://go.microsoft.com/fwlink/p/?linkid=249441%20)
