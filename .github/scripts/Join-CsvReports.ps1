$logsPath = Join-Path (Get-Location).Path "_logs"
$reportFileName = '_overview.all'
$idProperty = 'Sample'
$results = $null

Get-ChildItem -Path $logsPath -Filter '*.csv' | ForEach-Object {
    $csv = Import-Csv -Path $_
    if ($results -eq $null) {
        $results = $csv
    }
    else {
        $results = $csv | ForEach-Object {
            $id = $_.$idProperty
            $match = $results | Where-Object { $_.$idProperty -eq $id }
            if ($match) {
                $properties = $_ | Get-Member -MemberType NoteProperty | Where-Object { $_.Name -ne $idProperty } | Select-Object -ExpandProperty Name
                $newObject = New-Object PSObject
                # Add ID property separately to ensure it appears first
                $newObject | Add-Member -MemberType NoteProperty -Name $idProperty -Value $_.$idProperty
                foreach ($property in $properties) {
                    $newObject | Add-Member -MemberType NoteProperty -Name $property -Value $_.$property
                }
                foreach ($property in ($match | Get-Member -MemberType NoteProperty | Where-Object { $_.Name -ne $idProperty } | Select-Object -ExpandProperty Name)) {
                    if ($properties -notcontains $property) {
                        $newObject | Add-Member -MemberType NoteProperty -Name $property -Value $match.$property
                    }
                }
                $newObject
            }
        }
    }
}

$results | ConvertTo-Csv | Out-File (Join-Path $logsPath "$reportFileName.csv")
$results | ConvertTo-Html -Title "Overview" | Out-File (Join-Path $logsPath "$reportFileName.htm")
