/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    SampleService.cpp

Abstract:

    Provides a sample service class that derives from the service base class -
    CServiceBase. The sample service logs the service start and stop
    information to the Application event log, and shows how to run the main
    function of the service in a thread pool worker thread.

Environment:

    User mode

--*/

#pragma region Includes
#include "SampleService.h"
#include "ThreadPool.h"
#pragma endregion

/*++

Routine Description:

    The constructor of CSampleService. It initializes a new instance
    of the CSampleService class. The optional parameters (CanStop,
    CanShutdown and CanPauseContinue) allow you to specify whether the
    service can be stopped, paused and continued, or be notified when system
    shutdown occurs.  Inherits properties from the class CServiceBase.

Arguments:

    ServiceName      - The name of the service

    CanStop          - The service can be stopped

    CanShutdown      - The service is notified when system shutdown occurs

    CanPauseContinue - The service can be paused and continued

Return Value:

    VOID

--*/
CSampleService::CSampleService(
    PWSTR ServiceName, 
    BOOL  CanStop, 
    BOOL  CanShutdown, 
    BOOL  CanPauseContinue
    )
: CServiceBase(ServiceName, CanStop, CanShutdown, CanPauseContinue)
{
    m_fStopping = FALSE;

    //
    // Create a manual-reset event that is not signaled at first to indicate 
    // the stopped signal of the service.
    //
    m_hStoppedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (m_hStoppedEvent == NULL)
    {
        throw GetLastError();
    }
}


/*++

Routine Description:

    The virtual destructor of CSampleService.

Arguments:

    VOID

Return Value:

    VOID

--*/
CSampleService::~CSampleService()
{
    if (m_hStoppedEvent)
    {
        CloseHandle(m_hStoppedEvent);
        m_hStoppedEvent = NULL;
    }
}


/*++

Routine Description:

    This function is executed when a Start command is sent to the
    service by the SCM or when the operating system starts (for a service
    that starts automatically). It specifies actions to take when the
    service starts. In this code sample, OnStart logs a service-start
    message to the Application log, and queues the main service function for
    execution in a thread pool worker thread.

    NOTE: A service application is designed to be long running. Therefore,
          it usually polls or monitors something in the system. The monitoring is
          set up in the OnStart method. However, OnStart does not actually do the
          monitoring. The OnStart method must return to the operating system after
          the service's operation has begun. It must not loop forever or block. To
          set up a simple monitoring mechanism, one general solution is to create
          a timer in OnStart. The timer would then raise events in your code
          periodically, at which time your service could do its monitoring. The
          other solution is to spawn a new thread to perform the main service
          functions, which is demonstrated in this code sample.

Arguments:

    Argc - The number of command line arguments

    Argv - The array of command line arguments

Return Value:

    VOID

--*/
VOID
CSampleService::OnStart(
    DWORD  Argc,
    PWSTR *Argv
    )
{
	__debugbreak();

    //
    // Log a service start message to the Application log.
    //
    WriteToEventLog(L"SampleService in OnStart", 
                    EVENTLOG_INFORMATION_TYPE);

    //
    // Set up any variables the service needs.
    //
    SetVariables();

    //
    // Set up the context, and register for notifications.
    //
    InitializeContext(&m_Context);

    //
    // Queue the main service function for execution in a worker thread.
    //
    CThreadPool::QueueUserWorkItem(&CSampleService::ServiceWorkerThread, this);
}


/*++

Routine Description:

    This method performs the main function of the service. It runs
    on a thread pool worker thread.

Arguments:

    VOID

Return Value:

    VOID

--*/
VOID
CSampleService::ServiceWorkerThread()
{
    //
    // Periodically check if the service is stopping.
    //
    while (!m_fStopping)
    {
        //
        // Perform main service function here...
        //

        ControlDevice(m_Context);

        ::Sleep(2000);  // Simulate some lengthy operations.
    }

    //
    // Signal the stopped event.
    //
    SetEvent(m_hStoppedEvent);
}


/*++

Routine Description:

    This function is executed when a Stop command is sent to the service by SCM.
    It specifies actions to take when a service stops running.  In this code
    sample, OnStop logs a service-stop message to the Application log, and
    waits for the finish of the main service function.

    Be sure to periodically call ReportServiceStatus() with
    SERVICE_STOP_PENDING if the procedure is going to take a long time.

Arguments:

    VOID

Return Value:

    VOID

--*/
VOID
CSampleService::OnStop()
{
    //
    // Log a service stop message to the Application log.
    //
    WriteToEventLog(L"SampleService in OnStop", 
                    EVENTLOG_INFORMATION_TYPE);

    //
    // Indicate that the service is stopping and wait for the finish of the 
    // main service function (ServiceWorkerThread).
    //
    m_fStopping = TRUE;

    if (WaitForSingleObject(m_hStoppedEvent, INFINITE) != WAIT_OBJECT_0)
    {
        throw GetLastError();
    }

    //
    // Clean up the context after the worker thread has finished.
    //
    CloseContext(m_Context);
}