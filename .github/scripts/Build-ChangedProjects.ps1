param (
    [array]$ChangedFiles
)

$root = Get-Location

# To include in CI gate
$projectSet = @{}
foreach ($file in $ChangedFiles)
{
    $dir = (Get-Item $file).DirectoryName
    # Add condition to avoid getting at root
    while ((-not ($slnItems = (Get-ChildItem $dir '*.sln'))) -and ($dir -ne $root))
    {
        $dir = (Get-Item $dir).Parent.FullName
    }
    if ($dir -eq $root)
    {
        echo "❔ Changed file $file does not match a project."
        continue
    }
    $dir_norm = $dir.Replace($root, '').Trim('\').Replace('\', '.').ToLower()
    echo "🔎 Found project [$dir_norm] at $dir from changed file $file"
    if (-not ($projectSet.ContainsKey($dir_norm)))
    {
        $projectSet[$dir_norm] = $dir
    }
}

.\Build-ProjectSet -ProjectSet $projectSet

