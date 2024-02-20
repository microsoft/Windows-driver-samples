$folderPath = "..\.."

# Get all .sln files recursively
$slnFiles = Get-ChildItem -Path $folderPath -Filter "*.sln" -Recurse

foreach ($slnFile in $slnFiles) {
    # Read the contents of the .sln file
    $slnContent = Get-Content -Path $slnFile.FullName

    # Check if Debug|ARM64 configuration already exists
    $debugArm64Exists = $slnContent -match "Debug\|ARM64"

    if (-not $debugArm64Exists) {
        $slnNewContent = ""
        $ids = @()
        $projectFiles = @{}
        $inNestedProjects = $false
        foreach ($line in $slnContent) {
            $line = $line.TrimEnd()
            # Find <ID> if line matches Project("<GUID>") = "<string>", "<path>", "<ID>"
            if ($line -match "Project\(`"(.*)`"\) = `"(.*?)`", `"(.*?)`", `"(.*?)`"") {
                # Get <ID> and add it to the array
                $id = $matches[4]
                $projectFile = $matches[3]
                $ids += $id
                $projectFiles.Add($id, $projectFile)
            }
            if ($inNestedProjects) {
                if ($line -match "EndGlobalSection") {
                    $inNestedProjects = $false
                } else {
                    # Find <childId> if line matches "<parentId> = <childId>" and remove it from the array
                    if ($line -match "(.*) = (.*)") {
                        $childId = $matches[2]
                        $ids = $ids -ne $childId
                    }
                }
            }
            # Find line in $slnContent where "GlobalSection(SolutionConfigurationPlatforms) = preSolution" exists
            if ($line -match "GlobalSection\(NestedProjects\) = preSolution") {
                $inNestedProjects = $true
            }
        }
        if ($ids.Count -eq 0) {
            continue
        }
        foreach ($line in $slnContent) {
            $line = $line.TrimEnd()
            $slnNewContent += "$line`r`n"
            # Find line in $slnContent where "GlobalSection(SolutionConfigurationPlatforms) = preSolution" exists
            if ($line -match "GlobalSection\(SolutionConfigurationPlatforms\) = preSolution") {
                # Append text after that line
                $slnNewContent += "`t`tDebug|ARM64 = Debug|ARM64`r`n"
                $slnNewContent += "`t`tRelease|ARM64 = Release|ARM64`r`n"
            }
            # Add new configuration to "GlobalSection(ProjectConfigurationPlatforms) = postSolution"
            if ($line -match "GlobalSection\(ProjectConfigurationPlatforms\) = postSolution") {
                $configurations = @("Debug", "Release")
                foreach ($configuration in $configurations)
                {
                    foreach ($id in $ids) {
                        # Append text after that line
                        $slnNewContent += "`t`t$id.$configuration|ARM64.ActiveCfg = $configuration|ARM64`r`n"
                        # Ignore some projects that were intentionally not build for ARM64
                        if ($id -eq '{50458BE9-A423-44C1-AF44-21D4B5233063}') { # 60\netvmini60.vcxproj; NDIS version not supported
                            continue
                        }
                        if ($id -eq '{062B87F8-6021-4BE2-B677-3AEF2456CBA0}') { # 620\netvmini620.vcxproj; NDIS version not supported
                            continue
                        }
                        if ($id -eq '{65A3C0DB-248E-4365-83D2-E4DE6E764C6C}') { # SampleDSM.vcxproj; mpio.lib not supported
                            continue
                        }
                        $slnNewContent += "`t`t$id.$configuration|ARM64.Build.0 = $configuration|ARM64`r`n"
                        #$slnNewContent += "`t`t$id.$configuration|ARM64.Deploy.0 = $configuration|ARM64`r`n"
                    }
                }
            }
        }
        # Write the updated contents back to the .sln file
        $slnNewContent | Set-Content -Path $slnFile.FullName -NoNewline
        #$slnNewContent | Set-Content -Path "test.sln"
    
        foreach ($id in $ids) {
            $projectFile = $projectFiles[$id]
            $projectFilePath = Join-Path -Path $slnFile.DirectoryName -ChildPath $projectFile
            $projectContent = Get-Content -Path $projectFilePath
            $projectNewContent = ""
            # Check if Debug|ARM64 configuration already exists
            $debugArm64Exists = $projectContent -match "<ProjectConfiguration Include=`"Debug\|ARM64`">"
            # Override for testing
            #$debugArm64Exists = $false
            if (-not $debugArm64Exists)
            {
                $insideGroup = $false
                $noEntryPoint = $false
                $groupType = ""
                foreach ($line in $projectContent) {
                    $line = $line.TrimEnd()
                    if ([string]::IsNullOrWhiteSpace($line))
                    {
                        continue
                    }
                    $projectNewContent += "$line`r`n"
                    if ($line -match "<ItemGroup .*Label=`"ProjectConfigurations`".*>") {
                        $projectNewContent += "    <ProjectConfiguration Include=`"Debug|ARM64`">`r`n"
                        $projectNewContent += "      <Configuration>Debug</Configuration>`r`n"
                        $projectNewContent += "      <Platform>ARM64</Platform>`r`n"
                        $projectNewContent += "    </ProjectConfiguration>`r`n"
                        $projectNewContent += "    <ProjectConfiguration Include=`"Release|ARM64`">`r`n"
                        $projectNewContent += "      <Configuration>Release</Configuration>`r`n"
                        $projectNewContent += "      <Platform>ARM64</Platform>`r`n"
                        $projectNewContent += "    </ProjectConfiguration>`r`n"
                    }
                    if ($insideGroup)
                    {
                        # Remove BufferSecurityCheck from ARM64 configurations if no EntryPoint is defined
                        if ($line -match "<ControlFlowGuard>false</ControlFlowGuard>" -and $noEntryPoint)
                        {
                            $newItemDefinitionGroupContent += "      <BufferSecurityCheck>false</BufferSecurityCheck>`r`n"
                            $noEntryPoint = $false
                        }
                        if ($line -match "<NoEntryPoint>true</NoEntryPoint>") {
                            $noEntryPoint = $true
                        }
                        # Remove "/CETCOMPAT" linker option from ARM64 configurations
                        $line = $line.Replace('/CETCOMPAT', '')
                        $line = $line.Replace('<CETCompat>true</CETCompat>', '<CETCompat>false</CETCompat>')
                        # Adjust NDIS version
                        if ($line -match '<PreprocessorDefinitions>' -and $line -notmatch 'NDIS6[3-9]\d')
                        {
                            $line = $line -replace 'NDIS6[0-2]\d*', 'NDIS630'
                            $line = $line -replace 'NDIS_SUPPORT_NDIS6[0-2]\d*', 'NDIS_SUPPORT_NDIS630'
                            $line = $line -replace 'NDIS_SUPPORT_NDIS6[^\d]', 'NDIS_SUPPORT_NDIS630'
                        }
                        # Adjust ARM64 specific paths
                        if ($line -match '<(ReferencePath|LibraryPath|ExcludePath)>')
                        {
                            $line = $line -replace 'x64', 'ARM64'
                        }
                        # Adjust Inf2Cat version
                        if ($line -match "<Inf2CatWindowsVersionList>")
                        {
                            $line = $line -replace '10_([A-Z]+_)?x64', '10_NI_ARM64'
                        }
                        # Add missing condition for ARM64 in usbview sample
                        $line = $line.Replace("<BaseAddress Condition=`"'`$(Platform)'=='x64'`">", "<BaseAddress Condition=`"'`$(Platform)'=='x64' or '`$(Platform)'=='ARM64'`">")
                        $line = $line.Replace("<BaseAddress Condition=`"!('`$(Platform)'=='x64')`">", "<BaseAddress Condition=`"!('`$(Platform)'=='x64' or '`$(Platform)'=='ARM64')`">")
                        $newItemDefinitionGroupContent += $line + "`r`n"
                        if ($line -match "</$groupType>") {
                            $insideGroup = $false
                            $projectNewContent += $newItemDefinitionGroupContent
                        }
                    }
                    if ($line -match '<(ItemDefinitionGroup|PropertyGroup|ImportGroup) .*Condition=".*==.(.*)\|x64.".*>') {
                        $groupType = $matches[1]
                        #$configuration = $matches[2]
                        $insideGroup = $true
                        $noEntryPoint = $false
                        $newItemDefinitionGroupContent = $line.Replace('x64', 'ARM64') + "`r`n"
                    }
                }
                $projectNewContent | Set-Content -Path $projectFilePath -NoNewline
                #$projectNewContent | Set-Content -Path "test.vcxproj"
            }
        }
    }
}
