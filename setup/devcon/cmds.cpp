/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    devcon.cpp

Abstract:

    Device Console
    command-line interface for managing devices

--*/

#include "devcon.h"

struct GenericContext {
    DWORD count;
    DWORD control;
    BOOL  reboot;
    LPCTSTR strSuccess;
    LPCTSTR strReboot;
    LPCTSTR strFail;
};

#define FIND_DEVICE         0x00000001 // display device
#define FIND_STATUS         0x00000002 // display status of device
#define FIND_RESOURCES      0x00000004 // display resources of device
#define FIND_DRIVERFILES    0x00000008 // display drivers used by device
#define FIND_HWIDS          0x00000010 // display hw/compat id's used by device
#define FIND_DRIVERNODES    0x00000020 // display driver nodes for a device.
#define FIND_CLASS          0x00000040 // display device's setup class
#define FIND_STACK          0x00000080 // display device's driver-stack

struct SetHwidContext {
    int argc_right;
    LPTSTR * argv_right;
    DWORD prop;
    int skipped;
    int modified;
};

int cmdHelp(_In_ LPCTSTR BaseName, _In_opt_ LPCTSTR Machine, _In_ DWORD Flags, _In_ int argc, _In_reads_(argc) PTSTR argv[])
/*++

Routine Description:

    HELP command
    allow HELP or HELP <command>

Arguments:

    BaseName  - name of executable
    Machine   - if non-NULL, remote machine (ignored)
    argc/argv - remaining parameters

Return Value:

    EXIT_xxxx

--*/
{
    DWORD helptext = 0;
    int dispIndex;
    LPCTSTR cmd = NULL;
    BOOL unknown = FALSE;

    UNREFERENCED_PARAMETER(Machine);
    UNREFERENCED_PARAMETER(Flags);

    if(argc) {
        //
        // user passed in a command for help on... long help
        //
        for(dispIndex = 0;DispatchTable[dispIndex].cmd;dispIndex++) {
            if(_tcsicmp(argv[0],DispatchTable[dispIndex].cmd)==0) {
                cmd = DispatchTable[dispIndex].cmd;
                helptext = DispatchTable[dispIndex].longHelp;
                break;
            }
        }
        if(!cmd) {
            unknown = TRUE;
            cmd = argv[0];
        }
    }

    if(helptext) {
        //
        // long help
        //
        FormatToStream(stdout,helptext,BaseName,cmd);
    } else {
        //
        // help help
        //
        FormatToStream(stdout,unknown ? MSG_HELP_OTHER : MSG_HELP_LONG,BaseName,cmd);
        //
        // enumerate through each command and display short help for each
        //
        for(dispIndex = 0;DispatchTable[dispIndex].cmd;dispIndex++) {
            if(DispatchTable[dispIndex].shortHelp) {
                FormatToStream(stdout,DispatchTable[dispIndex].shortHelp,DispatchTable[dispIndex].cmd);
                fputs("\n",stdout);
            }
        }
    }
    return EXIT_OK;
}

int cmdClasses(_In_ LPCTSTR BaseName, _In_opt_ LPCTSTR Machine, _In_ DWORD Flags, _In_ int argc, _In_reads_(argc) PTSTR argv[])
/*++

Routine Description:

    CLASSES command
    lists classes on (optionally) specified machine
    format as <name>: <destination>

Arguments:

    BaseName  - name of executable
    Machine   - if non-NULL, remote machine
    argc/argv - remaining parameters - ignored

Return Value:

    EXIT_xxxx

--*/
{
    DWORD reqGuids = 128;
    DWORD numGuids;
    LPGUID guids = NULL;
    DWORD index;
    int failcode = EXIT_FAIL;

    UNREFERENCED_PARAMETER(BaseName);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    guids = new GUID[reqGuids];
    if(!guids) {
        goto final;
    }
    if(!SetupDiBuildClassInfoListEx(0,guids,reqGuids,&numGuids,Machine,NULL)) {
        do {
            if(GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
                goto final;
            }
            delete [] guids;
            reqGuids = numGuids;
            guids = new GUID[reqGuids];
            if(!guids) {
                goto final;
            }
        } while(!SetupDiBuildClassInfoListEx(0,guids,reqGuids,&numGuids,Machine,NULL));
    }
    FormatToStream(stdout,Machine?MSG_CLASSES_HEADER:MSG_CLASSES_HEADER_LOCAL,numGuids,Machine);
    for(index=0;index<numGuids;index++) {
        TCHAR className[MAX_CLASS_NAME_LEN];
        TCHAR classDesc[LINE_LEN];
        if(!SetupDiClassNameFromGuidEx(&guids[index],className,MAX_CLASS_NAME_LEN,NULL,Machine,NULL)) {
            if (FAILED(StringCchCopy(className,MAX_CLASS_NAME_LEN,TEXT("?")))) {
                goto final;
            }
        }
        if(!SetupDiGetClassDescriptionEx(&guids[index],classDesc,LINE_LEN,NULL,Machine,NULL)) {
            if (FAILED(StringCchCopy(classDesc,LINE_LEN,className))) {
                goto final;
            }
        }
        _tprintf(TEXT("%-20s: %s\n"),className,classDesc);
    }

    failcode = EXIT_OK;

final:

    if(guids) {
        delete [] guids;
    }
    return failcode;
}

int cmdListClass(_In_ LPCTSTR BaseName, _In_opt_ LPCTSTR Machine, _In_ DWORD Flags, _In_ int argc, _In_reads_(argc) PTSTR argv[])
/*++

Routine Description:

    LISTCLASS <name>....
    lists all devices for each specified class
    there can be more than one physical class for a class name (shouldn't be
    though) in such cases, list each class
    if machine given, list devices for that machine

Arguments:

    BaseName  - name of executable
    Machine   - if non-NULL, remote machine
    argc/argv - remaining parameters - list of class names

Return Value:

    EXIT_xxxx

--*/
{
    DWORD reqGuids = 16;
    int argIndex;
    int failcode = EXIT_FAIL;
    LPGUID guids = NULL;
    HDEVINFO devs = INVALID_HANDLE_VALUE;

    UNREFERENCED_PARAMETER(BaseName);
    UNREFERENCED_PARAMETER(Flags);

    if(!argc) {
        return EXIT_USAGE;
    }

    guids = new GUID[reqGuids];
    if(!guids) {
        goto final;
    }

    for(argIndex = 0;argIndex<argc;argIndex++) {
        DWORD numGuids;
        DWORD index;

        if(!(argv[argIndex] && argv[argIndex][0])) {
            continue;
        }

        //
        // there could be one to many name to GUID mapping
        //
        while(!SetupDiClassGuidsFromNameEx(argv[argIndex],guids,reqGuids,&numGuids,Machine,NULL)) {
            if(GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
                goto final;
            }
            delete [] guids;
            reqGuids = numGuids;
            guids = new GUID[reqGuids];
            if(!guids) {
                goto final;
            }
        }
        if(numGuids == 0) {
            FormatToStream(stdout,Machine?MSG_LISTCLASS_NOCLASS:MSG_LISTCLASS_NOCLASS_LOCAL,argv[argIndex],Machine);
            continue;
        }
        for(index = 0;index<numGuids;index++) {
            TCHAR className[MAX_CLASS_NAME_LEN];
            TCHAR classDesc[LINE_LEN];
            DWORD devCount = 0;
            SP_DEVINFO_DATA devInfo;
            DWORD devIndex;

            devs = SetupDiGetClassDevsEx(&guids[index],NULL,NULL,DIGCF_PRESENT,NULL,Machine,NULL);
            if(devs != INVALID_HANDLE_VALUE) {
                //
                // count number of devices
                //
                devInfo.cbSize = sizeof(devInfo);
                while(SetupDiEnumDeviceInfo(devs,devCount,&devInfo)) {
                    devCount++;
                }
            }

            if(!SetupDiClassNameFromGuidEx(&guids[index],className,MAX_CLASS_NAME_LEN,NULL,Machine,NULL)) {
                if (FAILED(StringCchCopy(className,MAX_CLASS_NAME_LEN,TEXT("?")))) {
                    goto final;
                }
            }
            if(!SetupDiGetClassDescriptionEx(&guids[index],classDesc,LINE_LEN,NULL,Machine,NULL)) {
                if (FAILED(StringCchCopy(classDesc,LINE_LEN,className))) {
                    goto final;
                }
            }

            //
            // how many devices?
            //
            if (!devCount) {
                FormatToStream(stdout,Machine?MSG_LISTCLASS_HEADER_NONE:MSG_LISTCLASS_HEADER_NONE_LOCAL,className,classDesc,Machine);
            } else {
                FormatToStream(stdout,Machine?MSG_LISTCLASS_HEADER:MSG_LISTCLASS_HEADER_LOCAL,devCount,className,classDesc,Machine);
                for(devIndex=0;SetupDiEnumDeviceInfo(devs,devIndex,&devInfo);devIndex++) {
                    DumpDevice(devs,&devInfo);
                }
            }
            if(devs != INVALID_HANDLE_VALUE) {
                SetupDiDestroyDeviceInfoList(devs);
                devs = INVALID_HANDLE_VALUE;
            }
        }
    }

    failcode = 0;

final:

    if(guids) {
        delete [] guids;
    }

    if(devs != INVALID_HANDLE_VALUE) {
        SetupDiDestroyDeviceInfoList(devs);
    }

    return failcode;
}

int FindCallback(_In_ HDEVINFO Devs, _In_ PSP_DEVINFO_DATA DevInfo, _In_ DWORD Index, _In_ LPVOID Context)
/*++

Routine Description:

    Callback for use by Find/FindAll
    just simply display the device

Arguments:

    Devs    )_ uniquely identify the device
    DevInfo )
    Index    - index of device
    Context  - GenericContext

Return Value:

    EXIT_xxxx

--*/
{
    GenericContext *pFindContext = (GenericContext*)Context;

    UNREFERENCED_PARAMETER(Index);

    if(!pFindContext->control) {
        DumpDevice(Devs,DevInfo);
        pFindContext->count++;
        return EXIT_OK;
    }
    if(!DumpDeviceWithInfo(Devs,DevInfo,NULL)) {
        return EXIT_OK;
    }
    if(pFindContext->control&FIND_DEVICE) {
        DumpDeviceDescr(Devs,DevInfo);
    }
    if(pFindContext->control&FIND_CLASS) {
        DumpDeviceClass(Devs,DevInfo);
    }
    if(pFindContext->control&FIND_STATUS) {
        DumpDeviceStatus(Devs,DevInfo);
    }
    if(pFindContext->control&FIND_RESOURCES) {
        DumpDeviceResources(Devs,DevInfo);
    }
    if(pFindContext->control&FIND_DRIVERFILES) {
        DumpDeviceDriverFiles(Devs,DevInfo);
    }
    if(pFindContext->control&FIND_STACK) {
        DumpDeviceStack(Devs,DevInfo);
    }
    if(pFindContext->control&FIND_HWIDS) {
        DumpDeviceHwIds(Devs,DevInfo);
    }
    if (pFindContext->control&FIND_DRIVERNODES) {
        DumpDeviceDriverNodes(Devs,DevInfo);
    }
    pFindContext->count++;
    return EXIT_OK;
}

int cmdFind(_In_ LPCTSTR BaseName, _In_opt_ LPCTSTR Machine, _In_ DWORD Flags, _In_ int argc, _In_reads_(argc) PTSTR argv[])
/*++

Routine Description:

    FIND <id> ...
    use EnumerateDevices to do hardwareID matching
    for each match, dump to stdout
    note that we only enumerate present devices

Arguments:

    BaseName  - name of executable
    Machine   - if non-NULL, remote machine
    argc/argv - remaining parameters - passed into EnumerateDevices

Return Value:

    EXIT_xxxx

--*/
{
    GenericContext context;
    int failcode;

    UNREFERENCED_PARAMETER(Flags);

    if(!argc) {
        return EXIT_USAGE;
    }

    context.count = 0;
    context.control = 0;
    failcode = EnumerateDevices(BaseName,Machine,DIGCF_PRESENT,argc,argv,FindCallback,&context);

    if(failcode == EXIT_OK) {

        if(!context.count) {
            FormatToStream(stdout,Machine?MSG_FIND_TAIL_NONE:MSG_FIND_TAIL_NONE_LOCAL,Machine);
        } else {
            FormatToStream(stdout,Machine?MSG_FIND_TAIL:MSG_FIND_TAIL_LOCAL,context.count,Machine);
        }
    }
    return failcode;
}

int cmdFindAll(_In_ LPCTSTR BaseName, _In_opt_ LPCTSTR Machine, _In_ DWORD Flags, _In_ int argc, _In_reads_(argc) PTSTR argv[])
/*++

Routine Description:

    FINDALL <id> ...
    use EnumerateDevices to do hardwareID matching
    for each match, dump to stdout
    like find, but also show not-present devices

Arguments:

    BaseName  - name of executable
    Machine   - if non-NULL, remote machine
    argc/argv - remaining parameters - passed into EnumerateDevices

Return Value:

    EXIT_xxxx

--*/
{
    GenericContext context;
    int failcode;

    UNREFERENCED_PARAMETER(Flags);

    if(!argc) {
        return EXIT_USAGE;
    }

    context.count = 0;
    context.control = 0;
    failcode = EnumerateDevices(BaseName,Machine,0,argc,argv,FindCallback,&context);

    if(failcode == EXIT_OK) {

        if(!context.count) {
            FormatToStream(stdout,Machine?MSG_FIND_TAIL_NONE:MSG_FIND_TAIL_NONE_LOCAL,Machine);
        } else {
            FormatToStream(stdout,Machine?MSG_FIND_TAIL:MSG_FIND_TAIL_LOCAL,context.count,Machine);
        }
    }
    return failcode;
}

int cmdStatus(_In_ LPCTSTR BaseName, _In_opt_ LPCTSTR Machine, _In_ DWORD Flags, _In_ int argc, _In_reads_(argc) PTSTR argv[])
/*++

Routine Description:

    STATUS <id> ...
    use EnumerateDevices to do hardwareID matching
    for each match, dump status to stdout
    note that we only enumerate present devices

Arguments:

    BaseName  - name of executable
    Machine   - if non-NULL, remote machine
    argc/argv - remaining parameters - passed into EnumerateDevices

Return Value:

    EXIT_xxxx

--*/
{
    GenericContext context;
    int failcode;

    UNREFERENCED_PARAMETER(Flags);

    if(!argc) {
        return EXIT_USAGE;
    }

    context.count = 0;
    context.control = FIND_DEVICE | FIND_STATUS;
    failcode = EnumerateDevices(BaseName,Machine,DIGCF_PRESENT,argc,argv,FindCallback,&context);

    if(failcode == EXIT_OK) {

        if(!context.count) {
            FormatToStream(stdout,Machine?MSG_FIND_TAIL_NONE:MSG_FIND_TAIL_NONE_LOCAL,Machine);
        } else {
            FormatToStream(stdout,Machine?MSG_FIND_TAIL:MSG_FIND_TAIL_LOCAL,context.count,Machine);
        }
    }
    return failcode;
}


int cmdResources(_In_ LPCTSTR BaseName, _In_opt_ LPCTSTR Machine, _In_ DWORD Flags, _In_ int argc, _In_reads_(argc) PTSTR argv[])
/*++

Routine Description:

    RESOURCES <id> ...
    use EnumerateDevices to do hardwareID matching
    for each match, dump resources to stdout
    note that we only enumerate present devices

Arguments:

    BaseName  - name of executable
    Machine   - if non-NULL, remote machine
    argc/argv - remaining parameters - passed into EnumerateDevices

Return Value:

    EXIT_xxxx

--*/
{
    GenericContext context;
    int failcode;

    UNREFERENCED_PARAMETER(Flags);

    if(!argc) {
        return EXIT_USAGE;
    }

    context.count = 0;
    context.control = FIND_DEVICE | FIND_RESOURCES;
    failcode = EnumerateDevices(BaseName,Machine,DIGCF_PRESENT,argc,argv,FindCallback,&context);

    if(failcode == EXIT_OK) {

        if(!context.count) {
            FormatToStream(stdout,Machine?MSG_FIND_TAIL_NONE:MSG_FIND_TAIL_NONE_LOCAL,Machine);
        } else {
            FormatToStream(stdout,Machine?MSG_FIND_TAIL:MSG_FIND_TAIL_LOCAL,context.count,Machine);
        }
    }
    return failcode;
}


int cmdDriverFiles(_In_ LPCTSTR BaseName, _In_opt_ LPCTSTR Machine, _In_ DWORD Flags, _In_ int argc, _In_reads_(argc) PTSTR argv[])
/*++

Routine Description:

    STATUS <id> ...
    use EnumerateDevices to do hardwareID matching
    for each match, dump driver files to stdout
    note that we only enumerate present devices

Arguments:

    BaseName  - name of executable
    Machine   - if non-NULL, remote machine
    argc/argv - remaining parameters - passed into EnumerateDevices

Return Value:

    EXIT_xxxx

--*/
{
    GenericContext context;
    int failcode;

    UNREFERENCED_PARAMETER(Flags);

    if(!argc) {
        return EXIT_USAGE;
    }
    if(Machine) {
        //
        // must be local machine as we need to involve class/co installers (FIND_DRIVERFILES)
        //
        return EXIT_USAGE;
    }

    context.count = 0;
    context.control = FIND_DEVICE | FIND_DRIVERFILES;
    failcode = EnumerateDevices(BaseName,Machine,DIGCF_PRESENT,argc,argv,FindCallback,&context);

    if(failcode == EXIT_OK) {

        if(!context.count) {
            FormatToStream(stdout,Machine?MSG_FIND_TAIL_NONE:MSG_FIND_TAIL_NONE_LOCAL,Machine);
        } else {
            FormatToStream(stdout,Machine?MSG_FIND_TAIL:MSG_FIND_TAIL_LOCAL,context.count,Machine);
        }
    }
    return failcode;
}

int cmdDriverNodes(_In_ LPCTSTR BaseName, _In_opt_ LPCTSTR Machine, _In_ DWORD Flags, _In_ int argc, _In_reads_(argc) PTSTR argv[])
/*++

Routine Description:

    STATUS <id> ...
    use EnumerateDevices to do hardwareID matching
    for each match, dump drivernodes to stdout
    note that we only enumerate present devices

Arguments:

    BaseName  - name of executable
    Machine   - if non-NULL, remote machine
    argc/argv - remaining parameters - passed into EnumerateDevices

Return Value:

    EXIT_xxxx

--*/
{
    GenericContext context;
    int failcode;

    UNREFERENCED_PARAMETER(Flags);

    if(!argc) {
        return EXIT_USAGE;
    }
    if(Machine) {
        //
        // must be local machine as we need to involve class/co installers (FIND_DRIVERNODES)
        //
        return EXIT_USAGE;
    }

    context.count = 0;
    context.control = FIND_DEVICE | FIND_DRIVERNODES;
    failcode = EnumerateDevices(BaseName,Machine,DIGCF_PRESENT,argc,argv,FindCallback,&context);

    if(failcode == EXIT_OK) {

        if(!context.count) {
            FormatToStream(stdout,Machine?MSG_FIND_TAIL_NONE:MSG_FIND_TAIL_NONE_LOCAL,Machine);
        } else {
            FormatToStream(stdout,Machine?MSG_FIND_TAIL:MSG_FIND_TAIL_LOCAL,context.count,Machine);
        }
    }
    return failcode;
}

int cmdHwIds(_In_ LPCTSTR BaseName, _In_opt_ LPCTSTR Machine, _In_ DWORD Flags, _In_ int argc, _In_reads_(argc) PTSTR argv[])
/*++

Routine Description:

    HWIDS <id> ...
    use EnumerateDevices to do hardwareID matching
    for each match, dump hw/compat id's to stdout
    note that we only enumerate present devices

Arguments:

    BaseName  - name of executable
    Machine   - if non-NULL, remote machine
    argc/argv - remaining parameters - passed into EnumerateDevices

Return Value:

    EXIT_xxxx

--*/
{
    GenericContext context;
    int failcode;

    UNREFERENCED_PARAMETER(Flags);

    if(!argc) {
        return EXIT_USAGE;
    }

    context.count = 0;
    context.control = FIND_DEVICE | FIND_HWIDS;
    failcode = EnumerateDevices(BaseName,Machine,DIGCF_PRESENT,argc,argv,FindCallback,&context);

    if(failcode == EXIT_OK) {

        if(!context.count) {
            FormatToStream(stdout,Machine?MSG_FIND_TAIL_NONE:MSG_FIND_TAIL_NONE_LOCAL,Machine);
        } else {
            FormatToStream(stdout,Machine?MSG_FIND_TAIL:MSG_FIND_TAIL_LOCAL,context.count,Machine);
        }
    }
    return failcode;
}

int cmdStack(_In_ LPCTSTR BaseName, _In_opt_ LPCTSTR Machine, _In_ DWORD Flags, _In_ int argc, _In_reads_(argc) PTSTR argv[])
/*++

Routine Description:

    STACK <id> ...
    use EnumerateDevices to do hardwareID matching
    for each match, dump device class and stack to stdout
    note that we only enumerate present devices

Arguments:

    BaseName  - name of executable
    Machine   - if non-NULL, remote machine
    argc/argv - remaining parameters - passed into EnumerateDevices

Return Value:

    EXIT_xxxx

--*/
{
    GenericContext context;
    int failcode;

    UNREFERENCED_PARAMETER(Flags);

    if(!argc) {
        return EXIT_USAGE;
    }

    context.count = 0;
    context.control = FIND_DEVICE | FIND_CLASS | FIND_STACK;
    failcode = EnumerateDevices(BaseName,Machine,DIGCF_PRESENT,argc,argv,FindCallback,&context);

    if(failcode == EXIT_OK) {

        if(!context.count) {
            FormatToStream(stdout,Machine?MSG_FIND_TAIL_NONE:MSG_FIND_TAIL_NONE_LOCAL,Machine);
        } else {
            FormatToStream(stdout,Machine?MSG_FIND_TAIL:MSG_FIND_TAIL_LOCAL,context.count,Machine);
        }
    }
    return failcode;
}




int ControlCallback(_In_ HDEVINFO Devs, _In_ PSP_DEVINFO_DATA DevInfo, _In_ DWORD Index, _In_ LPVOID Context)
/*++

Routine Description:

    Callback for use by Enable/Disable/Restart
    Invokes DIF_PROPERTYCHANGE with correct parameters
    uses SetupDiCallClassInstaller so cannot be done for remote devices
    Don't use CM_xxx API's, they bypass class/co-installers and this is bad.

    In Enable case, we try global first, and if still disabled, enable local

Arguments:

    Devs    )_ uniquely identify the device
    DevInfo )
    Index    - index of device
    Context  - GenericContext

Return Value:

    EXIT_xxxx

--*/
{
    SP_PROPCHANGE_PARAMS pcp;
    GenericContext *pControlContext = (GenericContext*)Context;
    SP_DEVINSTALL_PARAMS devParams;

    UNREFERENCED_PARAMETER(Index);

    switch(pControlContext->control) {
        case DICS_ENABLE:
            //
            // enable both on global and config-specific profile
            // do global first and see if that succeeded in enabling the device
            // (global enable doesn't mark reboot required if device is still
            // disabled on current config whereas vice-versa isn't true)
            //
            pcp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
            pcp.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
            pcp.StateChange = pControlContext->control;
            pcp.Scope = DICS_FLAG_GLOBAL;
            pcp.HwProfile = 0;
            //
            // don't worry if this fails, we'll get an error when we try config-
            // specific.
            if(SetupDiSetClassInstallParams(Devs,DevInfo,&pcp.ClassInstallHeader,sizeof(pcp))) {
               SetupDiCallClassInstaller(DIF_PROPERTYCHANGE,Devs,DevInfo);
            }
            //
            // now enable on config-specific
            //
            pcp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
            pcp.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
            pcp.StateChange = pControlContext->control;
            pcp.Scope = DICS_FLAG_CONFIGSPECIFIC;
            pcp.HwProfile = 0;
            break;

        default:
            //
            // operate on config-specific profile
            //
            pcp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
            pcp.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
            pcp.StateChange = pControlContext->control;
            pcp.Scope = DICS_FLAG_CONFIGSPECIFIC;
            pcp.HwProfile = 0;
            break;

    }

    if(!SetupDiSetClassInstallParams(Devs,DevInfo,&pcp.ClassInstallHeader,sizeof(pcp)) ||
       !SetupDiCallClassInstaller(DIF_PROPERTYCHANGE,Devs,DevInfo)) {
        //
        // failed to invoke DIF_PROPERTYCHANGE
        //
        DumpDeviceWithInfo(Devs,DevInfo,pControlContext->strFail);
    } else {
        //
        // see if device needs reboot
        //
        devParams.cbSize = sizeof(devParams);
        if(SetupDiGetDeviceInstallParams(Devs,DevInfo,&devParams) && (devParams.Flags & (DI_NEEDRESTART|DI_NEEDREBOOT))) {
                DumpDeviceWithInfo(Devs,DevInfo,pControlContext->strReboot);
                pControlContext->reboot = TRUE;
        } else {
            //
            // appears to have succeeded
            //
            DumpDeviceWithInfo(Devs,DevInfo,pControlContext->strSuccess);
        }
        pControlContext->count++;
    }
    return EXIT_OK;
}

int cmdEnable(_In_ LPCTSTR BaseName, _In_opt_ LPCTSTR Machine, _In_ DWORD Flags, _In_ int argc, _In_reads_(argc) PTSTR argv[])
/*++

Routine Description:

    ENABLE <id> ...
    use EnumerateDevices to do hardwareID matching
    for each match, attempt to enable global, and if needed, config specific

Arguments:

    BaseName  - name of executable
    Machine   - must be NULL (local machine only)
    argc/argv - remaining parameters - passed into EnumerateDevices

Return Value:

    EXIT_xxxx (EXIT_REBOOT if reboot is required)

--*/
{
    GenericContext context;
    TCHAR strEnable[80];
    TCHAR strReboot[80];
    TCHAR strFail[80];
    int failcode = EXIT_FAIL;

    UNREFERENCED_PARAMETER(Flags);

    if(!argc) {
        //
        // arguments required
        //
        return EXIT_USAGE;
    }
    if(Machine) {
        //
        // must be local machine as we need to involve class/co installers
        //
        return EXIT_USAGE;
    }
    if(!LoadString(NULL,IDS_ENABLED,strEnable,ARRAYSIZE(strEnable))) {
        return EXIT_FAIL;
    }
    if(!LoadString(NULL,IDS_ENABLED_REBOOT,strReboot,ARRAYSIZE(strReboot))) {
        return EXIT_FAIL;
    }
    if(!LoadString(NULL,IDS_ENABLE_FAILED,strFail,ARRAYSIZE(strFail))) {
        return EXIT_FAIL;
    }

    context.control = DICS_ENABLE; // DICS_PROPCHANGE DICS_ENABLE DICS_DISABLE
    context.reboot = FALSE;
    context.count = 0;
    context.strReboot = strReboot;
    context.strSuccess = strEnable;
    context.strFail = strFail;
    failcode = EnumerateDevices(BaseName,Machine,DIGCF_PRESENT,argc,argv,ControlCallback,&context);

    if(failcode == EXIT_OK) {

        if(!context.count) {
            FormatToStream(stdout,MSG_FIND_TAIL_NONE_LOCAL);
        } else if(!context.reboot) {
            FormatToStream(stdout,MSG_ENABLE_TAIL,context.count);
        } else {
            FormatToStream(stdout,MSG_ENABLE_TAIL_REBOOT,context.count);
            failcode = EXIT_REBOOT;
        }
    }
    return failcode;
}

int cmdDisable(_In_ LPCTSTR BaseName, _In_opt_ LPCTSTR Machine, _In_ DWORD Flags, _In_ int argc, _In_reads_(argc) PTSTR argv[])
/*++

Routine Description:

    DISABLE <id> ...
    use EnumerateDevices to do hardwareID matching
    for each match, attempt to disable global

Arguments:

    BaseName  - name of executable
    Machine   - must be NULL (local machine only)
    argc/argv - remaining parameters - passed into EnumerateDevices

Return Value:

    EXIT_xxxx (EXIT_REBOOT if reboot is required)

--*/
{
    GenericContext context;
    TCHAR strDisable[80];
    TCHAR strReboot[80];
    TCHAR strFail[80];
    int failcode = EXIT_FAIL;

    UNREFERENCED_PARAMETER(Flags);

    if(!argc) {
        //
        // arguments required
        //
        return EXIT_USAGE;
    }
    if(Machine) {
        //
        // must be local machine as we need to involve class/co installers
        //
        return EXIT_USAGE;
    }
    if(!LoadString(NULL,IDS_DISABLED,strDisable,ARRAYSIZE(strDisable))) {
        return EXIT_FAIL;
    }
    if(!LoadString(NULL,IDS_DISABLED_REBOOT,strReboot,ARRAYSIZE(strReboot))) {
        return EXIT_FAIL;
    }
    if(!LoadString(NULL,IDS_DISABLE_FAILED,strFail,ARRAYSIZE(strFail))) {
        return EXIT_FAIL;
    }

    context.control = DICS_DISABLE; // DICS_PROPCHANGE DICS_ENABLE DICS_DISABLE
    context.reboot = FALSE;
    context.count = 0;
    context.strReboot = strReboot;
    context.strSuccess = strDisable;
    context.strFail = strFail;
    failcode = EnumerateDevices(BaseName,Machine,DIGCF_PRESENT,argc,argv,ControlCallback,&context);

    if(failcode == EXIT_OK) {

        if(!context.count) {
            FormatToStream(stdout,MSG_FIND_TAIL_NONE_LOCAL);
        } else if(!context.reboot) {
            FormatToStream(stdout,MSG_DISABLE_TAIL,context.count);
        } else {
            FormatToStream(stdout,MSG_DISABLE_TAIL_REBOOT,context.count);
            failcode = EXIT_REBOOT;
        }
    }
    return failcode;
}

int cmdRestart(_In_ LPCTSTR BaseName, _In_opt_ LPCTSTR Machine, _In_ DWORD Flags, _In_ int argc, _In_reads_(argc) PTSTR argv[])
/*++

Routine Description:

    RESTART <id> ...
    use EnumerateDevices to do hardwareID matching
    for each match, attempt to restart by issueing a PROPCHANGE

Arguments:

    BaseName  - name of executable
    Machine   - must be NULL (local machine only)
    argc/argv - remaining parameters - passed into EnumerateDevices

Return Value:

    EXIT_xxxx (EXIT_REBOOT if reboot is required)

--*/
{
    GenericContext context;
    TCHAR strRestarted[80];
    TCHAR strReboot[80];
    TCHAR strFail[80];
    int failcode = EXIT_FAIL;

    UNREFERENCED_PARAMETER(Flags);

    if(!argc) {
        //
        // arguments required
        //
        return EXIT_USAGE;
    }
    if(Machine) {
        //
        // must be local machine as we need to involve class/co installers
        //
        return EXIT_USAGE;
    }
    if(!LoadString(NULL,IDS_RESTARTED,strRestarted,ARRAYSIZE(strRestarted))) {
        return EXIT_FAIL;
    }
    if(!LoadString(NULL,IDS_REQUIRES_REBOOT,strReboot,ARRAYSIZE(strReboot))) {
        return EXIT_FAIL;
    }
    if(!LoadString(NULL,IDS_RESTART_FAILED,strFail,ARRAYSIZE(strFail))) {
        return EXIT_FAIL;
    }

    context.control = DICS_PROPCHANGE;
    context.reboot = FALSE;
    context.count = 0;
    context.strReboot = strReboot;
    context.strSuccess = strRestarted;
    context.strFail = strFail;
    failcode = EnumerateDevices(BaseName,Machine,DIGCF_PRESENT,argc,argv,ControlCallback,&context);

    if(failcode == EXIT_OK) {

        if(!context.count) {
            FormatToStream(stdout,MSG_FIND_TAIL_NONE_LOCAL);
        } else if(!context.reboot) {
            FormatToStream(stdout,MSG_RESTART_TAIL,context.count);
        } else {
            FormatToStream(stdout,MSG_RESTART_TAIL_REBOOT,context.count);
            failcode = EXIT_REBOOT;
        }
    }
    return failcode;
}

int cmdReboot(_In_ LPCTSTR BaseName, _In_opt_ LPCTSTR Machine, _In_ DWORD Flags, _In_ int argc, _In_reads_(argc) PTSTR argv[])
/*++

Routine Description:

    REBOOT
    reboot local machine

Arguments:

    BaseName  - name of executable
    Machine   - must be NULL (local machine only)
    argc/argv - remaining parameters - ignored

Return Value:

    EXIT_xxxx

--*/
{
    UNREFERENCED_PARAMETER(BaseName);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    if(Machine) {
        //
        // must be local machine
        //
        return EXIT_USAGE;
    }
    FormatToStream(stdout,MSG_REBOOT);
    return Reboot() ? EXIT_OK : EXIT_FAIL;
}


int cmdUpdate(_In_ LPCTSTR BaseName, _In_opt_ LPCTSTR Machine, _In_ DWORD Flags, _In_ int argc, _In_reads_(argc) PTSTR argv[])
/*++

Routine Description:
    UPDATE
    update driver for existing device(s)

Arguments:

    BaseName  - name of executable
    Machine   - machine name, must be NULL
    argc/argv - remaining parameters

Return Value:

    EXIT_xxxx

--*/
{
    HMODULE newdevMod = NULL;
    int failcode = EXIT_FAIL;
    UpdateDriverForPlugAndPlayDevicesProto UpdateFn;
    BOOL reboot = FALSE;
    LPCTSTR hwid = NULL;
    LPCTSTR inf = NULL;
    DWORD flags = 0;
    DWORD res;
    TCHAR InfPath[MAX_PATH];

    UNREFERENCED_PARAMETER(BaseName);
    UNREFERENCED_PARAMETER(Flags);

    if(Machine) {
        //
        // must be local machine
        //
        return EXIT_USAGE;
    }
    if(argc<2) {
        //
        // at least HWID required
        //
        return EXIT_USAGE;
    }
    inf = argv[0];
    if(!inf[0]) {
        return EXIT_USAGE;
    }

    hwid = argv[1];
    if(!hwid[0]) {
        return EXIT_USAGE;
    }
    //
    // Inf must be a full pathname
    //
    res = GetFullPathName(inf,MAX_PATH,InfPath,NULL);
    if((res >= MAX_PATH) || (res == 0)) {
        //
        // inf pathname too long
        //
        return EXIT_FAIL;
    }
    if(GetFileAttributes(InfPath)==(DWORD)(-1)) {
        //
        // inf doesn't exist
        //
        return EXIT_FAIL;
    }
    inf = InfPath;
    flags |= INSTALLFLAG_FORCE;

    //
    // make use of UpdateDriverForPlugAndPlayDevices
    //
    newdevMod = LoadLibrary(TEXT("newdev.dll"));
    if(!newdevMod) {
        goto final;
    }
    UpdateFn = (UpdateDriverForPlugAndPlayDevicesProto)GetProcAddress(newdevMod,UPDATEDRIVERFORPLUGANDPLAYDEVICES);
    if(!UpdateFn)
    {
        goto final;
    }

    FormatToStream(stdout,inf ? MSG_UPDATE_INF : MSG_UPDATE,hwid,inf);

    if(!UpdateFn(NULL,hwid,inf,flags,&reboot)) {
        goto final;
    }

    FormatToStream(stdout,MSG_UPDATE_OK);

    failcode = reboot ? EXIT_REBOOT : EXIT_OK;

final:

    if(newdevMod) {
        FreeLibrary(newdevMod);
    }

    return failcode;
}

int cmdInstall(_In_ LPCTSTR BaseName, _In_opt_ LPCTSTR Machine, _In_ DWORD Flags, _In_ int argc, _In_reads_(argc) PTSTR argv[])
/*++

Routine Description:

    CREATE
    Creates a root enumerated devnode and installs drivers on it

Arguments:

    BaseName  - name of executable
    Machine   - machine name, must be NULL
    argc/argv - remaining parameters

Return Value:

    EXIT_xxxx

--*/
{
    HDEVINFO DeviceInfoSet = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA DeviceInfoData;
    GUID ClassGUID;
    TCHAR ClassName[MAX_CLASS_NAME_LEN];
    TCHAR hwIdList[LINE_LEN+4];
    TCHAR InfPath[MAX_PATH];
    int failcode = EXIT_FAIL;
    LPCTSTR hwid = NULL;
    LPCTSTR inf = NULL;

    if(Machine) {
        //
        // must be local machine
        //
        return EXIT_USAGE;
    }
    if(argc<2) {
        //
        // at least HWID required
        //
        return EXIT_USAGE;
    }
    inf = argv[0];
    if(!inf[0]) {
        return EXIT_USAGE;
    }

    hwid = argv[1];
    if(!hwid[0]) {
        return EXIT_USAGE;
    }

    //
    // Inf must be a full pathname
    //
    if(GetFullPathName(inf,MAX_PATH,InfPath,NULL) >= MAX_PATH) {
        //
        // inf pathname too long
        //
        return EXIT_FAIL;
    }

    //
    // List of hardware ID's must be double zero-terminated
    //
    ZeroMemory(hwIdList,sizeof(hwIdList));
    if (FAILED(StringCchCopy(hwIdList,LINE_LEN,hwid))) {
        goto final;
    }

    //
    // Use the INF File to extract the Class GUID.
    //
    if (!SetupDiGetINFClass(InfPath,&ClassGUID,ClassName,sizeof(ClassName)/sizeof(ClassName[0]),0))
    {
        goto final;
    }

    //
    // Create the container for the to-be-created Device Information Element.
    //
    DeviceInfoSet = SetupDiCreateDeviceInfoList(&ClassGUID,0);
    if(DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        goto final;
    }

    //
    // Now create the element.
    // Use the Class GUID and Name from the INF file.
    //
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    if (!SetupDiCreateDeviceInfo(DeviceInfoSet,
        ClassName,
        &ClassGUID,
        NULL,
        0,
        DICD_GENERATE_ID,
        &DeviceInfoData))
    {
        goto final;
    }

    //
    // Add the HardwareID to the Device's HardwareID property.
    //
    if(!SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
        &DeviceInfoData,
        SPDRP_HARDWAREID,
        (LPBYTE)hwIdList,
        ((DWORD)_tcslen(hwIdList)+1+1)*sizeof(TCHAR)))
    {
        goto final;
    }

    //
    // Transform the registry element into an actual devnode
    // in the PnP HW tree.
    //
    if (!SetupDiCallClassInstaller(DIF_REGISTERDEVICE,
        DeviceInfoSet,
        &DeviceInfoData))
    {
        goto final;
    }

    FormatToStream(stdout,MSG_INSTALL_UPDATE);
    //
    // update the driver for the device we just created
    //
    failcode = cmdUpdate(BaseName,Machine,Flags,argc,argv);

final:

    if (DeviceInfoSet != INVALID_HANDLE_VALUE) {
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
    }

    return failcode;
}

int cmdUpdateNI(_In_ LPCTSTR BaseName, _In_opt_ LPCTSTR Machine, _In_ DWORD Flags, _In_ int argc, _In_reads_(argc) PTSTR argv[])
/*++

Routine Description:
    UPDATE (non interactive version)
    update driver for existing device(s)

Arguments:

    BaseName  - name of executable
    Machine   - machine name, must be NULL
    argc/argv - remaining parameters

Return Value:

    EXIT_xxxx

--*/
{
    //
    // turn off interactive mode while doing the update
    //
    HMODULE setupapiMod = NULL;
    SetupSetNonInteractiveModeProto SetNIFn;
    int res;
    BOOL prev;

    setupapiMod = LoadLibrary(TEXT("setupapi.dll"));
    if(!setupapiMod) {
        return cmdUpdate(BaseName,Machine,Flags,argc,argv);
    }
    SetNIFn = (SetupSetNonInteractiveModeProto)GetProcAddress(setupapiMod,SETUPSETNONINTERACTIVEMODE);
    if(!SetNIFn)
    {
        FreeLibrary(setupapiMod);
        return cmdUpdate(BaseName,Machine,Flags,argc,argv);
    }
    prev = SetNIFn(TRUE);
    res = cmdUpdate(BaseName,Machine,Flags,argc,argv);
    SetNIFn(prev);
    FreeLibrary(setupapiMod);
    return res;
}

int RemoveCallback(_In_ HDEVINFO Devs, _In_ PSP_DEVINFO_DATA DevInfo, _In_ DWORD Index, _In_ LPVOID Context)
/*++

Routine Description:

    Callback for use by Remove
    Invokes DIF_REMOVE
    uses SetupDiCallClassInstaller so cannot be done for remote devices
    Don't use CM_xxx API's, they bypass class/co-installers and this is bad.

Arguments:

    Devs    )_ uniquely identify the device
    DevInfo )
    Index    - index of device
    Context  - GenericContext

Return Value:

    EXIT_xxxx

--*/
{
    SP_REMOVEDEVICE_PARAMS rmdParams;
    GenericContext *pControlContext = (GenericContext*)Context;
    SP_DEVINSTALL_PARAMS devParams;
    LPCTSTR action = NULL;
    //
    // need hardware ID before trying to remove, as we wont have it after
    //
    TCHAR devID[MAX_DEVICE_ID_LEN];
    SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail;

    UNREFERENCED_PARAMETER(Index);

    devInfoListDetail.cbSize = sizeof(devInfoListDetail);
    if((!SetupDiGetDeviceInfoListDetail(Devs,&devInfoListDetail)) ||
            (CM_Get_Device_ID_Ex(DevInfo->DevInst,devID,MAX_DEVICE_ID_LEN,0,devInfoListDetail.RemoteMachineHandle)!=CR_SUCCESS)) {
        //
        // skip this
        //
        return EXIT_OK;
    }

    rmdParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    rmdParams.ClassInstallHeader.InstallFunction = DIF_REMOVE;
    rmdParams.Scope = DI_REMOVEDEVICE_GLOBAL;
    rmdParams.HwProfile = 0;
    if(!SetupDiSetClassInstallParams(Devs,DevInfo,&rmdParams.ClassInstallHeader,sizeof(rmdParams)) ||
       !SetupDiCallClassInstaller(DIF_REMOVE,Devs,DevInfo)) {
        //
        // failed to invoke DIF_REMOVE
        //
        action = pControlContext->strFail;
    } else {
        //
        // see if device needs reboot
        //
        devParams.cbSize = sizeof(devParams);
        if(SetupDiGetDeviceInstallParams(Devs,DevInfo,&devParams) && (devParams.Flags & (DI_NEEDRESTART|DI_NEEDREBOOT))) {
            //
            // reboot required
            //
            action = pControlContext->strReboot;
            pControlContext->reboot = TRUE;
        } else {
            //
            // appears to have succeeded
            //
            action = pControlContext->strSuccess;
        }
        pControlContext->count++;
    }
    _tprintf(TEXT("%-60s: %s\n"),devID,action);

    return EXIT_OK;
}

int cmdRemove(_In_ LPCTSTR BaseName, _In_opt_ LPCTSTR Machine, _In_ DWORD Flags, _In_ int argc, _In_reads_(argc) PTSTR argv[])
/*++

Routine Description:

    REMOVE
    remove devices

Arguments:

    BaseName  - name of executable
    Machine   - machine name, must be NULL
    argc/argv - remaining parameters

Return Value:

    EXIT_xxxx

--*/
{
    GenericContext context;
    TCHAR strRemove[80];
    TCHAR strReboot[80];
    TCHAR strFail[80];
    int failcode = EXIT_FAIL;

    UNREFERENCED_PARAMETER(Flags);

    if(!argc) {
        //
        // arguments required
        //
        return EXIT_USAGE;
    }
    if(Machine) {
        //
        // must be local machine as we need to involve class/co installers
        //
        return EXIT_USAGE;
    }
    if(!LoadString(NULL,IDS_REMOVED,strRemove,ARRAYSIZE(strRemove))) {
        return EXIT_FAIL;
    }
    if(!LoadString(NULL,IDS_REMOVED_REBOOT,strReboot,ARRAYSIZE(strReboot))) {
        return EXIT_FAIL;
    }
    if(!LoadString(NULL,IDS_REMOVE_FAILED,strFail,ARRAYSIZE(strFail))) {
        return EXIT_FAIL;
    }

    context.reboot = FALSE;
    context.count = 0;
    context.strReboot = strReboot;
    context.strSuccess = strRemove;
    context.strFail = strFail;
    failcode = EnumerateDevices(BaseName,Machine,DIGCF_PRESENT,argc,argv,RemoveCallback,&context);

    if(failcode == EXIT_OK) {

        if(!context.count) {
            FormatToStream(stdout,MSG_REMOVE_TAIL_NONE);
        } else if(!context.reboot) {
            FormatToStream(stdout,MSG_REMOVE_TAIL,context.count);
        } else {
            FormatToStream(stdout,MSG_REMOVE_TAIL_REBOOT,context.count);
            failcode = EXIT_REBOOT;
        }
    }
    return failcode;
}

int cmdRescan(_In_ LPCTSTR BaseName, _In_opt_ LPCTSTR Machine, _In_ DWORD Flags, _In_ int argc, _In_reads_(argc) PTSTR argv[])
/*++

Routine Description:

    RESCAN
    rescan for new devices

Arguments:

    BaseName  - name of executable
    Machine   - machine name, must be NULL
    argc/argv - remaining parameters

Return Value:

    EXIT_xxxx

--*/
{

    //
    // reenumerate from the root of the devnode tree
    // totally CM based
    //
    int failcode = EXIT_FAIL;
    HMACHINE machineHandle = NULL;
    DEVINST devRoot;

    UNREFERENCED_PARAMETER(BaseName);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    if(Machine) {
        if(CM_Connect_Machine(Machine,&machineHandle) != CR_SUCCESS) {
            return failcode;
        }
    }

    if(CM_Locate_DevNode_Ex(&devRoot,NULL,CM_LOCATE_DEVNODE_NORMAL,machineHandle) != CR_SUCCESS) {
        goto final;
    }

    FormatToStream(stdout,Machine ? MSG_RESCAN : MSG_RESCAN_LOCAL);

    if(CM_Reenumerate_DevNode_Ex(devRoot, 0, machineHandle) != CR_SUCCESS) {
        goto final;
    }

    FormatToStream(stdout,MSG_RESCAN_OK);

    failcode = EXIT_OK;

final:
    if(machineHandle) {
        CM_Disconnect_Machine(machineHandle);
    }

    return failcode;
}

int cmdClassFilter(_In_ LPCTSTR BaseName, _In_opt_ LPCTSTR Machine, _In_ DWORD Flags, _In_ int argc, _In_reads_(argc) PTSTR argv[])
/*++

Routine Description:

    CLASSFILTER <name> <type> <subcmds>
    Allows tweaking of class filters
    Useful for filter driver development and for Product Support

    <subcmds> is a list of the following:
    @service - sets 'after' to the first match of service after 'after'
               (reset to -1 after any other command)
    !service - deletes first match of 'service' after 'after'
    -service - insert new service directly prior to 'after', or at start
    +service - insert new service directly after 'after', or at end

    if no <subcmds> given, list the services

Arguments:

    BaseName  - name of executable
    Machine   - if non-NULL, remote machine
    argc/argv - remaining parameters - list of class names

Return Value:

    EXIT_xxxx

--*/
{
    int failcode = EXIT_FAIL;
    int argIndex;
    DWORD numGuids = 0;
    GUID guid;
    LPTSTR regval = NULL;
    HKEY hk = (HKEY)INVALID_HANDLE_VALUE;
    LPTSTR * multiVal = NULL;
    int after;
    bool modified = false;
    int span;
    SC_HANDLE SCMHandle = NULL;
    SC_HANDLE ServHandle = NULL;

    UNREFERENCED_PARAMETER(BaseName);
    UNREFERENCED_PARAMETER(Flags);

    if((argc<2) || !argv[0][0]) {
        return EXIT_USAGE;
    }

    //
    // just take the first guid for the name
    //
    if(!SetupDiClassGuidsFromNameEx(argv[0],&guid,1,&numGuids,Machine,NULL)) {
        if(GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            goto final;
        }
    }
    if(numGuids == 0) {
        goto final;
    }
    if(_tcsicmp(argv[1],TEXT("upper"))==0) {
        regval = REGSTR_VAL_UPPERFILTERS;
    } else if(_tcsicmp(argv[1],TEXT("lower"))==0) {
        regval = REGSTR_VAL_LOWERFILTERS;
    } else {
        failcode = EXIT_USAGE;
        goto final;
    }

    hk = SetupDiOpenClassRegKeyEx(&guid,
                                  KEY_READ | (argc>2 ? KEY_WRITE : 0),
                                  DIOCR_INSTALLER,
                                  Machine,
                                  NULL
                                 );
    if(hk == INVALID_HANDLE_VALUE) {
        goto final;
    }
    multiVal = GetRegMultiSz(hk,regval);

    if(argc<=2) {
        //
        // just display
        //
        FormatToStream(stdout,MSG_CLASSFILTER_UNCHANGED);
        DumpArray(1,multiVal);
        failcode = EXIT_OK;
        goto final;
    }
    after = -1; // for the @service expressions
    span =  1;

    if(!multiVal) {
        multiVal = CopyMultiSz(NULL);
        if(!multiVal) {
            goto final;
        }
    }

    for(argIndex=2;argIndex<argc;argIndex++) {
        if((argv[argIndex] == NULL) ||
           (!argv[argIndex])) {
            failcode = EXIT_USAGE;
            break;
        }

        int op = argv[argIndex][0];
        LPTSTR serv = argv[argIndex]+1;
        int mark = 0;
        int cnt;
        int ent;
        LPTSTR * tmpArray;

        if(op == TEXT('=')) {
            after = -1;
            span = 1;
            op = serv[0];
            if(!op) {
                continue;
            }
            serv++;
        }

        if(!serv[0]) {
            failcode = EXIT_USAGE;
            goto final;
        }

        if((op == TEXT('@')) || (op == TEXT('!'))) {
            //
            // need to find specified service in list
            //
            for(after+=span;multiVal[after];after++) {
                if(_tcsicmp(multiVal[after],serv)==0) {
                    break;
                }
            }
            if(!multiVal[after]) {
                goto final;
            }
            if(op == TEXT('@')) {
                //
                // all we needed to do for '@'
                //
                span = 1; // span of 1
                continue;
            }
            //
            // we're modifying
            //
            int c;
            for(c = after;multiVal[c];c++) {
                multiVal[c] = multiVal[c+1];
            }
            LPTSTR * newArray = CopyMultiSz(multiVal);
            if(!newArray) {
                goto final;
            }
            DelMultiSz(multiVal);
            multiVal = newArray;
            span = 0; // span of 0 (deleted)
            modified = true;
            continue;
        }

        if(op == '+') {
            //
            // insert after
            //
            if(after<0) {
                int c;
                for(c = 0;multiVal[c];c++) {
                    // nothing
                }
                mark = c;
            }
            else {
                mark = after+span;
            }
        } else if(op == '-') {
            //
            // insert before
            //
            if(after<0) {
                mark = 0;
            } else {
                mark = after;
            }
        } else {
            //
            // not valid
            //
            failcode = EXIT_USAGE;
            goto final;
        }
        //
        // sanity - see if service exists
        //
        SCMHandle = OpenSCManager(Machine, NULL, GENERIC_READ);
        if(!SCMHandle) {
            goto final;
        }
        ServHandle = OpenService(SCMHandle,serv,GENERIC_READ);
        if(ServHandle) {
            CloseServiceHandle(ServHandle);
        }
        CloseServiceHandle(SCMHandle);
        if(!ServHandle) {
            goto final;
        }

        //
        // need an array a little bigger
        //
        for(cnt = 0;multiVal[cnt];cnt++) {
            // nothing
        }

        tmpArray = new LPTSTR[cnt+2];
        if(!tmpArray) {
            goto final;
        }
        _Analysis_assume_(mark < cnt);
        for(ent=0;ent<mark;ent++) {
            tmpArray[ent] = multiVal[ent];
        }
        tmpArray[ent] = serv;
        for(;ent<cnt;ent++) {
            tmpArray[ent+1] = multiVal[ent];
        }
        tmpArray[ent+1] = NULL;
        LPTSTR * newArray = CopyMultiSz(tmpArray);
        delete [] tmpArray;
        if(!newArray) {
            goto final;
        }
        DelMultiSz(multiVal);
        multiVal = newArray;
        modified = true;
        span = 1;
        after = mark;
    }

    if(modified) {
        if(multiVal[0]) {
            size_t len = 0;
            LPTSTR multiSz = multiVal[-1];
            LPTSTR p = multiSz;
            while(*p) {
                p+=_tcslen(p)+1;
            }
            p++; // skip past null
            len = (p-multiSz)*sizeof(TCHAR);
            if(len > DWORD_MAX) {
                goto final;
            }
            LONG err = RegSetValueEx(hk,regval,0,REG_MULTI_SZ,(LPBYTE)multiSz,(DWORD)len);
            if(err==NO_ERROR) {
                FormatToStream(stdout,MSG_CLASSFILTER_CHANGED);
                DumpArray(1,multiVal);
                failcode = EXIT_REBOOT;
            }
        } else {
            LONG err = RegDeleteValue(hk,regval);
            if((err == NO_ERROR) || (err == ERROR_FILE_NOT_FOUND)) {
                FormatToStream(stdout,MSG_CLASSFILTER_CHANGED);
                failcode = EXIT_REBOOT;
            }
        }
    } else {
        FormatToStream(stdout,MSG_CLASSFILTER_UNCHANGED);
        DumpArray(1,multiVal);
        failcode = EXIT_OK;
    }

final:

    if(multiVal) {
        DelMultiSz(multiVal);
    }
    if(hk != (HKEY)INVALID_HANDLE_VALUE) {
        RegCloseKey(hk);
    }

    return failcode;
}

int SetHwidCallback(_In_ HDEVINFO Devs, _In_ PSP_DEVINFO_DATA DevInfo, _In_ DWORD Index, _In_ LPVOID Context)
/*++

Routine Description:

    Callback for use by SetHwid

Arguments:

    Devs    )_ uniquely identify the device
    DevInfo )
    Index    - index of device
    Context  - SetHwidContext

Return Value:

    EXIT_xxxx

--*/
{
    SetHwidContext *pControlContext = (SetHwidContext*)Context;
    ULONG status;
    ULONG problem;
    LPTSTR * hwlist = NULL;
    bool modified = false;
    int result = EXIT_FAIL;

    //
    // processes the sub-commands on each callback
    // not most efficient way of doing things, but perf isn't important
    //
    TCHAR devID[MAX_DEVICE_ID_LEN];
    SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail;

    UNREFERENCED_PARAMETER(Index);

    devInfoListDetail.cbSize = sizeof(devInfoListDetail);
    if((!SetupDiGetDeviceInfoListDetail(Devs,&devInfoListDetail)) ||
            (CM_Get_Device_ID_Ex(DevInfo->DevInst,devID,MAX_DEVICE_ID_LEN,0,devInfoListDetail.RemoteMachineHandle)!=CR_SUCCESS) ||
            (CM_Get_DevNode_Status_Ex(&status,&problem,DevInfo->DevInst,0,devInfoListDetail.RemoteMachineHandle)!=CR_SUCCESS)) {
        //
        // skip this
        //
        return EXIT_OK;
    }
    //
    // this is how to verify it's root enumerated
    //
    if(!(status & DN_ROOT_ENUMERATED)) {
        _tprintf(TEXT("%-60s: "),devID);
        FormatToStream(stdout,MSG_SETHWID_NOTROOT);
        pControlContext->skipped++;
        return EXIT_OK;
    }
    hwlist = GetDevMultiSz(Devs,DevInfo,pControlContext->prop);
    if(hwlist == NULL) {
        hwlist = CopyMultiSz(NULL);
        if(hwlist == NULL) {
            return EXIT_FAIL;
        }
    }

    //
    // modify hwid list (only relevent for root-enumerated devices)
    //
    int i;
    int mark = -1;

    for(i=0;i<pControlContext->argc_right;i++) {
        LPTSTR op = pControlContext->argv_right[i];
        if(op[0] == TEXT('=')) {
            //
            // clear the hwid list first
            //
            hwlist[0] = NULL;
            mark = 0;
            op++;
        } else if(op[0] == TEXT('+')) {
            //
            // insert as better match
            //
            mark = 0;
            op++;
        } else if(op[0] == TEXT('-')) {
            //
            // insert as worse match
            //
            mark = -1;
            op++;
        } else if(op[0] == TEXT('!')) {
            //
            // delete
            //
            mark = -2;
            op++;
        } else {
            //
            // treat as a hardware id
            //
        }
        if(!*op) {
            result = EXIT_USAGE;
            goto final;
        }
        int cnt;
        for(cnt = 0;hwlist[cnt];cnt++) {
            // nothing
        }
        if((mark == -1) || (mark>cnt)) {
            mark = cnt;
        }
        LPTSTR * tmpArray = new LPTSTR[cnt+2];
        if(!tmpArray) {
            goto final;
        }
        int dst = 0;
        int ent;
        for(ent=0;ent<mark;ent++) {
            if(_tcsicmp(hwlist[ent],op)==0) {
                continue;
            }
            tmpArray[dst++] = hwlist[ent];
        }
        if(mark>=0) {
            tmpArray[dst++] = op;
        }
        for(;ent<cnt;ent++) {
            if(_tcsicmp(hwlist[ent],op)==0) {
                continue;
            }
            tmpArray[dst++] = hwlist[ent];
        }
        tmpArray[dst] = NULL;
        LPTSTR * newArray = CopyMultiSz(tmpArray);
        delete [] tmpArray;
        if(!newArray) {
            goto final;
        }
        DelMultiSz(hwlist);
        hwlist = newArray;
        modified = true;
        mark++;
    }

    //
    // re-set the hwid list
    //
    if(modified) {
        if(hwlist[0]) {
            size_t len = 0;
            LPTSTR multiSz = hwlist[-1];
            LPTSTR p = multiSz;
            while(*p) {
                p+=_tcslen(p)+1;
            }
            p++; // skip past final null
            len = (p-multiSz)*sizeof(TCHAR);
            if(len > DWORD_MAX) {
                result = EXIT_FAIL;
                goto final;
            }
            if(!SetupDiSetDeviceRegistryProperty(Devs,
                                                 DevInfo,
                                                 pControlContext->prop,
                                                 (LPBYTE)multiSz,
                                                 (DWORD)len)) {
                result = EXIT_FAIL;
                goto final;
            }
        } else {
            //
            // delete list
            //
            if(!SetupDiSetDeviceRegistryProperty(Devs,
                                                 DevInfo,
                                                 pControlContext->prop,
                                                 NULL,
                                                 0)) {
                result = EXIT_FAIL;
                goto final;
            }
        }
    }
    result = EXIT_OK;
    pControlContext->modified++;
    _tprintf(TEXT("%-60s: "),devID);
    for(mark=0;hwlist[mark];mark++) {
        if(mark > 0) {
            _tprintf(TEXT(","));
        }
        _tprintf(TEXT("%s"),hwlist[mark]);
    }
    _tprintf(TEXT("\n"));

    //
    // cleanup
    //

  final:

    if(hwlist) {
        DelMultiSz(hwlist);
    }

    return result;
}

int cmdSetHwid(_In_ LPCTSTR BaseName, _In_opt_ LPCTSTR Machine, _In_ DWORD Flags, _In_ int argc, _In_reads_(argc) PTSTR argv[])
/*++

Routine Description:

    SETHWID
    changes the hardware ID's of the listed root-enumerated devices
    This demonstrates how to differentiate between root-enumerated and
    non root-enumerated devices.
    It also demonstrates how to get/set hardware ID's of root-enumerated
    devices.

Arguments:

    BaseName  - name of executable
    Machine   - machine name, must be NULL
    argc/argv - remaining parameters

Return Value:

    EXIT_xxxx

--*/
{
    SetHwidContext context;
    int failcode = EXIT_FAIL;

    UNREFERENCED_PARAMETER(Flags);

    if(!SplitCommandLine(argc,argv,context.argc_right,context.argv_right)
       || (argc == 0)
       || (context.argc_right == 0)) {
        //
        // arguments required both left and right of ':='
        //
        return EXIT_USAGE;
    }
    context.skipped = 0;
    context.modified = 0;
    context.prop = SPDRP_HARDWAREID;

    failcode = EnumerateDevices(BaseName,Machine,DIGCF_PRESENT,argc,argv,SetHwidCallback,&context);

    if(failcode == EXIT_OK) {

        if(context.skipped) {
            FormatToStream(stdout,MSG_SETHWID_TAIL_SKIPPED,context.skipped,context.modified);
        } else if(context.modified) {
            FormatToStream(stdout,MSG_SETHWID_TAIL_MODIFIED,context.modified);
        } else {
            FormatToStream(stdout,MSG_SETHWID_TAIL_NONE);
        }
    }
    return failcode;
}

int cmdDPAdd(_In_ LPCTSTR BaseName, _In_opt_ LPCTSTR Machine, _In_ DWORD Flags, _In_ int argc, _In_reads_(argc) PTSTR argv[])
/*++

Routine Description:
    dp_add
    Add a driver package to the machine.

Arguments:

    BaseName  - name of executable
    Machine   - machine name, must be NULL
    argc/argv - remaining parameters

Return Value:

    EXIT_xxxx

--*/
{
    int failcode = EXIT_FAIL;
    DWORD res;
    TCHAR SourceInfFileName[MAX_PATH];
    TCHAR DestinationInfFileName[MAX_PATH];
    PTSTR DestinationInfFileNameComponent = NULL;
    PTSTR FilePart = NULL;

    UNREFERENCED_PARAMETER(BaseName);
    UNREFERENCED_PARAMETER(Machine);
    UNREFERENCED_PARAMETER(Flags);

    if(!argc) {
        return EXIT_USAGE;
    }

    res = GetFullPathName(argv[0],
                          ARRAYSIZE(SourceInfFileName),
                          SourceInfFileName,
                          &FilePart);
    if ((!res) || (res >= ARRAYSIZE(SourceInfFileName))) {
        FormatToStream(stdout,MSG_DPADD_INVALID_INF);
        goto final;
    }

    if (!SetupCopyOEMInf(SourceInfFileName,
                         NULL,
                         SPOST_PATH,
                         0,
                         DestinationInfFileName,
                         ARRAYSIZE(DestinationInfFileName),
                         NULL,
                         &DestinationInfFileNameComponent)) {
        FormatToStream(stdout,MSG_DPADD_FAILED);
        goto final;
    }

    //
    // Successfully added the driver package to the machine.
    //
    FormatToStream(stdout,MSG_DPADD_SUCCESS,DestinationInfFileNameComponent);
    failcode = EXIT_OK;

final:

    return failcode;
}

int cmdDPDelete(_In_ LPCTSTR BaseName, _In_opt_ LPCTSTR Machine, _In_ DWORD Flags, _In_ int argc, _In_reads_(argc) PTSTR argv[])
/*++

Routine Description:
    dp_delete
    Deletes a driver package to the machine.

Arguments:

    BaseName  - name of executable
    Machine   - machine name, must be NULL
    argc/argv - remaining parameters

Return Value:

    EXIT_xxxx

--*/
{
    int failcode = EXIT_FAIL;
    DWORD res;
    TCHAR InfFileName[MAX_PATH];
    PTSTR FilePart = NULL;
    HMODULE setupapiMod = NULL;
    SetupUninstallOEMInfProto SUOIFn;

    UNREFERENCED_PARAMETER(BaseName);
    UNREFERENCED_PARAMETER(Machine);

    if(!argc) {
        return EXIT_USAGE;
    }

    res = GetFullPathName(argv[0],
                          ARRAYSIZE(InfFileName),
                          InfFileName,
                          &FilePart);
    if ((!res) || (!FilePart)) {
        FormatToStream(stdout,MSG_DPADD_INVALID_INF);
        goto final;
    }

    setupapiMod = LoadLibrary(TEXT("setupapi.dll"));
    if(!setupapiMod) {
        goto final;
    }
    SUOIFn = (SetupUninstallOEMInfProto)GetProcAddress(setupapiMod,SETUPUNINSTALLOEMINF);
    if(!SUOIFn)
    {
        goto final;
    }

    if (!SUOIFn(FilePart,
                ((Flags & DEVCON_FLAG_FORCE) ? 1 : 0),
                NULL)) {
        if (GetLastError() == ERROR_INF_IN_USE_BY_DEVICES) {
            FormatToStream(stdout,MSG_DPDELETE_FAILED_IN_USE);
        } else if (GetLastError() == ERROR_NOT_AN_INSTALLED_OEM_INF) {
            FormatToStream(stdout,MSG_DPDELETE_FAILED_NOT_OEM_INF);
        } else {
            FormatToStream(stdout,MSG_DPDELETE_FAILED);
        }
        goto final;
    }

    //
    // Successfully added the driver package to the machine.
    //
    FormatToStream(stdout,MSG_DPDELETE_SUCCESS,FilePart);
    failcode = EXIT_OK;

final:
    if (setupapiMod) {
        FreeLibrary(setupapiMod);
    }

    return failcode;
}

int cmdDPEnumLegacy(_In_ LPCTSTR BaseName, _In_opt_ LPCTSTR Machine, _In_ DWORD Flags, _In_ int argc, _In_reads_(argc) PTSTR argv[])
/*++

Routine Description:
    dp_enumLegacy
    Enumerates installed Driver Packages on the machine pre Windows Longhorn

Arguments:

    BaseName  - name of executable
    Machine   - machine name, must be NULL
    argc/argv - remaining parameters

Return Value:

    EXIT_xxxx

--*/
{
    int failcode = EXIT_FAIL;
    TCHAR FindName[MAX_PATH];
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA wfd;

    UNREFERENCED_PARAMETER(BaseName);
    UNREFERENCED_PARAMETER(Machine);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(argv);

    if(argc) {
        return EXIT_USAGE;
    }

    if (!GetWindowsDirectory(FindName, ARRAYSIZE(FindName)) ||
        FAILED(StringCchCat(FindName, ARRAYSIZE(FindName), TEXT("\\INF\\OEM*.INF")))) {
        goto final;
    }

    hFind = FindFirstFile(FindName, &wfd);
    if (hFind == INVALID_HANDLE_VALUE) {
        //
        // No OEM driver packages on this machine.
        //
        FormatToStream(stdout,MSG_DPENUM_NO_OEM_INF);
        failcode = EXIT_OK;
        goto final;
    }

    FormatToStream(stdout,MSG_DPENUM_LIST_HEADER);

    do {
        FormatToStream(stdout,MSG_DPENUM_LIST_ENTRY,wfd.cFileName);
        DumpDriverPackageData(wfd.cFileName);
    } while (FindNextFile(hFind, &wfd));

    FindClose(hFind);

    failcode = EXIT_OK;

final:

    return failcode;
}


DispatchEntry DispatchTable[] = {
    { TEXT("classfilter"),  cmdClassFilter, MSG_CLASSFILTER_SHORT, MSG_CLASSFILTER_LONG },
    { TEXT("classes"),      cmdClasses,     MSG_CLASSES_SHORT,     MSG_CLASSES_LONG },
    { TEXT("disable"),      cmdDisable,     MSG_DISABLE_SHORT,     MSG_DISABLE_LONG },
    { TEXT("driverfiles"),  cmdDriverFiles, MSG_DRIVERFILES_SHORT, MSG_DRIVERFILES_LONG },
    { TEXT("drivernodes"),  cmdDriverNodes, MSG_DRIVERNODES_SHORT, MSG_DRIVERNODES_LONG },
    { TEXT("enable"),       cmdEnable,      MSG_ENABLE_SHORT,      MSG_ENABLE_LONG },
    { TEXT("find"),         cmdFind,        MSG_FIND_SHORT,        MSG_FIND_LONG },
    { TEXT("findall"),      cmdFindAll,     MSG_FINDALL_SHORT,     MSG_FINDALL_LONG },
    { TEXT("help"),         cmdHelp,        MSG_HELP_SHORT,        0 },
    { TEXT("hwids"),        cmdHwIds,       MSG_HWIDS_SHORT,       MSG_HWIDS_LONG },
    { TEXT("install"),      cmdInstall,     MSG_INSTALL_SHORT,     MSG_INSTALL_LONG },
    { TEXT("listclass"),    cmdListClass,   MSG_LISTCLASS_SHORT,   MSG_LISTCLASS_LONG },
    { TEXT("reboot"),       cmdReboot,      MSG_REBOOT_SHORT,      MSG_REBOOT_LONG },
    { TEXT("remove"),       cmdRemove,      MSG_REMOVE_SHORT,      MSG_REMOVE_LONG },
    { TEXT("rescan"),       cmdRescan,      MSG_RESCAN_SHORT,      MSG_RESCAN_LONG },
    { TEXT("resources"),    cmdResources,   MSG_RESOURCES_SHORT,   MSG_RESOURCES_LONG },
    { TEXT("restart"),      cmdRestart,     MSG_RESTART_SHORT,     MSG_RESTART_LONG },
    { TEXT("sethwid"),      cmdSetHwid,     MSG_SETHWID_SHORT,     MSG_SETHWID_LONG },
    { TEXT("stack"),        cmdStack,       MSG_STACK_SHORT,       MSG_STACK_LONG },
    { TEXT("status"),       cmdStatus,      MSG_STATUS_SHORT,      MSG_STATUS_LONG },
    { TEXT("update"),       cmdUpdate,      MSG_UPDATE_SHORT,      MSG_UPDATE_LONG },
    { TEXT("updateni"),     cmdUpdateNI,    MSG_UPDATENI_SHORT,    MSG_UPDATENI_LONG },
    { TEXT("dp_add"),       cmdDPAdd,       MSG_DPADD_SHORT,       MSG_DPADD_LONG },
    { TEXT("dp_delete"),    cmdDPDelete,    MSG_DPDELETE_SHORT,    MSG_DPDELETE_LONG },
    { TEXT("dp_enum"),      cmdDPEnumLegacy,MSG_DPENUM_SHORT,      MSG_DPENUM_LONG },
    { TEXT("?"),            cmdHelp,        0,                     0 },
    { NULL,NULL }
};



