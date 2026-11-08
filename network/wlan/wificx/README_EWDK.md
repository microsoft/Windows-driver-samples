# Building and testing WiFiCx sample on Azure VM (EWDK)

Step-by-step guide to build, sign, and install the WiFiCx sample driver
using the Enterprise WDK (EWDK) on an Azure VM. No Visual Studio required.

## Azure VM setup

Create a VM with:

| Setting   | Value                                      |
|-----------|--------------------------------------------|
| Size      | Standard D2s v3 (2 vcpus, 8 GiB memory)   |
| Region    | Any (tested with South India)              |
| Image     | Windows Server 2025 Datacenter             |

RDP into the VM and open an elevated Command Prompt for the steps below.

## Prerequisites

### 1. Install EWDK

Download the EWDK ISO from
<https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk>
and mount it. Launch the build environment:

```cmd
E:\LaunchBuildEnv.cmd
```

### 2. Install NuGet and PowerShell (one-time)

```cmd
winget install Microsoft.NuGet
winget install Microsoft.PowerShell
```

Close and reopen the EWDK terminal so the new tools are on PATH.

### 3. Clone the repository

```cmd
git clone --recurse-submodules https://github.com/microsoft/Windows-driver-samples.git
cd Windows-driver-samples
```

If you already cloned without `--recurse-submodules`:

```cmd
git submodule update --init
```

### 4. Restore NuGet packages

```cmd
nuget restore -PackagesDirectory .\packages
```

### 5. Enable test signing (requires reboot)

```cmd
bcdedit /set testsigning on
shutdown /r /t 0
```

After reboot, re-mount the EWDK ISO and launch the build environment again.

## Build, sign, and install

From the EWDK terminal, launch PowerShell and run the deploy script:

```cmd
pwsh
```

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\network\wlan\wificx\deploy.ps1
```

The script will:
1. Build both WiFiCx and NetAdapterCx solutions
2. Create or find a test certificate
3. Run StampInf, Inf2Cat, and signtool
4. Remove any stale WiFiCx devnodes, install two instances, and pair them
   deterministically — MACLastByte 1 and 2 — with no manual step or reboot

### Common options

```powershell
# Skip rebuild (use existing build output)
.\network\wlan\wificx\deploy.ps1 -SkipBuild

# Release build
.\network\wlan\wificx\deploy.ps1 -Config Release

# Install only (already built and signed)
.\network\wlan\wificx\deploy.ps1 -InstallOnly

# Uninstall all sample drivers
.\network\wlan\wificx\deploy.ps1 -Uninstall
```

## Test: connect and ping

`run.ps1` automates the full data-path test: it discovers the two paired
adapters, creates an open WLAN profile, connects both, assigns static IPs,
seeds static neighbor entries, and pings in both directions.

```powershell
.\network\wlan\wificx\run.ps1
```

A passing run reports `PASS: Bidirectional ping succeeded`.

### Manual steps (alternative)

```cmd
netsh wlan show networks
netsh wlan connect name="WFC_OPEN" interface="Wi-Fi"
netsh wlan connect name="WFC_OPEN" interface="Wi-Fi 2"
```

Both adapters share one host and subnet, so seed static neighbor entries
before pinging — otherwise each stack treats the peer IP as a local
duplicate and never answers its ARP:

```cmd
netsh interface ip set address "Wi-Fi"   static 192.168.100.1 255.255.255.0
netsh interface ip set address "Wi-Fi 2" static 192.168.100.2 255.255.255.0
netsh interface ipv4 add neighbors "Wi-Fi"   192.168.100.2 22-22-22-22-00-02
netsh interface ipv4 add neighbors "Wi-Fi 2" 192.168.100.1 22-22-22-22-00-01
ping 192.168.100.2 -S 192.168.100.1
```

## Architecture

This is a pure software simulation — no Wi-Fi hardware required.

```
WiFiCx Instance 1 (MACLastByte=1)  <--ENL-->  WiFiCx Instance 2 (MACLastByte=2)
    |                                              |
    +-- WifiCx control path (scan/connect/auth)    +-- WifiCx control path
    |                                              |
    +-- NetAdapter Tx Queue ---- ENL ---- Rx Queue +
    +-- NetAdapter Rx Queue ---- ENL ---- Tx Queue +
```

- **Control path**: Hardcoded scan results, simulated WPA2/WPA3-SAE
  authentication, fake link quality (RSSI -50, 30 Mbps).
- **Data path**: Two virtual adapters connected by the Emulated Network
  Link (ENL), a software packet forwarding engine in
  `netadaptercx/netvadapterlibrary/`.
- **AP relay**: Both stations associate (in software) to the same fake AP
  BSSID. With no real AP to forward between them, the ENL rewrites each
  forwarded 802.11 frame from "to-DS" to "from-DS" — as an AP would — so the
  receiving station accepts it.
- **No hardware**: Both drivers are root-enumerated virtual devices.

## Troubleshooting

| Problem | Fix |
|---------|-----|
| `wil/resource.h` not found | `git submodule update --init` |
| NuGet packages missing | `nuget restore -PackagesDirectory .\packages` |
| Driver install fails | Verify `bcdedit /enum {current}` shows `testsigning Yes` |
| Inf2Cat fails on netvadapter | The deploy script falls back to `New-FileCatalog` automatically |
| `pnputil` not found | It is built into Windows — should always be on PATH |
| Only one adapter, no `…00-02` | Re-run `deploy.ps1`; it clears stale devnodes and re-pairs |
| Ping fails with "Destination host unreachable" | Seed static neighbor entries (see manual steps); `run.ps1` does this automatically |
