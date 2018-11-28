<!---
    name: GPS/GNSS UMDF Sample Driver (UMDF Version 2)
    platform: UMDF2
    language: cpp
    category: GPS/GNSS
    description: Provides base sample driver that IHVs and partners can use to extend to build their custom Windows GPS/GNSS drivers
    samplefwlink: http://go.microsoft.com/fwlink/p/?LinkId=TBD
--->


GPS/GNSS UMDF Sample Driver (UMDF Version 2)
==============================================================================

# Overview

## Goals and Requirements

* Serves as base sample driver that IHVs and partners can use to extend to build their custom Windows GNSS drivers.
* Follows the WDF guidelines and best practices around PnP device arrival/removal, power management and driver installation/uninstallation.
* Supports the GNSS DDI mandatory requirements.
* Well-documented and keeps it easy for partners to identify the code that needs to be updated to build their driver
* This driver should pass HLK tests and WDF tests provided by Visual studio.

## Non-goals

* The sample does not support Geofence, SUPL, and AGNSS, as this is not mandatory GNSS DDI functionality.
* Not a production driver.

## Benefits

* Makes it easy for IHVS to write GNSS drivers for Windows devices
* Reduces the burden on Location dev team to support multiple IHVs writing GNSS drivers.

## What Partner needs to do?

* Customer can install WDK and run Location HLK tests to validate their driver. All test cases should be either pass or skip without failure. For test result of the sample driver, see Test Plan section below for detail.
* Partner owns the following: Installing the driver. Adding certificate and driver signing. Updating manufacturer name driver version etc. (the code has comments to update).
* For cloud-based scenario, they should update the code properly to adapter their scenario. This includes (not limited to): Do not use the NMEA parser. We disable this by default. Current Helper function of NMEA parser assumes that Serial interface as HW input source.
Define a way to inject a position for each session. In sample, it provides a sample function for how to fill out GNSS DDI data. The GNSS sample code has TODO comments. Partner should update them accordingly.


# Design

## Flow between GNSS driver/Location service

* We take WDF/UMDF 2.0 as baseline. Given that UMDF would be simpler to develop and it is enough to provide main functionality in Location. Only Windows 8.1 or beyond can run the UMDF 2.0 code.
* GNSS driver is provided by IHV in general. Our GNSS sample driver simply offers all the minimum functionality required by the DDI, e.g., it supports a single-shot position and continuous single-shot positions.
* LBS application built with Location API should work with the GNSS driver though Location service and WDF/UMDF 2.0. For example, 1st party apps such as Maps and Weather, and 3rd party app such as GPS Satellite works.
* The GNSS sample driver is built on WDF. This provides the basic functionality such as PnP device removal/arrival, power/hibernate, and driver install/uninstall.
* Test application such as HLK can run via WDF as well.
* Location service communicates the GNSS driver through GNSS driver IOCTLs.

## Design for Sample driver that gives fake location

* The plan is to build a sample software only driver that demonstrates WDF & GNSS DDI mandatory requirements that can be published to GitHub. And customer then builds a version on top of that sample driver. Each fake position should be written as GNSS DDI format by customer.
* This scenario is a baseline of this sample driver published in GitHub, even though the sample code can support another scenario below with simple modification.

## Design for Sample driver that uses real HW GPS

* The plan is to have GNSS driver sample that can be demonstrated to work with real GNSS device. The sample driver provides an example helper functions of using Serial port as incoming NMEA source. Hence, they can connect to USB GNSS device and see how it works between GPS device and App through Location service.
* To switch from fake location to real HW GPS scenario, customer simply calls a different function; rather than calling a function that reads a predefined set of Latitude and Longitude data of example, it calls a function that reads GPS incoming source from Serial port. The way to switch this is documented in the code as comments.
* The sample code provides an example of parsing NMEA messages of GPS only, i.e., US-based messages. But it is easily extensible to other types such as Beidou (China) and GLONASS (EU).
* The sample code assumes that GNSS/GPS device is recognized as COM port (Serial interface) in Device manager via PnP automatically, where NMEA 0183 format messages come out of it. Normally, USB adapter type is used to connect the GNSS/GPS. Other types of devices such as Bluetooth can be used as long as it is recognized as a COM port.
* Sample driver enumerates all the COM ports (0 – 255) in initialization and automatically picks up the first valid COM port.
* If there are more than one device recognized as COM interface in Device manager, the lowest COM port will be picked. Unfortunately, the order is not configurable in Windows Location service.
