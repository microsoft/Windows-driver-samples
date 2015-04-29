Standard SD Host Controller Miniport
===================

This is a sample for a Secure Digital (SD) Host Controller miniport driver. The driver works in conjunction with sdport.sys, which implements SD/SDIO/eMMC protocol and WDM interfaces, to provide the host register interface.

## Universal Windows Driver Compliant
This sample builds a Universal Windows Driver. It uses only APIs and DDIs that are included in OneCoreUAP.

This driver, sdhc.sys, provides a functional miniport implementation for a standard SD host controller. However, it does not have support for many more recent features such as:

- UHS-I speed modes.
- HS400
- SD 4.0