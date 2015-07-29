////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_Service.cpp
//
//   Abstract:
//      This module contains functions which assist actions pertaining to services.
//
//   Naming Convention:
//
//      <Scope><Module><Object><Action><Modifier>
//  
//      i.e.
//
//       <Scope>
//          {
//                         - Function is likely visible to other modules
//            Prv          - Function is private to this module.
//          }
//       <Module>
//          {
//            Hlpr         - Function is from HelperFunctions_* Modules.
//          }
//       <Object>
//          {
//            Service      - Function pertains to services.
//            ServiceState - Function pertains to a service's state
//          }
//       <Action>
//          {
//            Control      - Function sends a specified control to the object.
//            Query        - Function gets information about the object.
//            Start        - Function sends a stop control to the object.
//            Stop         - Function sends a stop control to the object.
//            To           - Function converts one value type to another.
//          }
//       <Modifier>
//          {
//            State      - Funstion acts on the object's state.
//            String     - Function acts on a null terminated wide character string.
//          }
//
//   Private Functions:
//      PrvHlprServiceControl(),
//      PrvHlprServiceQueryState(),
//      PrvHlprServiceStateToString(),
//
//   Public Functions:
//      HlprServiceStart(),
//      HlprServiceStop()
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "HelperFunctions_Include.h" /// .

/**
 @helper_function="PrvHlprServiceQueryState"
 
   Purpose: Return the state of the service.                                                    <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS685974.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 PrvHlprServiceQueryState(_In_ PCWSTR pServiceName,
                                _Inout_ UINT32* pServiceState,
                                _Inout_opt_ UINT32* pWait = 0)
{
   ASSERT(pServiceName);
   ASSERT(pServiceState);

   UINT32                 status        = NO_ERROR;
   SC_HANDLE              scmHandle     = 0;
   SC_HANDLE              svcHandle     = 0;
   UINT32                 bytesRequired = 0;
   SERVICE_STATUS_PROCESS srvStatus     = {0};

   scmHandle = OpenSCManager(0,
                             SERVICES_ACTIVE_DATABASE,
                             SC_MANAGER_CONNECT);
   if(scmHandle == 0)
   {
      status = GetLastError();

      HlprLogError(L"PrvHlprServiceQueryState: OpenSCManager() [status: %#x]",
                   status);

      HLPR_BAIL;
   }

   svcHandle = OpenService(scmHandle,
                           pServiceName,
                           SERVICE_QUERY_STATUS);
   if(svcHandle == 0)
   {
      status = GetLastError();

      HlprLogError(L"PrvHlprServiceQueryState: OpenService() [status: %#x]",
                   status);

      HLPR_BAIL;
   }

   if(!QueryServiceStatusEx(svcHandle,
                            SC_STATUS_PROCESS_INFO,
                            (BYTE*)&srvStatus,
                            sizeof(SERVICE_STATUS_PROCESS),
                            (DWORD*)&bytesRequired))
   {
      status = GetLastError();

      HlprLogError(L"PrvHlprServiceQueryState: QueryServiceStatus() [status: %#x]",
                   status);

      HLPR_BAIL;
   }
   else
   {
      status = NO_ERROR;

      *pServiceState = srvStatus.dwCurrentState;

      if(pWait)
         *pWait = srvStatus.dwWaitHint;
   }

   HLPR_BAIL_LABEL:

   if(status != NO_ERROR)
   {
      *pServiceState = 0;

      if(pWait)
         *pWait = 0;
   }

   HLPR_CLOSE_SERVICE_HANDLE(svcHandle);

   HLPR_CLOSE_SERVICE_HANDLE(scmHandle);

   return status;
}

/**
 @helper_function="PrvHlprServiceStateToString"
 
   Purpose: Return a string representation of the service state.                                <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS685974.aspx              <br>
*/
_Success_(return != 0)
PCWSTR PrvHlprServiceStateToString(_In_ const UINT32 state)
{
   switch(state)
   {
      case SERVICE_STOPPED:
         return L"SERVICE_STOPPED";
      case SERVICE_START_PENDING:
         return L"SERVICE_START_PENDING";
      case SERVICE_STOP_PENDING:
         return L"SERVICE_STOP_PENDING";
      case SERVICE_RUNNING:
         return L"SERVICE_RUNNING";
      case SERVICE_CONTINUE_PENDING:
         return L"SERVICE_CONTINUE_PENDING";
      case SERVICE_PAUSE_PENDING:
         return L"SERVICE_PAUSE_PENDING";
      case SERVICE_PAUSED:
         return L"SERVICE_PAUSED";
   }

   return L"Unknown";
}

/**
 @helper_function="PrvHlprServiceControl"
 
   Purpose: Issue a control to the specified service.                                           <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS685974.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 PrvHlprServiceControl(_In_ PCWSTR pServiceName,
                             _In_ const HLPR_SERVICE_COMMAND command)
{
   ASSERT(pServiceName);
   ASSERT(command < HLPR_SERVICE_COMMAND_MAX);

   UINT32 status       = NO_ERROR;
   UINT32 serviceState = SERVICE_STOPPED;

   status = PrvHlprServiceQueryState(pServiceName,
                                     &serviceState);

   if(status == NO_ERROR &&
      ((command == HLPR_SERVICE_COMMAND_STOP &&
      serviceState != SERVICE_STOPPED) ||
      (command == HLPR_SERVICE_COMMAND_START &&
      serviceState != SERVICE_RUNNING)))
   {
      SC_HANDLE      scmHandle = 0;
      SC_HANDLE      svcHandle = 0;
      SERVICE_STATUS srvStatus = {0};

      scmHandle = OpenSCManager(0,
                                SERVICES_ACTIVE_DATABASE,
                                SC_MANAGER_CONNECT);
      if(scmHandle == 0)
      {
         status = GetLastError();

         HlprLogError(L"PrvHlprServiceControl : OpenSCManager() [status: %#x]",
                      status);

         HLPR_BAIL;
      }

      svcHandle = OpenService(scmHandle,
                              pServiceName,
                              SERVICE_START | SERVICE_STOP);
      if(svcHandle == 0)
      {
         status = GetLastError();

         HlprLogError(L"PrvHlprServiceControl : OpenService() [status: %#x]",
                      status);

         HLPR_BAIL;
      }

      if(command == HLPR_SERVICE_COMMAND_START)
      {
         if(!StartService(svcHandle,
                          0,
                          0))
         {
            status = GetLastError();

            if(status != ERROR_SERVICE_ALREADY_RUNNING)
               HlprLogError(L"HlprServiceStopStart: StartService() [status: %#x][Starting %s]",
                            status,
                            pServiceName);
         }
      }
      else
      {
         if(!ControlService(svcHandle,
                            SERVICE_CONTROL_STOP,
                            &srvStatus))
         {
            status = GetLastError();

            if(status == NO_ERROR ||
               status == ERROR_INVALID_SERVICE_CONTROL ||
               status == ERROR_SERVICE_CANNOT_ACCEPT_CTRL ||
               status == ERROR_SERVICE_NOT_ACTIVE)
               HlprLogError(L"PrvHlprServiceControl : ControlService() [status: %#x][Stopping %s][ServiceType: %#x][CurrentState: %s][ControlsAccepted: %#x][Win32ExitCode: %#x][ServiceSpecificExitCode: %#x][CheckPoint: %#x][WaitHint: %#x]",
                            status,
                            pServiceName,
                            srvStatus.dwServiceType,
                            PrvHlprServiceStateToString(srvStatus.dwCurrentState),
                            srvStatus.dwControlsAccepted,
                            srvStatus.dwWin32ExitCode,
                            srvStatus.dwServiceSpecificExitCode,
                            srvStatus.dwCheckPoint,
                            srvStatus.dwWaitHint);
            else
               HlprLogError(L"PrvHlprServiceControl : ControlService() [status: %#x][Stopping %s]",
                            status,
                            pServiceName);

            HLPR_BAIL;
         }
      }

      status = NO_ERROR;

      HLPR_BAIL_LABEL:

      HLPR_CLOSE_SERVICE_HANDLE(svcHandle);

      HLPR_CLOSE_SERVICE_HANDLE(scmHandle);
   }

   return status;
}

/**
 @helper_function="HlprServiceNotificationRegister"
 
   Purpose: Register a notification function which is triggered by the service's transition in 
            state.                                                                              <br>
                                                                                                <br>
   Notes:   The caller is responsible for closing the service handle using 
            HLPR_CLOSE_SERVICE_HANDLE.                                                          <br>
                                                                                                <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS684276.aspx              <br>
*/
_When_(return != NO_ERROR, _At_(*pSCMHandle, _Post_ _Null_))
_When_(return != NO_ERROR, _At_(*pSvcHandle, _Post_ _Null_))
_When_(return == NO_ERROR, _At_(*pSCMHandle, _Post_ _Notnull_))
_When_(return == NO_ERROR, _At_(*pSvcHandle, _Post_ _Notnull_))
_Success_(return == NO_ERROR)
UINT32 HlprServiceNotificationStateChangeRegister(_In_ PCWSTR pServiceName,
                                                  _In_ SERVICE_NOTIFY* pSvcNotify,
                                                  _In_ UINT32 notifyMask,
                                                  _Out_ SC_HANDLE* pSCMHandle,
                                                  _Out_ SC_HANDLE* pSvcHandle)
{
   ASSERT(pServiceName);
   ASSERT(pSvcNotify);
   ASSERT(pSCMHandle);
   ASSERT(pSvcHandle);

   UINT32 status = NO_ERROR;

   *pSCMHandle = 0;

   *pSvcHandle = 0;

   *pSCMHandle = OpenSCManager(0,
                               SERVICES_ACTIVE_DATABASE,
                               SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE);
   if(*pSCMHandle == 0)
   {
      status = GetLastError();

      HlprLogError(L"HlprServiceNotificationStateChangeRegister: OpenSCManager() [status: %#x]",
                   status);

      HLPR_BAIL;
   }

   *pSvcHandle = OpenService(*pSCMHandle,
                             pServiceName,
                             SERVICE_QUERY_STATUS);
   if(*pSvcHandle == 0)
   {
      status = GetLastError();

      HlprLogError(L"HlprServiceNotificationStateChangeRegister: OpenService() [status: %#x]",
                   status);

      HLPR_BAIL;
   }

   status = NotifyServiceStatusChange(*pSvcHandle,
                                      notifyMask,
                                      pSvcNotify);
   if(status != NO_ERROR)
      HlprLogError(L"HlprServiceNotificationStateChangeRegister: NotifyServiceStatusChange() [status: %#x]",
                   status);

   HLPR_BAIL_LABEL:

   if(status != NO_ERROR)
   {
      HLPR_CLOSE_SERVICE_HANDLE(*pSvcHandle);

      HLPR_CLOSE_SERVICE_HANDLE(*pSCMHandle);
   }

   return status;
}

/**
 @helper_function="HlprServiceQueryState"
 
   Purpose: Return the state of the service.                                                    <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return != 0)
UINT32 HlprServiceQueryState(_In_ PCWSTR pServiceName)
{
   ASSERT(pServiceName);

   UINT32 serviceState = 0;
   UINT32 wait         = 0;

   PrvHlprServiceQueryState(pServiceName,
                            &serviceState,
                            &wait);

   return serviceState;
}

/**
 @helper_function="HlprServiceStart"
 
   Purpose: Issue a start control to the specified service.                                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprServiceStart(_In_ PCWSTR pServiceName)
{
   ASSERT(pServiceName);

   UINT32       status        = NO_ERROR;
   const UINT64 MAX_WAIT_TIME = 5000;
   UINT64       now           = 0;

   status = PrvHlprServiceControl(pServiceName,
                                  HLPR_SERVICE_COMMAND_START);
   HLPR_BAIL_ON_FAILURE(status);

   for(UINT64 then = 0;
       MAX_WAIT_TIME > now - then;
       )
   {
      UINT32 serviceState = SERVICE_STOPPED;
      UINT32 wait         = 0;

      if(then == 0)
         then = GetTickCount64();

      status = PrvHlprServiceQueryState(pServiceName,
                                        &serviceState,
                                        &wait);
      HLPR_BAIL_ON_FAILURE(status);

      if(serviceState != SERVICE_RUNNING)
      {
         Sleep(wait);

         now = GetTickCount64();
      }
      else
         break;
   }

   HLPR_BAIL_LABEL:

   return status;
}

/**
 @helper_function="HlprServiceStop"
 
   Purpose: Issue a stop control to the specified service.                                      <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprServiceStop(_In_ PCWSTR pServiceName)
{
   ASSERT(pServiceName);

   UINT32       status        = NO_ERROR;
   const UINT64 MAX_WAIT_TIME = 5000;
   UINT64       now           = 0;

   status = PrvHlprServiceControl(pServiceName,
                                  HLPR_SERVICE_COMMAND_STOP);
   HLPR_BAIL_ON_FAILURE(status);

   for(UINT64 then = 0;
       MAX_WAIT_TIME > now - then;
       )
   {
      UINT32 serviceState = SERVICE_RUNNING;
      UINT32 wait         = 0;

      if(then == 0)
         then = GetTickCount64();

      status = PrvHlprServiceQueryState(pServiceName,
                                        &serviceState,
                                        &wait);
      HLPR_BAIL_ON_FAILURE(status);

      if(serviceState != SERVICE_STOPPED)
      {
         Sleep(wait);

         now = GetTickCount64();
      }
      else
         break;
   }

   HLPR_BAIL_LABEL:

   return status;
}
