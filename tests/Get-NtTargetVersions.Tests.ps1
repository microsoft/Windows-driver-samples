#Requires -Version 7.0

$ErrorActionPreference = 'Stop'
$fixtureRoot = Join-Path ([System.IO.Path]::GetTempPath()) "wds-nt-target-tests-$([guid]::NewGuid().ToString('N'))"
$environmentNames = @('ProgramFiles', 'ProgramFiles(x86)', 'WDKContentRoot', 'WindowsSdkDir', 'BuildLab')
$savedEnvironment = @{}
foreach ($name in $environmentNames) {
    $savedEnvironment[$name] = [Environment]::GetEnvironmentVariable($name, 'Process')
}

function New-TestRule {
    [Diagnostics.CodeAnalysis.SuppressMessageAttribute('PSUseShouldProcessForStateChangingFunctions', '', Justification = 'Only writes disposable test fixtures.')]
    [CmdletBinding()]
    param([string]$Root, [int]$Build, [string]$Code)

    $directory = Join-Path $Root "build\10.0.$Build.0\1033"
    New-Item -ItemType Directory -Path $directory -Force | Out-Null
    $path = Join-Path $directory 'DriverGeneral.xml'
    @"
<ProjectSchemaDefinitions>
  <Rule>
    <EnumProperty Name="_NT_TARGET_VERSION">
      <EnumValue Name="$Code" DisplayName="Windows 10.0.$Build" />
      <EnumValue Name="0xA00000B" DisplayName="Windows 10.0.22000" />
    </EnumProperty>
  </Rule>
</ProjectSchemaDefinitions>
"@ | Set-Content -LiteralPath $path
    return $path
}

function Assert-NewestTarget {
    param([string]$Mode, [int]$Build, [string]$Code)

    $actual = @(& $discoveryScript -RunMode $Mode -Newest 1)
    if ($actual.Count -ne 1 -or $actual[0].Version -ne "10.0.$Build" -or $actual[0].Code -ne $Code) {
        throw "RunMode '$Mode': expected 10.0.$Build ($Code), got $($actual | ConvertTo-Json -Compress)."
    }
}

function Assert-MissingSource {
    param([string]$Mode)

    try {
        & $discoveryScript -RunMode $Mode | Out-Null
    }
    catch {
        if ($_.Exception.Message -like "*Could not locate DriverGeneral.xml for RunMode '$Mode'*") {
            return
        }
        throw
    }
    throw "RunMode '$Mode' silently fell back to another source."
}

try {
    New-Item -ItemType Directory -Path $fixtureRoot | Out-Null
    $discoveryScript = Join-Path $fixtureRoot 'Get-NtTargetVersions.ps1'
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot '..\Get-NtTargetVersions.ps1') -Destination $discoveryScript
    $env:ProgramFiles = Join-Path $fixtureRoot 'ProgramFiles'
    ${env:ProgramFiles(x86)} = Join-Path $fixtureRoot 'ProgramFilesX86'
    $env:WDKContentRoot = Join-Path $fixtureRoot 'EWDK'
    $env:WindowsSdkDir = Join-Path $fixtureRoot 'EWDK-SDK'
    $env:BuildLab = $null

    $packages = Join-Path $fixtureRoot 'packages'
    $null = New-TestRule -Root (Join-Path $packages 'Microsoft.Windows.WDK.x64.10.0.22621.0\c') -Build 22621 -Code '0xA00000C'
    $nugetRule = New-TestRule -Root (Join-Path $packages 'Microsoft.Windows.WDK.x64.10.0.26100.0\c') -Build 26100 -Code '0xA00000F'
    $null = New-TestRule -Root (Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10') -Build 28000 -Code '0xA000012'
    $null = New-TestRule -Root (Join-Path $env:ProgramFiles 'Windows Kits\10') -Build 26100 -Code '0xA00000F'
    $null = New-TestRule -Root $env:WDKContentRoot -Build 22000 -Code '0xA00000B'
    $null = New-TestRule -Root $env:WindowsSdkDir -Build 22621 -Code '0xA00000C'

    $defaultTarget = & $discoveryScript -Newest 1
    if ($defaultTarget.Version -ne '10.0.26100') {
        throw 'Default discovery did not prefer restored NuGet over the newer host WDK.'
    }
    Assert-NewestTarget -Mode NuGet -Build 26100 -Code '0xA00000F'
    Assert-NewestTarget -Mode WDK -Build 28000 -Code '0xA000012'
    Assert-NewestTarget -Mode EWDK -Build 22621 -Code '0xA00000C'
    Assert-NewestTarget -Mode Auto -Build 26100 -Code '0xA00000F'

    $env:BuildLab = 'test_branch.22621.1'
    Assert-NewestTarget -Mode Auto -Build 22621 -Code '0xA00000C'
    Assert-NewestTarget -Mode NuGet -Build 26100 -Code '0xA00000F'
    Assert-NewestTarget -Mode WDK -Build 28000 -Code '0xA000012'
    $env:BuildLab = $null

    $explicitTarget = & $discoveryScript -XmlPath $nugetRule -RunMode WDK -Newest 1
    if ($explicitTarget.Version -ne '10.0.26100') {
        throw 'XmlPath did not override RunMode discovery.'
    }
    $json = & $discoveryScript -RunMode NuGet -Newest 2 -AsMatrixJson
    $matrix = @($json | ConvertFrom-Json)
    if ($matrix.Count -ne 2 -or $matrix[0].tag -ne '26100' -or $matrix[1].tag -ne '22000') {
        throw 'Filtered JSON discovery did not preserve newest-first matrix output.'
    }

    Move-Item -LiteralPath $packages -Destination (Join-Path $fixtureRoot 'saved-packages')
    Assert-MissingSource -Mode NuGet
    Assert-NewestTarget -Mode Auto -Build 28000 -Code '0xA000012'
    $env:ProgramFiles = Join-Path $fixtureRoot 'missing-programfiles'
    ${env:ProgramFiles(x86)} = Join-Path $fixtureRoot 'missing-programfiles-x86'
    Assert-MissingSource -Mode WDK
    $env:WDKContentRoot = $null
    $env:WindowsSdkDir = $null
    Assert-MissingSource -Mode EWDK
    $env:BuildLab = 'test_branch.22621.1'
    Assert-MissingSource -Mode Auto

    Write-Output 'Passed: source isolation, Auto precedence, explicit XmlPath, JSON ordering, and missing-source failures.'
}
finally {
    foreach ($name in $environmentNames) {
        [Environment]::SetEnvironmentVariable($name, $savedEnvironment[$name], 'Process')
    }
    if (Test-Path -LiteralPath $fixtureRoot) {
        Remove-Item -LiteralPath $fixtureRoot -Recurse -Force
    }
}
