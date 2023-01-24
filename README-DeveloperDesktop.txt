How to build locally:

Step 1: Install git and pwsh 7.3.0:
* winget install --id Microsoft.Powershell --source winget
* winget install --id Git.Git --source winget

Step 2: Create a "driver build environment".  
* There are multiple ways to achieve this.  For example install Visual Studio
* and the Windows Driver Kit.  Or for example just download and mount the 
* EWDK. In the following example that is what we will do:
* * Navigate to https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk
* * Download the Windows 11, version 22H2 EWDK
* * Download EWDK_ni_release_svc_prod1_22621_220804-1759.iso
* * Mount ISO
* * .\LaunchBuildEnv

Step 3: Clone Windows Driver Samples and checkout main branch:
cd path\to\your\repos
git clone https://github.com/microsoft/Windows-driver-samples.git
cd Windows-driver-samples

Step 4: Check all samples builds with expected results for all flavors:
pwsh
git status
.\Build-AllProjects-DeveloperDesktop.ps1

Expected output:
---
LogFilesDirectory: _logfiles
Overview: _logfiles\overview.htm
NumberOfLogicalProcessors: 12
SolutionsInParallel: 60
SolutionsTotal: 160

Shorthands

T: Total: 160
B: Built
R: Build is running currently
P: Build is pending an available build slot

S: Built and result was 'Succeeded'
E: Built and result was 'Excluded'
U: Built and result was 'Unsupported' (Platform and Configuration combination)
F: Built and result was 'Failed'

Building driver solutions...

Built solutions.

Total elapsed time:   5 minutes, 57 seconds.
SolutionsTotal:       160
SolutionsSucceeded:   145
SolutionsExcluded:    14
SolutionsUnsupported: 0
SolutionsFailed:      1

Results saved to .\SampleBuilder.htm

---


TODO:
* Integrate this file with README.md
* Integrate changes to Build-Project.ps1
*  Create a "Profile" concept. See profiles.csv
* Baseline
* Clickable links in overview.htm
* Exclusions.csv <-- Make exclusion "per profile"?
* Generated overview.xlsx
