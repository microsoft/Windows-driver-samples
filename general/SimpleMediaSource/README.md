---
page_type: sample
description: "Demonstrates how to write a custom media source and driver package."
languages:
- cpp
products:
- windows
- windows-wdk
---

# SimpleMediaSource sample

This sample demonstrates how to create a custom media source and driver package that can be installed as a camera and produce frames.

For more information, see the accompanying documentation at [Frame Server Custom Media Source](https://docs.microsoft.com/windows-hardware/drivers/stream/frame-server-custom-media-source).

## Contents

- MediaSource - COM DLL project for the custom media source
- SimpleMediaSourceDriver - UMDF driver install package

## Installation

1. Build the solution.

1. Navigate to the output folder, e.g. Windows-driver-samples\general\SimpleMediaSource\x64\Release, and the driver package will be in a directory also called SimpleMediaSourceDriver. Check that the folder has `SimpleMediaSource.dll`, `simplemediasourcedriver.cat`, `SimpleMediaSourceDriver.dll`, and `SimpleMediaSourceDriver.inf`.

1. Deploy the driver package with the following command:

    `devgen /add /bus ROOT /hardwareid root\SimpleMediaSource`
    `pnputil /add-driver SimpleMediaSourceDriver.inf /install`

1. Verify installation with Device Manager, locate **SimpleMediaSource Capture Source**, under the Camera category. If device does not appear there, check %windir%\inf\setupapi.dev.log for installation logs. The device instance ID will be available as the output of the devgen command and can be used as input to pnputil to determine status of the device.

    `pnputil /enum-devices /instanceid "<InstanceID of SimpleMediaSource>" /deviceids /services /stack /drivers`

1. Open the Microsoft Camera App, switch cameras if necessary until the camera is streaming from the SimpleMediaSource. You should see a scrolling black and white gradient.
