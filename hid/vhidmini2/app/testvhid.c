/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    testvhid.c

Environment:

    user mode only

Author:

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <cfgmgr32.h>
#include <hidsdi.h>
#include "common.h"
#include <time.h>

//
// These are the default device attributes set in the driver
// which are used to identify the device.
//
#define HIDMINI_DEFAULT_PID              0xFEED
#define HIDMINI_DEFAULT_VID              0xDEED

//
// These are the device attributes returned by the mini driver in response
// to IOCTL_HID_GET_DEVICE_ATTRIBUTES.
//
#define HIDMINI_TEST_PID              0xDEEF
#define HIDMINI_TEST_VID              0xFEED
#define HIDMINI_TEST_VERSION          0x0505

USAGE g_MyUsagePage = 0xFF00;
USAGE g_MyUsage = 0x01;

#define USE_ALT_USAGE_PAGE "UseAltUsagePage"

//
// Function prototypes
//
BOOLEAN
GetFeature(
    HANDLE file
    );

BOOLEAN
SetFeature(
    HANDLE file
    );

BOOLEAN
GetInputReport(
    HANDLE file
    );

BOOLEAN
SetOutputReport(
    HANDLE file
    );

BOOLEAN
CheckIfOurDevice(
    HANDLE file
    );

BOOLEAN
ReadInputData(
    _In_ HANDLE file
    );

BOOLEAN
WriteOutputData(
    _In_ HANDLE file
    );

BOOLEAN
GetIndexedString(
    HANDLE File
    );

BOOLEAN
GetStrings(
    HANDLE File
    );

BOOLEAN
FindMatchingDevice(
    _In_ LPGUID   Guid,
    _Out_ HANDLE* Handle
    );

//
// Implementation
//
INT __cdecl
main(
    _In_ ULONG argc,
    _In_reads_(argc) PCHAR argv[]
    )
{
    GUID hidguid;
    HANDLE file = INVALID_HANDLE_VALUE;
    BOOLEAN found = FALSE;
    BOOLEAN bSuccess = FALSE;


    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);


    srand( (unsigned)time( NULL ) );

    HidD_GetHidGuid(&hidguid);

    found = FindMatchingDevice(&hidguid, &file);
    if (found) {
        printf("...sending control request to our device\n");

        //
        // Get/Set feature loopback
        //
        bSuccess = GetFeature(file);
        if (bSuccess == FALSE) {
            goto cleanup;
        }

        bSuccess = SetFeature(file);
        if (bSuccess == FALSE) {
            goto cleanup;
        }

        bSuccess = GetFeature(file);
        if (bSuccess == FALSE) {
            goto cleanup;
        }

        //
        // Get/Set report loopback
        //
        bSuccess = GetInputReport(file);
        if (bSuccess == FALSE) {
            goto cleanup;
        }

        bSuccess = SetOutputReport(file);
        if (bSuccess == FALSE) {
            goto cleanup;
        }

        bSuccess = GetInputReport(file);
        if (bSuccess == FALSE) {
            goto cleanup;
        }

        //
        // Read/Write report loopback
        //
        bSuccess = ReadInputData(file);
        if (bSuccess == FALSE) {
            goto cleanup;
        }

        bSuccess = WriteOutputData(file);
        if (bSuccess == FALSE) {
            goto cleanup;
        }

        bSuccess = ReadInputData(file);
        if (bSuccess == FALSE) {
            goto cleanup;
        }

        //
        // Get Strings
        //
        bSuccess = GetIndexedString(file);
        if (bSuccess == FALSE) {
            goto cleanup;
        }

        bSuccess = GetStrings(file);
        if (bSuccess == FALSE) {
            goto cleanup;
        }

    }
    else {
        printf("Failure: Could not find our HID device \n");
    }

cleanup:

    if (found && bSuccess == FALSE) {
        printf("****** Failure: one or more commands to device failed *******\n");
    }

    if (file != INVALID_HANDLE_VALUE) {
        CloseHandle(file);
    }

    return (bSuccess ? 0 : 1);
}

BOOLEAN
FindMatchingDevice(
    _In_  LPGUID  InterfaceGuid,
    _Out_ HANDLE* Handle
)
{
    CONFIGRET cr = CR_SUCCESS;
    PWSTR deviceInterfaceList = NULL;
    ULONG deviceInterfaceListLength = 0;
    PWSTR currentInterface;
    BOOLEAN bRet = FALSE;
    HANDLE devHandle = INVALID_HANDLE_VALUE;

    if (NULL == Handle) {
        printf("Error: Invalid device handle parameter\n");
        return FALSE;
    }

    *Handle = INVALID_HANDLE_VALUE;

    cr = CM_Get_Device_Interface_List_Size(
            &deviceInterfaceListLength,
            InterfaceGuid,
            NULL,
            CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cr != CR_SUCCESS) {
        printf("Error 0x%x retrieving device interface list size.\n", cr);
        goto clean0;
    }

    if (deviceInterfaceListLength <= 1) {
        bRet = FALSE;
        printf("Error: No active device interfaces found.\n"
            " Is the sample driver loaded?");
        goto clean0;
    }

    deviceInterfaceList = (PWSTR)malloc(deviceInterfaceListLength * sizeof(WCHAR));
    if (deviceInterfaceList == NULL) {
        printf("Error allocating memory for device interface list.\n");
        goto clean0;
    }
    ZeroMemory(deviceInterfaceList, deviceInterfaceListLength * sizeof(WCHAR));

    cr = CM_Get_Device_Interface_List(
            InterfaceGuid,
            NULL,
            deviceInterfaceList,
            deviceInterfaceListLength,
            CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cr != CR_SUCCESS) {
        printf("Error 0x%x retrieving device interface list.\n", cr);
        goto clean0;
    }

    //
    // Enumerate devices of this interface class
    //
    printf("\n....looking for our HID device (with UP=0x%04X "
        "and Usage=0x%02X)\n", g_MyUsagePage, g_MyUsage);

    for (currentInterface = deviceInterfaceList;
        *currentInterface;
        currentInterface += wcslen(currentInterface) + 1) {

        devHandle = CreateFile(currentInterface,
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, // no SECURITY_ATTRIBUTES structure
                        OPEN_EXISTING, // No special create flags
                        0, // No special attributes
                        NULL); // No template file

        if (INVALID_HANDLE_VALUE == devHandle) {
            printf("Warning: CreateFile failed: %d\n", GetLastError());
            continue;
        }

        if (CheckIfOurDevice(devHandle)) {
            bRet = TRUE;
            *Handle = devHandle;
        }
        else {
            CloseHandle(devHandle);
        }
    }

clean0:
    if (deviceInterfaceList != NULL) {
        free(deviceInterfaceList);
    }
    if (CR_SUCCESS != cr) {
        bRet = FALSE;
    }
    return bRet;
}

BOOLEAN
CheckIfOurDevice(
    HANDLE file)
{
    PHIDP_PREPARSED_DATA Ppd; // The opaque parser info describing this device
    HIDP_CAPS                       Caps; // The Capabilities of this hid device.
    HIDD_ATTRIBUTES attr; // Device attributes

    if (!HidD_GetAttributes(file, &attr))
    {
        printf("Error: HidD_GetAttributes failed \n");
        return FALSE;
    }

    printf("Device Attributes - PID: 0x%x, VID: 0x%x \n", attr.ProductID, attr.VendorID);
    if ((attr.VendorID != HIDMINI_DEFAULT_VID) || (attr.ProductID != HIDMINI_DEFAULT_PID))
    {
        printf("Device attributes doesn't match the sample \n");
        return FALSE;
    }

    if (!HidD_GetPreparsedData (file, &Ppd))
    {
        printf("Error: HidD_GetPreparsedData failed \n");
        return FALSE;
    }

    if (!HidP_GetCaps (Ppd, &Caps))
    {
        printf("Error: HidP_GetCaps failed \n");
        HidD_FreePreparsedData (Ppd);
        return FALSE;
    }

    if ((Caps.UsagePage == g_MyUsagePage) && (Caps.Usage == g_MyUsage)){
        printf("Success: Found my device.. \n");
        return TRUE;

    }

    return FALSE;

}

BOOLEAN
GetFeature(
    HANDLE file
    )
{
    PMY_DEVICE_ATTRIBUTES myDevAttributes = NULL;
    ULONG bufferSize;
    PUCHAR buffer;
    BOOLEAN bSuccess;

    //
    // Allocate memory equal to 1 byte for report ID + size of
    // feature report. Buffer size for get feature should be atleast equal to
    // the size of feature report.
    //
    bufferSize = FEATURE_REPORT_SIZE_CB + 1;
    buffer = (PUCHAR) malloc (bufferSize);
    if (!buffer )
    {
        printf("malloc failed\n");
        return FALSE;
    }

    ZeroMemory(buffer, bufferSize);

    //
    // Fill the first byte with report ID of control collection
    //
    buffer[0] = CONTROL_COLLECTION_REPORT_ID;

    //
    // Send Hid control code thru HidD_GetFeature API
    //
    bSuccess = HidD_GetFeature(file,  // HidDeviceObject,
                                   buffer,    // ReportBuffer,
                                   bufferSize // ReportBufferLength
                                   );
    if (!bSuccess)
    {
        printf("failed HidD_GetFeature\n");
    }
    else
    {
        myDevAttributes = (PMY_DEVICE_ATTRIBUTES) (buffer + 1); // +1 to skip report id

        printf("Received following feature attributes from device: \n"
               "    VendorID: 0x%x, \n"
               "    ProductID: 0x%x, \n"
               "    VersionNumber: 0x%x\n",
               myDevAttributes->VendorID,
               myDevAttributes->ProductID,
               myDevAttributes->VersionNumber);
    }

    free(buffer);
    return bSuccess;
}

BOOLEAN
SetFeature(
    HANDLE file
    )
{
    PHIDMINI_CONTROL_INFO  controlInfo = NULL;
    PMY_DEVICE_ATTRIBUTES myDevAttributes = NULL;
    ULONG bufferSize;
    BOOLEAN bSuccess;

    //
    // Allocate memory equal to HIDMINI_CONTROL_INFO
    //
    bufferSize = sizeof(HIDMINI_CONTROL_INFO);
    controlInfo = (PHIDMINI_CONTROL_INFO) malloc (bufferSize);
    if (!controlInfo )
    {
        printf("malloc failed\n");
        return FALSE;
    }

    ZeroMemory(controlInfo, bufferSize);

    //
    // Fill the control structure with the report ID of the control collection and
    // the control code.
    //
    controlInfo->ReportId = CONTROL_COLLECTION_REPORT_ID;
    controlInfo->ControlCode = HIDMINI_CONTROL_CODE_SET_ATTRIBUTES;
    myDevAttributes = (PMY_DEVICE_ATTRIBUTES)&controlInfo->u.Attributes;
    myDevAttributes->VendorID = HIDMINI_TEST_VID;
    myDevAttributes->ProductID = HIDMINI_TEST_PID;
    myDevAttributes->VersionNumber = HIDMINI_TEST_VERSION;

    //
    // Set feature
    //
    bSuccess = HidD_SetFeature(file,         // HidDeviceObject,
                         controlInfo,  // ReportBuffer,
                         bufferSize    // ReportBufferLength
                         );
    if (!bSuccess)
    {
        printf("failed HidD_SetFeature \n");
    }
    else
    {
        printf("Set following feature attributes on device: \n"
               "    VendorID: 0x%x, \n"
               "    ProductID: 0x%x, \n"
               "    VersionNumber: 0x%x\n",
               myDevAttributes->VendorID,
               myDevAttributes->ProductID,
               myDevAttributes->VersionNumber);
    }

    free(controlInfo);

    return bSuccess;
}

BOOLEAN
GetInputReport(
    HANDLE file
    )
{
    ULONG bufferSize;
    PUCHAR buffer;
    BOOLEAN bSuccess;

    //
    // Allocate memory
    //
    bufferSize = sizeof(HIDMINI_INPUT_REPORT);
    buffer = (PUCHAR) malloc (bufferSize);
    if (!buffer )
    {
        printf("malloc failed\n");
        return FALSE;
    }

    ZeroMemory(buffer, bufferSize);

    //
    // Fill the first byte with report ID of collection
    //
    buffer[0] = CONTROL_COLLECTION_REPORT_ID;

    //
    // Send Hid control code
    //
    bSuccess = HidD_GetInputReport(file,  // HidDeviceObject,
                               buffer,    // ReportBuffer,
                               bufferSize // ReportBufferLength
                               );
    if (!bSuccess)
    {
        printf("failed HidD_GetInputReport\n");
    }
    else
    {
        printf("Received following data in input report: %d\n",
           ((PHIDMINI_INPUT_REPORT) buffer)->Data);
    }

    free(buffer);
    return bSuccess;
}


BOOLEAN
SetOutputReport(
    HANDLE file
    )
{
    ULONG bufferSize;
    PHIDMINI_OUTPUT_REPORT buffer;
    BOOLEAN bSuccess;

    //
    // Allocate memory
    //
    bufferSize = sizeof(HIDMINI_OUTPUT_REPORT);
    buffer = (PHIDMINI_OUTPUT_REPORT) malloc (bufferSize);
    if (!buffer )
    {
        printf("malloc failed\n");
        return FALSE;
    }

    ZeroMemory(buffer, bufferSize);

    //
    // Fill the report
    //
    buffer->ReportId = CONTROL_COLLECTION_REPORT_ID;
    buffer->Data = (UCHAR) (rand() % UCHAR_MAX);

    //
    // Send Hid control code
    //
    bSuccess = HidD_SetOutputReport(file,  // HidDeviceObject,
                               buffer,    // ReportBuffer,
                               bufferSize // ReportBufferLength
                               );
    if (!bSuccess)
    {
        printf("failed HidD_SetOutputReport\n");
    }
    else
    {
        printf("Set following data in output report: %d\n",
           ((PHIDMINI_OUTPUT_REPORT) buffer)->Data);
    }

    free(buffer);
    return bSuccess;
}

BOOLEAN
ReadInputData(
    _In_ HANDLE file
    )
{
    PHIDMINI_INPUT_REPORT report;
    ULONG bufferSize;
    BOOL bSuccess;
    DWORD bytesRead;

    //
    // Allocate memory
    //
    bufferSize = sizeof(HIDMINI_INPUT_REPORT);
    report = (PHIDMINI_INPUT_REPORT) malloc (bufferSize);
    if (!report )
    {
        printf("malloc failed\n");
        return FALSE;
    }

    ZeroMemory(report, bufferSize);

    report->ReportId = CONTROL_COLLECTION_REPORT_ID;

    //
    // get input data.
    //
    bSuccess = ReadFile(
              file,        // HANDLE hFile,
              report,      // LPVOID lpBuffer,
              bufferSize,  // DWORD nNumberOfBytesToRead,
              &bytesRead,  // LPDWORD lpNumberOfBytesRead,
              NULL         // LPOVERLAPPED lpOverlapped
            );

    if (!bSuccess)
    {
        printf("failed ReadFile \n");
    }
    else
    {
        printf("Read following byte from device: %d\n",
            report->Data);
    }

    free(report);

    return (BOOLEAN) bSuccess;
}

BOOLEAN
WriteOutputData(
    _In_ HANDLE file
    )
{
    PHIDMINI_OUTPUT_REPORT outputReport;
    ULONG outputReportSize;
    BOOL bSuccess;
    DWORD bytesWritten;

    //
    // Allocate memory for outtput report
    //
    outputReportSize = sizeof(HIDMINI_OUTPUT_REPORT);
    outputReport = (PHIDMINI_OUTPUT_REPORT) malloc (outputReportSize);
    if (!outputReport )
    {
        printf("malloc failed\n");
        return FALSE;
    }

    ZeroMemory(outputReport, outputReportSize);

    outputReport->ReportId = CONTROL_COLLECTION_REPORT_ID;
    outputReport->Data = (UCHAR) (rand() % UCHAR_MAX);

    //
    // Wrute output data.
    //
    bSuccess = WriteFile(
              file,        // HANDLE hFile,
              (PVOID) outputReport,      // LPVOID lpBuffer,
              outputReportSize,  // DWORD nNumberOfBytesToRead,
              &bytesWritten,  // LPDWORD lpNumberOfBytesRead,
              NULL         // LPOVERLAPPED lpOverlapped
            );

    if (!bSuccess)
    {
        printf("failed WriteFile \n");
    }
    else
    {
        printf("Wrote following byte to device: %d\n",
            outputReport->Data);
    }

    free(outputReport);

    return (BOOLEAN) bSuccess;
}

BOOLEAN
GetIndexedString(
    HANDLE File
    )
{
    BOOLEAN bSuccess;
    BYTE* buffer;
    ULONG bufferLength;

    bufferLength = MAXIMUM_STRING_LENGTH;
    buffer = malloc(bufferLength);
    if (!buffer )
    {
        printf("malloc failed\n");
        return FALSE;
    }

    ZeroMemory(buffer, bufferLength);

    bSuccess = HidD_GetIndexedString(
        File,
        VHIDMINI_DEVICE_STRING_INDEX,  // IN ULONG  StringIndex,
        (PVOID) buffer,  //OUT PVOID  Buffer,
        bufferLength // IN ULONG  BufferLength
        ) ;

    if (!bSuccess)
    {
        printf("failed WriteFile \n");
    }
    else
    {
        printf("Indexed string: %S\n", (PWSTR) buffer);
    }

    free(buffer);

    return bSuccess;
}

BOOLEAN
GetStrings(
    HANDLE File
    )
{
    BOOLEAN bSuccess;
    BYTE* buffer;
    ULONG bufferLength;

    bufferLength = MAXIMUM_STRING_LENGTH;
    buffer = malloc(bufferLength);
    if (!buffer )
    {
        printf("malloc failed\n");
        return FALSE;
    }

    ZeroMemory(buffer, bufferLength);
    bSuccess = HidD_GetProductString(
        File,
        (PVOID) buffer,  //OUT PVOID  Buffer,
        bufferLength // IN ULONG  BufferLength
        );

    if (!bSuccess)
    {
        printf("failed HidD_GetProductString \n");
        goto exit;
    }
    else
    {
        printf("Product string: %S\n", (PWSTR) buffer);
    }

    ZeroMemory(buffer, bufferLength);
    bSuccess = HidD_GetSerialNumberString(
        File,
        (PVOID) buffer,  //OUT PVOID  Buffer,
        bufferLength // IN ULONG  BufferLength
        );

    if (!bSuccess)
    {
        printf("failed HidD_GetSerialNumberString \n");
        goto exit;
    }
    else
    {
        printf("Serial number string: %S\n", (PWSTR) buffer);
    }

    ZeroMemory(buffer, bufferLength);
    bSuccess = HidD_GetManufacturerString(
        File,
        (PVOID) buffer,  //OUT PVOID  Buffer,
        bufferLength // IN ULONG  BufferLength
        );

    if (!bSuccess)
    {
        printf("failed HidD_GetManufacturerString \n");
        goto exit;
    }
    else
    {
        printf("Manufacturer string: %S\n", (PWSTR) buffer);
    }

exit:

    free(buffer);

    return bSuccess;
}


