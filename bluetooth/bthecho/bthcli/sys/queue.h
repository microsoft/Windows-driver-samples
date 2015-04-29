/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Queue.h

Abstract:

    Queue events for bthecho client

Environment:

    Kernel mode only


--*/

EVT_WDF_IO_QUEUE_IO_READ BthEchoCliEvtQueueIoRead ;

EVT_WDF_IO_QUEUE_IO_WRITE BthEchoCliEvtQueueIoWrite;

EVT_WDF_IO_QUEUE_IO_STOP BthEchoCliEvtQueueIoStop;

EVT_WDF_REQUEST_COMPLETION_ROUTINE BthEchoCliReadWriteCompletion;
