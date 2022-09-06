$root = Get-Location
$solutionFiles = Get-ChildItem -Path $root -Recurse -Filter *.sln | Select-Object -ExpandProperty FullName

# To include in CI gate
$projectSet = @{}
foreach ($file in $solutionFiles)
{
    $dir = (Get-Item $file).DirectoryName
    $dir_norm = $dir.Replace($root, '').Trim('\').Replace('\', '.').ToLower()
    Write-Output "`u{1F50E} Found project [$dir_norm] at $dir"
    $projectSet[$dir_norm] = $dir
}

.\Build-ProjectSet -ProjectSet $projectSet

