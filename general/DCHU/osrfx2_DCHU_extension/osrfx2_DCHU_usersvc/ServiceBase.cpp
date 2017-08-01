/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    ServiceBase.cpp

Abstract:

    Provides a base class for a service that will exist as part of a service
    application. CServiceBase must be derived from when creating a new service
    class.

Environment:

    User mode

--*/

#pragma region Includes
#include "ServiceBase.h"
#include "Main.h"
#include "Utils.h"
#include <assert.h>
#include <strsafe.h>
#pragma endregion


#pragma region Static Members

//
// Initialize the singleton service instance.
//
CServiceBase *CServiceBase::s_service = NULL;


/*++

Routine Description:

    Register the executable for a service with the Service Control
    Manager (SCM).  After you call Run(ServiceBase), the SCM issues a Start
    command, which results in a call to the OnStart method in the service.
    This method blocks until the service has stopped.

Arguments:

    Service - The reference to a CServiceBase object.  It will become the
              singleton service instance of this service application.

Return Value:

    If the function succeeds, the return value is TRUE.  If the function
    fails, the return value is FALSE.  To get extended error information,
    call GetLastError.

--*/
BOOL
CServiceBase::Run(
    CServiceBase &Service
    )
{
    s_service = &Service;

    SERVICE_TABLE_ENTRY serviceTable[] = 
    {
        { Service.m_name, ServiceMain },
        { NULL, NULL }
    };

    //
    // Connects the main thread of a service process to the service control 
    // manager, which causes the thread to be the service control dispatcher 
    // thread for the calling process. This call returns when the service has 
    // stopped. The process should simply terminate when the call returns.
    //
    return StartServiceCtrlDispatcher(serviceTable);
}


/*++

Routine Description:

    The entry point for the service.  It registers the handler function
    for the service and starts the service.

Arguments:

    Argc - The number of command line arguments

    Argv - The array of command line arguments

Return Value:

    VOID

--*/
VOID
WINAPI
CServiceBase::ServiceMain(
    DWORD Argc,
    PWSTR *Argv
    )
{
    assert(s_service != NULL);

    //
    // Register the handler function for the service.
    //
    s_service->m_statusHandle = RegisterServiceCtrlHandler(s_service->m_name,
                                                           ServiceCtrlHandler);

    if (s_service->m_statusHandle == NULL)
    {
        throw GetLastError();
    }

    //
    // Start the service.
    //
    s_service->Start(Argc, Argv);
}


/*++

Routine Description:

    Called by the SCM whenever a control code is sent to the service.

Arguments:

    CtrlCode - The control code.  This parameter can be one of
               the following values:

               SERVICE_CONTROL_CONTINUE
               SERVICE_CONTROL_INTERROGATE
               SERVICE_CONTROL_NETBINDADD
               SERVICE_CONTROL_NETBINDDISABLE
               SERVICE_CONTROL_NETBINDREMOVE
               SERVICE_CONTROL_PARAMCHANGE
               SERVICE_CONTROL_PAUSE
               SERVICE_CONTROL_SHUTDOWN
               SERVICE_CONTROL_STOP

               This parameter can also be a user-defined control
               code ranging from 128 to 255.

Return Value:

    VOID

--*/
VOID
WINAPI
CServiceBase::ServiceCtrlHandler(
    DWORD Ctrl
    )
{
    switch (Ctrl)
    {
    case SERVICE_CONTROL_STOP:
        s_service->Stop();
        break;
    case SERVICE_CONTROL_PAUSE:
        s_service->Pause();
        break;
    case SERVICE_CONTROL_CONTINUE:
        s_service->Continue();
        break;
    case SERVICE_CONTROL_SHUTDOWN:
        s_service->Shutdown();
        break;
    case SERVICE_CONTROL_INTERROGATE:
        break;
    default:
        break;
    }
}

#pragma endregion


#pragma region Service Constructor and Destructor

/*++

Routine Description:

    The constructor of CServiceBase. It initializes a new instance
    of the CServiceBase class. The optional parameters (CanStop,
    CanShutdown and CanPauseContinue) allow you to specify whether the
    service can be stopped, paused and continued, or be notified when system
    shutdown occurs.

Arguments:

    ServiceName      - The name of the service

    CanStop          - The service can be stopped

    CanShutdown      - The service is notified when system shutdown occurs

    CanPauseContinue - The service can be paused and continued

Return Value:

    VOID

--*/
CServiceBase::CServiceBase(
    PWSTR ServiceName, 
    BOOL  CanStop, 
    BOOL  CanShutdown, 
    BOOL  CanPauseContinue
    )
{
    //
    // Service name must be a valid string and cannot be NULL.
    //
    m_name = (ServiceName == NULL) ? L"" : ServiceName;

    m_statusHandle = NULL;

    //
    // The service runs in its own process.
    //
    m_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;

    //
    // The service is starting.
    //
    m_status.dwCurrentState = SERVICE_START_PENDING;

    //
    // The accepted commands of the service.
    //
    DWORD dwControlsAccepted = 0;

    if (CanStop)
    {
        dwControlsAccepted |= SERVICE_ACCEPT_STOP;
    }

    if (CanShutdown)
    {
        dwControlsAccepted |= SERVICE_ACCEPT_SHUTDOWN;
    }

    if (CanPauseContinue)
    {
        dwControlsAccepted |= SERVICE_ACCEPT_PAUSE_CONTINUE;
    }

    m_status.dwControlsAccepted = dwControlsAccepted;

    m_status.dwWin32ExitCode = NO_ERROR;
    m_status.dwServiceSpecificExitCode = 0;
    m_status.dwCheckPoint = 0;
    m_status.dwWaitHint = 0;

    SetupEvents();
}


/*++

Routine Description:

    The virtual destructor of CServiceBase.

Arguments:

    VOID

Return Value:

    VOID

--*/
CServiceBase::~CServiceBase()
{
    DestroyEvents();
}

#pragma endregion


#pragma region Service Start, Stop, Pause, Continue, and Shutdown

/*++

Routine Description:

    This function starts the service.  It calls the OnStart virtual function
    in which you can specify the actions to take when the service starts.  If
    an error occurs during the startup, the error will be logged in the
    Application event log, and the service will be stopped.

Arguments:

    Argc - The number of command line arguments

    Argv - The array of command line arguments

Return Value:

    VOID

--*/
VOID
CServiceBase::Start(
    DWORD  Argc,
    PWSTR *Argv
)
{
    try
    {
        //
        // Tell SCM that the service is starting.
        //
        SetServiceStatus(SERVICE_START_PENDING);

        //
        // Perform service-specific initialization.
        //
        OnStart(Argc, Argv);

        //
        // Tell SCM that the service is started.
        //
        SetServiceStatus(SERVICE_RUNNING);
    }
    catch (DWORD Error)
    {
        //
        // Log the error.
        //
        WriteToErrorLog(L"Service Start", Error);

        //
        // Set the service status to be stopped.
        //
        SetServiceStatus(SERVICE_STOPPED, Error);
    }
    catch (...)
    {
        //
        // Log the error.
        //
        WriteToEventLog(L"Service failed to start.", EVENTLOG_ERROR_TYPE);

        //
        // Set the service status to be stopped.
        //
        SetServiceStatus(SERVICE_STOPPED);
    }
}


/*++

Routine Description:

    When implemented in a derived class, executes when a Start
    command is sent to the service by the SCM or when the operating system
    starts (for a service that starts automatically). Specifies actions to
    take when the service starts. Be sure to periodically call
    CServiceBase::SetServiceStatus() with SERVICE_START_PENDING if the
    procedure is going to take long time. You may also consider spawning a
    new thread in OnStart to perform time-consuming initialization tasks.

Arguments:

    Argc - The number of command line arguments

    Argv - The array of command line arguments

Return Value:

    VOID

--*/
VOID
CServiceBase::OnStart(
    DWORD Argc,
    PWSTR *Argv
)
{
    SetVariables();
}


/*++

Routine Description:

    This function stops the service.  It calls the OnStop virtual
    function in which you can specify the actions to take when the service
    stops.  If an error occurs, the error will be logged in the Application
    event log, and the service will be restored to the original state.

Arguments:

    VOID

Return Value:

    VOID

--*/
VOID
CServiceBase::Stop()
{
    DWORD OriginalState = m_status.dwCurrentState;

    try
    {
        //
        // Tell SCM that the service is stopping.
        //
        SetServiceStatus(SERVICE_STOP_PENDING);

        //
        // Perform service-specific stop operations.
        //
        OnStop();

        //
        // Tell SCM that the service is stopped.
        //
        SetServiceStatus(SERVICE_STOPPED);
    }
    catch (DWORD Error)
    {
        //
        // Log the error.
        //
        WriteToErrorLog(L"Service Stop", Error);

        //
        // Set the orginal service status.
        //
        SetServiceStatus(OriginalState);
    }
    catch (...)
    {
        //
        // Log the error.
        //
        WriteToEventLog(L"Service failed to stop.", EVENTLOG_ERROR_TYPE);

        //
        // Set the orginal service status.
        //
        SetServiceStatus(OriginalState);
    }
}


/*++

Routine Description:

    When implemented in a derived class, executes when a Stop
    command is sent to the service by the SCM. Specifies actions to take
    when a service stops running. Be sure to periodically call
    CServiceBase::SetServiceStatus() with SERVICE_STOP_PENDING if the
    procedure is going to take long time.

Arguments:

    VOID

Return Value:

    VOID

--*/
VOID
CServiceBase::OnStop()
{
}


/*++

Routine Description:

    The function pauses the service if the service supports pause
    and continue. It calls the OnPause virtual function in which you can
    specify the actions to take when the service pauses. If an error occurs,
    the error will be logged in the Application event log, and the service
    will become running.

Arguments:

    VOID

Return Value:

    VOID

--*/
VOID
CServiceBase::Pause()
{
    try
    {
        //
        // Tell SCM that the service is pausing.
        //
        SetServiceStatus(SERVICE_PAUSE_PENDING);

        //
        // Perform service-specific pause operations.
        //
        OnPause();

        //
        // Tell SCM that the service is paused.
        //
        SetServiceStatus(SERVICE_PAUSED);
    }
    catch (DWORD Error)
    {
        //
        // Log the error.
        //
        WriteToErrorLog(L"Service Pause", Error);

        //
        // Tell SCM that the service is still running.
        //
        SetServiceStatus(SERVICE_RUNNING);
    }
    catch (...)
    {
        //
        // Log the error.
        //
        WriteToEventLog(L"Service failed to pause.", EVENTLOG_ERROR_TYPE);

        //
        // Tell SCM that the service is still running.
        //
        SetServiceStatus(SERVICE_RUNNING);
    }
}


/*++

Routine Description:

    When implemented in a derived class, executes when a Pause
    command is sent to the service by the SCM. Specifies actions to take
    when a service pauses.

Arguments:

    VOID

Return Value:

    VOID

--*/
VOID
CServiceBase::OnPause()
{
}


/*++

Routine Description:

    The function resumes normal functioning after being paused if
    the service supports pause and continue. It calls the OnContinue virtual
    function in which you can specify the actions to take when the service
    continues. If an error occurs, the error will be logged in the
    Application event log, and the service will still be paused.

Arguments:

    VOID

Return Value:

    VOID

--*/
VOID
CServiceBase::Continue()
{
    try
    {
        //
        // Tell SCM that the service is resuming.
        //
        SetServiceStatus(SERVICE_CONTINUE_PENDING);

        //
        // Perform service-specific continue operations.
        //
        OnContinue();

        //
        // Tell SCM that the service is running.
        //
        SetServiceStatus(SERVICE_RUNNING);
    }
    catch (DWORD Error)
    {
        //
        // Log the error.
        //
        WriteToErrorLog(L"Service Continue", Error);

        //
        // Tell SCM that the service is still paused.
        //
        SetServiceStatus(SERVICE_PAUSED);
    }
    catch (...)
    {
        //
        // Log the error.
        //
        WriteToEventLog(L"Service failed to resume.", EVENTLOG_ERROR_TYPE);

        //
        // Tell SCM that the service is still paused.
        //
        SetServiceStatus(SERVICE_PAUSED);
    }
}


/*++

Routine Description:

    When implemented in a derived class, OnContinue runs when a
    Continue command is sent to the service by the SCM. Specifies actions to
    take when a service resumes normal functioning after being paused.

Arguments:

    VOID

Return Value:

    VOID

--*/
VOID
CServiceBase::OnContinue()
{
}


/*++

Routine Description:

    The function executes when the system is shutting down. It
    calls the OnShutdown virtual function in which you can specify what
    should occur immediately prior to the system shutting down. If an error
    occurs, the error will be logged in the Application event log.

Arguments:

    VOID

Return Value:

    VOID

--*/
VOID
CServiceBase::Shutdown()
{
    try
    {
        //
        // Perform service-specific shutdown operations.
        //
        OnShutdown();

        //
        // Tell SCM that the service is stopped.
        //
        SetServiceStatus(SERVICE_STOPPED);
    }
    catch (DWORD Error)
    {
        //
        // Log the error.
        //
        WriteToErrorLog(L"Service Shutdown", Error);
    }
    catch (...)
    {
        //
        // Log the error.
        //
        WriteToEventLog(L"Service failed to shut down.", EVENTLOG_ERROR_TYPE);
    }
}


/*++

Routine Description:

    When implemented in a derived class, executes when the system
    is shutting down. Specifies what should occur immediately prior to the
    system shutting down.

Arguments:

    VOID

Return Value:

    VOID

--*/
VOID
CServiceBase::OnShutdown()
{
}

#pragma endregion


#pragma region Helper Functions

/*++

Routine Description:

    The function sets the service status and reports the status to the SCM.

Arguments:

    CurrentState  - The current state of the service

    Win32ExitCode - The error code to report

    WaitHint      - The estimated time for pending operation, in milliseconds

Return Value:

    VOID

--*/
VOID
CServiceBase::SetServiceStatus(
    DWORD CurrentState, 
    DWORD Win32ExitCode, 
    DWORD WaitHint
    )
{
    static DWORD CheckPoint = 1;

    //
    // Fill in the SERVICE_STATUS structure of the service.
    //

    m_status.dwCurrentState = CurrentState;
    m_status.dwWin32ExitCode = Win32ExitCode;
    m_status.dwWaitHint = WaitHint;

    m_status.dwCheckPoint = ((CurrentState == SERVICE_RUNNING) ||
                             (CurrentState == SERVICE_STOPPED)) ? 0 :
                                                                  CheckPoint++;

    //
    // Report the status of the service to the SCM.
    //
    ::SetServiceStatus(m_statusHandle, &m_status);
}

#pragma endregion