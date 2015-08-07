Driver Device Transform Sample
==============================
Illustrative example for a *Driver Device Transform* which loads in a process streaming an Avstream based camera device using Media Foundation. 

A *Driver Device Transform* is a new kind of a transform that's used with a specific camera when capturing video. The *Driver Device Transform* is also known as DeviceMFT because it is the first Device Transform applied to the video source. This *Driver Device Transform* is an alternative to the Driver MFT i.e. MFT0  in that, it caters to the source rather than the streams . An N stream source supporting DeviceMFT will have a single instance of the *Device Driver Transform* loaded, while MFT0 will have *N* instances for each pipeline process. The DeviceMFT can advertise multiple streams at the output, which can differ from the number of streams advertised by the source. This is analogous to having a user mode driver in the MF pipeline which intercepts and processes commands before they enter/ leave the Kernel mode driver loaded for the streaming source.

In this sample, the Device Transform , when enabled, will replicate a photo sequence in the user mode from a one pin device . It acts as a passthrough for sources exposing more than one pins. 

This sample is designed to be used with a specific camera. To run the sample, you need the your camera's device ID and device metadata package.


Related topics
--------------

**Concepts**

[Windows Store device apps for cameras](http://go.microsoft.com/fwlink/p/?LinkId=306683)

[Media Foundation Transforms](http://msdn.microsoft.com/en-us/library/windows/hardware/ms703138)

[Roadmap for Developing Streaming Media Drivers](http://msdn.microsoft.com/en-us/library/windows/hardware/ff568130)

[Universal camera driver design guide for Windows 10](https://msdn.microsoft.com/en-us/Library/Windows/Hardware/dn937080)

**Samples**

[Windows Store device app for camera sample](http://go.microsoft.com/fwlink/p/?linkid=249442)

[Camera Capture UI sample](http://go.microsoft.com/fwlink/p/?linkid=249441%20)