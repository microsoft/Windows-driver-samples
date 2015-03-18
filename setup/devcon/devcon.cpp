/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    devcon.cpp

Abstract:

    Device Console
    command-line interface for managing devices

--*/

#include "devcon.h"

struct IdEntry {
    LPCTSTR String;     // string looking for
    LPCTSTR Wild;       // first wild character if any
    BOOL    InstanceId;
};

void FormatToStream(_In_ FILE * stream, _In_ DWORD fmt,...)
/*++

Routine Description:

    Format text to stream using a particular msg-id fmt
    Used for displaying localizable messages

Arguments:

    stream              - file stream to output to, stdout or stderr
    fmt                 - message id
    ...                 - parameters %1...

Return Value:

    none

--*/
{
    va_list arglist;
    LPTSTR locbuffer = NULL;
    DWORD count;

    va_start(arglist, fmt);
    count = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE|FORMAT_MESSAGE_ALLOCATE_BUFFER,
                          NULL,
                          fmt,
                          0,              // LANGID
                          (LPTSTR) &locbuffer,
                          0,              // minimum size of buffer
                          &arglist);

    if(locbuffer) {
        if(count) {
            int c;
            int back = 0;
            //
            // strip any trailing "\r\n"s and replace by a single "\n"
            //
            while(((c = *CharPrev(locbuffer,locbuffer+count)) == TEXT('\r')) ||
                  (c == TEXT('\n'))) {
                count--;
                back++;
            }
            if(back) {
                locbuffer[count++] = TEXT('\n');
                locbuffer[count] = TEXT('\0');
            }
            //
            // now write to apropriate stream
            //
            _fputts(locbuffer,stream);
        }
        LocalFree(locbuffer);
    }
}

void Padding(_In_ int pad)
/*++

Routine Description:

    Insert padding into line before text

Arguments:

    pad - number of padding tabs to insert

Return Value:

    none

--*/
{
    int c;

    for(c=0;c<pad;c++) {
        fputs("    ",stdout);
    }
}


void Usage(_In_ LPCTSTR BaseName)
/*++

Routine Description:

    Display simple usage text

Arguments:

    BaseName            - name of executable

Return Value:

    none

--*/
{
    FormatToStream(stderr,MSG_USAGE,BaseName);
}

void CommandUsage(_In_ LPCTSTR BaseName, _In_ LPCTSTR Cmd)
/*++

Routine Description:

    Invalid command usage
    Display how to get help on command

Arguments:

    BaseName            - name of executable

Return Value:

    none

--*/
{
    FormatToStream(stderr,MSG_COMMAND_USAGE,BaseName,Cmd);
}

void Failure(_In_ LPCTSTR BaseName, _In_ LPCTSTR Cmd)
/*++

Routine Description:

    Display simple error text for general failure

Arguments:

    BaseName            - name of executable

Return Value:

    none

--*/
{
    FormatToStream(stderr,MSG_FAILURE,BaseName,Cmd);
}

BOOL Reboot()
/*++

Routine Description:

    Attempt to reboot computer

Arguments:

    none

Return Value:

    TRUE if API suceeded

--*/
{
    HANDLE Token;
    TOKEN_PRIVILEGES NewPrivileges;
    LUID Luid;

    //
    // we need to "turn on" reboot privilege
    // if any of this fails, try reboot anyway
    //
    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES,&Token)) {
        goto final;
    }

    if(!LookupPrivilegeValue(NULL,SE_SHUTDOWN_NAME,&Luid)) {
        CloseHandle(Token);
        goto final;
    }

    NewPrivileges.PrivilegeCount = 1;
    NewPrivileges.Privileges[0].Luid = Luid;
    NewPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    AdjustTokenPrivileges(
            Token,
            FALSE,
            &NewPrivileges,
            0,
            NULL,
            NULL
            );

    CloseHandle(Token);

final:

    //
    // attempt reboot - inform system that this is planned hardware install
    //
    // warning 28159 is a warning to rearchitect to avoid rebooting.  However,
    // sometimes during device installation, a reboot is needed, so this warning
    // is being suppressed for the call to InitiateSystemShutdownEx.
    //
#pragma warning( suppress: 28159)
    return InitiateSystemShutdownEx(NULL,
                                    NULL,
                                    0,
                                    FALSE,
                                    TRUE,
                                    REASON_PLANNED_FLAG | REASON_HWINSTALL);

}

LPTSTR GetDeviceStringProperty(_In_ HDEVINFO Devs, _In_ PSP_DEVINFO_DATA DevInfo, _In_ DWORD Prop)
/*++

Routine Description:

    Return a string property for a device, otherwise NULL

Arguments:

    Devs    )_ uniquely identify device
    DevInfo )
    Prop     - string property to obtain

Return Value:

    string containing description

--*/
{
    LPTSTR buffer;
    DWORD size;
    DWORD reqSize;
    DWORD dataType;
    DWORD szChars;

    size = 1024; // initial guess
    buffer = new TCHAR[(size/sizeof(TCHAR))+1];
    if(!buffer) {
        return NULL;
    }
    while(!SetupDiGetDeviceRegistryProperty(Devs,DevInfo,Prop,&dataType,(LPBYTE)buffer,size,&reqSize)) {
        if(GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            goto failed;
        }
        if(dataType != REG_SZ) {
            goto failed;
        }
        size = reqSize;
        delete [] buffer;
        buffer = new TCHAR[(size/sizeof(TCHAR))+1];
        if(!buffer) {
            goto failed;
        }
    }
    szChars = reqSize/sizeof(TCHAR);
    buffer[szChars] = TEXT('\0');
    return buffer;

failed:
    if(buffer) {
        delete [] buffer;
    }
    return NULL;
}

LPTSTR GetDeviceDescription(_In_ HDEVINFO Devs, _In_ PSP_DEVINFO_DATA DevInfo)
/*++

Routine Description:

    Return a string containing a description of the device, otherwise NULL
    Always try friendly name first

Arguments:

    Devs    )_ uniquely identify device
    DevInfo )

Return Value:

    string containing description

--*/
{
    LPTSTR desc;
    desc = GetDeviceStringProperty(Devs,DevInfo,SPDRP_FRIENDLYNAME);
    if(!desc) {
        desc = GetDeviceStringProperty(Devs,DevInfo,SPDRP_DEVICEDESC);
    }
    return desc;
}

IdEntry GetIdType(_In_ LPCTSTR Id)
/*++

Routine Description:

    Determine if this is instance id or hardware id and if there's any wildcards
    instance ID is prefixed by '@'
    wildcards are '*'


Arguments:

    Id - ptr to string to check

Return Value:

    IdEntry

--*/
{
    IdEntry Entry;

    Entry.InstanceId = FALSE;
    Entry.Wild = NULL;
    Entry.String = Id;

    if(Entry.String[0] == INSTANCEID_PREFIX_CHAR) {
        Entry.InstanceId = TRUE;
        Entry.String = CharNext(Entry.String);
    }
    if(Entry.String[0] == QUOTE_PREFIX_CHAR) {
        //
        // prefix to treat rest of string literally
        //
        Entry.String = CharNext(Entry.String);
    } else {
        //
        // see if any wild characters exist
        //
        Entry.Wild = _tcschr(Entry.String,WILD_CHAR);
    }
    return Entry;
}

__drv_allocatesMem(object)
LPTSTR * GetMultiSzIndexArray(_In_ __drv_aliasesMem LPTSTR MultiSz)
/*++

Routine Description:

    Get an index array pointing to the MultiSz passed in

Arguments:

    MultiSz - well formed multi-sz string

Return Value:

    array of strings. last entry+1 of array contains NULL
    returns NULL on failure

--*/
{
    LPTSTR scan;
    LPTSTR * array;
    int elements;

    for(scan = MultiSz, elements = 0; scan[0] ;elements++) {
        scan += lstrlen(scan)+1;
    }
    array = new LPTSTR[elements+2];
    if(!array) {
        return NULL;
    }
    array[0] = MultiSz;
    array++;
    if(elements) {
        for(scan = MultiSz, elements = 0; scan[0]; elements++) {
            array[elements] = scan;
            scan += lstrlen(scan)+1;
        }
    }
    array[elements] = NULL;
    return array;
}

__drv_allocatesMem(object)
LPTSTR * CopyMultiSz(_In_opt_ PZPWSTR Array)
/*++

Routine Description:

    Creates a new array from old
    old array need not have been allocated by GetMultiSzIndexArray

Arguments:

    Array - array of strings, last entry is NULL

Return Value:

    MultiSz array allocated by GetMultiSzIndexArray

--*/
{
    LPTSTR multiSz = NULL;
    HRESULT hr;
    int cchMultiSz = 0;
    int c;
    if(Array) {
        for(c=0;Array[c];c++) {
            cchMultiSz+=lstrlen(Array[c])+1;
        }
    }
    cchMultiSz+=1; // final Null
    multiSz = new TCHAR[cchMultiSz];
    if(!multiSz) {
        return NULL;
    }
    int len = 0;
    if(Array) {
        for(c=0;Array[c];c++) {
            hr = StringCchCopy(multiSz+len,cchMultiSz-len,Array[c]);
            if(FAILED(hr)){
                if(multiSz)
                    delete [] multiSz;
                return NULL;
            }

#pragma prefast(suppress:__WARNING_BUFFER_OVERFLOW, "ESP:732")
            len+=lstrlen(multiSz+len)+1;
        }
    }
    
    if( len < cchMultiSz ){
        multiSz[len] = TEXT('\0');
    } else {
        // This should never happen!
        multiSz[cchMultiSz-1] = TEXT('\0');
    }
    
    LPTSTR * pRes = GetMultiSzIndexArray(multiSz);
    if(pRes) {
        return pRes;
    }
    delete [] multiSz;

    return NULL;
}

void DelMultiSz(_In_opt_ __drv_freesMem(object) PZPWSTR Array)
/*++

Routine Description:

    Deletes the string array allocated by GetDevMultiSz/GetRegMultiSz/GetMultiSzIndexArray

Arguments:

    Array - pointer returned by GetMultiSzIndexArray

Return Value:

    None

--*/
{
    if(Array) {
        Array--;
        if(Array[0]) {
            delete [] Array[0];
        }
        delete [] Array;
    }
}

__drv_allocatesMem(object)
LPTSTR * GetDevMultiSz(_In_ HDEVINFO Devs, _In_ PSP_DEVINFO_DATA DevInfo, _In_ DWORD Prop)
/*++

Routine Description:

    Get a multi-sz device property
    and return as an array of strings

Arguments:

    Devs    - HDEVINFO containing DevInfo
    DevInfo - Specific device
    Prop    - SPDRP_HARDWAREID or SPDRP_COMPATIBLEIDS

Return Value:

    array of strings. last entry+1 of array contains NULL
    returns NULL on failure

--*/
{
    LPTSTR buffer;
    DWORD size;
    DWORD reqSize;
    DWORD dataType;
    LPTSTR * array;
    DWORD szChars;

    size = 8192; // initial guess, nothing magic about this
    buffer = new TCHAR[(size/sizeof(TCHAR))+2];
    if(!buffer) {
        return NULL;
    }
    while(!SetupDiGetDeviceRegistryProperty(Devs,DevInfo,Prop,&dataType,(LPBYTE)buffer,size,&reqSize)) {
        if(GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            goto failed;
        }
        if(dataType != REG_MULTI_SZ) {
            goto failed;
        }
        size = reqSize;
        delete [] buffer;
        buffer = new TCHAR[(size/sizeof(TCHAR))+2];
        if(!buffer) {
            goto failed;
        }
    }
    szChars = reqSize/sizeof(TCHAR);
    buffer[szChars] = TEXT('\0');
    buffer[szChars+1] = TEXT('\0');
    array = GetMultiSzIndexArray(buffer);
    if(array) {
        return array;
    }

failed:
    if(buffer) {
        delete [] buffer;
    }
    return NULL;
}

__drv_allocatesMem(object)
LPTSTR * GetRegMultiSz(_In_ HKEY hKey, _In_ LPCTSTR Val)
/*++

Routine Description:

    Get a multi-sz from registry
    and return as an array of strings

Arguments:

    hKey    - Registry Key
    Val     - Value to query

Return Value:

    array of strings. last entry+1 of array contains NULL
    returns NULL on failure

--*/
{
    LPTSTR buffer;
    DWORD size;
    DWORD reqSize;
    DWORD dataType;
    LPTSTR * array;
    DWORD szChars;
    LONG regErr;

    size = 8192; // initial guess, nothing magic about this
    buffer = new TCHAR[(size/sizeof(TCHAR))+2];
    if(!buffer) {
        return NULL;
    }
    reqSize = size;
    regErr = RegQueryValueEx(hKey,Val,NULL,&dataType,(PBYTE)buffer,&reqSize);
    while((regErr != NO_ERROR)) {
        if(GetLastError() != ERROR_MORE_DATA) {
            goto failed;
        }
        if(dataType != REG_MULTI_SZ) {
            goto failed;
        }
        size = reqSize;
        delete [] buffer;
        buffer = new TCHAR[(size/sizeof(TCHAR))+2];
        if(!buffer) {
            goto failed;
        }
	regErr = RegQueryValueEx(hKey,Val,NULL,&dataType,(PBYTE)buffer,&reqSize);
    }
    szChars = reqSize/sizeof(TCHAR);
    buffer[szChars] = TEXT('\0');
    buffer[szChars+1] = TEXT('\0');

    array = GetMultiSzIndexArray(buffer);
    if(array) {
        return array;
    }

failed:
    if(buffer) {
        delete [] buffer;
    }
    return NULL;
}

BOOL WildCardMatch(_In_ LPCTSTR Item, _In_ const IdEntry & MatchEntry)
/*++

Routine Description:

    Compare a single item against wildcard
    I'm sure there's better ways of implementing this
    Other than a command-line management tools
    it's a bad idea to use wildcards as it implies
    assumptions about the hardware/instance ID
    eg, it might be tempting to enumerate root\* to
    find all root devices, however there is a CfgMgr
    API to query status and determine if a device is
    root enumerated, which doesn't rely on implementation
    details.

Arguments:

    Item - item to find match for eg a\abcd\c
    MatchEntry - eg *\*bc*\*

Return Value:

    TRUE if any match, otherwise FALSE

--*/
{
    LPCTSTR scanItem;
    LPCTSTR wildMark;
    LPCTSTR nextWild;
    size_t matchlen;

    //
    // before attempting anything else
    // try and compare everything up to first wild
    //
    if(!MatchEntry.Wild) {
        return _tcsicmp(Item,MatchEntry.String) ? FALSE : TRUE;
    }
    if(_tcsnicmp(Item,MatchEntry.String,MatchEntry.Wild-MatchEntry.String) != 0) {
        return FALSE;
    }
    wildMark = MatchEntry.Wild;
    scanItem = Item + (MatchEntry.Wild-MatchEntry.String);

    for(;wildMark[0];) {
        //
        // if we get here, we're either at or past a wildcard
        //
        if(wildMark[0] == WILD_CHAR) {
            //
            // so skip wild chars
            //
            wildMark = CharNext(wildMark);
            continue;
        }
        //
        // find next wild-card
        //
        nextWild = _tcschr(wildMark,WILD_CHAR);
        if(nextWild) {
            //
            // substring
            //
            matchlen = nextWild-wildMark;
        } else {
            //
            // last portion of match
            //
            size_t scanlen = lstrlen(scanItem);
            matchlen = lstrlen(wildMark);
            if(scanlen < matchlen) {
                return FALSE;
            }
            return _tcsicmp(scanItem+scanlen-matchlen,wildMark) ? FALSE : TRUE;
        }
        if(_istalpha(wildMark[0])) {
            //
            // scan for either lower or uppercase version of first character
            //

            //
            // the code suppresses the warning 28193 for the calls to _totupper
            // and _totlower.  This suppression is done because those functions
            // have a check return annotation on them.  However, they don't return
            // error codes and the check return annotation is really being used
            // to indicate that the return value of the function should be looked
            // at and/or assigned to a variable.  The check return annotation means
            // the return value should always be checked in all code paths.
            // We assign the return values to variables but the while loop does not 
            // examine both values in all code paths (e.g. when scanItem[0] == 0, 
            // neither u nor l will be examined) and it doesn't need to examine 
            // the values in all code paths.
            //
#pragma warning( suppress: 28193)
            TCHAR u = _totupper(wildMark[0]);
#pragma warning( suppress: 28193)
            TCHAR l = _totlower(wildMark[0]);
            while(scanItem[0] && scanItem[0]!=u && scanItem[0]!=l) {
                scanItem = CharNext(scanItem);
            }
            if(!scanItem[0]) {
                //
                // ran out of string
                //
                return FALSE;
            }
        } else {
            //
            // scan for first character (no case)
            //
            scanItem = _tcschr(scanItem,wildMark[0]);
            if(!scanItem) {
                //
                // ran out of string
                //
                return FALSE;
            }
        }
        //
        // try and match the sub-string at wildMark against scanItem
        //
        if(_tcsnicmp(scanItem,wildMark,matchlen)!=0) {
            //
            // nope, try again
            //
            scanItem = CharNext(scanItem);
            continue;
        }
        //
        // substring matched
        //
        scanItem += matchlen;
        wildMark += matchlen;
    }
    return (wildMark[0] ? FALSE : TRUE);
}

BOOL WildCompareHwIds(_In_ PZPWSTR Array, _In_ const IdEntry & MatchEntry)
/*++

Routine Description:

    Compares all strings in Array against Id
    Use WildCardMatch to do real compare

Arguments:

    Array - pointer returned by GetDevMultiSz
    MatchEntry - string to compare against

Return Value:

    TRUE if any match, otherwise FALSE

--*/
{
    if(Array) {
        while(Array[0]) {
            if(WildCardMatch(Array[0],MatchEntry)) {
                return TRUE;
            }
            Array++;
        }
    }
    return FALSE;
}

bool SplitCommandLine(
    _In_ int & argc, 
    _In_reads_(argc) LPTSTR * & argv, 
    _Out_ int & argc_right, 
    _Outref_result_buffer_(argc_right) LPTSTR * & argv_right)
/*++

Routine Description:

    Splits a command line into left and right of :=
    this is used for some of the more complex commands

Arguments:

    argc/argv - in/out
                - in, specifies the existing argc/argv
                - out, specifies the argc/argv to left of :=
    arc_right/argv_right - out
                - specifies the argc/argv to right of :=

Return Value:

    true - ":=" appears in line, false otherwise

--*/
{
    int i;
    for(i = 0;i<argc;i++) {
        if(_tcsicmp(argv[i],SPLIT_COMMAND_SEP)==0) {
            argc_right = argc-(i+1);
            argv_right = argv+(i+1);
            argc = i;
            return true;
        }
    }
    argc_right = 0;
    argv_right = argv+argc;
    return false;
}

int EnumerateDevices(_In_ LPCTSTR BaseName, _In_opt_ LPCTSTR Machine, _In_ DWORD Flags, _In_ int argc, _In_reads_(argc) PWSTR* argv, _In_ CallbackFunc Callback, _In_ LPVOID Context)
/*++

Routine Description:

    Generic enumerator for devices that will be passed the following arguments:
    <id> [<id>...]
    =<class> [<id>...]
    where <id> can either be @instance-id, or hardware-id and may contain wildcards
    <class> is a class name

Arguments:

    BaseName - name of executable
    Machine  - name of machine to enumerate
    Flags    - extra enumeration flags (eg DIGCF_PRESENT)
    argc/argv - remaining arguments on command line
    Callback - function to call for each hit
    Context  - data to pass function for each hit

Return Value:

    EXIT_xxxx

--*/
{
    HDEVINFO devs = INVALID_HANDLE_VALUE;
    IdEntry * templ = NULL;
    int failcode = EXIT_FAIL;
    int retcode;
    int argIndex;
    DWORD devIndex;
    SP_DEVINFO_DATA devInfo;
    SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail;
    BOOL doSearch = FALSE;
    BOOL match;
    BOOL all = FALSE;
    GUID cls;
    DWORD numClass = 0;
    int skip = 0;

    UNREFERENCED_PARAMETER(BaseName);

    if(!argc) {
        return EXIT_USAGE;
    }

    templ = new IdEntry[argc];
    if(!templ) {
        goto final;
    }

    //
    // determine if a class is specified
    //
    if(argc>skip && argv[skip][0]==CLASS_PREFIX_CHAR && argv[skip][1]) {
        if(!SetupDiClassGuidsFromNameEx(argv[skip]+1,&cls,1,&numClass,Machine,NULL) &&
            GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            goto final;
        }
        if(!numClass) {
            failcode = EXIT_OK;
            goto final;
        }
        skip++;
    }
    if(argc>skip && argv[skip][0]==WILD_CHAR && !argv[skip][1]) {
        //
        // catch convinient case of specifying a single argument '*'
        //
        all = TRUE;
        skip++;
    } else if(argc<=skip) {
        //
        // at least one parameter, but no <id>'s
        //
        all = TRUE;
    }

    //
    // determine if any instance id's were specified
    //
    // note, if =<class> was specified with no id's
    // we'll mark it as not doSearch
    // but will go ahead and add them all
    //
    for(argIndex=skip;argIndex<argc;argIndex++) {
        templ[argIndex] = GetIdType(argv[argIndex]);
        if(templ[argIndex].Wild || !templ[argIndex].InstanceId) {
            //
            // anything other than simple InstanceId's require a search
            //
            doSearch = TRUE;
        }
    }
    if(doSearch || all) {
        //
        // add all id's to list
        // if there's a class, filter on specified class
        //
        devs = SetupDiGetClassDevsEx(numClass ? &cls : NULL,
                                     NULL,
                                     NULL,
                                     (numClass ? 0 : DIGCF_ALLCLASSES) | Flags,
                                     NULL,
                                     Machine,
                                     NULL);

    } else {
        //
        // blank list, we'll add instance id's by hand
        //
        devs = SetupDiCreateDeviceInfoListEx(numClass ? &cls : NULL,
                                             NULL,
                                             Machine,
                                             NULL);
    }
    if(devs == INVALID_HANDLE_VALUE) {
        goto final;
    }
    for(argIndex=skip;argIndex<argc;argIndex++) {
        //
        // add explicit instances to list (even if enumerated all,
        // this gets around DIGCF_PRESENT)
        // do this even if wildcards appear to be detected since they
        // might actually be part of the instance ID of a non-present device
        //
        if(templ[argIndex].InstanceId) {
            SetupDiOpenDeviceInfo(devs,templ[argIndex].String,NULL,0,NULL);
        }
    }

    devInfoListDetail.cbSize = sizeof(devInfoListDetail);
    if(!SetupDiGetDeviceInfoListDetail(devs,&devInfoListDetail)) {
        goto final;
    }

    //
    // now enumerate them
    //
    if(all) {
        doSearch = FALSE;
    }

    devInfo.cbSize = sizeof(devInfo);
    for(devIndex=0;SetupDiEnumDeviceInfo(devs,devIndex,&devInfo);devIndex++) {

        if(doSearch) {
            for(argIndex=skip,match=FALSE;(argIndex<argc) && !match;argIndex++) {
                TCHAR devID[MAX_DEVICE_ID_LEN];
                LPTSTR *hwIds = NULL;
                LPTSTR *compatIds = NULL;
                //
                // determine instance ID
                //
                if(CM_Get_Device_ID_Ex(devInfo.DevInst,devID,MAX_DEVICE_ID_LEN,0,devInfoListDetail.RemoteMachineHandle)!=CR_SUCCESS) {
                    devID[0] = TEXT('\0');
                }

                if(templ[argIndex].InstanceId) {
                    //
                    // match on the instance ID
                    //
                    if(WildCardMatch(devID,templ[argIndex])) {
                        match = TRUE;
                    }
                } else {
                    //
                    // determine hardware ID's
                    // and search for matches
                    //
                    hwIds = GetDevMultiSz(devs,&devInfo,SPDRP_HARDWAREID);
                    compatIds = GetDevMultiSz(devs,&devInfo,SPDRP_COMPATIBLEIDS);

                    if(WildCompareHwIds(hwIds,templ[argIndex]) ||
                        WildCompareHwIds(compatIds,templ[argIndex])) {
                        match = TRUE;
                    }
                }
                DelMultiSz(hwIds);
                DelMultiSz(compatIds);
            }
        } else {
            match = TRUE;
        }
        if(match) {
            retcode = Callback(devs,&devInfo,devIndex,Context);
            if(retcode) {
                failcode = retcode;
                goto final;
            }
        }
    }

    failcode = EXIT_OK;

final:
    if(templ) {
        delete [] templ;
    }
    if(devs != INVALID_HANDLE_VALUE) {
        SetupDiDestroyDeviceInfoList(devs);
    }
    return failcode;

}

int
__cdecl
_tmain(_In_ int argc, _In_reads_(argc) PWSTR* argv)
/*++

Routine Description:

    Main entry point
    interpret -m:<machine>
    and hand off execution to command

Arguments:

    argc/argv - parameters passed to executable

Return Value:

    EXIT_xxxx

--*/
{
    LPCTSTR cmd;
    LPCTSTR baseName;
    LPCTSTR machine = NULL;
    int dispIndex;
    int firstArg = 1;
    int retval = EXIT_USAGE;
    BOOL autoReboot = FALSE;
    DWORD flags = 0;

    //
    // syntax:
    //
    // [options] [-]command [<arg> [<arg>]]
    //
    // options:
    // -m:<machine>  - remote
    // -r            - auto reboot
    // -f            - force operation
    //
    baseName = _tcsrchr(argv[0],TEXT('\\'));
    if(!baseName) {
        baseName = argv[0];
    } else {
        baseName = CharNext(baseName);
    }
    while((argc > firstArg) && ((argv[firstArg][0] == TEXT('-')) || (argv[firstArg][0] == TEXT('/')))) {
        if((argv[firstArg][1]==TEXT('m')) || (argv[firstArg][1]==TEXT('M'))) {
            if((argv[firstArg][2]!=TEXT(':')) || (argv[firstArg][3]==TEXT('\0'))) {
                //
                // don't recognize this switch
                //
                break;
            }
            machine = argv[firstArg]+3;
        } else if((argv[firstArg][1]==TEXT('r')) || (argv[firstArg][1]==TEXT('R'))) {
            if((argv[firstArg][2]!=TEXT('\0')) ) {
                //
                // don't recognize this switch
                //
                break;
            } else {
                autoReboot = TRUE;
            }
        } else if((argv[firstArg][1]==TEXT('f')) || (argv[firstArg][1]==TEXT('F'))) {
            if((argv[firstArg][2]!=TEXT('\0')) ) {
                //
                // don't recognize this switch
                //
                break;
            } else {
                flags |= DEVCON_FLAG_FORCE;
            }
        } else {
            //
            // don't recognize this switch
            //
            break;
        }
        firstArg++;
    }

    if((argc-firstArg) < 1) {
        //
        // after switches, must at least be command
        //
        Usage(baseName);
        return EXIT_USAGE;
    }
    cmd = argv[firstArg];
    if((cmd[0]==TEXT('-')) || (cmd[0]==TEXT('/'))) {
        //
        // command may begin '-' or '/'
        // eg, people might do devcon -help
        //
        cmd = CharNext(cmd);
    }
    firstArg++;
    for(dispIndex = 0;DispatchTable[dispIndex].cmd;dispIndex++) {
        if(_tcsicmp(cmd,DispatchTable[dispIndex].cmd)==0) {
            retval = DispatchTable[dispIndex].func(baseName,machine,flags,argc-firstArg,argv+firstArg);
            switch(retval) {
                case EXIT_USAGE:
                    CommandUsage(baseName,DispatchTable[dispIndex].cmd);
                    break;
                case EXIT_REBOOT:
                    if(autoReboot) {
                        Reboot();
                    }
                    break;
                case EXIT_OK:
                    break;
                default:
                    Failure(baseName,DispatchTable[dispIndex].cmd);
                    break;
            }
            return retval;
        }
    }
    Usage(baseName);
    return EXIT_USAGE;
}


