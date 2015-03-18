//
// File History Sample Setup Tool
// Copyright (c) Microsoft Corporation.  All Rights Reserved.
//

#include <fhsetup.h>

HRESULT ScheduleBackups()
/*++

Routine Description:

    This function starts the File History service if it is stopped
    and schedules regular backups.

Arguments:

    None

Return Value:

    S_OK if successful
    HRESULT from underlying functions

--*/
{
    HRESULT backupHr = S_OK;
    HRESULT pipeHr = S_OK;
    FH_SERVICE_PIPE_HANDLE pipe = NULL;

    pipeHr = FhServiceOpenPipe(TRUE, &pipe);
    if (SUCCEEDED(pipeHr))
    {
        backupHr = FhServiceReloadConfiguration(pipe);
        pipeHr = FhServiceClosePipe(pipe);
    }

    // The HRESULT from the backup operation is more important than
    // the HRESULT from pipe operations
    return FAILED(backupHr) ? backupHr : pipeHr;
}

HRESULT ConfigureFileHistory(
    _In_ PWSTR TargetPath
    )
/*++

Routine Description:

    This function configures a target for File History.
    It will only succeed if the user has never configured File History
    before and there is no File History data on the target.

Arguments:

    TargetPath -
        The path of the File History target

Return Value:

    S_OK if successful
    E_INVALIDARG if TargetPath is NULL
    E_FAIL if configuration failed because File History is disabled by
           group policy or the target is not valid
    HRESULT from underlying functions

--*/
{
    HRESULT hr = S_OK;
    FH_BACKUP_STATUS backupStatus;
    FH_DEVICE_VALIDATION_RESULT validationResult;
    CComPtr<IFhConfigMgr> configMgr;
    CComBSTR targetPath;
    CComBSTR targetName;

    // TargetPath must not be NULL
    if (TargetPath == NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    // Copy the target path into a local variable and set the target name
    // to an empty string to allow the config manager to set a name
    _ATLTRY
    {
        targetPath = TargetPath;
        targetName = L"";
    }
    _ATLCATCH(e)
    {
        hr = e;
        goto Cleanup;
    }

    // The configuration manager is used to create and load configuration 
    // files, get/set the backup status, validate a target, etc
    hr = configMgr.CoCreateInstance(CLSID_FhConfigMgr);
    if (FAILED(hr))
    {
        wprintf(L"Error: CoCreateInstance failed (0x%X)\n", hr);
        goto Cleanup;
    }
    
    // Create a new default configuration file - do not overwrite if one
    // already exists
    wprintf(L"Creating default configuration\n");
    hr = configMgr->CreateDefaultConfiguration(FALSE);
    if (FAILED(hr))
    {
        if (hr == FHCFG_E_CONFIG_ALREADY_EXISTS)
        {
            wprintf(L"Error: File History has previously been configured\n");
        }
        else
        {
            wprintf(L"Error: CreateDefaultConfiguration failed (0x%X)\n", hr);
        }
        goto Cleanup;
    }

    // Check the backup status
    // If File History is disabled by group policy, quit
    wprintf(L"Getting backup status\n");
    hr = configMgr->GetBackupStatus(&backupStatus);
    if (FAILED(hr))
    {
        wprintf(L"Error: GetBackupStatus failed (0x%X)\n", hr);
        goto Cleanup;
    }
    if (backupStatus == FH_STATUS_DISABLED_BY_GP)
    {
        wprintf(L"Error: File History is disabled by group policy\n");
        hr = E_FAIL;
        goto Cleanup;
    }

    // Make sure the target is valid to be used for File History
    wprintf(L"Validating target\n");
    hr = configMgr->ValidateTarget(targetPath, &validationResult);
    if (FAILED(hr))
    {
        wprintf(L"Error: ValidateTarget failed (0x%X)\n", hr);
        goto Cleanup;
    }
    if (validationResult != FH_VALID_TARGET)
    {
        // If the target is inaccessible, read-only, an invalid drive type
        // (such as a CD), already being used for File History, or part of
        // the protected namespace - don't enable File History
        wprintf(L"Error: %ws is not a valid target\n", targetPath.m_str);
        hr = E_FAIL;
        goto Cleanup;
    }

    // Provision the target to be used for File History and set
    // it as the default target
    wprintf(L"Provisioning and setting target\n");
    hr = configMgr->ProvisionAndSetNewTarget(targetPath, targetName);
    if (FAILED(hr))
    {
        wprintf(L"Error: ProvisionAndSetNewTarget failed (0x%X)\n", hr);
        goto Cleanup;
    }
    
    // Enable File History
    wprintf(L"Enabling File History\n");
    hr = configMgr->SetBackupStatus(FH_STATUS_ENABLED);
    if (FAILED(hr))
    {
        wprintf(L"Error: SetBackupStatus failed (0x%X)\n", hr);
        goto Cleanup;
    }

    // Save the configuration to disk
    wprintf(L"Saving configuration\n");
    hr = configMgr->SaveConfiguration();
    if (FAILED(hr))
    {
        wprintf(L"Error: SaveConfiguration failed (0x%X)\n", hr);
        goto Cleanup;
    }

    // Tell the File History service to schedule backups
    wprintf(L"Scheduling regular backups\n");
    hr = ScheduleBackups();
    if (FAILED(hr))
    {
        wprintf(L"Error: ScheduleBackups failed (0x%X)\n", hr);
        goto Cleanup;
    }

    // Recommend the target to other Homegroup members
    wprintf(L"Recommending target to Homegroup\n");
    HRESULT hrRecommend = configMgr->ChangeDefaultTargetRecommendation(TRUE);
    if (FAILED(hrRecommend))
    {
        wprintf(L"Warning: Failed to recommend target to Homegroup (0x%X)\n", hrRecommend);
    }

    wprintf(L"Success! File History is now enabled\n");

Cleanup:
    return hr;
}

int __cdecl wmain(
    _In_ int Argc,
    _In_reads_(Argc) PWSTR Argv[]
    )
/*++

Routine Description:

    This is the main entry point of the console application.

Arguments:

    Argc - the number of command line arguments
    Argv - command line arguments

Return Value:

    exit code

--*/
{
    HRESULT hr = S_OK;
    BOOL comInitialized = FALSE;

    wprintf(L"\nFile History Sample Setup Tool\n");
    wprintf(L"Copyright (C) Microsoft Corporation. All rights reserved.\n\n");

    // If there are fewer than 2 command-line arguments, print the correct
    // usage and exit
    if (Argc < 2)
    {
        wprintf(L"Usage: fhsetup <path>\n\n");
        wprintf(L"Examples:\n");
        wprintf(L"    fhsetup D:\\\n");
        wprintf(L"    fhsetup \\\\server\\share\\\n\n");
        goto Cleanup;
    }

    // COM is needed to use the Config Manager
    wprintf(L"Initializing COM...\n");
    hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
        wprintf(L"Error: CoInitialize failed (0x%X)\n", hr);
        goto Cleanup;
    }
    comInitialized = TRUE;

    hr = ConfigureFileHistory(Argv[1]);
    if (FAILED(hr))
    {
        wprintf(L"File History configuration failed (0x%X)\n", hr);
        goto Cleanup;
    }

Cleanup:
    // If COM was initialized, make sure it is uninitialized
    if (comInitialized)
    {
        CoUninitialize();
        comInitialized = FALSE;
    }

    return 0;
}
