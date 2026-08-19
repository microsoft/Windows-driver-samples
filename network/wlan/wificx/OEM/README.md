# OEM Device Service Sample (`OemDeviceServiceApplication`)

A user-mode console application that demonstrates how an OEM/IHV utility communicates
with the WiFiCx sample driver through a **WLAN device service**. The app sends the
request string `"Hello, My Driver"` and prints the driver's reply
`"Nice to meet you, My OEM"`.

## What it does

The tool walks through the standard WLAN client flow:

1. **`WlanOpenHandle`** — opens a client handle using `WLAN_API_VERSION_2_0`.
2. **`WlanEnumInterfaces`** — enumerates all WLAN interfaces on the machine.
3. **`WlanGetSupportedDeviceServices`** — for each interface, enumerates the device
   service GUIDs the driver advertises and checks whether the OEM sample service
   (`GUID_OEM_SAMPLE_DEVICE_SERVICE`) is present.
4. **`WlanDeviceServiceCommand`** — when the service is advertised, sends the
   `OEM_DEVICE_SERVICE_OPCODE_HELLO` opcode with the request payload and prints the
   bytes returned by the driver.
5. **`WlanFreeMemory` / `WlanCloseHandle`** — releases the interface list and the
   client handle.

If the OEM sample device service is not advertised on an interface, the command is
skipped for that interface.

## Device service contract

These values **must stay in sync** with the driver-side definitions in
[`drivercode/SharedTypes.h`](../drivercode/SharedTypes.h):

| Item | Value |
| --- | --- |
| Service GUID | `{2d6f9a14-3a1d-4f0a-9b7e-1c2e3a4b5c6d}` (`GUID_OEM_SAMPLE_DEVICE_SERVICE`) |
| Opcode | `0x00000001` (`OEM_DEVICE_SERVICE_OPCODE_HELLO`) |
| Request string | `"Hello, My Driver"` |
| Response string | `"Nice to meet you, My OEM"` |

The driver advertises the service GUID via `WDI_GET_SUPPORTED_DEVICE_SERVICES`, so the
GUID must match on both sides for the exchange to succeed.

## Source layout

| File | Purpose |
| --- | --- |
| [`OemDeviceServiceApp.cpp`](OemDeviceServiceApp.cpp) | Application entry point and device service logic. |
| [`OemDeviceService.vcxproj`](OemDeviceService.vcxproj) | MSBuild project for the console app. |

## Build

The project (`OemDeviceService.vcxproj`) builds as a console **Application** using the
`WindowsApplicationForDrivers10.0` platform toolset.

- **Configurations:** `Debug`, `Release`
- **Platforms:** `x64`, `ARM64`
- **Linked libraries:** `wlanapi.lib`, `ole32.lib`

Build it from Visual Studio as part of the solution, or from the command line:

```cmd
msbuild OEM\OemDeviceService.vcxproj /p:Configuration=Release /p:Platform=x64
```

## Run

Run the resulting executable from an elevated command prompt on a machine where the
WiFiCx sample driver is installed:

```cmd
OemDeviceServiceApplication.exe
```

### Example output

```text
Found 1 WLAN interface(s).

Interface[0]: WiFiCx Sample Client Device
Supported device services: 1
  [0] {2D6F9A14-3A1D-4F0A-9B7E-1C2E3A4B5C6D}  <-- OEM sample service
Sending device service command: "Hello, My Driver"
Driver responded: "Nice to meet you, My OEM" (25 bytes)
```

## Requirements

- The WiFiCx sample driver must be installed and the adapter present.
- The driver must advertise `GUID_OEM_SAMPLE_DEVICE_SERVICE`; otherwise the app prints
  that the service is not advertised and skips the command.
- WLAN API (`wlanapi.lib`) and COM (`ole32.lib` for `StringFromGUID2`) are available on
  the host.
