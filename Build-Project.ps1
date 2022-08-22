param(
    $Directory,
    [string]$ProjectName,
    $LogFilesDirectory = "_logfiles"
)

#TODO Validate $LogFilesDirectory
New-Item -ItemType Directory -Force -Path $LogFilesDirectory | Out-Null

if ([string]::IsNullOrWhitespace($ProjectName))
{
    $ProjectName = $Directory.Replace([System.IO.Path]::PathSeparator, ".")
}

$errorLogFilePath = "$LogFilesDirectory\$ProjectName.err"
$warnLogFilePath = "$LogFilesDirectory\$ProjectName.wrn"
Write-Output "[$ProjectName] ⚒️ Building project..."
msbuild $Directory -clp:Verbosity=m -t:clean,build -property:Configuration=$Configuration -property:Platform=$Platform -p:TargetVersion=Windows10 -p:InfVerif_AdditionalOptions="/msft /sw1205 /sw1324 /sw1420 /sw1421" -p:SignToolWS=/fdws -p:DriverCFlagAddOn=/wd4996 -flp1:errorsonly`;logfile=$errorLogFilePath -flp2:WarningsOnly`;logfile=$warnLogFilePath -noLogo
if ($LASTEXITCODE -ne 0)
{
    Write-Error "Build for [$ProjectName] failed."
    Write-Output "❌ Build failed. Log available at $errorLogFilePath"
}
