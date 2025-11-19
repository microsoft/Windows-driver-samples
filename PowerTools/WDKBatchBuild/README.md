WDKBatchBuild

JakobL@2025/11/19

Build this repo in "batch mode" using multiple EWDKs. 

Once batch build is complete you will find complete copies of built repo with generated artifacts ready for your favorite diff algorithm.

Files:
* WDKBatchBuild.ps1 <-- Main script. Change bottom part to refine functionality.
* WDKBatchBuild_Internal.ps1. Auxillary function.

Tricks:
* Nifty lazy ISO file copy.  Only copies if necessary.
* Nifty lazy ISO extraction.  Caches EWDKs.  Only extracts what is necessary.
* Nifty trick to invoke build environment and build:
  * Does following N times:
    * Starts out in main PowerShell script
    * Calls cmd script EWDK\BuildEnv\SetupBuildEnv.cmd to setup build environment
    * Continuation into auxillary PowerShell script

Warning:
* Will run "clean -xdf" in root of repo thereby wiping out anything not checked in.
* Make sure to check changes to this script in before running.

Assumptions:
* Requires new pwsh (rather than old powershell).
* Hard coded in script WDKBatchBuild.ps1:
  * Repo in D:\wds\wds1
  * Input ISOs in D:\wds\ISOs
  * Temporary EWDKs extracted to D:\wds\EWDKs
  * Output runs in D:\wds\Runs
