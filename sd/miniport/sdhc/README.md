---
page_type: sample
description: "Provides a functional miniport implementation for a standard SD host controller."
languages:
- cpp
products:
- windows
- windows-wdk
---


<!---
    name: Standard SD Host Controller Miniport
    platform: WDM
    language: cpp
    category: Storage
    description: Provides a functional miniport implementation for a standard SD host controller.
    samplefwlink: http://go.microsoft.com/fwlink/p/?LinkId=617952
--->

# Standard SD Host Controller Miniport

This is a sample for a Secure Digital (SD) Host Controller miniport driver. The driver works in conjunction with sdport.sys, which implements SD/SDIO/eMMC protocol and WDM interfaces, to provide the host register interface.

## Universal Compliant

This sample builds a Windows Universal driver. It uses only APIs and DDIs that are included in Windows Core.

This driver, sdhc.sys, provides a functional miniport implementation for a standard SD host controller. However, it does not have support for many more recent features such as:

- UHS-I speed modes.
- HS400
- SD 4.0