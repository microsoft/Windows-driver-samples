/*++

Copyright (c) Microsoft Corporation

Module Name:

    PowerFxApp.cpp

Abstract:

    This application can be used to exercise KMDF sample drivers for the 
    new power framework. See application "usage" details for more information.

Environment:

    user mode only

--*/

#include "include.h"

int __cdecl
wmain(
    _In_ int argc,
    _In_reads_(argc) PWSTR argv[]
    )
{
    DWORD err;
    WCHAR devicePath[MAX_DEVPATH_LENGTH] = {UNICODE_NULL};
    HANDLE hDevice = INVALID_HANDLE_VALUE;
    HANDLE hCompletionPort = NULL;

    
    //
    // Process user input.
    //
    err = ProcessUserInput(argc, argv);
    if (ERROR_SUCCESS != err)
    {
        goto clean0;
    }

    if ( !GetDevicePath(
            (LPGUID) &GUID_DEVINTERFACE_POWERFX,
            devicePath,
            COUNT_OF(devicePath)))
    {
        printf("Unable to get device path. Has the device driver been installed? \n");
        err = ERROR_OPEN_FAILED;
        goto clean0;
    }

    hDevice = CreateFile(devicePath,
                         GENERIC_READ|GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,
                         OPEN_EXISTING,
                         FILE_FLAG_OVERLAPPED,
                         NULL );

    if (hDevice == INVALID_HANDLE_VALUE) {
        err = GetLastError();
        printf("Failed to open device. Error %d.\n",err);
        goto clean0;
    }

    hCompletionPort = CreateIoCompletionPort(hDevice, NULL, 1, 0);
    if (hCompletionPort == NULL) {
        err = GetLastError();
        printf("Cannot open completion port %d.\n",err);
        goto clean0;
    }

    err = SendIO(hDevice,
                hCompletionPort,
                GetSetting(COMPONENT),
                GetSetting(MAX_OUTSTANDING_IO),
                GetSetting(DELAY),
                (BOOLEAN)GetSetting(CANCEL));
    if (ERROR_SUCCESS != err)
    {
        goto clean0;
    }
        
clean0:
    if (INVALID_HANDLE_VALUE != hDevice)
    {
        CloseHandle(hDevice);        
    }
    if (NULL != hCompletionPort)
    {
        CloseHandle(hCompletionPort);
    }
    return err;
}

DWORD Initialize(
    _In_ ULONG Count,
    _Out_ LPOVERLAPPED  *pOverlappedPtr,
    _Out_ PPOWERFX_READ_COMPONENT_INPUT *pInput,
    _Out_ PPOWERFX_READ_COMPONENT_OUTPUT *pOutput
    )
{
    PPOWERFX_READ_COMPONENT_INPUT inputBuffer = NULL;
    PPOWERFX_READ_COMPONENT_OUTPUT outputBuffer = NULL;
    LPOVERLAPPED pOverlapped = NULL;
    DWORD err = ERROR_SUCCESS;
    
    pOverlapped  = new OVERLAPPED[Count];
    if (NULL == pOverlapped)
    {
        err = ERROR_OUTOFMEMORY;
        goto clean0;
    }
    
    inputBuffer = new POWERFX_READ_COMPONENT_INPUT[Count];
    if (NULL == inputBuffer)
    {
        err = ERROR_OUTOFMEMORY;
        goto clean0;
    }

    outputBuffer = new POWERFX_READ_COMPONENT_OUTPUT[Count];
    if (NULL == outputBuffer)
    {
        err = ERROR_OUTOFMEMORY;
        goto clean0;
    }

    ZeroMemory(pOverlapped, 
            sizeof(OVERLAPPED)*Count);    
    
    for (UINT i = 0; i < Count; i++)
    {
        //
        // When the component number is set to UNUSED it indicates 
        // that a request has not been issued (or has completed)
        // using this input buffer. Hence the buffer along with the
        // overlapped structure at the corresponding index is available 
        // for issuing a request. 
        //
        inputBuffer[i].ComponentNumber = UNUSED;
    }    
    
    ZeroMemory(outputBuffer, 
            sizeof(POWERFX_READ_COMPONENT_OUTPUT)*Count);    

clean0:
    if (err != ERROR_SUCCESS)
    {
        delete[] pOverlapped;
        delete[] inputBuffer;
        delete[] outputBuffer;   
    }
    else
    {
        *pOverlappedPtr = pOverlapped;
        *pInput = inputBuffer;
        *pOutput = outputBuffer;
    }
    return err;
}
    
/*++

Routine Description:

    This function sends requests to the driver based on the settings passed 
    in the arguments. The requests are sent indefinitely until an error occurs.

    Depending on the number of maximum outstanding I/O requests,
    an array of overlapped structures and input/output buffers is allocated.
    The method then loops through the overlapped structure array
    to issue asynchronous requests. When a  request completes, the 
    overlapped structure for that request is not immediately re-used to issue 
    a new request. Instead the method goes in-order through the array to 
    ensure that each issued request is completed in a reasonable amount 
    of time and it is able to detect if one or more requests do not complete
    at all (or within the specified timeout).

--*/        
DWORD SendIO(
    _In_ HANDLE DeviceHandle,
    _In_ HANDLE CompletionPortHandle,
    _In_ ULONG Component,
    _In_ ULONG MaxOutstandingIo,
    _In_ ULONG Delay,
    _In_ BOOLEAN Cancel
    )
{
    LPOVERLAPPED pOverlapped = NULL;
    LPOVERLAPPED pOv = NULL;
    PPOWERFX_READ_COMPONENT_INPUT inputBuffer = NULL;
    PPOWERFX_READ_COMPONENT_OUTPUT outputBuffer = NULL;
    DWORD err = ERROR_SUCCESS;
    UINT outstandingIoCount = 0;
    UINT index = 0;

    srand((DWORD)GetTickCount64());

    err = Initialize(MaxOutstandingIo,
                    &pOverlapped,
                    &inputBuffer,
                    &outputBuffer);
    if (ERROR_SUCCESS != err)
    {
        goto clean0;
    }
    
    UINT k = 0;
    
    for (;;k++)
    {
        k = k % MaxOutstandingIo;
        
        if (UNUSED == inputBuffer[k].ComponentNumber)
        {            
            //
            // This indicates the input buffer and corresponding overlapped
            // structure is available to issue a new request.
            //
            pOv = &pOverlapped[k];
        }
        else
        {
            //
            // Wait for request #k to complete.
            //
            DWORD completionStatus;
            ULONGLONG startTime = GetTickCount64();

            for(;;)            
            {
                ULONG_PTR completedRequestIndex;
                
                err = WaitForIoCompletion(CompletionPortHandle,
                                        &pOv, 
                                        &completionStatus);
                if (ERROR_SUCCESS != err) {
                    goto clean0;
                }

                completedRequestIndex = pOv-pOverlapped;

                printf(" Request %d completed with status 0x%X.\n",
                        (DWORD)completedRequestIndex, completionStatus);   
                
                if (ERROR_SUCCESS != completionStatus)
                {
                    if (!Cancel ||
                        ERROR_OPERATION_ABORTED != completionStatus)
                    {
                        //
                        // If there is a setting to cancel requests it is ok for
                        //  the requests to complete with aborted status.
                        //
                        err = completionStatus;
                        printf(" Unexpected completion status %d. \n",
                            completionStatus);
                        goto clean0;
                    }
                }
                else
                {
                    //
                    // If request completed successfully verify the contents of the buffer.
                    //                
                    if (! VerifyRequest(&inputBuffer[completedRequestIndex],
                                        &outputBuffer[completedRequestIndex]))
                    {
                        printf(" Request completed with unexpected data in"
                               " output buffer. \n");
                        err = ERROR_INVALID_DATA;
                        goto clean0;
                    }                    
                }
                
                inputBuffer[completedRequestIndex].ComponentNumber = UNUSED;
                outstandingIoCount--;

                if (k == completedRequestIndex)
                {
                    //
                    // The request we are looking for has completed. 
                    //
                    pOv = &pOverlapped[k];
                    break;
                }
                else if (GetTickCount64() - startTime > REQUEST_TIMEOUT)
                {
                    //
                    // The request we are looking for did not complete on time.
                    //
                    err = ERROR_TIMEOUT;
                    printf(" Request %d did not complete within the expected"
                           " time. \n", k); 
                    assert(0);
                    goto clean0;
                }                                            
            }
            
        }

        //
        // We now have an overlapped structure to use. Set the input buffer 
        // to the target component and send the request.
        //
        ZeroMemory(pOv, sizeof(OVERLAPPED));
        inputBuffer[k].ComponentNumber = (Component == RANDOM_COMPONENT) ? 
                                        (rand() % COMPONENT_COUNT):
                                        Component;

        if (0 != Delay)
        {            
            //
            // If there is a setting to introduce a delay, sleep and then send the 
            // request.
            // 
            Sleep(rand() % Delay);
        }
        
        err = SendRequest(DeviceHandle,
                        pOv,
                        &inputBuffer[k],
                        &outputBuffer[k]);
        if (ERROR_SUCCESS != err)
        {
            goto clean0;
        }

        outstandingIoCount++;

        printf(" Request number %d sent to component %d.\n", k,
                                inputBuffer[k].ComponentNumber);
        
        if (Cancel)
        {
            // 
            // If there is a setting to cancel the request then cancel it after
            // issuing it.
            //
            CancelIoEx(DeviceHandle,
                        pOv);
        }
    }

clean0:
    if (outstandingIoCount > 0)
    {
        CancelIo(DeviceHandle);
        for (index=0; index < outstandingIoCount; index++)
        {
            WaitForIoCompletion(CompletionPortHandle, 
                            NULL,
                            NULL);
        }
    }
    delete[] pOverlapped;
    delete[] inputBuffer;
    delete[] outputBuffer;    
    return err;
}

BOOLEAN
VerifyRequest(
    _In_ PPOWERFX_READ_COMPONENT_INPUT input,
    _In_ PPOWERFX_READ_COMPONENT_OUTPUT output)
{
    return (output->ComponentData == ~input->ComponentNumber);
}
    
DWORD
SendRequest(
    _In_ HANDLE DeviceHandle,
    _In_ LPOVERLAPPED OverlappedPtr,
    _In_ PPOWERFX_READ_COMPONENT_INPUT inputBuffer,
    _In_ PPOWERFX_READ_COMPONENT_OUTPUT outputBuffer)
{

    BOOL bResult = FALSE;
    DWORD err = ERROR_SUCCESS;
    
    bResult = DeviceIoControl(DeviceHandle,
                            (DWORD) IOCTL_POWERFX_READ_COMPONENT,
                            (PVOID)inputBuffer,
                            sizeof(POWERFX_READ_COMPONENT_INPUT),
                            (PVOID)outputBuffer,
                            sizeof(POWERFX_READ_COMPONENT_OUTPUT),
                            NULL,
                            OverlappedPtr);
    if (FALSE == bResult)
    {
        err = GetLastError();
        if (ERROR_IO_PENDING == err)
        {
            //
            // This is not really an error.
            //
            err = ERROR_SUCCESS;
        }
        else
        {
            printf("Unable to send request. DeviceIoControl failed with "
                "error 0x%X. \n", err);
            goto clean0;
        }
    }
clean0:
    return err;
}

DWORD
WaitForIoCompletion(
    _In_ HANDLE CompletionPortHandle,
    _In_opt_ LPOVERLAPPED* POvPtr,
    _Out_opt_ PDWORD CompletionStatus)
{
    BOOL bResult;
    DWORD err;
    DWORD numBytes;
    ULONG_PTR completionKey;
    LPOVERLAPPED ovPtr;
    DWORD completionStatus;

    //
    // Assume successful completion of I/O request
    //
    completionStatus = ERROR_SUCCESS;

    //
    // Dequeue a completion packet
    //
    bResult = GetQueuedCompletionStatus(CompletionPortHandle,
                                        &numBytes,
                                        &completionKey,
                                        &ovPtr,
                                        REQUEST_TIMEOUT);
    if (FALSE == bResult) 
    {
        err = GetLastError();
        if (NULL == ovPtr) 
        {
           printf("Could not dequeue a completion packet. "
                  "GetQueuedCompletionStatus failed with error 0x%X.", 
                        err);           
            assert(0);
            goto clean0;
        }
        
        //
        // We dequeued a completion packet for an I/O operation that failed. 
        // Make a note of the failure status, but we need to return success from
        // this function because we got a completion packet (even though it was
        // for a failed I/O operation).
        //
        completionStatus = err;
        err = ERROR_SUCCESS;
    }

    if (NULL != POvPtr) {
        *POvPtr = ovPtr;
    }

    if (NULL != CompletionStatus) {
        *CompletionStatus = completionStatus;
    }

    err = ERROR_SUCCESS;

clean0:
    return err;        
}

BOOL
GetDevicePath(
    IN  LPGUID InterfaceGuid,
    _Out_writes_(BufLen) PWSTR DevicePath,
    _In_ size_t BufLen
    )
{
    CONFIGRET cr = CR_SUCCESS;
    PWSTR deviceInterfaceList = NULL;
    ULONG deviceInterfaceListLength = 0;
    PWSTR nextInterface;
    HRESULT hr = E_FAIL;
    BOOL bRet = TRUE;

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

    nextInterface = deviceInterfaceList + wcslen(deviceInterfaceList) + 1;
    if (*nextInterface != UNICODE_NULL) {
        printf("Warning: More than one device interface instance found. \n"
            "Selecting first matching device.\n\n");
    }

    hr = StringCchCopy(DevicePath, BufLen, deviceInterfaceList);
    if (FAILED(hr)) {
        bRet = FALSE;
        printf("Error: StringCchCopy failed with HRESULT 0x%x", hr);
        goto clean0;
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


