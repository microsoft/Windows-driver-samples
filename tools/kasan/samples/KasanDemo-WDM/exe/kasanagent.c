/*++

Copyright (c) 2024  Microsoft Corporation All Rights Reserved

Module Name:

    agent.c

Abstract:

    Usermode agent that interacts with the kasantrigger.sys driver to
    demonstrate how KASAN detects illegal memory accesses.

Environment:

    Win32 console multi-threaded application.

--*/

#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strsafe.h>
#include <sys\kasantrigger.h>

VOID __cdecl
main(
    _In_ ULONG argc,
    _In_reads_(argc) PCHAR argv[]
    )
{
    ULONG bytesReturned;
    INT choice;
    HANDLE hDevice;
    ULONG index;
    DWORD ioctlCode;
    BOOLEAN isKasanEnabledOnDriver;
    BOOL success;
    UCHAR value;

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    printf(
        " _   __                       ______                     \n"
        "| | / /                       |  _  \\                    \n"
        "| |/ /  __ _ ___  __ _ _ __   | | | |___ _ __ ___   ___  \n"
        "|    \\ / _` / __|/ _` | '_ \\  | | | / _ \\ '_ ` _ \\ / _ \\ \n"
        "| |\\  \\ (_| \\__ \\ (_| | | | | | |/ /  __/ | | | | | (_) |\n"
        "\\_| \\_/\\__,_|___/\\__,_|_| |_| |___/ \\___|_| |_| |_|\\___/  \n\n"
    );

    //
    // Open the device.
    //

    hDevice = CreateFile("\\\\.\\KasanTrigger",
                         GENERIC_READ | GENERIC_WRITE,
                         0,
                         NULL,
                         CREATE_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL);

    if (hDevice == INVALID_HANDLE_VALUE) {
        printf("CreateFile failed: %d. Did you forget to load kasantrigger.sys?\n",
               GetLastError());
        return;
    }

    //
    // Check whether KASAN is enabled on the driver.
    //

    success = DeviceIoControl(hDevice,
                              (DWORD)IOCTL_KASANTRIGGER_INFO,
                              NULL,
                              0,
                              &isKasanEnabledOnDriver,
                              sizeof(isKasanEnabledOnDriver),
                              &bytesReturned,
                              NULL);
    if (!success) {
        printf("DeviceIoControl failed: %d\n\n", GetLastError());
        return;
    }

    if (!isKasanEnabledOnDriver) {
        printf("WARNING: KASAN is not enabled on kasantrigger.sys\n\n");
    }

    printf("Enter a number in the menu below to trigger the desired condition:\n");
    printf("1. Trigger a stack out-of-bounds read\n");
    printf("2. Trigger a global out-of-bounds read\n");
    printf("3. Trigger a heap out-of-bounds read\n\n");

    while (TRUE) {
        printf("> ");
        while ((choice = getchar()) == '\n');

        switch (choice) {
        case '1':
            ioctlCode = (DWORD)IOCTL_KASANTRIGGER_OOBR_STACK;
            break;
        case '2':
            ioctlCode = (DWORD)IOCTL_KASANTRIGGER_OOBR_GLOBAL;
            break;
        case '3':
            ioctlCode = (DWORD)IOCTL_KASANTRIGGER_OOBR_HEAP;
            break;
        default:
            printf("Incorrect input, try again\n");
            continue;
        }

        printf("You have selected %c. Excellent choice. ", choice);
        Sleep(1000);
        printf("Triggering in 3...");
        Sleep(1000);
        printf(" 2...");
        Sleep(1000);
        printf(" 1...");
        Sleep(1000);
        printf("\n");

        index = KASANTRIGGER_ARRAY_SIZE;
        value = 0;

        success = DeviceIoControl(hDevice,
                                  ioctlCode,
                                  &index,
                                  sizeof(index),
                                  &value,
                                  sizeof(value),
                                  &bytesReturned,
                                  NULL);
        if (!success) {
            printf("DeviceIoControl failed: %d\n", GetLastError());
            return;
        }

        //
        // KASAN should have normally triggered a bugcheck, so this should be
        // unreachable.
        //

        if (isKasanEnabledOnDriver) {
            printf("Still alive. The bugcheck was not issued. This is not expected.\n");
        } else {
            printf("Still alive, because KASAN was not enabled on the driver. The illegal access went undetected.\n");
        }
    }

    CloseHandle(hDevice);
}