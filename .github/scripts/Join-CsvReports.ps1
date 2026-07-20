<#
.SYNOPSIS
    Joins the per-job Build-Samples CSV reports (one per _NT_TARGET_VERSION x configuration x
    platform) into a single overview, and writes an easy-to-scan summary to the GitHub Actions
    run page ($GITHUB_STEP_SUMMARY).

.DESCRIPTION
    Each build job uploads a "_logs" folder containing a report named
        _overview.<ntTag>.<configuration>.<platform>.csv
    with columns: Sample, <Configuration|Platform>  (one combination per file). This script:
      * parses the _NT_TARGET_VERSION tag and combination from every report,
      * collapses each sample's combinations into one status per version,
      * writes _overview.all.csv / _overview.all.htm (a colour-coded sample x version matrix), and
      * appends a Markdown summary (per-version totals + a failures table) to $GITHUB_STEP_SUMMARY
        so failures are obvious from the run page without opening any logs.

    The older 2-part name (_overview.<configuration>.<platform>.csv, no version) is still
    understood and bucketed under the "latest" column.
#>

$logsPath       = Join-Path (Get-Location).Path "_logs"
$reportFileName = '_overview.all'

if (-not (Test-Path $logsPath)) {
    Write-Warning "No _logs directory found at $logsPath - nothing to report."
    return
}

# --- Load every per-job CSV ---------------------------------------------------
# data[sample][tag][combo] = status
$data       = @{}
$allSamples = [System.Collections.Generic.SortedSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
$tagSet     = [System.Collections.Generic.HashSet[string]]::new()

Get-ChildItem -Path $logsPath -Filter '_overview.*.csv' |
    Where-Object { $_.Name -notlike '_overview.all.*' } |
    ForEach-Object {
        # _overview.<tag>.<config>.<platform>  ->  drop _overview; last two are config/platform.
        $parts = [IO.Path]::GetFileNameWithoutExtension($_.Name).Split('.')
        $parts = $parts[1..($parts.Count - 1)]          # drop the leading "_overview"
        if ($parts.Count -ge 3) { $tag = ($parts[0..($parts.Count - 3)] -join '.') }
        else                    { $tag = 'latest' }
        [void]$tagSet.Add($tag)

        Import-Csv -Path $_.FullName | ForEach-Object {
            $sample = $_.Sample
            if (-not $sample) { return }
            [void]$allSamples.Add($sample)
            if (-not $data.ContainsKey($sample)) { $data[$sample] = @{} }
            if (-not $data[$sample].ContainsKey($tag)) { $data[$sample][$tag] = @{} }
            foreach ($col in ($_.PSObject.Properties.Name | Where-Object { $_ -ne 'Sample' })) {
                $data[$sample][$tag][$col] = "$($_.$col)".Trim()
            }
        }
    }

if ($tagSet.Count -eq 0) {
    Write-Warning "No per-job '_overview.*.csv' reports were found in $logsPath."
    return
}

# Order versions newest-first (numeric tags descending; non-numeric last)
$tags = $tagSet | Sort-Object @{ Expression = { if ($_ -match '^\d+$') { [int]$_ } else { 0 } }; Descending = $true }, @{ Expression = { $_ } }

function Get-Status {
    # Collapse one sample/version's combinations into a single status object.
    param([hashtable]$Combos)
    $c = @{ Succeeded = 0; Failed = 0; Sporadic = 0; Unsupported = 0; Excluded = 0 }
    $details = @()
    if ($Combos) {
        foreach ($k in ($Combos.Keys | Sort-Object)) {
            switch ($Combos[$k]) {
                'Succeeded' { $c.Succeeded++ } 'Failed' { $c.Failed++ } 'Sporadic' { $c.Sporadic++ }
                'Unsupported' { $c.Unsupported++ } 'Excluded' { $c.Excluded++ }
            }
            $details += "$k = $($Combos[$k])"
        }
    }
    $buildable = $c.Succeeded + $c.Failed + $c.Sporadic
    if (-not $Combos -or $Combos.Count -eq 0) { $label = 'n/a'; $klass = 'na' }
    elseif ($buildable -eq 0) { $label = '--'; $klass = 'na' }
    elseif ($c.Failed -eq 0 -and $c.Sporadic -eq 0) { $label = "PASS ($($c.Succeeded)/$buildable)"; $klass = 'pass' }
    elseif ($c.Failed -eq 0) { $label = "PASS* ($($c.Succeeded + $c.Sporadic)/$buildable)"; $klass = 'flaky' }
    elseif ($c.Failed -eq $buildable) { $label = "FAIL ($($c.Failed)/$buildable)"; $klass = 'fail' }
    else { $label = "PARTIAL ($($c.Failed) failed / $buildable)"; $klass = 'partial' }
    [pscustomobject]@{ Label = $label; Class = $klass; Tooltip = ($details -join ' | '); Counts = $c; Buildable = $buildable }
}

# --- Build per-version totals + the matrix ------------------------------------
$totals = @{}; foreach ($t in $tags) { $totals[$t] = [pscustomobject]@{ S = 0; F = 0; O = 0; U = 0; E = 0; pass = 0; flaky = 0; partial = 0; fail = 0; na = 0 } }
$failuresList = [System.Collections.ArrayList]::new()
$csvRows = @()
$bodyRows = New-Object System.Text.StringBuilder

foreach ($sample in $allSamples) {
    $csvRow = [ordered]@{ Sample = $sample }
    $cells = ''
    foreach ($t in $tags) {
        $combos = $null
        if ($data[$sample].ContainsKey($t)) { $combos = $data[$sample][$t] }
        $st = Get-Status -Combos $combos
        $tt = $totals[$t]
        $tt.S += $st.Counts.Succeeded; $tt.F += $st.Counts.Failed; $tt.O += $st.Counts.Sporadic
        $tt.U += $st.Counts.Unsupported; $tt.E += $st.Counts.Excluded
        switch ($st.Class) { 'pass' { $tt.pass++ } 'flaky' { $tt.flaky++ } 'partial' { $tt.partial++ } 'fail' { $tt.fail++ } 'na' { $tt.na++ } }
        $csvRow["$t"] = $st.Label
        $tip = [System.Web.HttpUtility]::HtmlEncode($st.Tooltip)
        $cells += "<td class='$($st.Class)' title='$tip'>$($st.Label)</td>"
        if ($combos) {
            foreach ($k in ($combos.Keys | Sort-Object)) {
                if ($combos[$k] -eq 'Failed') { [void]$failuresList.Add([pscustomobject]@{ Sample = $sample; Version = $t; Combo = $k }) }
            }
        }
    }
    $csvRows += [pscustomobject]$csvRow
    $enc = [System.Web.HttpUtility]::HtmlEncode($sample)
    [void]$bodyRows.Append("<tr><td class='sample'>$enc</td>$cells</tr>`n")
}

Add-Type -AssemblyName System.Web -ErrorAction SilentlyContinue

# --- CSV ----------------------------------------------------------------------
$csvRows | Export-Csv -Path (Join-Path $logsPath "$reportFileName.csv") -NoTypeInformation

# --- HTML (colour-coded sample x version matrix) ------------------------------
$generated = Get-Date -Format 'yyyy-MM-dd HH:mm:ss'
$sumHead = "<tr><th>_NT_TARGET_VERSION</th><th>Pass</th><th>Flaky</th><th>Partial</th><th>Fail</th><th>n/a</th><th>Combos OK</th><th>Sporadic</th><th>Failed</th><th>Excluded</th><th>Pass rate</th></tr>"
$sumRows = ''
foreach ($t in $tags) {
    $x = $totals[$t]; $tot = $x.pass + $x.flaky + $x.partial + $x.fail + $x.na; $elig = $tot - $x.na
    $rate = if ($elig -gt 0) { '{0:N0}%' -f (100.0 * ($x.pass + $x.flaky) / $elig) } else { 'n/a' }
    $sumRows += "<tr><td class='sample'>$t</td><td class='pass'>$($x.pass)</td><td class='flaky'>$($x.flaky)</td><td class='partial'>$($x.partial)</td><td class='fail'>$($x.fail)</td><td class='na'>$($x.na)</td><td>$($x.S)</td><td>$($x.O)</td><td>$($x.F)</td><td>$($x.E)</td><td><b>$rate</b></td></tr>`n"
}
$matHead = "<tr><th class='sample'>Sample</th>"
foreach ($t in $tags) { $matHead += "<th>$t</th>" }
$matHead += "</tr>"

$html = @"
<!DOCTYPE html><html lang="en"><head><meta charset="utf-8"/>
<title>WDK Driver Samples - Build Overview</title>
<style>
 body{font-family:'Segoe UI',Arial,sans-serif;margin:24px;color:#1b1b1b}
 h1{font-size:22px;margin-bottom:4px}h2{font-size:17px;margin-top:28px}
 .meta{color:#555;font-size:13px;margin-bottom:8px}
 table{border-collapse:collapse;margin-top:8px;font-size:13px}
 th,td{border:1px solid #cfcfcf;padding:5px 9px;text-align:center}
 th{background:#f0f3f7;position:sticky;top:0}
 td.sample,th.sample{text-align:left;font-family:Consolas,monospace;white-space:nowrap}
 .sub{font-weight:normal;color:#666;font-size:11px}
 .pass{background:#c8e6c9}.flaky{background:#fff59d}.partial{background:#ffcc80}.fail{background:#ef9a9a}.na{background:#eee;color:#888}
 .legend span{display:inline-block;padding:3px 9px;margin-right:6px;border:1px solid #cfcfcf;border-radius:3px;font-size:12px}
</style></head><body>
<h1>WDK Driver Samples &mdash; Build Overview</h1>
<div class="meta">Generated: $generated &nbsp;|&nbsp; columns are <b>_NT_TARGET_VERSION</b> (library link version); hover a cell for the per-combination breakdown.</div>
<div class="legend"><span class="pass">PASS</span><span class="flaky">PASS* (retry)</span><span class="partial">PARTIAL</span><span class="fail">FAIL</span><span class="na">-- n/a</span></div>
<h2>Summary by _NT_TARGET_VERSION</h2>
<table>$sumHead
$sumRows</table>
<h2>Sample &times; _NT_TARGET_VERSION</h2>
<table>$matHead
$($bodyRows.ToString())</table>
</body></html>
"@
$html | Out-File -FilePath (Join-Path $logsPath "$reportFileName.htm") -Encoding UTF8

# --- GitHub Actions run summary (Markdown) ------------------------------------
if ($env:GITHUB_STEP_SUMMARY) {
    $totalFailed = ($totals.Values | Measure-Object -Property fail -Sum).Sum + ($totals.Values | Measure-Object -Property partial -Sum).Sum
    $icon = if ($failuresList.Count -gt 0) { ':x:' } else { ':white_check_mark:' }

    $md = [System.Text.StringBuilder]::new()
    [void]$md.AppendLine("# $icon WDK Driver Samples &mdash; Build Overview")
    [void]$md.AppendLine()
    [void]$md.AppendLine("Columns are **_NT_TARGET_VERSION** (the WDK library version drivers link against). Each version was built for Debug/Release x x64/arm64.")
    [void]$md.AppendLine()
    [void]$md.AppendLine("## Summary by _NT_TARGET_VERSION")
    [void]$md.AppendLine("| _NT_TARGET_VERSION | :white_check_mark: Pass | :warning: Flaky | :large_orange_diamond: Partial | :x: Fail | :heavy_minus_sign: n/a | Pass rate |")
    [void]$md.AppendLine("|---|---:|---:|---:|---:|---:|---:|")
    foreach ($t in $tags) {
        $x = $totals[$t]; $tot = $x.pass + $x.flaky + $x.partial + $x.fail + $x.na; $elig = $tot - $x.na
        $rate = if ($elig -gt 0) { '{0:N0}%' -f (100.0 * ($x.pass + $x.flaky) / $elig) } else { 'n/a' }
        [void]$md.AppendLine("| ``$t`` | $($x.pass) | $($x.flaky) | $($x.partial) | $($x.fail) | $($x.na) | **$rate** |")
    }
    [void]$md.AppendLine()

    if ($failuresList.Count -gt 0) {
        [void]$md.AppendLine("## :x: Failures ($($failuresList.Count))")
        [void]$md.AppendLine("| Sample | _NT_TARGET_VERSION | Config/Platform |")
        [void]$md.AppendLine("|---|---|---|")
        foreach ($f in ($failuresList | Sort-Object Sample, Version, Combo)) {
            [void]$md.AppendLine("| ``$($f.Sample)`` | $($f.Version) | $($f.Combo.Replace('|','/')) |")
        }
        [void]$md.AppendLine()
        [void]$md.AppendLine("> Open the matching **build** job's summary (or the ``logs-*`` artifact) for the exact compiler error.")
    }
    else {
        [void]$md.AppendLine(":tada: **All combinations built successfully.**")
    }

    $md.ToString() | Out-File -FilePath $env:GITHUB_STEP_SUMMARY -Append -Encoding utf8
}
