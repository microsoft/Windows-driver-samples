function Assert-Pwsh {
    [CmdletBinding()]
    param()

    # Always log current edition and version
    Write-Host "Current PowerShell Edition: $($PSVersionTable.PSEdition)"
    Write-Host "Current PowerShell Version: $($PSVersionTable.PSVersion)"

    # Check edition
    if ($PSVersionTable.PSEdition -ne 'Core') {
        throw "This script requires PowerShell (pwsh) Core edition. Current edition: $($PSVersionTable.PSEdition)"
    }

    # Optional: enforce minimum version (e.g., 7.0)
    if ($PSVersionTable.PSVersion -lt [Version]'7.0') {
        throw "This script requires PowerShell 7.0 or later. Current version: $($PSVersionTable.PSVersion)"
    }
}

function Copy-IfMissingSizeTime {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$Source,
        [Parameter(Mandatory)][string]$Destination
    )

    Write-Host "Copy-IfMissingSizeTime $Source {"

    $ErrorActionPreference = 'Stop'

    # Normalize UNC long path (\\server\share\... -> \\?\UNC\server\share\...)
    function To-LongUnc([string]$p) {
        if ($p.StartsWith('\\')) {
            # Strip leading \\ then rebuild
            $trim = $p.TrimStart('\')
            return "\\?\UNC\" + $trim
        }
        return $p
    }

    # Use long-path version for source if UNC; destination is local so normal path is fine
    $srcPath = if ($Source.StartsWith('\\')) { To-LongUnc $Source } else { $Source }

    # Validate source file
    $srcItem = Get-Item -LiteralPath $srcPath -ErrorAction Stop
    if ($srcItem.PSIsContainer) { throw "Source '$Source' is a directory; this helper is for single files." }

    # Ensure destination directory exists

    $destDir = Split-Path -LiteralPath $Destination
    if ($destDir -and -not (Test-Path -LiteralPath $destDir)) {
        New-Item -ItemType Directory -Path $destDir -Force | Out-Null
    }

    if (Test-Path -LiteralPath $Destination) {
        $dstItem  = Get-Item -LiteralPath $Destination
        $sameSize = ($srcItem.Length -eq $dstItem.Length)
        $sameTime = ($srcItem.LastWriteTimeUtc -eq $dstItem.LastWriteTimeUtc)

        if ($sameSize -and $sameTime) {
            Write-Verbose "Skipping: destination matches source by size and timestamp."
            Write-Host "Copy-IfMissingSizeTime $Source }"
            return $false
        }

        # Overwrite if different
        Copy-Item -LiteralPath $srcPath -Destination $Destination -Force
        # Align timestamp (sometimes Copy-Item preserves; enforce to be sure)
        (Get-Item -LiteralPath $Destination).LastWriteTimeUtc = $srcItem.LastWriteTimeUtc
        Write-Host "Copy-IfMissingSizeTime $Source }"
        return $true
    }
    else {
        # Destination missing -> copy
        Copy-Item -LiteralPath $srcPath -Destination $Destination -Force
        (Get-Item -LiteralPath $Destination).LastWriteTimeUtc = $srcItem.LastWriteTimeUtc
        Write-Host "Copy-IfMissingSizeTime $Source }"
        return $true
    }
    Write-Host "Copy-IfMissingSizeTime $Source }"
}


function Copy-IsoContent {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory, Position = 0)]
        [ValidateNotNullOrEmpty()]
        [string] $IsoPath,

        [Parameter(Mandatory, Position = 1)]
        [ValidateNotNullOrEmpty()]
        [string] $Destination
    )

    $ErrorActionPreference = 'Stop'

    Write-Host "Copy-IsoContent $IsoPath {"

    # Resolve paths
    $isoFull = (Resolve-Path -LiteralPath $IsoPath).ProviderPath
    $destFull = if (Test-Path -LiteralPath $Destination) {
        (Resolve-Path -LiteralPath $Destination).ProviderPath
    } else {
        [IO.Path]::GetFullPath($Destination)
    }

    # Ensure destination exists
    if (-not (Test-Path -LiteralPath $destFull)) {
        New-Item -ItemType Directory -Path $destFull -Force | Out-Null
    }

    # Mount ISO
    $diskImage = Mount-DiskImage -ImagePath $isoFull -PassThru
    try {
        # Get drive letter
        $volume = Get-Volume -DiskImage $diskImage | Where-Object { $_.DriveLetter } | Select-Object -First 1
        if (-not $volume) {
            throw "Failed to determine a drive letter for mounted ISO: $isoFull"
        }
        $src = ($volume.DriveLetter + ':\')

        # Build robocopy args (copy all, keep timestamps, avoid ACLs from ISO, clear R)
        $args = @(
            "$src",              # source
            "`"$destFull`"",         # destination
            '/E',                    # include empty directories
            '/COPY:DAT',             # Data, Attr, Timestamps (avoid ACL/Owner from ISO)
            '/DCOPY:DAT',            # preserve dir timestamps too
            '/A-:R',                 # remove read-only on destination
            '/R:2','/W:2',           # quick retry policy
            '/MT:16',                # multithreaded copy
            '/NP','/NFL','/NDL',     # quieter output
            '/XJ'                    # ignore junctions
        )

        $proc = Start-Process -FilePath 'robocopy.exe' -ArgumentList $args -NoNewWindow -PassThru -Wait
        $rc = $proc.ExitCode

        # Robocopy: 0..7 are success-ish; >=8 is failure
        if ($rc -ge 8) {
            throw "Robocopy failed with exit code $rc."
        }

        Write-Host "Copy-IsoContent $IsoPath }"
        # Also reflect exit code in $LASTEXITCODE for callers
        Set-Variable -Name LASTEXITCODE -Value $rc -Scope Global
        #return $rc
        return
    }
    finally {
        try {
            Dismount-DiskImage -ImagePath $isoFull -ErrorAction Stop | Out-Null
        } catch {
            Write-Warning "Failed to unmount ISO '$isoFull': $($_.Exception.Message)"
        }
    }
    Write-Host "Copy-IsoContent $IsoPath }"
}

function WithEWDK {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory, Position = 1)]
        [ValidateNotNullOrEmpty()]
        [string] $EWDK
    )

    $ErrorActionPreference = 'Stop'

    Write-Host "WithEWDK $EWDK {"

    git clean -xdf
    git status --ignored

    if (Test-Path -LiteralPath "D:\wds\EWDK") {
        Write-Host "Removing old junction D:\wds\EWDK {"
        # Remove-Item -Recurse -Force -ErrorAction Stop -LiteralPath "D:\wds\EWDK" 
        Remove-Item -LiteralPath "D:\wds\EWDK" -ErrorAction Stop
        Write-Host "Removing old junction D:\wds\EWDK }"
    }
    New-Item -ItemType Junction -Path "D:\wds\EWDK" -Target "D:\wds\EWDKs\$EWDK"
    #robocopy /mir /nfl /ndl "D:\wds\EWDKs\$EWDK" "D:\wds\EWDK"

    $CmdScriptPath = "D:\wds\EWDK\BuildEnv\SetupBuildEnv.cmd"

    $pwshArgs = @(
            '-NoLogo','-NoProfile',
            '-ExecutionPolicy','Bypass',
            '-File', 'D:\wds\wds1\PowerTools\WDKBatchBuild\WDKBatchBuild_Internal.ps1',
            '-Phase','AfterCmd'
        ) -join ' '

    $cmdPayload = ('call "{0}" && pwsh {1} || exit /b %errorlevel%' -f $CmdScriptPath, $pwshArgs)

    Write-Host "& $env:ComSpec /s /c $cmdPayload"
    & $env:ComSpec /s /c $cmdPayload

    robocopy /mir /nfl /ndl "." "D:\wds\Runs\$EWDK"

    git clean -xdf
    git status --ignored

    Write-Host "Removing junction D:\wds\EWDK {"
    Remove-Item -LiteralPath "D:\wds\EWDK" -ErrorAction Stop
    Write-Host "Removing junction D:\wds\EWDK }"

    Write-Host "WithEWDK $EWDK }"
}

Assert-Pwsh

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

cd D:\wds\wds1

Write-Host "Copy-IsoContent {"
Copy-IsoContent -IsoPath 'D:\wds\ISOs\EWDK_19041.685.201201-2105.iso' -Destination 'D:\wds\EWDKs\EWDK_19041.685.201201-2105'
Copy-IsoContent -IsoPath 'D:\wds\ISOs\EWDK_19041.685.201201-2105.iso' -Destination 'D:\wds\EWDKs\EWDK_19041.685.201201-2105-copy'
Copy-IsoContent -IsoPath 'D:\wds\ISOs\EWDK_19041.5738.250408-1639.iso' -Destination 'D:\wds\EWDKs\EWDK_19041.5738.250408-1639'
Copy-IsoContent -IsoPath 'D:\wds\ISOs\EWDK_26100.1.240331-1435.iso' -Destination 'D:\wds\EWDKs\EWDK_26100.1.240331-1435'
Copy-IsoContent -IsoPath 'D:\wds\ISOs\EWDK_26100.6725.250925-1604.iso' -Destination 'D:\wds\EWDKs\EWDK_26100.6725.250925-1604'
Write-Host "Copy-IsoContent }"

Write-Host "Build {"
WithEWDK 'EWDK_19041.685.201201-2105'
WithEWDK 'EWDK_19041.685.201201-2105-copy'
WithEWDK 'EWDK_19041.5738.250408-1639'
WithEWDK 'EWDK_26100.1.240331-1435'
WithEWDK 'EWDK_26100.6725.250925-1604'
Write-Host "Build }"
