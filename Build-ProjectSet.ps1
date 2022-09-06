param(
    [hashtable]$ProjectSet,
    [string]$Configuration = $env:Configuration,
    [string]$Platform = $env:Platform
)

if ([string]::IsNullOrEmpty($Configuration))
{
    $Configuration = "Debug"
}

if ([string]::IsNullOrEmpty($Platform))
{
    $Platform = "x64"
}

$oldPreference = $ErrorActionPreference
$ErrorActionPreference = "stop"
try
{
    # Check that msbuild can be called before trying anything.
    Get-Command "msbuild" | Out-Null
}
catch
{
    Write-Host "`u{274C} msbuild cannot be called from current environment. Check that msbuild is set in current path (for example, that it is called from a Visual Studio developer command)."
    Write-Error "msbuild cannot be called from current environment."
    exit 1
}
finally
{
    $ErrorActionPreference = $oldPreference
}


$exclusionsSet = @{}
$failSet = @()
Import-Csv 'exclusions.csv' | ForEach-Object {
    $exclusionsSet[$_.Path.Replace($root, '').Trim('\').Replace('\', '.').ToLower()] = $_.Reason
}

$ProjectSet.GetEnumerator() | ForEach-Object {
    $projectName = $_.Key
    if ($exclusionsSet.ContainsKey($projectName))
    {
        Write-Output "[$projectName] `u{23E9} Excluded and skipped. Reason: $($exclusionsSet[$projectName])"
        return;
    }
    $directory = $_.Value
    .\Build-Project -Directory $directory -ProjectName $ProjectName -Configuration $Configuration -Platform $Platform
    if ($LASTEXITCODE -ne 0)
    {
        $failSet += $ProjectName
    }
}

if ($failSet.Count -gt 0)
{
    Write-Output "Some projects were built with errors:"
    foreach ($failedProject in $failSet)
    {
        Write-Output "$failedProject"
    }
    Write-Error "Some projects were built with errors."
}