/*++

Copyright (c) Microsoft Corporation

Module Name:

    sampletest.cpp

Abstract:

    This file demonstrates I/O to TrEE secure services.

Environment:

    User mode

--*/

#pragma warning(disable:4127)

#include <initguid.h>
#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <Windows.h>
#include <winioctl.h>
#include <cfgmgr32.h>
#include <stdio.h>
#include <stdlib.h>

typedef LONG NTSTATUS;

#include <TrustedRT.h>
#include <SampleSecureService.h>
#include <SampleOSService.h>

#define TEST_ASSERT(_Assertion, _Message, ...) do { \
        if (!(_Assertion)) { \
            wprintf(_Message L"\n", __VA_ARGS__); \
            Success = FALSE; \
            goto End; \
        }\
    } while (0)

#define TEST_COMMENT(_Message, ...) wprintf(_Message L"\n", __VA_ARGS__)

#define TEST_STRING L"This is a test string.\n"

HANDLE
OpenServiceHandleByInterface(
    _In_ LPCGUID ServiceGuid
    )

/*++

Routine Description:

    This routine opens a handle to the service device specified the GUID using
    PnP device interface. The device interface gives a symbolic link to the
    service device that can be fed to CreateFile function.

Arguments:

    ServiceGuid - Supplies the GUID of the service to open.

Return Value:

    HANDLE to the secure service, NULL if error occurred.

--*/

{
    CONFIGRET ConfigRet;
    WCHAR InterfaceSymlink[1024];
    HANDLE ServiceHandle;
    BOOL Success;

    ServiceHandle = INVALID_HANDLE_VALUE;
    ConfigRet = CM_Get_Device_Interface_ListW(
        const_cast<LPGUID>(ServiceGuid),
        NULL,
        InterfaceSymlink,
        _countof(InterfaceSymlink),
        CM_GET_DEVICE_INTERFACE_LIST_PRESENT
        );

    TEST_ASSERT(ConfigRet == CR_SUCCESS, L"GetDeviceInterface failed, configret=%d", ConfigRet);
    TEST_COMMENT(L"Symlink=%ws", InterfaceSymlink);

    ServiceHandle = CreateFileW(
        InterfaceSymlink,
        FILE_READ_DATA | FILE_WRITE_DATA,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL);

    TEST_ASSERT(ServiceHandle != INVALID_HANDLE_VALUE, L"Service open failed");

End:
    return ServiceHandle;
}

HANDLE
OpenServiceHandleByFilename(
    _In_ LPCGUID ServiceGuid
    )

/*++

Routine Description:

    This routine opens a handle to the service device specified the GUID using
    TrEE-namespace filename. The filename has format \\.\WindowsTrustedRT\{GUID}.
    The filename is parsed in IRP_MJ_CREATE handler in TrEE class extension.
    Then request is forwarded to corresponding service device.

Arguments:

    ServiceGuid - Supplies the GUID of the service to open.

Return Value:

    HANDLE to the secure service, NULL if error occurred.

--*/

{
    WCHAR InterfaceGuid[1024];
    WCHAR InterfaceSymlink[1024];
    HANDLE ServiceHandle;
    BOOL Success;

    ServiceHandle = INVALID_HANDLE_VALUE;
    swprintf_s(InterfaceGuid,
               L"{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
               ServiceGuid->Data1,
               ServiceGuid->Data2,
               ServiceGuid->Data3,
               ServiceGuid->Data4[0],
               ServiceGuid->Data4[1],
               ServiceGuid->Data4[2],
               ServiceGuid->Data4[3],
               ServiceGuid->Data4[4],
               ServiceGuid->Data4[5],
               ServiceGuid->Data4[6],
               ServiceGuid->Data4[7]
               );

    swprintf_s(InterfaceSymlink, L"\\\\.\\WindowsTrustedRT\\%ws", InterfaceGuid);

    TEST_COMMENT(L"Guid=%ws", InterfaceGuid);
    TEST_COMMENT(L"Symlink=%ws", InterfaceSymlink);

    ServiceHandle = CreateFileW(
        InterfaceSymlink,
        FILE_READ_DATA | FILE_WRITE_DATA,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL);

    TEST_ASSERT(ServiceHandle != INVALID_HANDLE_VALUE, L"Service open failed");

End:
    return ServiceHandle;
}

BOOL
CallTrEEService(
    _In_ HANDLE ServiceHandle,
    _In_ ULONG FunctionCode,
    _In_reads_bytes_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_(OutputBufferLength, *BytesWritten) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _Out_ PULONG BytesWritten
    )

/*++

Routine Description:

    This routine is a wrapper for DeviceIoControl to conveniently send TrEE
    secure function requests. Input and output buffers are packed to TrEE
    request structs and number of bytes written is unpacked from response
    struct.

Arguments:

    ServiceHandle - Supplies a handle to the secure service on which the
                    request is to be sent.

    FunctionCode - Supplies the function code of the request.

    InputBuffer - Supplies a pointer to the input buffer that contains the data
                  required to perform the operation

    InputBufferLength - Supplies the size of the input buffer, in bytes.

    OutputBuffer - Supplies a pointer to the output buffer that is to receive
                   the data returned by the operation.

    OutputBufferLength - Supplies the size of the output buffer, in bytes.

    BytesWritten - Supplies a pointer to a variable that receives the size of
                   the data stored in the output buffer, in bytes.

Return Value:

    TRUE if successful.

--*/

{
    TR_SERVICE_REQUEST Request;
    TR_SERVICE_REQUEST_RESPONSE Response;
    BOOL Success;
    ULONG ResponseBytesWritten;

    Request.FunctionCode = FunctionCode;
    Request.InputBuffer = InputBuffer;
    Request.InputBufferSize = InputBufferLength;
    Request.OutputBuffer = OutputBuffer;
    Request.OutputBufferSize = OutputBufferLength;

    Success = DeviceIoControl(ServiceHandle,
                              IOCTL_TR_EXECUTE_FUNCTION,
                              &Request,
                              sizeof(Request),
                              &Response,
                              sizeof(Response),
                              &ResponseBytesWritten,
                              NULL);

    if (ResponseBytesWritten >= sizeof(Response)) {
        *BytesWritten = (ULONG)Response.BytesWritten;
        return Success;

    } else {

        *BytesWritten = 0;
        return FALSE;
    }
}

typedef struct _ASYNC_TREE_CALL {
    OVERLAPPED Overlapped;
    TR_SERVICE_REQUEST Request;
    TR_SERVICE_REQUEST_RESPONSE Response;
} ASYNC_TREE_CALL, *PASYNC_TREE_CALL;

BOOL
CallTrEEServiceEx(
    _In_ HANDLE ServiceHandle,
    _In_ ULONG FunctionCode,
    _In_reads_bytes_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_(OutputBufferLength, *BytesWritten) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _Out_ PULONG BytesWritten,
    _Deref_out_ PASYNC_TREE_CALL* pAsyncContext
    )

/*++

Routine Description:

    This routine is a wrapper for DeviceIoControl similar to CallTrEEService,
    except that it performs asynchronous I/O.

Arguments:

    ServiceHandle - Supplies a handle to the secure service on which the
                    request is to be sent. The handle must have been opened
                    with FILE_FLAG_OVERLAPPED flag.

    FunctionCode - Supplies the function code of the request.

    InputBuffer - Supplies a pointer to the input buffer that contains the data
                  required to perform the operation

    InputBufferLength - Supplies the size of the input buffer, in bytes.

    OutputBuffer - Supplies a pointer to the output buffer that is to receive
                   the data returned by the operation.

    OutputBufferLength - Supplies the size of the output buffer, in bytes.

    BytesWritten - Supplies a pointer to a variable that receives the size of
                   the data stored in the output buffer, in bytes.

    pAsyncContext - Supplies a pointer to a variable that receives the pointer
                    to ASYNC_TREE_CALL which contains necessary input and
                    output buffer for the request and an OVERLAPPED struct to
                    track the progress of the request.

Return Value:

    TRUE if the operation was completed synchronously and successfully.

    FALSE if the operation is undergoing asynchronously or has failed. Check
    the value of GetLastError to distinguish the case.

--*/

{
    PASYNC_TREE_CALL AsyncContext;
    BOOL Success;
    ULONG ResponseBytesWritten;

    AsyncContext = new ASYNC_TREE_CALL;
    memset(AsyncContext, 0, sizeof(ASYNC_TREE_CALL));
    AsyncContext->Request.FunctionCode = FunctionCode;
    AsyncContext->Request.InputBuffer = InputBuffer;
    AsyncContext->Request.InputBufferSize = InputBufferLength;
    AsyncContext->Request.OutputBuffer = OutputBuffer;
    AsyncContext->Request.OutputBufferSize = OutputBufferLength;
    AsyncContext->Overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    *pAsyncContext = AsyncContext;

    Success = DeviceIoControl(ServiceHandle,
                              IOCTL_TR_EXECUTE_FUNCTION,
                              &AsyncContext->Request,
                              sizeof(TR_SERVICE_REQUEST),
                              &AsyncContext->Response,
                              sizeof(TR_SERVICE_REQUEST_RESPONSE),
                              &ResponseBytesWritten,
                              &AsyncContext->Overlapped);

    if (ResponseBytesWritten >= sizeof(TR_SERVICE_REQUEST_RESPONSE)) {
        *BytesWritten = (ULONG)AsyncContext->Response.BytesWritten;
        return Success;

    } else {

        *BytesWritten = 0;
        return FALSE;
    }
}

VOID
CleanupAsyncTrEECallContext(
    _In_ PASYNC_TREE_CALL AsyncContext
    )

/*++

Routine Description:

    This routine cleans up the memory and resources used by ASYNC_TREE_CALL
    struct returned from CallTrEEServiceEx function.

Arguments:

    AsyncContext - Supplies a pointer to ASYNC_TREE_CALL struct returned from
                   CallTrEEServiceEx function.

Return Value:

    None.

--*/

{
    CloseHandle(AsyncContext->Overlapped.hEvent);
    delete AsyncContext;
}

int
__cdecl
wmain(
    int argc,
    const wchar_t** argv
    )
{
    DWORD BytesWritten;
    HANDLE TestServiceHandle;
    HANDLE Test2ServiceHandle;
    HANDLE MasterDeviceHandle;
    BOOL Success;
    ULONG64 TestFlag;

    Success = TRUE;
    TestServiceHandle = INVALID_HANDLE_VALUE;
    Test2ServiceHandle = INVALID_HANDLE_VALUE;
    MasterDeviceHandle = INVALID_HANDLE_VALUE;

    if (argc > 1) {
        swscanf_s(argv[1], L"%I64x", &TestFlag);

    } else {

        TestFlag = ~0UI64;
    }

    //
    // Open the service using TrEE-namespace filename.
    //
    TestServiceHandle = OpenServiceHandleByFilename(&GUID_SAMPLE_TEST_SERVICE);
    TEST_ASSERT(TestServiceHandle != INVALID_HANDLE_VALUE, L"Test service open failed");

    if ((TestFlag & 0x1) != 0) {
        TEST_COMMENT(L"\n::::: TEST_SERVICE_HELLO_WORLD");

        wchar_t* HelloWorld;

        //
        // First call the service with no output buffer to detect how large
        // output buffer should be.
        //
        Success = CallTrEEService(TestServiceHandle,
                                  TEST_SERVICE_HELLO_WORLD,
                                  NULL, 0,
                                  NULL, 0,
                                  &BytesWritten);

        TEST_ASSERT(!Success, L"CallTrEEService (get size) unexpectedly succeeded");
        TEST_ASSERT(BytesWritten > 0, L"CallTrEEService (get size) returned 0 bytes required size");

        //
        // The request fails, but BytesWritten variable will contain the
        // required size of output buffer.
        //
        HelloWorld = (wchar_t*)malloc(BytesWritten);
        Success = CallTrEEService(TestServiceHandle,
                                  TEST_SERVICE_HELLO_WORLD,
                                  NULL, 0,
                                  HelloWorld, BytesWritten,
                                  &BytesWritten);

        TEST_ASSERT(Success, L"CallTrEEService (read data) failed (err=%d)", GetLastError());
        TEST_ASSERT(BytesWritten > 0, L"CallTrEEService (read data) returned 0 bytes written");

        //
        // The service will return classic "Hello, world!" message.
        //
        TEST_COMMENT(L"TEST_SERVICE_HELLO_WORLD returned: %ws", HelloWorld);
    }

    if ((TestFlag & 0x2) != 0) {
        TEST_COMMENT(L"\n::::: TEST_SERVICE_GET_INTERRUPT_TIME");

        ULONG64 InterruptTime;

        Success = CallTrEEService(TestServiceHandle,
                                  TEST_SERVICE_GET_INTERRUPT_TIME,
                                  NULL, 0,
                                  &InterruptTime, sizeof(InterruptTime),
                                  &BytesWritten);

        TEST_ASSERT(Success, L"CallTrEEService (read data) failed (err=%d)", GetLastError());
        TEST_ASSERT(BytesWritten == sizeof(ULONG64),
                    L"CallTrEEService (read data) returned unexpected number of bytes (BytesWritten=%d)",
                    BytesWritten);

        TEST_COMMENT(L"TEST_SERVICE_GET_INTERRUPT_TIME returned: %I64x", InterruptTime);
    }

    if ((TestFlag & 0x4) != 0) {
        TEST_COMMENT(L"\n::::: TEST_SERVICE_KERNEL_ONLY");

        DWORD Error;

        //
        // This call is available only from kernel mode. The service will fail
        // the request if it came from usermode.
        //
        Success = CallTrEEService(TestServiceHandle,
                                  TEST_SERVICE_KERNEL_ONLY,
                                  NULL, 0,
                                  NULL, 0,
                                  &BytesWritten);

        Error = GetLastError();

        TEST_ASSERT(!Success, L"CallTrEEService unexpectedly succeeded");

        TEST_COMMENT(L"TEST_SERVICE_KERNEL_ONLY failed with err=%d", Error);
    }

    {
        DWORD Error;
        ULONG Input;
        ULONG Output;
        PASYNC_TREE_CALL AsyncContext;

        if ((TestFlag & 0x8) != 0) {
            TEST_COMMENT(L"\n::::: TEST_SERVICE_DELAYED_COMPLETION(1500ms, synchronous wait)");
            Input = 1500;
            Output = 0;
            Success = CallTrEEService(TestServiceHandle,
                                      TEST_SERVICE_DELAYED_COMPLETION,
                                      &Input, sizeof(Input),
                                      &Output, sizeof(Output),
                                      &BytesWritten);

            TEST_ASSERT(Success, L"CallTrEEService failed (err=%d)", GetLastError());
            TEST_ASSERT(BytesWritten == sizeof(Output),
                        L"CallTrEEService returned unexpected number of bytes (BytesWritten=%d)",
                        BytesWritten);

            TEST_COMMENT(L"Output buffer contains: %08x", Output);
        }

        if ((TestFlag & 0x10) != 0) {
            TEST_COMMENT(L"\n::::: TEST_SERVICE_DELAYED_COMPLETION(5000ms, asynchronous wait)");
            Input = 5000;
            Output = 0;

            //
            // CallTrEEServiceEx is called to demonstrate asynchronous operation.
            //
            Success = CallTrEEServiceEx(TestServiceHandle,
                                        TEST_SERVICE_DELAYED_COMPLETION,
                                        &Input, sizeof(Input),
                                        &Output, sizeof(Output),
                                        &BytesWritten,
                                        &AsyncContext);

            Error = GetLastError();

            TEST_ASSERT(!Success, L"CallTrEEServiceEx unexpectedly succeeded");
            TEST_ASSERT(GetLastError() == ERROR_IO_PENDING,
                        L"CallTrEEServiceEx returned unexpected error code (err=%d)",
                        Error);

            while (true) {
                TEST_COMMENT(L"Waiting for completion...");
                Error = WaitForSingleObject(AsyncContext->Overlapped.hEvent, 500);

                TEST_ASSERT((Error == STATUS_TIMEOUT) || (Error == STATUS_WAIT_0),
                            L"WaitForSingleObject returned unexpected value %d",
                            Error);

                if (Error == STATUS_WAIT_0) {
                    break;
                }
            }

            TEST_ASSERT(AsyncContext->Overlapped.Internal == STATUS_SUCCESS,
                        L"Request failed (NTSTATUS=%08x)",
                        (NTSTATUS)AsyncContext->Overlapped.Internal);

            //
            // IOCTL's output buffer points to TR_SERVICE_REQUEST_RESPONSE. So this
            // request's number of bytes written is always sizeof(TR_SERVICE_REQUEST_RESPONSE).
            //
            TEST_ASSERT(AsyncContext->Overlapped.InternalHigh == sizeof(TR_SERVICE_REQUEST_RESPONSE),
                        L"Request response size incorrect (BytesWritten=%u)",
                        (ULONG)AsyncContext->Overlapped.InternalHigh);

            //
            // Number of bytes written to secure request's output buffer is
            // returned in TR_SERVICE_REQUEST_RESPONSE::BytesWritten.
            //
            TEST_ASSERT(AsyncContext->Response.BytesWritten == sizeof(ULONG),
                        L"Number of bytes written to output buffer incorrect (BytesWritten=%u)",
                        (ULONG)AsyncContext->Response.BytesWritten);

            TEST_COMMENT(L"Output buffer contains: %08x", Output);

            CleanupAsyncTrEECallContext(AsyncContext);
        }

        if ((TestFlag & 0x20) != 0) {
            TEST_COMMENT(L"\n::::: TEST_SERVICE_DELAYED_COMPLETION(5000ms, asynchronous wait + cancel)");
            Input = 5000;
            Output = 0;
            Success = CallTrEEServiceEx(TestServiceHandle,
                                        TEST_SERVICE_DELAYED_COMPLETION,
                                        &Input, sizeof(Input),
                                        &Output, sizeof(Output),
                                        &BytesWritten,
                                        &AsyncContext);

            Error = GetLastError();

            TEST_ASSERT(!Success, L"CallTrEEServiceEx unexpectedly succeeded");
            TEST_ASSERT(GetLastError() == ERROR_IO_PENDING,
                        L"CallTrEEServiceEx returned unexpected error code (err=%d)",
                        Error);

            Sleep(1000);

            TEST_COMMENT(L"Cancelling...");

            //
            // The request is cancelled before it can be completed.
            //
            Success = CancelIoEx(TestServiceHandle, &AsyncContext->Overlapped);

            TEST_ASSERT(Success, L"CancelIoEx failed (err=%d)", GetLastError());

            //
            // When the cancellation is handled by the miniport driver, the request
            // will have status STATUS_CANCELLED.
            //
            for (int i = 0; i < 10; ++i) {
                TEST_COMMENT(L"Request status = %08x",
                             (NTSTATUS)AsyncContext->Overlapped.Internal);

                if ((NTSTATUS)AsyncContext->Overlapped.Internal == STATUS_CANCELLED) {
                    break;
                }

                Sleep(20);
            }

            TEST_ASSERT((NTSTATUS)AsyncContext->Overlapped.Internal == STATUS_CANCELLED,
                        L"Request not cancelled (NTSTATUS=%08x)",
                        (NTSTATUS)AsyncContext->Overlapped.Internal);

            CleanupAsyncTrEECallContext(AsyncContext);
        }
    }

    {
        DWORD Error;
        ULONG Input;
        ULONG Output;
        OVERLAPPED Overlapped;

        if ((TestFlag & 0x40) != 0) {
            TEST_COMMENT(L"\n::::: IOCTL_TEST_DELAYED_COMPLETION(3000ms + async wait)");

            memset(&Overlapped, 0, sizeof(Overlapped));
            Overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

            //
            // Other I/O doesn't use TR_SERVICE_REQUEST structs. Use
            // DeviceIocontrol and specify input and output buffers directly in
            // parameters.
            //
            Input = 3000;
            Output = 0;
            Success = DeviceIoControl(TestServiceHandle,
                                      IOCTL_TEST_DELAYED_COMPLETION,
                                      &Input, sizeof(Input),
                                      &Output, sizeof(Output),
                                      &BytesWritten,
                                      &Overlapped);

            Error = GetLastError();

            TEST_ASSERT(!Success, L"CallTrEEServiceEx unexpectedly succeeded");
            TEST_ASSERT(GetLastError() == ERROR_IO_PENDING,
                        L"CallTrEEServiceEx returned unexpected error code (err=%d)",
                        Error);

            while (true) {
                TEST_COMMENT(L"Waiting for completion...");
                Error = WaitForSingleObject(Overlapped.hEvent, 500);

                TEST_ASSERT((Error == STATUS_TIMEOUT) || (Error == STATUS_WAIT_0),
                            L"WaitForSingleObject returned unexpected value %d",
                            Error);

                if (Error == STATUS_WAIT_0) {
                    break;
                }
            }

            TEST_ASSERT(Overlapped.Internal == STATUS_SUCCESS,
                        L"Request failed (NTSTATUS=%08x)",
                        (NTSTATUS)Overlapped.Internal);

            //
            // IOCTL's output buffer points to ULONG Output.
            //
            TEST_ASSERT(Overlapped.InternalHigh == sizeof(ULONG),
                        L"Request response size incorrect (BytesWritten=%u)",
                        (ULONG)Overlapped.InternalHigh);

            TEST_COMMENT(L"Output buffer contains: %08x", Output);
        }

        if ((TestFlag & 0x80) != 0) {
            TEST_COMMENT(L"\n::::: IOCTL_TEST_DELAYED_COMPLETION(5000ms, asynchronous wait + cancel)");
            Overlapped.Internal = 0;
            Overlapped.InternalHigh = 0;
            Overlapped.Offset = 0;
            Overlapped.OffsetHigh = 0;
            Overlapped.Pointer = 0;
            Input = 5000;
            Output = 0;
            Success = DeviceIoControl(TestServiceHandle,
                                      IOCTL_TEST_DELAYED_COMPLETION,
                                      &Input, sizeof(Input),
                                      &Output, sizeof(Output),
                                      &BytesWritten,
                                      &Overlapped);

            Error = GetLastError();

            TEST_ASSERT(!Success, L"CallTrEEServiceEx unexpectedly succeeded");
            TEST_ASSERT(GetLastError() == ERROR_IO_PENDING,
                        L"CallTrEEServiceEx returned unexpected error code (err=%d)",
                        Error);

            Sleep(1000);

            TEST_COMMENT(L"Cancelling...");

            //
            // The request is cancelled before it can be completed.
            //
            Success = CancelIoEx(TestServiceHandle, &Overlapped);

            TEST_ASSERT(Success, L"CancelIoEx failed (err=%d)", GetLastError());

            //
            // When the cancellation is handled by the miniport driver, the request
            // will have status STATUS_CANCELLED.
            //
            for (int i = 0; i < 10; ++i) {
                TEST_COMMENT(L"Request status = %08x",
                             (NTSTATUS)Overlapped.Internal);

                if ((NTSTATUS)Overlapped.Internal == STATUS_CANCELLED) {
                    break;
                }

                Sleep(20);
            }

            TEST_ASSERT((NTSTATUS)Overlapped.Internal == STATUS_CANCELLED,
                        L"Request not cancelled (NTSTATUS=%08x)",
                        (NTSTATUS)Overlapped.Internal);

            CloseHandle(Overlapped.hEvent);
        }
    }

    //
    // Open the service using PnP device interface.
    //
    Test2ServiceHandle = OpenServiceHandleByInterface(&GUID_SAMPLE_TEST2_SERVICE);
    TEST_ASSERT(Test2ServiceHandle != INVALID_HANDLE_VALUE, L"Test2 service open failed");

    {
        char Input[64];
        size_t InputLength;
        char Output[128];

        if ((TestFlag & 0x100) != 0)
        {
            //
            // This service talks to another driversin OS. Refer to
            // miniport\TestService.c to see how the miniport driver sends requests
            // to other drivers.
            //
            TEST_COMMENT(L"\n::::: TEST2_SERVICE_ECHO");

            strcpy_s(Input, "Hello from usermode");
            InputLength = strlen(Input);
            memset(Output, 0, sizeof(Output));
            Success = CallTrEEService(Test2ServiceHandle,
                                      TEST2_SERVICE_ECHO,
                                      Input, (ULONG)InputLength,
                                      Output, sizeof(Output),
                                      &BytesWritten);

            TEST_ASSERT(Success, L"CallTrEEService failed (err=%d)", GetLastError());
            TEST_ASSERT(BytesWritten == InputLength,
                        L"CallTrEEService returned unexpected number of bytes (BytesWritten=%d)",
                        BytesWritten);

            TEST_COMMENT(L"Output buffer contains: %hs", Output);
        }

        if ((TestFlag & 0x200) != 0)
        {
            TEST_COMMENT(L"\n::::: TEST2_SERVICE_TWICE_REVERSED");

            strcpy_s(Input, "Hello from usermode");
            InputLength = strlen(Input);
            memset(Output, 0, sizeof(Output));
            Success = CallTrEEService(Test2ServiceHandle,
                                      TEST2_SERVICE_ECHO_TWICE_REVERSED,
                                      Input, (ULONG)InputLength,
                                      Output, sizeof(Output),
                                      &BytesWritten);

            TEST_ASSERT(Success, L"CallTrEEService failed (err=%d)", GetLastError());
            TEST_ASSERT(BytesWritten == InputLength * 2,
                        L"CallTrEEService returned unexpected number of bytes (BytesWritten=%d)",
                        BytesWritten);

            TEST_COMMENT(L"Output buffer contains: %hs", Output);
        }
    }

    //
    // Open the master device using PnP device interface.
    //
    MasterDeviceHandle = CreateFileW(L"\\\\.\\SampleTrEEDriver",
                                     FILE_READ_DATA | FILE_WRITE_DATA,
                                     0,
                                     NULL,
                                     OPEN_EXISTING,
                                     FILE_FLAG_OVERLAPPED,
                                     NULL);

    TEST_ASSERT(MasterDeviceHandle != INVALID_HANDLE_VALUE, L"Master device open failed");

    if ((TestFlag & 0x400) != 0)
    {
        TEST_COMMENT(L"\n::::: IOCTL_SAMPLE_DBGPRINT");

        Success = DeviceIoControl(MasterDeviceHandle,
                                  IOCTL_SAMPLE_DBGPRINT,
                                  TEST_STRING, sizeof(TEST_STRING),
                                  NULL, 0,
                                  &BytesWritten,
                                  NULL);

        TEST_ASSERT(Success, L"CallTrEEService failed (err=%d)", GetLastError());
    }

End:
    if (TestServiceHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(TestServiceHandle);
    }

    if (Test2ServiceHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(Test2ServiceHandle);
    }

    if (MasterDeviceHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(MasterDeviceHandle);
    }

    return 0;
}