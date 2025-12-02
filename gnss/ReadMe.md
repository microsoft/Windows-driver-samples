---
page_type: sample
description: "Provides base sample driver that IHVs and partners can use to extend to build their custom Windows GPS/GNSS drivers."
languages:
- cpp
products:
- windows
- windows-wdk
---

# GNSS UMDF Sample Driver (UMDF Version 2)

Provides a base sample driver that IHVs and partners can use to extend to build their custom Windows GPS/GNSS drivers.

## Overview

### What is covered

- This is a sample that adheres to GNSS driver design for Windows 10 outlined in the [GNSS driver design guide for Windows 10](https://docs.microsoft.com/windows-hardware/drivers/gnss/gnss-driver-design-guide-for-windows-10).

- Serves as base sample driver that IHVs and partners can use as a template and guidance to extend to build their custom Windows GNSS drivers.

- Follows the WDF guidelines and best practices around PnP device arrival/removal, power management and driver installation/uninstallation.

- Supports the GNSS DDI mandatory requirements.

- This driver successfully passes HLK tests and WDF tests provided by Visual studio.

### What is not covered

- This sample currently doesn't support Geofence, SUPL, and AGNSS, as this is not mandatory GNSS DDI functionality.

- SUPL and AGNSS are mandatory only if required by mobile operator, and the sample does not support them currently.

- Not a production driver.

### What a Partner needs to do

- Customer can install WDK and run Location HLK tests to validate their driver. All test cases should be either pass or skip without failure.

- Partner owns the following:

  - Installing the driver.

  - Adding certificate and driver signing.

  - Updating manufacturer name driver version etc. (the code has comments to update).

- The GNSS sample code has "FIX ME" comments. Partner should update them accordingly.

## Design

- The sample demonstrates a very basic software only driver that meets the minimum requirements from the GNSS DDI.

- The sample currently always returns a fake hardcoded location. Driver developers can extend this to fetch fake positions from a file or inject through a custom IOCTL or instead implement getting real positions from GNSS hardware.

- Driver developer can extend this to pull the fake positions from a file or through custom IOCTLs if needed.

## Trace Logging

- The sample code provides Trace logging WPRP file.

- To capture trace log, copy GnssUmdfSampleDriver.wprp in the solution folder into the device under test. On command line (MyLog.etl is an example file name below):

    ```cmd
    wpr -start GnssUmdfSampleDriver.wprp -filemode
        <Reproduce the issue>
    wpr -stop MyLog.etl
    ```

- Inspect MyLog.etl with trace file analyzer. Note that corresponding PDB file is needed to decode the logging information.

## Test Plan

Customer should validate their driver after customizing it.

### Test method 1

- This tests WDF fundamental functionality (PnP etc.), rather than Location-specific features.

- Run Visual studio built-in run-time test for WDF.

- General information available at [](https://docs.microsoft.com/windows-hardware/drivers/develop/testing-a-driver-at-runtime).

- Expected result / Pass criteria: Pass rate 100%

### Test method 2

- This tests Location-specific functionality (Getting fix etc.).

- Run [HLK](https://docs.microsoft.com/windows-hardware/test/hlk/windows-hardware-lab-kit) tests with existing and standard GNSS driver tests.

- Run "TE.exe GNSSDriverTest.dll".

- Expected result / Pass criteria: Pass rate 100%

- Note that this sample does not support AGNSS, SUPL and Geofencing currently. Based on the capability of driver, HLK skips them automatically.

### Test method 3

- Test the [Windows Geolocation API](https://docs.microsoft.com/windows/desktop/locationapi/windows-location-api-portal) layer on real apps.

- Get current location by clicking on "Show my location" from the In-box Windows Maps app.

- Run the tracking scenario from the [Geolocation Sample app](https://github.com/Microsoft/Windows-universal-samples/tree/master/Samples/Geolocation).

- Get current location by clicking on "Start" from GPS Satellite app in Windows Store.

## Resources

[GNSS DDI reference](https://docs.microsoft.com/windows-hardware/drivers/ddi/content/gnssdriver/index)

[Getting started with Windows drivers](https://docs.microsoft.com/windows-hardware/drivers/gettingstarted)

[Windows Driver Kit documentation](https://docs.microsoft.com/windows-hardware/drivers)

[NMEA format documentation](https://navspark.mybigcommerce.com/content/NMEA_Format_v0.1.pdf)
