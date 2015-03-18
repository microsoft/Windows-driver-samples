/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    toastva.c

Abstract:

    The TOASTVA application illustrates installation techniques that can be
    used to seamlessly integrate PnP device installation with installation of
    value-added software, regardless of whether the software installation
    preceeded the hardware installation, or vice versa.

--*/

#include "precomp.h"
#pragma hdrstop


//
// Constants
//
#if defined(_IA64_)
    #define TOASTVA_PLATFORM_SUBDIRECTORY L"ia64"
#elif defined(_X86_)
    #define TOASTVA_PLATFORM_SUBDIRECTORY L"i386"
#elif defined(_AMD64_)
    #define TOASTVA_PLATFORM_SUBDIRECTORY L"amd64"
#elif defined(_ARM_)
    #define TOASTVA_PLATFORM_SUBDIRECTORY L"arm"
#elif defined(_ARM64_)
    #define TOASTVA_PLATFORM_SUBDIRECTORY L"arm64"
#else
#error Unsupported platform
#endif

#define TOASTVA_PLATFORM_SUBDIRECTORY_SIZE (sizeof(TOASTVA_PLATFORM_SUBDIRECTORY) / sizeof(WCHAR))

//
// Globals
//
HINSTANCE g_hInstance;


//
// Implementation
//

INT
WINAPI
WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd
    )
{
    LPWSTR *ArgList;
    INT NumArgs;
    WCHAR MediaRootDirectory[MAX_PATH];
    LPWSTR FileNamePart;
    DWORD DirPathLength;

    g_hInstance = hInstance;

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nShowCmd);

    //
    // Windows 2000 doesn't suppress auto-run applications when media (e.g.,
    // a CD) is inserted while a "Found New Hardware" popup is onscreen.  This
    // means that, by default, inserting a CD in order to supply PnP with the
    // necessary INF and driver files will result in the autorun app launching,
    // and obscuring the wizard, causing user confusion, etc.
    //
    // To avoid this, we retrieve an entrypoint to a Windows 2000 (and later)
    // Configuration Manager (CM) API that allows us to detect when a device
    // installation is in-progress, and suppress our own application from
    // starting.
    //
    if(IsDeviceInstallInProgress()) {
        //
        // We don't want to startup right now.  Don't worry--the value-added
        // software part of the device installation will be invoked (if
        // necessary) by our device co-installer during finish-install
        // processing.
        //
        return 0;
    }

    //
    // Retrieve the full directory path from which our setup program was
    // invoked.
    //
    ArgList = CommandLineToArgvW(GetCommandLine(), &NumArgs);

    if(ArgList && (NumArgs >= 1)) {

        DirPathLength = GetFullPathName(ArgList[0],
                                        MAX_PATH,
                                        MediaRootDirectory,
                                        &FileNamePart
                                       );

        if(DirPathLength >= MAX_PATH) {
            //
            // The directory is too large for our buffer.  Set our directory
            // path length to zero so we'll simply bail out in this rare case.
            //
            DirPathLength = 0;
        }

        if(DirPathLength) {
            //
            // Strip the filename off the path.
            //
            *FileNamePart = L'\0';

            DirPathLength = (DWORD)(FileNamePart - MediaRootDirectory);
        }

    } else {
        //
        // For some reason, we couldn't get the command line arguments that
        // were used when invoking our setup app.  Assume current directory
        // instead.
        //
        DirPathLength = GetCurrentDirectory(MAX_PATH, MediaRootDirectory);

        if(DirPathLength >= MAX_PATH) {
            //
            // The current directory is too large for our buffer.  Set our
            // directory path length to zero so we'll simply bail out in this
            // rare case.
            //
            DirPathLength = 0;
        }

        if(DirPathLength) {
            //
            // Ensure that path ends in a path separator character.
            //
            if((MediaRootDirectory[DirPathLength-1] != L'\\') &&
               (MediaRootDirectory[DirPathLength-1] != L'/'))
            {
                MediaRootDirectory[DirPathLength++] = L'\\';

                if(DirPathLength < MAX_PATH) {
                    MediaRootDirectory[DirPathLength] = L'\0';
                } else {
                    //
                    // Not enough room in buffer to add path separator char
                    //
                    DirPathLength = 0;
                }
            }
        }
    }

    if(ArgList) {
        GlobalFree(ArgList);
    }

    if(!DirPathLength) {
        //
        // Couldn't figure out what the root directory of our installation
        // media was.  Bail out.
        //
        return 0;
    }

    //
    // If we're being invoked from a platform-specific subdirectory (i.e.,
    // \i386 or \ia64), then strip off that subdirectory to get the true media
    // root path.
    //
    if(DirPathLength > TOASTVA_PLATFORM_SUBDIRECTORY_SIZE) {
        //
        // We know that the last character in our MediaRootDirectory string is
        // a path separator character.  Check to see if the preceding
        // characters match our platform-specific subdirectory.
        //
        if(!_wcsnicmp(&(MediaRootDirectory[DirPathLength - TOASTVA_PLATFORM_SUBDIRECTORY_SIZE]),
                      TOASTVA_PLATFORM_SUBDIRECTORY,
                      TOASTVA_PLATFORM_SUBDIRECTORY_SIZE - 1)) {
            //
            // Platform-specific part matches, just make sure preceding char
            // is a path separator char.
            //
            if((MediaRootDirectory[DirPathLength - TOASTVA_PLATFORM_SUBDIRECTORY_SIZE - 1] == L'\\') ||
               (MediaRootDirectory[DirPathLength - TOASTVA_PLATFORM_SUBDIRECTORY_SIZE - 1] == L'/')) {

                MediaRootDirectory[DirPathLength - TOASTVA_PLATFORM_SUBDIRECTORY_SIZE] = L'\0';
            }
        }
    }

    DoValueAddWizard(MediaRootDirectory);

    return 0;
}
