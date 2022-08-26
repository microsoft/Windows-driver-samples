param(
    [hashtable]$ProjectSet,
    [string]$Configuration = $env:Configuration,
    [string]$Platform = $env:Platform
)

#TODO validate params

$exclusionsSet = @{}
$failSet = @()
Import-Csv 'exclusions.csv' | ForEach-Object {
    $exclusionsSet[$_.Path.Replace($root, '').Trim('\').Replace('\', '.').ToLower()] = $_.Reason
}

$ProjectSet.GetEnumerator() | ForEach-Object {
    $projectName = $_.Key
    if ($exclusionsSet.ContainsKey($projectName))
    {
        Write-Output "[$projectName] ⏩ Excluded and skipped. Reason: $($exclusionsSet[$projectName])"
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