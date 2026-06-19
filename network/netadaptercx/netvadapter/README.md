---
page_type: sample
description: "A virtual NIC (NetAdapterCx) miniport driver built on top of netvadapterlibrary, in both KMDF and UMDF flavors."
languages:
- cpp
products:
- windows
- windows-wdk
---

# netvadapter — Virtual NIC Sample

`netvadapter` is a **virtual Ethernet NIC** driver that uses **NetAdapterCx** for its data
path. It links against the shared **netvadapterlibrary** static library (the same library used
by the WIFICX sample) and ships in both **KMDF** and **UMDF** flavors.

Two `netvadapter` instances can be paired together through the library's **Emulated Network
Link (ENL)**: packets sent on one adapter are delivered to the other and vice‑versa, so you can
run connectivity tests (ping, throughput) entirely in software with no physical hardware.

---

## Layout

| Path | Description |
| --- | --- |
| `netvadapter.sln` | Solution containing both drivers + the library project references. |
| `build.cmd` | Build wrapper (see **Building** — pins UMDF to 2.33). |
| `km\netvadapterkm.vcxproj` | KMDF driver → `netvadapter.sys` (INF: `km\netvadapter.inf`). |
| `um\netvadapterum.vcxproj` | UMDF driver → `netvadapterum.dll` (INF: `um\netvadapterum.inf`). |
| `drivercode\` | Shared driver source (see below). |

## Building

The UMDF driver targets **UMDF 2.33** (to match `netvadapterum.inf`'s
`UmdfLibraryVersion = 2.33.0`), while the shared `netvadapterlibrary` is checked in at **UMDF
2.35** (so the WIFICX sample is unaffected). Because the WDF version must match within a single
binary, **build through `build.cmd`**, which forces the library and driver to 2.33 for this
solution only:

```cmd
build.cmd                 :: Debug   x64  (defaults)
build.cmd Release x64
```

> A plain `msbuild netvadapter.sln` or a Visual Studio IDE build will fail to link with an
> unresolved `WdfFunctions_02035` symbol, because it does not apply the 2.33 override.

### Signing the package (test signing)

The build signs the binaries but does **not** produce a catalog. To create and test‑sign the
catalogs (required to install while in test‑signing mode):

```cmd
:: From an EWDK environment, for each package folder under x64\<Config>\:
Inf2Cat /driver:.\netvadapterkm  /os:10_X64,Server10_X64
Inf2Cat /driver:.\netvadapterum  /os:10_X64,Server10_X64

signtool sign /fd SHA256 /sha1 <WDKTestCertThumbprint> /tr http://timestamp.digicert.com /td SHA256 .\netvadapterkm\netvadapter.cat
signtool sign /fd SHA256 /sha1 <WDKTestCertThumbprint> /tr http://timestamp.digicert.com /td SHA256 .\netvadapterum\netvadapterum.cat
```

---

## Installing

Enable test signing once (elevated, then reboot), and install the test certificate into the
**Root** and **TrustedPublisher** stores on the target machine. Then install the driver with
`devcon`:

```cmd
.\devcon.exe install .\netvadapterum.inf root\netvadapterum
```

If the command fails, inspect the device‑install log at:

```
C:\Windows\INF\setupapi.dev.log
```

### Pairing two adapters

A working link requires **two** `netvadapter` instances. Create both by running the install
command **twice**:

```cmd
.\devcon.exe install .\netvadapterum.inf root\netvadapterum
.\devcon.exe install .\netvadapterum.inf root\netvadapterum
```

Then set the **`MACLastByte`** advanced keyword to 1 and 2 on both adapters via Device Manager → adapter → **Advanced**, or the network adapter
property pages.

> ⚠️ `MACLastByte` defaults to **0**. If an adapter is left at the default, the device fails
> to start and shows a **yellow bang** (error) in Device Manager. Assign distinct values such as
> `1` and `2` for the pair to link correctly.

---

## Managing the driver

List installed `net`‑class drivers (to find the `oemNN.inf` published name):

```cmd
pnputil /enum-drivers /class net
```

Remove the driver and prepare for a clean reinstall (substitute the `oemNN.inf` from the command
above):

```cmd
pnputil /delete-driver oem2.inf /uninstall /force
```
