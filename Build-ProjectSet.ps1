param(
    [hashtable]$ProjectSet,
    [string]$Configuration = $env:Configuration,
    [string]$Platform = $env:Platform
)

#TODO validate params

$exclusionsSet = @{}
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
    .\Build-Project -Directory $directory -ProjectName $ProjectName
}