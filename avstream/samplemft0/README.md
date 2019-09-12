---
page_type: sample
description: "A driver MFT for use with a camera's Windows device app."
languages:
- cpp
products:
- windows
- windows-wdk
---

# Driver MFT Sample

Provides a *driver MFT* for use with a camera's UWP device app.A *driver MFT* is a Media Foundation Transform that's used with a specific camera when capturing video. The driver MFT is also known as MFT0 because it is the first MFT applied to the video stream captured from the camera. This MFT can provide a video effect or other processing when capturing photos or video from the camera. It can be distributed along with the driver package for a camera.

In this sample, the driver MFT, when enabled, replaces a portion of the captured video with a green box. To test this sample, download the [UWP device app for camera sample](http://go.microsoft.com/fwlink/p/?linkid=249442) and the [Camera Capture UI sample](http://go.microsoft.com/fwlink/p/?linkid=249441).

The **UWP device app for camera sample** provides a *UWP device app* that controls the effect implemented by the driver MFT.

The **Camera Capture UI sample** provides a way to invoke the **UWP device app**.

This sample is designed to be used with a specific camera. To run the sample, you need the your camera's device ID and device metadata package.

## Related topics

### Concepts

[UWP device apps for cameras](https://docs.microsoft.com/windows-hardware/drivers/devapps/uwp-device-apps-for-webcams)

[Media Foundation Transforms](https://docs.microsoft.com/windows/win32/medfound/media-foundation-transforms)

[Streaming media device driver design guide](https://docs.microsoft.com/windows-hardware/drivers/stream)

[Universal camera driver design guide for Windows 10](https://docs.microsoft.com/windows-hardware/drivers/stream/windows-10-technical-preview-camera-drivers-design-guide)

### Samples

[Device app for camera sample](http://go.microsoft.com/fwlink/p/?linkid=249442)

[Camera Capture UI sample](http://go.microsoft.com/fwlink/p/?linkid=249441%20)
