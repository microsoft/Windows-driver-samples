[CmdletBinding()]
param (
    [array]$ChangedFiles
)

$Verbose = $false
if ($PSBoundParameters.ContainsKey('Verbose')) {
    $Verbose = $PsBoundParameters.Get_Item('Verbose')
}

$root = (Get-Location).Path

# To include in CI gate
$sampleSet = @{}
foreach ($file in $ChangedFiles)
{
    if (-not (Test-Path $file)) {
        Write-Verbose "`u{2754} Changed file $file cannot be found"
        continue
    }
    $dir = (Get-Item $file).DirectoryName
    while ((-not ($slnItems = (Get-ChildItem $dir '*.sln'))) -and ($dir -ne $root))
    {
        $dir = (Get-Item $dir).Parent.FullName
    }
    if ($dir -eq $root)
    {
        Write-Verbose "`u{2754} Changed file $file does not match a sample."
        continue
    }
    $sampleName = $dir.Replace($root, '').Trim('\').Replace('\', '.').ToLower()
    Write-Verbose "`u{1F50E} Found sample [$sampleName] at $dir from changed file $file"
    if (-not ($sampleSet.ContainsKey($sampleName)))
    {
        $sampleSet[$sampleName] = $dir
    }
}

.\Build-SampleSet -SampleSet $sampleSet -Verbose:$Verbose -LogFilesDirectory (Join-Path $root "_logs")

