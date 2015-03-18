/*++

Copyright (c) 2003  Microsoft Corporation

Module Name:

    plx.hpp

Abstract:

    This module defines the PLX class and the
    values neccessary for Window operation and IOCTL control.

Environment:

    User Mode Win2k or Later

--*/

#pragma once

#include <windows.h>
#include <setupapi.h>

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include "public.h"

//
// scanf_s is not available in the DDK build environment.
// So redefining it to use scanf
//
#if defined(DDKBUILD)

#define scanf_s scanf

#endif


#define DEFAULT_READ_BUFFER_SIZE 1024
#define DEFAULT_WRITE_BUFFER_SIZE 1024

#define DEFAULT_THREAD_COUNT 2

typedef struct _THREAD_CONTEXT
{
    HANDLE  hDevice;
    BOOL    quite;
    ULONG   BufferSize;
    PUCHAR  Buffer;

} THREAD_CONTEXT, *PTHREAD_CONTEXT;

typedef enum {

    MENU_TEST       = 0,
    READ_TEST       = 1,
    WRITE_TEST      = 2,
    READ_WRITE_TEST = 3,
    THREAD_TEST     = 4,
    DEVICE_PATH     = 5,
    SET_SIZE        = 6,
    COMPARE_BUFFERS = 7,
    DISPLAY_BUFFERS = 8,
    THREAD_TIME     = 9,
    COMMAND_LINE    = 10,

} COMMAND;

class PLX
{

public:
    PLX();
    ~PLX();

    BOOL
    Initialize();

    void
    Menu();

    BOOL
    GetDevicePath();

    BOOL
    ReadTest();

    BOOL
    WriteTest();

    BOOL
    ReadWriteTest();

    BOOL
    CompareReadWriteBuffers();

    BOOL
    SetReadBufferSize(ULONG size);

    BOOL
    SetWriteBufferSize(ULONG size);

    BOOL
    SetBufferSizes(ULONG size);

    void
    DisplayReadWriteBuffers();

    BOOL
    ThreadedReadWriteTest();

    void
    SetThreadLifeTime(ULONG time);

    BOOL  Quite;
    ULONG Status;

private:

    BOOL
    GetDeviceHandle();

    HDEVINFO hDevInfo;
    PSP_DEVICE_INTERFACE_DETAIL_DATA pDeviceInterfaceDetail;
    HANDLE hDevice;

    ULONG ReadBufferSize;
    ULONG WriteBufferSize;
    _Field_size_bytes_(ReadBufferSize) PUCHAR ReadBuffer;
    _Field_size_bytes_(WriteBufferSize) PUCHAR WriteBuffer;

    _Field_size_(ThreadCount) HANDLE *Threads;
    _Field_size_(ThreadCount) PTHREAD_CONTEXT Contexts;
    int ProcessorCount;
    int ThreadCount;

    CRITICAL_SECTION CriticalSection;
    BOOL CSInitialized;

    ULONG ThreadTimer;

    BOOL console;
};

