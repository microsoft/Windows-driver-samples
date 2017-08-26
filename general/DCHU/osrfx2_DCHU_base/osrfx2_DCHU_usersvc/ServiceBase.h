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

#pragma once

#include <windows.h>
#include "Utils.h"

class CServiceBase
{
public:

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
    static BOOL Run(CServiceBase &service);


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
    CServiceBase(PWSTR ServiceName, 
                 BOOL  CanStop = TRUE, 
                 BOOL  CanShutdown = TRUE, 
                 BOOL  CanPauseContinue = FALSE);


    /*++

    Routine Description:

        The virtual destructor of CServiceBase.

    Arguments:

        VOID

    Return Value:

        VOID

    --*/ 
    virtual ~CServiceBase();


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
    VOID Stop();

protected:

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
    virtual VOID OnStart(DWORD Argc, PWSTR *Argv);


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
    virtual VOID OnStop();


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
    virtual VOID OnPause();


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
    virtual VOID OnContinue();


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
    virtual VOID OnShutdown();


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
    VOID SetServiceStatus(DWORD CurrentState,
                          DWORD Win32ExitCode = NO_ERROR,
                          DWORD WaitHint = 0);

private:

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
    static VOID WINAPI ServiceMain(DWORD Argc, PWSTR *Argv);


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
    static VOID WINAPI ServiceCtrlHandler(DWORD Ctrl);


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
    VOID Start(DWORD Argc, PWSTR *Argv);

    
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
    VOID Pause();


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
    VOID Continue();


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
    VOID Shutdown();


    //
    // The singleton service instance.
    //
    static CServiceBase *s_service;

    //
    // The name of the service.
    //
    PWSTR m_name;

    //
    // The status of the service.
    //
    SERVICE_STATUS m_status;

    //
    // The service status handle.
    //
    SERVICE_STATUS_HANDLE m_statusHandle;
};