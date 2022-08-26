param (
    [array]$ChangedFiles
)

$root = (Get-Location).Path

# To include in CI gate
$projectSet = @{}
foreach ($file in $ChangedFiles)
{
    if (-not (Test-Path $file)) {
        Write-Output "❔ Changed file $file cannot be found"
        continue
    }
    $dir = (Get-Item $file).DirectoryName
    while ((-not ($slnItems = (Get-ChildItem $dir '*.sln'))) -and ($dir -ne $root))
    {
        $dir = (Get-Item $dir).Parent.FullName
    }
    if ($dir -eq $root)
    {
        Write-Output "❔ Changed file $file does not match a project."
        continue
    }
    $projectName = $dir.Replace($root, '').Trim('\').Replace('\', '.').ToLower()
    Write-Output "🔎 Found project [$projectName] at $dir from changed file $file"
    if (-not ($projectSet.ContainsKey($projectName)))
    {
        $projectSet[$projectName] = $dir
    }
}

.\Build-ProjectSet -ProjectSet $projectSet

