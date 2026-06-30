# ======================================================================
#
# Manufacturer: Eaton Tripp Lite Switched Power Distribution Unit (PDU)
# Website: https://www.eaton.com/us/en-us/skuPage.PDUMH15NET.html
# Model: PDUMH15NET
# SNMP/Web Management Accessory Card: WEBCARDLX
#
# HW Requirements: Ethernet cable or USBA-to-MicroUSB cable.
# SW Requirements: Plink.exe from PuTTY terminal client SW (if Ethernet)
# PuTTY: https://www.chiark.greenend.org.uk/~sgtatham/putty/latest.html
#
# Script: PowerShell script for turning ON/OFF and power CYCLING the PDU
# Usage: .\Pdu.ps1 [on | off | cycle] [1-16]
#
# ======================================================================

param(
    [Parameter(Mandatory=$true, Position=0)]
    [ValidateSet("cycle","on","off")]
    [string]$Action,

    [Parameter(Mandatory=$true, Position=1)]
    [ValidateRange(1,16)]
    [int]$LoadNumber
)

# =========================
# Configuration
# =========================
$ComNumber   = 3
$ComPort     = "COM$ComNumber"

$PduIP       = "169.254.0.1"
$SshPort     = 22

$Username    = "localadmin"
$Password    = "@Password123"

$Plink       = "C:\Program Files\PuTTY\plink.exe"

# =========================
# Helper Functions
# =========================

function Test-PduSerialConnection
{
    [bool]$Connected = $false

    try
    {
        $Port = New-Object System.IO.Ports.SerialPort $ComPort,115200,None,8,one
        $Port.ReadTimeout = 3000
        $Port.WriteTimeout = 3000
        $Port.Open()
        Start-Sleep 5
        $Port.DiscardOutBuffer()
        $Port.DiscardInBuffer()
        $null = $Port.ReadExisting()

        # Wake up console
        $Port.WriteLine("")
        Start-Sleep 2

        $Banner = $Port.ReadExisting()
        if ($Banner -match "login|localadmin|PowerAlert|Ubuntu") {
            Write-Host "PDU detected on serial connection at $ComPort"
            $Connected = $true
        } else {
            Write-Host "PDU NOT detected on serial connection at $ComPort" -ForegroundColor Yellow
        }
    }
    catch
    {
        Write-Host "Unable to communicate on serial connection at $ComPort" -ForegroundColor Yellow
        Write-Host $_.Exception.Message
    }
    finally 
    {
        if ($Port -and $Port.IsOpen) {
            $Port.DiscardOutBuffer()
            $Port.DiscardInBuffer()
            if ($Port.BytesToRead -gt 0) {
                $null = $Port.ReadExisting()
            }
            $Port.Close()
        }

        $Port.Dispose()
        $Port = $null
    }
    return $Connected
}

function Send-PduSerialCommand
{
    param([string]$Action)

    $Port = New-Object System.IO.Ports.SerialPort $ComPort,115200,None,8,one

    try
    {
        $Port.ReadTimeout = 3000
        $Port.WriteTimeout = 3000

        $Port.Open()
        Start-Sleep 5
        $Port.DiscardOutBuffer()
        $Port.DiscardInBuffer()
        $null = $Port.ReadExisting()

        Write-Host "Using SERIAL connection..." -ForegroundColor Green
        $Port.WriteLine("")
        Start-Sleep 2

        Write-Host "Sending Username to PDU..."
        $Port.WriteLine($Username)
        Start-Sleep 2

        Write-Host "Sending Password to PDU..."
        $Port.WriteLine($Password)
        Start-Sleep 5

        # Sending [ cycle | on | off ] command to PDU.
        Write-Host "Sending '$Action' command to PDU..."
        $Port.WriteLine("device; load $LoadNumber; $Action force")
        Start-Sleep 2

        # "Sending Exit command to PDU..."
        $Port.WriteLine("exit; exit; exit")
        Start-Sleep 2
    }
    catch
    {
        Write-Host "Unable to communicate with $ComPort"
        Write-Host $_.Exception.Message
    }
    finally 
    {
        if ($Port -and $Port.IsOpen) {
            $Port.DiscardOutBuffer()
            $Port.DiscardInBuffer()
            if ($Port.BytesToRead -gt 0) {
                $null = $Port.ReadExisting()
            }
            $Port.Close()
            Write-Host "Closed serial port $ComPort"
        }
        if ($Port)
        {
            $Port.Dispose()
        }
        $Port = $null
    }
}

function Test-PduNetworkConnection
{
    [bool]$Connected = $false

    try
    {
        $Client = New-Object System.Net.Sockets.TcpClient
#        $Client.Connect($PduIP,$SshPort)

        $Result = $Client.BeginConnect($PduIP,$SshPort,$null,$null)
        if (-not $Result.AsyncWaitHandle.WaitOne(3000))
        {
            throw "Connection timeout"
        }
        $Client.EndConnect($Result)

        $Stream = $Client.GetStream()
        Start-Sleep 1

        $Buffer = New-Object byte[] 1024
        $Stream.ReadTimeout = 3000
        $Bytes = $Stream.Read($Buffer,0,$Buffer.Length)

        $Banner = [System.Text.Encoding]::ASCII.GetString($Buffer,0,$Bytes)

        if ($Banner -match "login|localadmin|PowerAlert|Ubuntu") {
            Write-Host "PDU detected on network connection at IP: $PduIP, Port: $SshPort"
            $Connected = $true
        }
        else {
            Write-Host "PDU NOT detected on network connection at IP: $PduIP, Port: $SshPort"  -ForegroundColor Yellow
        }

        $Client.Close()
    }
    catch
    {
        Write-Host "Unable to establish network connection at IP: $PduIP, Port: $SshPort"  -ForegroundColor Yellow
        Write-Host $_.Exception.Message
    }
    return $Connected
}

function Send-PduNetworkCommand
{
    param([string]$Action)

    if (-not (Test-Path $Plink))
    {
        throw "plink.exe not found: $Plink"
    }

    Write-Host "Using NETWORK connection..." -ForegroundColor Green

    $Command = "device; load $LoadNumber; $Action force; exit; exit; exit"

    $Command | & $Plink `
        -ssh `
        -batch `
        -pw $Password `
        "$Username@$PduIP" `
}

# =========================
# Main Logic
# =========================

Write-Host ""
Write-Host "Requested Action : $Action"
Write-Host ""

# First choice = Serial
if (Test-PduSerialConnection)
{
    Send-PduSerialCommand $Action
    exit 0
}

Write-Host "Serial connection not available."
Write-Host "Trying network connection..."

# Second choice = Network
if (Test-PduNetworkConnection)
{
    Send-PduNetworkCommand $Action
    exit 0
}

Write-Host ""
Write-Host "ERROR: No PDU connection available." -ForegroundColor Red
Write-Host "  Serial : $ComPort"
Write-Host "  Ethernet : $PduIP"
Write-Host ""

exit 1