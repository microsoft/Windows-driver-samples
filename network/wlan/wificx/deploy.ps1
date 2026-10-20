#Requires -RunAsAdministrator
<#
.SYNOPSIS
    Build, sign, and install the WiFiCx + NetAdapterCx virtual adapter sample drivers.
.DESCRIPTION
    Run from an elevated EWDK PowerShell session. Can be invoked from any directory.
    Targets Windows Server 2025 / Windows 11.

    Prerequisites (run once before this script):
      git submodule update --init
      nuget restore -PackagesDirectory .\packages
      bcdedit /set testsigning on   # then reboot
.PARAMETER Config
    Build configuration: Debug or Release (default: Debug)
.PARAMETER Platform
    Build platform: x64 or ARM64 (default: x64)
.PARAMETER CertThumbprint
    SHA1 thumbprint of the code-signing certificate.
    If omitted, the script finds or creates a self-signed WDKTestCert.
.PARAMETER SkipBuild
    Skip building — use existing build output.
.PARAMETER SkipSign
    Skip catalog creation and signing.
.PARAMETER InstallOnly
    Jump straight to driver installation.
.PARAMETER Uninstall
    Remove all sample drivers and exit.
#>
param(
    [ValidateSet("Debug","Release")]
    [string]$Config = "Debug",

    [ValidateSet("x64","ARM64")]
    [string]$Platform = "x64",

    [string]$CertThumbprint = "",

    [switch]$SkipBuild,
    [switch]$SkipSign,
    [switch]$InstallOnly,
    [switch]$Uninstall
)

$ErrorActionPreference = "Stop"

$ScriptDir = if ($PSScriptRoot) { $PSScriptRoot } else { $PWD.Path }
# Script lives in network/wlan/wificx — repo root is three levels up
$RepoRoot  = Resolve-Path "$ScriptDir\..\..\..\"

$WificxDir = "$RepoRoot\network\wlan\wificx"
$NetvadDir = "$RepoRoot\network\netadaptercx\netvadapter"

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
function Write-Step($msg) { Write-Host "`n==> $msg" -ForegroundColor Cyan }
function Write-Ok($msg)   { Write-Host "    OK: $msg" -ForegroundColor Green }
function Write-Warn($msg) { Write-Host "    WARNING: $msg" -ForegroundColor Yellow }
function Write-Fail($msg) { Write-Host "    FAIL: $msg" -ForegroundColor Red; exit 1 }

function Invoke-Checked($description, [scriptblock]$cmd) {
    Write-Step $description
    & $cmd
    if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
        Write-Fail "$description (exit code $LASTEXITCODE)"
    }
    Write-Ok $description
}

# ---------------------------------------------------------------------------
# WiFiCx devnode helpers (shared by install + uninstall)
# ---------------------------------------------------------------------------
$WificxHwId = 'Root\wificxsampleclientkm'

# Every ROOT\NET devnode for our hardware ID — both the NIC mains and the
# WiFiDirect companions the framework spawns report this same hardware ID,
# present or phantom.
function Get-WificxDevnodes {
    Get-PnpDevice -Class Net -ErrorAction SilentlyContinue | Where-Object {
        $_.InstanceId -like 'ROOT\NET\*' -and
        ((Get-PnpDeviceProperty -InstanceId $_.InstanceId `
            -KeyName 'DEVPKEY_Device_HardwareIds' -EA SilentlyContinue).Data -contains $WificxHwId)
    }
}

# The per-adapter software (Class) key, e.g.
#   HKLM\SYSTEM\CurrentControlSet\Control\Class\{4d36e972-...}\0017
# This is where NetAdapterCx (NetDeviceOpenConfiguration -> MACLastByte) reads
# its config, NOT the Enum\<id>\Device Parameters hardware key.
function Get-WificxSoftwareKey($InstanceId) {
    $drv = (Get-PnpDeviceProperty -InstanceId $InstanceId `
        -KeyName 'DEVPKEY_Device_Driver' -EA SilentlyContinue).Data
    if ($drv) { "HKLM:\SYSTEM\CurrentControlSet\Control\Class\$drv" }
}

function Get-WificxMacLastByte($InstanceId) {
    $k = Get-WificxSoftwareKey $InstanceId
    if ($k) { (Get-ItemProperty $k -Name MACLastByte -EA SilentlyContinue).MACLastByte }
}

function Set-WificxMacLastByte($InstanceId, $Value) {
    $k = Get-WificxSoftwareKey $InstanceId
    if (-not $k) { return $false }
    Set-ItemProperty -Path $k -Name MACLastByte -Value "$Value" -Type String -EA SilentlyContinue
    return $true
}

# Disable/enable forces the framework to re-read MACLastByte and re-run adapter
# start; works even when the device is currently in the FAILED_START state.
function Restart-WificxDevice($InstanceId) {
    try { Disable-PnpDevice -InstanceId $InstanceId -Confirm:$false -EA Stop } catch {}
    Start-Sleep -Milliseconds 500
    try { Enable-PnpDevice  -InstanceId $InstanceId -Confirm:$false -EA Stop } catch {}
}

function Remove-AllWificxDevnodes {
    $nodes = @(Get-WificxDevnodes)
    foreach ($d in $nodes) {
        pnputil /remove-device "$($d.InstanceId)" /force 2>&1 | Out-Null
    }
    return $nodes.Count
}

function Get-WificxMacPair {
    @(Get-NetAdapter -IncludeHidden -ErrorAction SilentlyContinue |
        Where-Object { $_.MacAddress -in '22-22-22-22-00-01', '22-22-22-22-00-02' } |
        Select-Object -ExpandProperty MacAddress -Unique)
}

# ---------------------------------------------------------------------------
# Uninstall
# ---------------------------------------------------------------------------
if ($Uninstall) {
    Write-Step "Removing WiFiCx devnodes (present and phantom)"
    $removed = Remove-AllWificxDevnodes
    Write-Ok "Removed $removed WiFiCx devnode(s)"

    Write-Step "Scanning for installed sample drivers"
    $raw = pnputil /enum-drivers /class net | Out-String
    Write-Host $raw

    $oems = [regex]::Matches($raw,
        '(?ms)Published Name\s*:\s*(oem\d+\.inf).*?Original Name\s*:\s*(wificx|netvadapter)\w*\.inf') |
        ForEach-Object {
            [PSCustomObject]@{ Oem = $_.Groups[1].Value; Orig = $_.Groups[2].Value }
        }

    if ($oems.Count -eq 0) { Write-Warn "No sample drivers found."; exit 0 }

    foreach ($d in $oems) {
        Invoke-Checked "Removing $($d.Oem) ($($d.Orig))" {
            pnputil /delete-driver $d.Oem /uninstall /force
        }
    }
    exit 0
}

# ---------------------------------------------------------------------------
# Pre-flight
# ---------------------------------------------------------------------------
Write-Step "Pre-flight checks"

if (-not (Test-Path "$RepoRoot\wil\include\wil\resource.h")) {
    Write-Fail "wil submodule missing. Run: git submodule update --init"
}
Write-Ok "wil submodule present"

if (-not (Test-Path "$RepoRoot\packages\Microsoft.Windows.WDK*")) {
    Write-Fail "NuGet packages missing. Run: nuget restore -PackagesDirectory .\packages"
}
Write-Ok "NuGet packages present"

$ts = bcdedit /enum '{current}' | Select-String "testsigning\s+Yes"
if (-not $ts) {
    Write-Warn "Test signing is OFF. Run 'bcdedit /set testsigning on' and reboot."
} else {
    Write-Ok "Test signing enabled"
}

$devcon = Get-Command devcon.exe -ErrorAction SilentlyContinue |
    Select-Object -ExpandProperty Source
if (-not $devcon) {
    $wdkToolsDirs = @(
        "F:\Program Files\Windows Kits\10\Tools",
        "E:\Program Files\Windows Kits\10\Tools",
        "$env:SystemDrive\Program Files (x86)\Windows Kits\10\Tools"
    )
    foreach ($dir in $wdkToolsDirs) {
        $candidate = Get-ChildItem "$dir\*\x64\devcon.exe" -ErrorAction SilentlyContinue |
            Select-Object -First 1 -ExpandProperty FullName
        if ($candidate) { $devcon = $candidate; break }
    }
}
if ($devcon) {
    Write-Ok "devcon: $devcon"
} else {
    Write-Fail "devcon.exe not found. Run from an EWDK terminal or add WDK tools to PATH."
}

# ---------------------------------------------------------------------------
# Build
# ---------------------------------------------------------------------------
if (-not $SkipBuild -and -not $InstallOnly) {
    Invoke-Checked "Building WiFiCx sample ($Config|$Platform)" {
        msbuild "$WificxDir\wificxsampleclient.sln" `
            /t:Rebuild /p:Configuration=$Config /p:Platform=$Platform /m
    }

    Invoke-Checked "Building NetAdapterCx sample ($Config|$Platform)" {
        & "$NetvadDir\build.cmd" $Config $Platform
    }
}

# ---------------------------------------------------------------------------
# Locate build output  (solution-level: <sln_dir>\<Platform>\<Config>\<project>\)
# ---------------------------------------------------------------------------
$drivers = @(
    @{
        Name   = "WiFiCx KM"
        OutDir = "$WificxDir\$Platform\$Config\wificxsampleclientkm"
        Binary = "wificxsampleclientkm.sys"
        Inf    = "wificxsampleclientkm.inf"
        Cat    = "wificxsampleclientkm.cat"
        HwId   = "Root\wificxsampleclientkm"
    },
    @{
        Name   = "WiFiCx UM"
        OutDir = "$WificxDir\$Platform\$Config\wificxsampleclientum"
        Binary = "wificxsampleclientum.dll"
        Inf    = "wificxsampleclientum.inf"
        Cat    = "wificxsampleclientum.cat"
        HwId   = $null
    },
    @{
        Name   = "NetVAd KM"
        OutDir = "$NetvadDir\$Platform\$Config\netvadapterkm"
        Binary = "netvadapter.sys"
        Inf    = "netvadapter.inf"
        Cat    = "netvadapter.cat"
        HwId   = "root\netvadapter"
    },
    @{
        Name   = "NetVAd UM"
        OutDir = "$NetvadDir\$Platform\$Config\netvadapterum"
        Binary = "netvadapterum.dll"
        Inf    = "netvadapterum.inf"
        Cat    = "netvadapterum.cat"
        HwId   = $null
    }
)

Write-Step "Checking build output"
foreach ($d in $drivers) {
    $ok = (Test-Path "$($d.OutDir)\$($d.Binary)") -and (Test-Path "$($d.OutDir)\$($d.Inf)")
    if ($ok) { Write-Ok "$($d.Name): $($d.OutDir)" }
    else     { Write-Warn "$($d.Name): build output missing in $($d.OutDir)" }
}

# ---------------------------------------------------------------------------
# Certificate
# ---------------------------------------------------------------------------
if (-not $SkipSign -and -not $InstallOnly) {
    if ($CertThumbprint -eq "") {
        Write-Step "Locating code-signing certificate"
        $cert = Get-ChildItem Cert:\CurrentUser\My -CodeSigningCert |
            Where-Object { $_.Subject -eq "CN=WDKTestCert" } |
            Select-Object -First 1

        if ($cert) {
            $CertThumbprint = $cert.Thumbprint
            Write-Ok "Found WDKTestCert: $CertThumbprint"
        }
        else {
            Write-Step "Creating self-signed test certificate"
            $cert = New-SelfSignedCertificate -Type CodeSigningCert `
                -Subject "CN=WDKTestCert" -CertStoreLocation Cert:\CurrentUser\My
            $CertThumbprint = $cert.Thumbprint
            $cerFile = "$RepoRoot\WDKTestCert.cer"
            Export-Certificate -Cert $cert -FilePath $cerFile | Out-Null
            certutil -addstore Root $cerFile | Out-Null
            certutil -f -addstore TrustedPublisher $cerFile | Out-Null
            Write-Ok "Created and trusted: $CertThumbprint"
        }
    }

    # -------------------------------------------------------------------
    # StampInf — expand $ARCH$ and set DriverVer where the build missed it
    # -------------------------------------------------------------------
    $archMap = @{ "x64" = "amd64"; "ARM64" = "arm64" }
    $stampArch = $archMap[$Platform]
    $ntArch    = if ($Platform -eq "x64") { "NTAMD64" } else { "NTARM64" }

    foreach ($d in $drivers) {
        $infPath = "$($d.OutDir)\$($d.Inf)"
        if (-not (Test-Path $infPath)) { continue }

        $needsFix = Select-String -Path $infPath -Encoding unicode `
            -Pattern '\$ARCH\$' -Quiet

        if ($needsFix) {
            Write-Step "StampInf $($d.Name)"
            stampinf -f "$infPath" -a $stampArch -d * -v "1.0.0.0" -k "1.33" 2>&1 | Out-Null

            if (Select-String -Path $infPath -Encoding unicode -Pattern '\$ARCH\$' -Quiet) {
                $raw = [System.IO.File]::ReadAllText($infPath, [System.Text.Encoding]::Unicode)
                $raw = $raw.Replace('NT$ARCH$', $ntArch).Replace('$ARCH$', $ntArch)
                [System.IO.File]::WriteAllText($infPath, $raw, [System.Text.Encoding]::Unicode)
            }

            $raw = [System.IO.File]::ReadAllText($infPath, [System.Text.Encoding]::Unicode)
            $today = (Get-Date).ToString("MM/dd/yyyy")
            if ($raw -match 'DriverVer\s*=\s*;') {
                $raw = $raw -replace 'DriverVer\s*=\s*;[^\r\n]*', "DriverVer = $today,1.0.0.0"
                [System.IO.File]::WriteAllText($infPath, $raw, [System.Text.Encoding]::Unicode)
            }
            Write-Ok "StampInf $($d.Name)"
        }
    }

    # -------------------------------------------------------------------
    # Inf2Cat + signtool
    # -------------------------------------------------------------------
    foreach ($d in $drivers) {
        $infPath = "$($d.OutDir)\$($d.Inf)"
        $catPath = "$($d.OutDir)\$($d.Cat)"
        if (-not (Test-Path $infPath)) {
            Write-Warn "Skipping $($d.Name) - no INF"
            continue
        }

        Write-Step "Catalog for $($d.Name)"
        Inf2Cat /driver:"$($d.OutDir)" /os:10_X64,Server10_X64 2>&1 | Out-Null
        if ($LASTEXITCODE -ne 0) {
            Write-Warn "Inf2Cat failed, falling back to New-FileCatalog"
            New-FileCatalog -Path "$($d.OutDir)" -CatalogFilePath $catPath `
                -CatalogVersion 2 | Out-Null
        }
        if (-not (Test-Path $catPath)) { Write-Fail "No catalog for $($d.Name)" }
        Write-Ok "Catalog for $($d.Name)"

        Invoke-Checked "Signing $($d.Name)" {
            signtool sign /fd SHA256 /sha1 $CertThumbprint `
                /tr http://timestamp.digicert.com /td SHA256 $catPath
        }
    }
}

# ---------------------------------------------------------------------------
# Clean slate: remove every existing WiFiCx devnode (mains, companions, and
# phantom leftovers) so freshly installed instances cannot collide with stale
# nodes still defaulting to MACLastByte=1.
# ---------------------------------------------------------------------------
Write-Step "Clean slate: removing all existing WiFiCx devnodes"
$removed = Remove-AllWificxDevnodes
if ($removed) {
    Write-Ok "Removed $removed pre-existing WiFiCx devnode(s)"
    Start-Sleep -Seconds 2
} else {
    Write-Ok "No pre-existing WiFiCx devnodes"
}

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
Write-Step "Installing drivers"

$wificxKmOut = ($drivers | Where-Object { $_.Name -eq "WiFiCx KM" }).OutDir

$needsReboot = $false

Write-Step "Install WiFiCx KMDF instance 1 (MACLastByte=1)"
& $devcon install "$wificxKmOut\wificxsampleclientkm.inf" Root\wificxsampleclientkm
if ($LASTEXITCODE -eq 1) { $needsReboot = $true; Write-Ok "Installed (reboot needed)" }
elseif ($LASTEXITCODE -and $LASTEXITCODE -ne 0) { Write-Fail "Install failed (exit code $LASTEXITCODE)" }
else { Write-Ok "Install WiFiCx KMDF instance 1" }

Write-Step "Install WiFiCx KMDF instance 2 (MACLastByte=2)"
& $devcon install "$wificxKmOut\wificxsampleclientkm.inf" Root\wificxsampleclientkm
if ($LASTEXITCODE -eq 1) { $needsReboot = $true; Write-Ok "Installed (reboot needed)" }
elseif ($LASTEXITCODE -and $LASTEXITCODE -ne 0) { Write-Fail "Install failed (exit code $LASTEXITCODE)" }
else { Write-Ok "Install WiFiCx KMDF instance 2" }

Start-Sleep -Seconds 3

# ---------------------------------------------------------------------------
# Pair the two adapters: one must end up at MACLastByte=1 (MAC ...00-01) and
# one at MACLastByte=2 (MAC ...00-02).  On a clean slate both mains install
# with the INF default (1) and collide on ...00-01, so one starts and the
# other lands in FAILED_START with no MAC.  We promote a second main to
# ...00-02 by writing MACLastByte to its SOFTWARE (Class) key -- the location
# NetAdapterCx (NetDeviceOpenConfiguration) actually reads -- then restart it.
# The previous script wrote Enum\<id>\Device Parameters, which the driver never
# reads, so pairing silently never took effect.
# ---------------------------------------------------------------------------
$have     = Get-WificxMacPair
$haveMac1 = $have -contains '22-22-22-22-00-01'
$haveMac2 = $have -contains '22-22-22-22-00-02'

if ($haveMac1 -and $haveMac2) {
    Write-Ok "MAC pair already present (00-01 and 00-02)"
}
else {
    $mac1Owner = Get-NetAdapter -IncludeHidden -ErrorAction SilentlyContinue |
        Where-Object MacAddress -eq '22-22-22-22-00-01' |
        Select-Object -First 1 -ExpandProperty PnpDeviceID

    # Candidate second mains: our devnodes (excluding the 00-01 owner) whose
    # software key exposes the MACLastByte keyword -- i.e. the NIC instances.
    # A WiFiDirect companion never yields a 00-02 NIC, so if a candidate turns
    # out to be one, we revert it and try the next.
    $candidates = @(Get-WificxDevnodes | Where-Object {
        $_.InstanceId -ne $mac1Owner -and ($null -ne (Get-WificxMacLastByte $_.InstanceId))
    })

    Write-Step "Promoting a second adapter to MACLastByte=2 (software key)"
    foreach ($cand in $candidates) {
        $id = $cand.InstanceId
        if (-not (Set-WificxMacLastByte $id 2)) { continue }
        Write-Host "    trying $id ..."
        Restart-WificxDevice $id

        $deadline = (Get-Date).AddSeconds(15)
        do {
            Start-Sleep -Seconds 2
            $haveMac2 = (Get-WificxMacPair) -contains '22-22-22-22-00-02'
        } while (-not $haveMac2 -and (Get-Date) -lt $deadline)

        if ($haveMac2) { Write-Ok "Second adapter is now 00-02 ($id)"; break }

        # Not a usable main -- revert so nodes don't fight over 00-02.
        Set-WificxMacLastByte $id 1 | Out-Null
        Restart-WificxDevice $id
    }
}

# ---------------------------------------------------------------------------
# Verify the real success signal: both 00-01 and 00-02 present.
# ---------------------------------------------------------------------------
$have   = Get-WificxMacPair
$pairOk = ($have -contains '22-22-22-22-00-01') -and ($have -contains '22-22-22-22-00-02')
if ($pairOk) { Write-Ok "MAC pair complete (00-01 and 00-02)" }
else         { Write-Warn "MAC pair incomplete (have: $($have -join ', '))" }

# ---------------------------------------------------------------------------
# Remove leftover Error-state companion devnodes, protecting the two real
# mains (the PnpDeviceIDs that own each 00:0X MAC).
# ---------------------------------------------------------------------------
Write-Step "Cleaning up companion Error-state devices"
if ($pairOk) {
    $mainIds = @(Get-NetAdapter -IncludeHidden -ErrorAction SilentlyContinue |
        Where-Object { $_.MacAddress -in '22-22-22-22-00-01', '22-22-22-22-00-02' } |
        Select-Object -ExpandProperty PnpDeviceID)
    $companions = @(Get-WificxDevnodes |
        Where-Object { $_.Status -eq 'Error' -and $mainIds -notcontains $_.InstanceId })
    if ($companions) {
        foreach ($dev in $companions) {
            pnputil /remove-device "$($dev.InstanceId)" /force 2>&1 | Out-Null
            Write-Ok "Removed companion $($dev.InstanceId)"
        }
    } else {
        Write-Ok "No companion devices to remove"
    }
} else {
    Write-Ok "Companion cleanup skipped — leaving devices untouched"
}

# ---------------------------------------------------------------------------
# Verify
# ---------------------------------------------------------------------------
Write-Step "Verifying"
pnputil /enum-drivers /class net
Write-Host ""
ipconfig /all | Select-String -Pattern "adapter|IPv4|Description" -Context 0,1

if ($needsReboot) {
    Write-Warn "A reboot is required for the second adapter to start."
    Write-Warn "Run: shutdown /r /t 0"
}

Write-Host "`n==> Done!" -ForegroundColor Green
Write-Host @"
  Two WiFiCx adapters installed and paired via ENL (MACLastByte 1 & 2).

  Next steps:
    1. Open Settings > Network > Wi-Fi and scan for networks
    2. Connect to one of the fake SSIDs
    3. Assign IPs and ping between the two WiFi adapters:
       netsh interface ip set address "Wi-Fi" static 192.168.100.1 255.255.255.0
       netsh interface ip set address "Wi-Fi 2" static 192.168.100.2 255.255.255.0
       ping 192.168.100.2 -S 192.168.100.1

  To remove: .\deploy.ps1 -Uninstall
"@
