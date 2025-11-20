---
page_type: sample
description: "Demonstrates how to use WIFICX for control flow and NetAdapterCx for data flow."
languages:
- cpp
products:
- windows
- windows-wdk
---

# WIFICX and NetAdapterCx Samples

This sample illustrates how to leverage **WIFICX** for control flow and **NetAdapterCx** for data flow. It supports both **KMDF** and **UMDF** drivers.

## How to Build
1. Mount the EWDK ISO from a local drive (network share paths are not supported).
2. Run `LaunchBuildEnv.cmd`.
3. In the environment created in step 2, type `SetupVSEnv` and press **Enter**.
4. Navigate to the current folder and open the solution file, for example:  
   `X:\Windows-driver-samples\network\wlan\WIFICX\wificxsampleclient.sln`
5. Build the solution from the Visual Studio UI.

## Interfaces and Abstraction
The sample interacts with three OS components:

- **WDF**  
  `driver.cpp`, `device.cpp`, and `adapter.cpp` demonstrate proper registration for PnP and power event callbacks. WDF manages system PnP and power requests (e.g., power IRPs), so IHV drivers should follow the flow triggered through these callbacks.

- **WDF Wi-Fi Class Extension (WIFICX)**  
  `wifixxxxx.cpp` files implement the **control path**, handling commands sent to Wi-Fi firmware (e.g., scan access points, connect, disconnect).

- **WDF NetAdapter Class Extension (NetAdapterCx)**  
  `netvxxxx.cpp` files implement the **data path**, managing network buffers and synchronizing with the control path for transfer start/stop operations.

## Data Buffers from Firmware
The control path uses hardcoded data since this sample does not target real hardware. The data path uses **Emulated Network Link (ENL)**, which connects two virtual network adapters directly. Packets sent over one adapter are delivered to the other and vice versa.

## Supported Scenarios
- [x] Scan access points and report BSS entries to the Windows UI.
- [x] Connect to an open access point from the Windows UI.
- [x] Disconnect from the connected access point.
- [ ] Transfer data between two Wi-Fi device instances (e.g., ping and throughput test).
