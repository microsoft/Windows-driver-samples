/*++

Copyright (c) Microsoft Corporation

Module Name:

    plx.cpp

Abstract:

    This module implements the PLX class which tests the DMA of PLX devices.

    Example usage:
        plx.exe /wr /wb=100         # write-then-read once with a buffer of 100 bytes
        plx.exe /thread             # repeat write-then-read for default 5000 millisecs.
        plx.exe /thread /time=1000  # repeat write-then-read for 1000 millisecs.

    NOTE: The /quite option will suppress most non-error messages.
    NOTE: The options and parameters are case sensitive.

Environment:

    User Mode Win2k or Later

--*/

#define INITGUID

#include "plx.hpp"

//
// Define the spin count to be used for critical sections. The value
// specified below is arbitrary. Change it based on your requirements.
//
#define SPIN_COUNT_FOR_CS       0x4000

int g_TimeUp =0;

DWORD WINAPI
ReadThreadProc(
    LPVOID lpParameter
    )
{
    ULONG bytes;
    PTHREAD_CONTEXT Context = (PTHREAD_CONTEXT)lpParameter;

    while(!g_TimeUp) {

        if(ReadFile(Context->hDevice,
                    Context->Buffer,
                    Context->BufferSize,
                    &bytes,
                    NULL)) {

            if (!Context->quite) {
                printf("Read sucessful.\n");
            }

        } else {

            printf("Read failed.\n");
            ExitProcess(1);
        }
    }
    ExitThread(0);
}

DWORD WINAPI
WriteThreadProc(
    LPVOID lpParameter
    )
{
    ULONG bytes;
    PTHREAD_CONTEXT Context = (PTHREAD_CONTEXT)lpParameter;

    while(!g_TimeUp) {

        if(WriteFile(Context->hDevice,
                     Context->Buffer,
                     Context->BufferSize,
                     &bytes,
                     NULL)) {

            if (!Context->quite) {
                printf("Write sucessful.\n");
            }

        } else {

            printf("Write failed.\n");
            ExitProcess(1);
        }
    }
    ExitThread(0);
}

int __cdecl
main(
    _In_              int argc,
    _In_reads_(argc) char* argv[]
    )
{
    PLX Plx;
    BOOL status = TRUE;
    ULONG test = MENU_TEST;

    if (!Plx.Initialize()) {
        printf("Failied to Initialize Test class.\n");
        printf("exit(%u)\n", Plx.Status);
        exit(Plx.Status);
    }

    if(argc > 1) {

        for(int i=1; (i < argc) && status; i++) {

            char delims[]   = "-/=";
            char delims2[]  = "=";
            char *command;
            char *data;
            char *state = NULL;

            data = NULL;

            #pragma prefast(suppress:6385, "i < argc-1 before it is incremented below");
            command =  strtok_s(argv[i], delims, &state);
            if(command == NULL) {
                status = FALSE;
                break;
            }

            if(strcmp(command, "rb") == 0) {

                data = strtok_s(NULL, delims2, &state);

                if (!data && i < argc-1) {
                    data = argv[++i];
                }

                ULONG size = atol(data);
                if (size > 0) {
                    Plx.SetReadBufferSize(size);
                } else {
                    status = FALSE;
                }

            } else if(strcmp(command, "wb") == 0) {

                ULONG size = 0;
                data = strtok_s(NULL, delims2, &state);

                if (!data && i < argc-1) {
                    data = argv[++i];
                }
                if (data) {
                    size = atol(data);
                }

                if (size > 0) {
                    Plx.SetWriteBufferSize(size);
                } else {
                    status = FALSE;
                }

            } else if (strcmp(command, "bs") == 0) {
                data = strtok_s(NULL, delims2, &state);

                if (!data && i < argc-1) {
                    data = argv[++i];
                }

                ULONG size = atol(data);
                if(size > 0) {
                    Plx.SetWriteBufferSize(size);
                } else {
                    status = FALSE;
                }

            } else if(strcmp(command, "wt") == 0) {

                test = WRITE_TEST;

            } else if(strcmp(command, "rt") == 0) {

                test = READ_TEST;

            } else if(strcmp(command, "quite") == 0) {

                Plx.Quite = TRUE;

            } else if(strcmp(command, "thread") == 0) {

                test = THREAD_TEST;

            } else if (strcmp(command, "time") == 0) {

                data = strtok_s(NULL, delims, &state);

                if(!data && i < argc-1) {
                    #pragma prefast(suppress:6385, "i < argc-1 before it is incremented");
                    data = argv[++i];
                }
                ULONG size = (NULL != data) ? atol(data) : 0;
                Plx.SetThreadLifeTime(size);

            } else {
                status = FALSE;
            }

            if (!Plx.Quite) {
                if (data) {
                    printf("Arg %d: Command: %s  Parameter: %s\n", i, command, data);
                } else {
                    printf("Arg %d: Command: %s\n", i, command);
                }
            }
        }
    }

    if (status) {

        switch (test) {
            case READ_TEST:
                Plx.ReadTest();
                break;

            case WRITE_TEST:
                Plx.WriteTest();
                break;

            case READ_WRITE_TEST:
                Plx.ReadWriteTest();
                break;

            case THREAD_TEST:
                Plx.ThreadedReadWriteTest();
                break;

            case MENU_TEST:
            default:
                Plx.Menu();
        }

    } else {

        printf("Invalid command line parameter.\n");
        Plx.Status = 1;
    }

    printf("exit(%u)\n", Plx.Status);

    exit( Plx.Status );
}

PLX::PLX()
{
    ReadBuffer = WriteBuffer = NULL;
    hDevInfo = pDeviceInterfaceDetail = NULL;
    hDevice = INVALID_HANDLE_VALUE;
    console = TRUE;
    Contexts = NULL;
    Threads = NULL;
    ProcessorCount = 0;
    CSInitialized = FALSE;
    ThreadTimer = 5000;  // 5000 milliseconds (5 seconds)

    Quite = FALSE;
    Status = 0;
}

PLX::~PLX()
{

    if (CSInitialized) {
        DeleteCriticalSection(&CriticalSection);
    }
    if (hDevInfo) {
        SetupDiDestroyDeviceInfoList(hDevInfo);
    }

    if (pDeviceInterfaceDetail) {
        free(pDeviceInterfaceDetail);
    }

    if (Contexts) {
        _Analysis_assume_(ProcessorCount <= ThreadCount);
        for(int i = 0; i < ProcessorCount; i++) {
            if (Contexts[i].Buffer) {
                delete Contexts[i].Buffer;
            }
        }

        delete Contexts;
        Contexts = NULL;
    }

    if (hDevice != INVALID_HANDLE_VALUE) {
        CloseHandle(hDevice);
        hDevice = INVALID_HANDLE_VALUE;
    }
}

BOOL
PLX::Initialize()
{
    BOOL retValue = TRUE;

    retValue = SetBufferSizes(DEFAULT_READ_BUFFER_SIZE);
    if (!retValue) {
        return retValue;
    }

    retValue = GetDevicePath();
    if (!retValue) {
        return retValue;
    }

    if (!CSInitialized) {
        retValue = InitializeCriticalSectionAndSpinCount(&CriticalSection, SPIN_COUNT_FOR_CS);
        if (!retValue) {
            printf("InitializeCritialSection failed.\n");
            Status = GetLastError();
            return retValue;
        }
        CSInitialized = TRUE;
    }

    return retValue;
}

void
PLX::Menu()
{
    int menu = -1;
    const char* menuItems[MENU_MAX];
    BOOL status, toggle;

    menuItems[MENU_TEST] = "Quit";
    menuItems[READ_TEST] = "Read from device";
    menuItems[WRITE_TEST] = "Write to device";
    menuItems[READ_WRITE_TEST] = "Read/Write from device";
    menuItems[THREAD_TEST] = "Read/Write Thread Test";
    menuItems[DEVICE_PATH] = "Select new device";
    menuItems[SET_SIZE] = "Change Buffer Size";
    menuItems[COMPARE_BUFFERS] = "Compare Read/Write Buffers";
    menuItems[DISPLAY_BUFFERS] = "Display Read/Write Buffers";
    menuItems[THREAD_TIME] = "Change Thread Lifetime";
    menuItems[SINGLE_TRANSFER_TOGGLE] = "Toggle single transfer requirement";
    menuItems[COMMAND_LINE] = "Command Line Options";

    while(menu != MENU_TEST && pDeviceInterfaceDetail) {
        for (int i = MENU_TEST + 1; i < MENU_MAX; i++) {
            printf("\n%2x - %s", i, menuItems[i]);
        }
        printf("\n%2x - %s\n", MENU_TEST, menuItems[MENU_TEST]);

        if (scanf_s("%x", &menu) == 0) {
            break;
        }

        switch(menu) {
            case READ_TEST:
                ReadTest();
                break;

            case WRITE_TEST:
                WriteTest();
                break;

            case READ_WRITE_TEST:
                ReadWriteTest();
                break;

            case THREAD_TEST:
                ThreadedReadWriteTest();
                break;

            case DEVICE_PATH:
                GetDevicePath();
                break;

            case SET_SIZE:
                ULONG size;
                printf("\nEnter new buffer size: ");
                if (scanf_s("%u", &size) != 0) {
                    SetBufferSizes(size);
                }
                break;

            case COMPARE_BUFFERS:
                CompareReadWriteBuffers();
                break;

            case DISPLAY_BUFFERS:
                DisplayReadWriteBuffers();
                break;

            case THREAD_TIME:
                printf("\nEnter new Thread Lifetime (ms): ");
                if (scanf_s("%u", &ThreadTimer) == 0) {
                    break;
                }
                break;

            case SINGLE_TRANSFER_TOGGLE:
                status = SendSingleTransferIoctl(&toggle);
                if (status) {
                    printf("Single transfer requirement is %s\n",
                        toggle ? "on" : "off");
                }
                else {
                    printf("Could not toggle the single transfer "
                           "requirement\n");
                }
                break;

            case COMMAND_LINE:
                printf("Command Line Options\n"
                       " Set Read Buffer Size:           '/rb=xx'\n"
                       " Set Write Buffer Size:          '/wb=xx'\n"
                       " Set Both Buffer Sizes:          '/bs=xx'\n"
                       " Perform Write Test:             '/wt'\n"
                       " Perform Read Test:              '/rt'\n"
                       " Perform Read/Write Test:        '/wr'\n"
                       " Perform Read/Write Thread Test: '/thread'\n");
                break;

            default:
                break;
        }
    }
}

BOOL
PLX::ThreadedReadWriteTest()
{
    BOOL status = TRUE;
    DWORD_PTR pAffinity, sAffinity;
    int i;

    HANDLE hThread;
    HANDLE hProcess = GetCurrentProcess();

    GetProcessAffinityMask(hProcess, &pAffinity, &sAffinity);
    ProcessorCount = 0;

    while(pAffinity) {
        ProcessorCount++;
        pAffinity = pAffinity >> 1;
    }

    if (ProcessorCount == 1) {
        ThreadCount = DEFAULT_THREAD_COUNT;
    } else {
        ThreadCount = ProcessorCount;
    }

    Contexts = new THREAD_CONTEXT[ThreadCount];
    Threads = new HANDLE[ThreadCount];

    if (Contexts == NULL || Threads == NULL) {
        return FALSE;
    }

    if (hDevice == INVALID_HANDLE_VALUE) {
        status = GetDeviceHandle();
    }

    if (!Quite) {
        printf("Creating %d threads...\n", ThreadCount);
    }

    pAffinity = 1;

    for(i = 0; i < ThreadCount; i++) {

        if ((i % 2) == 0) {

            //
            //  Create Read Thread
            //
            Contexts[i].hDevice = hDevice;
            Contexts[i].BufferSize = ReadBufferSize;
            Contexts[i].Buffer = new UCHAR[ReadBufferSize];
            Contexts[i].quite  = Quite;

            hThread = CreateThread(NULL,
                                   0,
                                   ReadThreadProc,
                                   &Contexts[i],
                                   0,
                                   NULL);

            if (NULL == hThread) {
                printf( "Failed to create thread %d\n", i );
                this->Status = 1;
                break;

            } else {
                Threads[i] = hThread;
            }

        } else {

            //
            //  Create Write Thread
            //
            Contexts[i].hDevice = hDevice;
            Contexts[i].BufferSize = WriteBufferSize;
            Contexts[i].Buffer = new UCHAR[WriteBufferSize];
            Contexts[i].quite  = Quite;

            hThread = CreateThread(NULL,
                                   0,
                                   WriteThreadProc,
                                   &Contexts[i],
                                   0,
                                   NULL);

            if (NULL == hThread) {
                printf( "Failed to create thread %d\n", i );
                this->Status = 1;
                break;

            } else {
                Threads[i] = hThread;
            }
        }

        //
        //  Set Affinity
        //
        SetThreadAffinityMask(Threads[i], pAffinity);

        pAffinity = pAffinity << 1;
    }

    if (i != ThreadCount) {

        //
        // some create thread failed, bail out
        //
        printf( "Some CreateThread was failed, stop\n" );
        g_TimeUp = 1;

    } else {

        //
        // wait till either all quit or time is due
        //
        DWORD error;

        error = WaitForMultipleObjects(i, Threads, TRUE, ThreadTimer);

        if (error == WAIT_TIMEOUT) {

            //
            // Stop the threads if time is up
            //
            g_TimeUp = 2;
            error = WaitForMultipleObjects(i, Threads, TRUE, 100000) ;

            if (error) {
                printf("WaitForMultipleObjects[%d] error %u\n", i, error);
            }
        } else {
            if (error) {
                printf("WaitForMultipleObjects[%d] error %u\n", i, error);
            }
        }

        if (!error) {
            //
            // Reset the TimeUp flag
            //
            g_TimeUp = 0;
        }
    }

    if (Contexts) {
        for (i = 0; i < ThreadCount; i++) {
            if (Contexts[i].Buffer) {
                delete Contexts[i].Buffer;
            }
        }

        delete Contexts;
        Contexts = NULL;
    }

    if (hDevice != INVALID_HANDLE_VALUE) {
        CloseHandle(hDevice);
        hDevice = INVALID_HANDLE_VALUE;
    }

    return status;
}

BOOL
PLX::ReadTest()
{
    BOOL status = TRUE;
    ULONG bytes = 0;

    if (!ReadBuffer) {
        status = FALSE;
    }

    if ((status == TRUE) && (hDevice == INVALID_HANDLE_VALUE)) {
        status = GetDeviceHandle();
    }

    if (status) {
        if (ReadFile(hDevice,
                    ReadBuffer,
                    ReadBufferSize,
                    &bytes,
                    NULL)){

            EnterCriticalSection(&CriticalSection);
            if (!Quite) {
                printf("Read sucessful.\n");
            }
            LeaveCriticalSection(&CriticalSection);

        } else {

            EnterCriticalSection(&CriticalSection);
            printf("Read failed.\n");
            this->Status = 1;
            LeaveCriticalSection(&CriticalSection);
        }
    }

    if (hDevice != INVALID_HANDLE_VALUE) {
        CloseHandle(hDevice);
        hDevice = INVALID_HANDLE_VALUE;
    }

    return status;
}

BOOL
PLX::WriteTest()
{
    ULONG bytes = 0;
    BOOL status = TRUE;

    if (!WriteBuffer) {
        status = FALSE;
    }

    if ((status == TRUE) && (hDevice == INVALID_HANDLE_VALUE)) {
        status = GetDeviceHandle();
    }

    if (status) {
        FillMemory(WriteBuffer, WriteBufferSize, 0xAB);

        if (WriteFile(hDevice,
                      WriteBuffer,
                      WriteBufferSize,
                      &bytes,
                      NULL)) {

            EnterCriticalSection(&CriticalSection);
            if (!Quite) {
                printf("Write sucessful.\n");
            }
            LeaveCriticalSection(&CriticalSection);

        } else {

            EnterCriticalSection(&CriticalSection);
            printf("Write failed.\n");
            this->Status = 1;
            LeaveCriticalSection(&CriticalSection);
        }
    }

    if (hDevice != INVALID_HANDLE_VALUE) {
        CloseHandle(hDevice);
        hDevice = INVALID_HANDLE_VALUE;
    }

    return status;
}

BOOL
PLX::ReadWriteTest()
{
    return (WriteTest() && ReadTest() && CompareReadWriteBuffers());
}

BOOL
PLX::CompareReadWriteBuffers()
{
    BOOL status = TRUE;
    ULONG size;
    PUCHAR WTraverse;
    PUCHAR RTraverse;

    if (ReadBufferSize <= WriteBufferSize) {
        size = ReadBufferSize;
    } else {
        size = WriteBufferSize;
    }

    WTraverse = WriteBuffer;
    RTraverse = ReadBuffer;

    for(ULONG i = 0; i < size; i++) {
        if (*WTraverse++ != *RTraverse++) {
            status = FALSE;
        }
    }

    if (status) {
        if (!Quite) {
            printf("Buffers are identical\n");
        }
    } else {
        printf("Buffers not identical\n");
        this->Status = 1;
    }

    return status;
}

void
PLX::DisplayReadWriteBuffers()
{
    if (!Quite) {

        PUCHAR WTraverse;
        PUCHAR RTraverse;

        WTraverse = WriteBuffer;
        RTraverse = ReadBuffer;

        printf("Write: ");
        for(ULONG i = 0; i < WriteBufferSize; i++) {
            printf("%X ", *WTraverse++);
        }

        printf("\n\n\nRead: ");
        for(ULONG i = 0; i < ReadBufferSize; i++) {
            printf("%X ", *RTraverse++);
        }
        printf("\n");
    }
}

BOOL
PLX::SetReadBufferSize(ULONG size)
{
    BOOL status = TRUE;

    if (ReadBuffer) {
        free(ReadBuffer);
    }

    ReadBufferSize = size;

    ReadBuffer = (PUCHAR)malloc(ReadBufferSize);

    if (!ReadBuffer) {
        status = FALSE;
    }

    return status;
}

BOOL
PLX::SetWriteBufferSize(ULONG size)
{
    BOOL status = TRUE;

    if (WriteBuffer) {
        free(WriteBuffer);
    }

    WriteBufferSize = size;

    WriteBuffer = (PUCHAR)malloc(WriteBufferSize);

    if (!WriteBuffer) {
        status = FALSE;
    }

    return status;
}

BOOL
PLX::SetBufferSizes(ULONG size)
{
    BOOL status;

    status = SetReadBufferSize(size);
    if (status) {
        status = SetWriteBufferSize(size);
    }

    return status;
}

BOOL
PLX::SendSingleTransferIoctl(BOOL* Toggle)
{
    BOOL status = TRUE;
    UCHAR outBuffer = 0;
    DWORD bytesReceived = 0;

    if (hDevice == INVALID_HANDLE_VALUE) {
        status = GetDeviceHandle();
        if (status == FALSE) {
            goto Done;
        }
    }

    status = DeviceIoControl(hDevice,
                             IOCTL_PLX9X5X_TOGGLE_SINGLE_TRANSFER,
                             NULL,
                             0,
                             &outBuffer,
                             sizeof(outBuffer),
                             &bytesReceived,
                             NULL);
    if (status == FALSE || bytesReceived != sizeof(outBuffer)) {
        printf("DeviceIoControl failed 0x%x\n", GetLastError());
        goto Done;
    }

    *Toggle = outBuffer;

Done:
    if (hDevice != INVALID_HANDLE_VALUE) {
        CloseHandle(hDevice);
        hDevice = INVALID_HANDLE_VALUE;
    }

    return status;
}

BOOL
PLX::GetDevicePath()
{
    SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
    SP_DEVINFO_DATA DeviceInfoData;

    ULONG size;
    int count, i, index;
    BOOL status = TRUE;
    TCHAR *DeviceName = NULL;
    TCHAR *DeviceLocation = NULL;

    //
    //  Retreive the device information for all PLX devices.
    //
    hDevInfo = SetupDiGetClassDevs(&GUID_PLX_INTERFACE,
                                   NULL,
                                   NULL,
                                   DIGCF_DEVICEINTERFACE |
                                   DIGCF_PRESENT);

    //
    //  Initialize the SP_DEVICE_INTERFACE_DATA Structure.
    //
    DeviceInterfaceData.cbSize  = sizeof(SP_DEVICE_INTERFACE_DATA);

    //
    //  Determine how many devices are present.
    //
    count = 0;
    while(SetupDiEnumDeviceInterfaces(hDevInfo,
                                      NULL,
                                      &GUID_PLX_INTERFACE,
                                      count++,  //Cycle through the available devices.
                                      &DeviceInterfaceData)
          );

    //
    // Since the last call fails when all devices have been enumerated,
    // decrement the count to get the true device count.
    //
    count--;

    //
    //  If the count is zero then there are no devices present.
    //
    if (count == 0) {
        printf("No PLX devices are present and enabled in the system.\n");
        this->Status = 1;
        return FALSE;
    }

    //
    //  Initialize the appropriate data structures in preparation for
    //  the SetupDi calls.
    //
    DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    //
    //  Loop through the device list to allow user to choose
    //  a device.  If there is only one device, select it
    //  by default.
    //
    i = 0;
    while (SetupDiEnumDeviceInterfaces(hDevInfo,
                                       NULL,
                                       (LPGUID)&GUID_PLX_INTERFACE,
                                       i,
                                       &DeviceInterfaceData)) {

        //
        // Determine the size required for the DeviceInterfaceData
        //
        SetupDiGetDeviceInterfaceDetail(hDevInfo,
                                        &DeviceInterfaceData,
                                        NULL,
                                        0,
                                        &size,
                                        NULL);

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            printf("SetupDiGetDeviceInterfaceDetail failed, Error: %u", GetLastError());
            this->Status = 1;
            return FALSE;
        }

        pDeviceInterfaceDetail = (PSP_DEVICE_INTERFACE_DETAIL_DATA) malloc(size);

        if (!pDeviceInterfaceDetail) {
            printf("Insufficient memory.\n");
            this->Status = 1;
            return FALSE;
        }

        //
        // Initialize structure and retrieve data.
        //
        pDeviceInterfaceDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
        status = SetupDiGetDeviceInterfaceDetail(hDevInfo,
                                                 &DeviceInterfaceData,
                                                 pDeviceInterfaceDetail,
                                                 size,
                                                 NULL,
                                                 &DeviceInfoData);

        free(pDeviceInterfaceDetail);

        if (!status) {
            printf("SetupDiGetDeviceInterfaceDetail failed, Error: %u", GetLastError());
            this->Status = 1;
            return status;
        }

        //
        //  Get the Device Name
        //  Calls to SetupDiGetDeviceRegistryProperty require two consecutive
        //  calls, first to get required buffer size and second to get
        //  the data.
        //
        SetupDiGetDeviceRegistryProperty(hDevInfo,
                                        &DeviceInfoData,
                                        SPDRP_DEVICEDESC,
                                        NULL,
                                        (PBYTE)DeviceName,
                                        0,
                                        &size);

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            printf("SetupDiGetDeviceRegistryProperty failed, Error: %u", GetLastError());
            this->Status = 1;
            return FALSE;
        }

        DeviceName = (TCHAR*) malloc(size);
        if (!DeviceName) {
            printf("Insufficient memory.\n");
            this->Status = 1;
            return FALSE;
        }

        status = SetupDiGetDeviceRegistryProperty(hDevInfo,
                                                  &DeviceInfoData,
                                                  SPDRP_DEVICEDESC,
                                                  NULL,
                                                  (PBYTE)DeviceName,
                                                  size,
                                                  NULL);
        if (!status) {
            printf("SetupDiGetDeviceRegistryProperty failed, Error: %u",
                   GetLastError());
            free(DeviceName);
            this->Status = 1;
            return status;
        }

        //
        //  Now retrieve the Device Location.
        //
        SetupDiGetDeviceRegistryProperty(hDevInfo,
                                         &DeviceInfoData,
                                         SPDRP_LOCATION_INFORMATION,
                                         NULL,
                                         (PBYTE)DeviceLocation,
                                         0,
                                         &size);

        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            DeviceLocation = (TCHAR*) malloc(size);

            if (DeviceLocation != NULL) {

                status = SetupDiGetDeviceRegistryProperty(hDevInfo,
                                                          &DeviceInfoData,
                                                          SPDRP_LOCATION_INFORMATION,
                                                          NULL,
                                                          (PBYTE)DeviceLocation,
                                                          size,
                                                          NULL);
                if (!status) {
                    free(DeviceLocation);
                    DeviceLocation = NULL;
                }
            }

        } else {
            DeviceLocation = NULL;
        }

        //
        // If there is more than one device print description.
        //
        if (count > 1 && console) {
            printf("%d- ", i);
        }

        printf("%s\n", DeviceName);

        if (DeviceLocation) {
            printf("        %s\n", DeviceLocation);
        }

        free(DeviceName);
        DeviceName = NULL;

        if (DeviceLocation) {
            free(DeviceLocation);
            DeviceLocation = NULL;
        }

        i++; // Cycle through the available devices.
    }

    //
    //  Select device.
    //
    index = 0;
    if (count > 1) {
        printf("\nSelect Device: ");

        if (scanf_s("%d", &index) == 0) {
            return ERROR_INVALID_DATA;
        }
    }

    //
    //  Get information for specific device.
    //
    status = SetupDiEnumDeviceInterfaces(hDevInfo,
                                    NULL,
                                    (LPGUID)&GUID_PLX_INTERFACE,
                                    index,
                                    &DeviceInterfaceData);

    if (!status) {
        printf("SetupDiEnumDeviceInterfaces failed, Error: %u", GetLastError());
        return status;
    }

    //
    // Determine the size required for the DeviceInterfaceData
    //
    SetupDiGetDeviceInterfaceDetail(hDevInfo,
                                    &DeviceInterfaceData,
                                    NULL,
                                    0,
                                    &size,
                                    NULL);

    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        printf("SetupDiGetDeviceInterfaceDetail failed, Error: %u", GetLastError());
        this->Status = 1;
        return FALSE;
    }

    pDeviceInterfaceDetail = (PSP_DEVICE_INTERFACE_DETAIL_DATA) malloc(size);

    if (!pDeviceInterfaceDetail) {
        printf("Insufficient memory.\n");
        this->Status = 1;
        return FALSE;
    }

    //
    // Initialize structure and retrieve data.
    //
    pDeviceInterfaceDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

    status = SetupDiGetDeviceInterfaceDetail(hDevInfo,
                                             &DeviceInterfaceData,
                                             pDeviceInterfaceDetail,
                                             size,
                                             NULL,
                                             &DeviceInfoData);
    if (!status) {
        printf("SetupDiGetDeviceInterfaceDetail failed, Error: %u", GetLastError());
        this->Status = 1;
        return status;
    }

    return status;
}

BOOL
PLX::GetDeviceHandle()
{
    BOOL status = TRUE;

    if (pDeviceInterfaceDetail == NULL) {
        status = GetDevicePath();
    }
    if (pDeviceInterfaceDetail == NULL) {
        status = FALSE;
    }

    if (status) {

        //
        //  Get handle to device.
        //
        hDevice = CreateFile(pDeviceInterfaceDetail->DevicePath,
                             GENERIC_READ|GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,
                             OPEN_EXISTING,
                             0,
                             NULL);

        if (hDevice == INVALID_HANDLE_VALUE) {
            status = FALSE;
            printf("CreateFile failed.  Error:%u", GetLastError());
            this->Status = 1;
        }
    }

    return status;
}

void
PLX::SetThreadLifeTime(ULONG time)
{
    ThreadTimer = time;
}


