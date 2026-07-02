#Requires -RunAsAdministrator
<#
.SYNOPSIS
    Connect both WiFiCx virtual adapters and verify data path with ping.
.DESCRIPTION
    Automates what the GUI does manually: discover WiFiCx interfaces,
    ensure MACLastByte pairing, create a WLAN profile, connect both
    adapters, assign static IPs, and ping between them.

    Run after deploy.ps1 has installed both WiFiCx adapter instances.
.PARAMETER SSID
    SSID to connect to (default: WFC_OPEN).
.PARAMETER Ip1
    Static IP for the first adapter (default: 192.168.100.1).
.PARAMETER Ip2
    Static IP for the second adapter (default: 192.168.100.2).
.PARAMETER Mask
    Subnet mask (default: 255.255.255.0).
#>
param(
    [string]$SSID = "WFC_OPEN",
    [string]$Ip1  = "192.168.100.1",
    [string]$Ip2  = "192.168.100.2",
    [string]$Mask = "255.255.255.0"
)

$ErrorActionPreference = "Stop"

function Write-Step($msg) { Write-Host "`n==> $msg" -ForegroundColor Cyan }
function Write-Ok($msg)   { Write-Host "    OK: $msg" -ForegroundColor Green }
function Write-Warn($msg) { Write-Host "    WARNING: $msg" -ForegroundColor Yellow }
function Write-Fail($msg) { Write-Host "    FAIL: $msg" -ForegroundColor Red }

function Dump-Debug {
    Write-Host "`n--- Debug dump ---" -ForegroundColor Yellow
    Write-Host "`n[netsh wlan show interfaces]"
    netsh wlan show interfaces
    Write-Host "`n[ipconfig]"
    ipconfig
    Write-Host "`n[arp -a]"
    arp -a
    Write-Host "`n[Get-PnpDevice WiFiCx]"
    Get-PnpDevice -FriendlyName "*WiFiCx*" -ErrorAction SilentlyContinue |
        Format-Table Status, InstanceId, FriendlyName -AutoSize
    Write-Host "--- End debug dump ---`n" -ForegroundColor Yellow
}

# ---------------------------------------------------------------------------
# 1. Discover WiFiCx adapters
# ---------------------------------------------------------------------------
Write-Step "Discovering WiFiCx adapters"

$wificxDevices = Get-PnpDevice -FriendlyName "*WiFiCx*" -Status OK -ErrorAction SilentlyContinue |
    Sort-Object InstanceId

if ($wificxDevices.Count -lt 2) {
    Write-Fail "Need 2 WiFiCx adapters in OK state, found $($wificxDevices.Count). Run deploy.ps1 first."
    Dump-Debug
    exit 1
}

Write-Ok "Found $($wificxDevices.Count) WiFiCx adapters"

# Pairing is owned by deploy.ps1; run.ps1 must not touch MACLastByte or
# disable/enable here -- doing so re-enumerates the queues and tears down the
# ENL pairing right before the ping.  Adapters are matched by MAC below.

# ---------------------------------------------------------------------------
# 3. Find WLAN interface names
# ---------------------------------------------------------------------------
Write-Step "Mapping WLAN interfaces"

$wlanOutput = netsh wlan show interfaces
$interfaces = @()
$currentIf = $null

foreach ($line in $wlanOutput -split "`n") {
    if ($line -match '^\s*Name\s*:\s*(.+)$') {
        if ($currentIf) { $interfaces += $currentIf }
        $currentIf = @{ Name = $Matches[1].Trim(); MAC = ""; State = "" }
    }
    elseif ($currentIf -and $line -match '^\s*Physical address\s*:\s*(.+)$') {
        $currentIf.MAC = $Matches[1].Trim()
    }
    elseif ($currentIf -and $line -match '^\s*State\s*:\s*(.+)$') {
        $currentIf.State = $Matches[1].Trim()
    }
}
if ($currentIf) { $interfaces += $currentIf }

$wificxIfs = $interfaces | Where-Object { $_.MAC -match "22:22:22:22:00:" }

if ($wificxIfs.Count -lt 2) {
    Write-Fail "Found $($wificxIfs.Count) WiFiCx WLAN interfaces (need 2)"
    Dump-Debug
    exit 1
}

$if1 = ($wificxIfs | Where-Object { $_.MAC -match "00:01$" })
$if2 = ($wificxIfs | Where-Object { $_.MAC -match "00:02$" })

if (-not $if1 -or -not $if2) {
    Write-Fail "Cannot identify adapter1 (00:01) and adapter2 (00:02)"
    Dump-Debug
    exit 1
}

$name1 = $if1.Name
$name2 = $if2.Name
Write-Ok "Adapter 1: '$name1' ($($if1.MAC))"
Write-Ok "Adapter 2: '$name2' ($($if2.MAC))"

# ---------------------------------------------------------------------------
# 4. Enable interfaces and ensure Wi-Fi radio is on
# ---------------------------------------------------------------------------
Write-Step "Enabling Wi-Fi interfaces"
netsh interface set interface "$name1" enable 2>&1 | Out-Null
netsh interface set interface "$name2" enable 2>&1 | Out-Null
Start-Sleep -Seconds 2
Write-Ok "Interfaces enabled"

# ---------------------------------------------------------------------------
# 5. Create WLAN profile and connect
# ---------------------------------------------------------------------------
Write-Step "Creating WLAN profile for '$SSID'"

$profileXml = @"
<?xml version="1.0"?>
<WLANProfile xmlns="http://www.microsoft.com/networking/WLAN/profile/v1">
    <name>$SSID</name>
    <SSIDConfig>
        <SSID>
            <name>$SSID</name>
        </SSID>
    </SSIDConfig>
    <connectionType>ESS</connectionType>
    <connectionMode>manual</connectionMode>
    <MSM>
        <security>
            <authEncryption>
                <authentication>open</authentication>
                <encryption>none</encryption>
                <useOneX>false</useOneX>
            </authEncryption>
        </security>
    </MSM>
</WLANProfile>
"@

$profilePath = "$env:TEMP\wificx_$SSID.xml"
$profileXml | Out-File -Encoding UTF8 -FilePath $profilePath

netsh wlan add profile filename="$profilePath" interface="$name1" 2>&1 | Out-Null
netsh wlan add profile filename="$profilePath" interface="$name2" 2>&1 | Out-Null
Write-Ok "Profile added to both interfaces"

Write-Step "Connecting '$name1' to $SSID"
netsh wlan connect name="$SSID" interface="$name1" 2>&1 | Out-Null

Write-Step "Connecting '$name2' to $SSID"
netsh wlan connect name="$SSID" interface="$name2" 2>&1 | Out-Null

# Poll until both connected (scan + connect is async and can take 10-20s)
$connTimeout = 30
$connStart = Get-Date
$connected1 = $false
$connected2 = $false

Write-Step "Waiting for WLAN connections (timeout ${connTimeout}s)"
do {
    Start-Sleep -Seconds 2
    $wlanText = (netsh wlan show interfaces) -join "`n"
    $blocks = $wlanText -split "(?=\s+Name\s*:)"

    foreach ($block in $blocks) {
        if ($block -match [regex]::Escape($name1) -and $block -match "State\s*:\s*connected") {
            if (-not $connected1) { Write-Ok "'$name1' connected to $SSID" }
            $connected1 = $true
        }
        if ($block -match [regex]::Escape($name2) -and $block -match "State\s*:\s*connected") {
            if (-not $connected2) { Write-Ok "'$name2' connected to $SSID" }
            $connected2 = $true
        }
    }

    $elapsed = [int]((Get-Date) - $connStart).TotalSeconds
    if (-not $connected1 -or -not $connected2) {
        Write-Host "    [$elapsed s] '$name1'=$connected1  '$name2'=$connected2"
    }
} while ((-not $connected1 -or -not $connected2) -and
         ((Get-Date) - $connStart).TotalSeconds -lt $connTimeout)

if (-not $connected1) { Write-Warn "'$name1' did not connect within ${connTimeout}s" }
if (-not $connected2) { Write-Warn "'$name2' did not connect within ${connTimeout}s" }

if (-not $connected1 -and -not $connected2) {
    Write-Fail "Neither adapter connected"
    Dump-Debug; exit 1
}

# ---------------------------------------------------------------------------
# 5. Assign static IPs
# ---------------------------------------------------------------------------
Write-Step "Assigning static IPs"

netsh interface ip set address "$name1" static $Ip1 $Mask 2>&1 | Out-Null
Write-Ok "$name1 = $Ip1"

netsh interface ip set address "$name2" static $Ip2 $Mask 2>&1 | Out-Null
Write-Ok "$name2 = $Ip2"

Start-Sleep -Seconds 2

# ---------------------------------------------------------------------------
# 5b. Seed static ARP neighbors
# ---------------------------------------------------------------------------
# Both adapters share one host and subnet, so each stack treats the other's IP
# as a local duplicate and won't answer its ARP.  Seed static neighbor entries
# to bypass ARP and let the ICMP frames traverse the ENL link.
Write-Step "Seeding static ARP neighbors"

$mac1 = ($if1.MAC -replace ':', '-')   # 22-22-22-22-00-01
$mac2 = ($if2.MAC -replace ':', '-')   # 22-22-22-22-00-02

netsh interface ipv4 delete neighbors "$name1" $Ip2 2>&1 | Out-Null
netsh interface ipv4 add    neighbors "$name1" $Ip2 $mac2 2>&1 | Out-Null
Write-Ok "${name1}: $Ip2 -> $mac2"

netsh interface ipv4 delete neighbors "$name2" $Ip1 2>&1 | Out-Null
netsh interface ipv4 add    neighbors "$name2" $Ip1 $mac1 2>&1 | Out-Null
Write-Ok "${name2}: $Ip1 -> $mac1"

# ---------------------------------------------------------------------------
# 6. Ping test
# ---------------------------------------------------------------------------
Write-Step "Ping $Ip1 -> $Ip2"
$ping1 = ping $Ip2 -S $Ip1 -n 4 -w 1000
$ping1 | ForEach-Object { Write-Host "    $_" }
$pass1 = ($ping1 | Select-String "Reply from $Ip2" | Measure-Object).Count -gt 0

Write-Step "Ping $Ip2 -> $Ip1"
$ping2 = ping $Ip1 -S $Ip2 -n 4 -w 1000
$ping2 | ForEach-Object { Write-Host "    $_" }
$pass2 = ($ping2 | Select-String "Reply from $Ip1" | Measure-Object).Count -gt 0

# ---------------------------------------------------------------------------
# 7. Result
# ---------------------------------------------------------------------------
Write-Host ""
if ($pass1 -and $pass2) {
    Write-Ok "PASS: Bidirectional ping succeeded between WiFiCx adapters"
    exit 0
} else {
    Write-Fail "FAIL: Ping did not succeed"
    if (-not $pass1) { Write-Fail "  ${Ip1} -> ${Ip2}: no reply" }
    if (-not $pass2) { Write-Fail "  ${Ip2} -> ${Ip1}: no reply" }
    Dump-Debug
    exit 1
}
