////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation. All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_WinSock.cpp
//
//   Abstract:
//      This module contains functions for assisting in operations pertaining to Windows Sockets.
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
 @helper_function="HlprWinSockCleanup"
 
   Purpose:  Terminates use of the Winsock DLL for the process's use.                           <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS741549.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprWinSockCleanup()
{
   UINT32 status = NO_ERROR;

   status = WSACleanup();
   if(status != NO_ERROR)
   {
      status = WSAGetLastError();

      HlprLogError(L"HlprWinSockCleanup : WSACleanup() [status: %#x]",
                   status);
   }

   return status;
}

/**
 @helper_function="HlprWinSockInitialize"
 
   Purpose:  Initializes use of the Winsock DLL for the process's use.                          <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS742213.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprWinSockInitialize()
{
   UINT32  status           = NO_ERROR;
   UINT16  versionRequested = MAKEWORD(2,
                                       2);
   WSADATA wsaData          = {0};

   status = WSAStartup(versionRequested,
                       &wsaData);
   if(status != NO_ERROR)
      HlprLogError(L"HlprWinSockInitialize : WSAStartup() [status: %#x]",
                   status);

   return status;
}

/**
 @helper_function="HlprWinSockDestroyWSAEvent"
 
   Purpose:  Free a WSAEvent.                                                                   <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS741551.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprWinSockDestroyWSAEvent(_Inout_ WSAEVENT* pWSAEvent)
{
   ASSERT(pWSAEvent);

   UINT32 status = NO_ERROR;

   if(!WSACloseEvent(*pWSAEvent))
   {
      status = WSAGetLastError();

      HlprLogError(L"HlprWinSockDestroyWSAEvent : WSACloseEvent() [status: %#x]",
                   status);
   }
   else
      *pWSAEvent = WSA_INVALID_EVENT;

   return status;
}

/**
 @helper_function="HlprWinSockSetWSAEvent"
 
   Purpose:  Set a WSAEvent to signaled.                                                        <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS742208.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprWinSockSetWSAEvent(_In_ WSAEVENT wsaEvent)
{
   UINT32 status = NO_ERROR;

   if(!WSASetEvent(wsaEvent))
   {
      status = WSAGetLastError();

      HlprLogError(L"HlprWinSockSetWSAEvent : WSASetEvent() [status: %#x]",
                   status);
   }

   return status;
}

/**
 @helper_function="HlprWinSockCreateWSAEvent"
 
   Purpose:  Create a WSAEvent.                                                                 <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS741561.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprWinSockCreateWSAEvent(_Inout_ WSAEVENT* pWSAEvent)
{
   ASSERT(pWSAEvent);

   UINT32 status = NO_ERROR;

   *pWSAEvent = WSACreateEvent();
   if(*pWSAEvent == WSA_INVALID_EVENT)
   {
      status = WSAGetLastError();

      HlprLogError(L"HlprWinSockCreateWSAEvent : WSACreateEvent() [status: %#x]",
                   status);
   }

   return status;
}

/**
 @helper_function="HlprWinSockDestroySocket"
 
   Purpose:  Destroy a socket gracefully.                                                       <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS740481.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS737582.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprWinSockDestroySocket(_Inout_ SOCKET* pSocket)
{
   ASSERT(pSocket);

   UINT32 status = NO_ERROR;

   if(*pSocket != INVALID_SOCKET)
   {
      status = shutdown(*pSocket,
                        SD_BOTH);
      if(status != NO_ERROR)
      {
         status = WSAGetLastError();

         HlprLogError(L"HlprWinSockDestroySocket : shutdown() [status: %#x]",
                      status);

         HLPR_BAIL;
      }

      status = closesocket(*pSocket);
      if(status != NO_ERROR)
      {
         status = WSAGetLastError();

         HlprLogError(L"HlprWinSockDestroySocket : shutdown() [status: %#x]",
                      status);

         HLPR_BAIL;
      }

      *pSocket = INVALID_SOCKET;
   }

   HLPR_BAIL_LABEL:

   return status;
}

/**
 @helper_function="HlprWinSockCreateSocket"
 
   Purpose:  Create a new socket.                                                               <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS742212.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprWinSockCreateSocket(_In_ SOCKADDR_STORAGE* pSockAddrStorage,
                               _In_ UINT8 protocol,
                               _Inout_ SOCKET* pSocket)
{
   ASSERT(pSockAddrStorage);
   ASSERT(pSocket);

   UINT32 status = NO_ERROR;
   UINT32 type   = SOCK_RAW;

   if(protocol == IPPROTO_TCP)
      type = SOCK_STREAM;
   else if(protocol == IPPROTO_UDP)
      type = SOCK_DGRAM;

   *pSocket = WSASocket(pSockAddrStorage->ss_family,
                        type,
                        protocol,
                        0,
                        0,
                        WSA_FLAG_OVERLAPPED);
   if(*pSocket == INVALID_SOCKET)
   {
      status = WSAGetLastError();

      HlprLogError(L"HlprWinSockCreateSocket : WSASocket() [status:%#x]",
                   status);
   }

   return status;
}

/**
 @helper_function="HlprWinSockSetAbortiveDisconnect"
 
   Purpose:  Set the linger socket option to FALSE.                                             <br>
                                                                                                <br>
   Notes:    This function needs to be called prior to HlprWinSockDestroySocket for stream 
             sockets that get a WSAECONNRESET.                                                  <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS740476.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprWinSockSetAbortiveDisconnect(_In_ SOCKET abortedSocket)
{
   ASSERT(abortedSocket != INVALID_SOCKET);

   UINT32 status = NO_ERROR;
   LINGER linger = {0};

   linger.l_onoff  = TRUE;
   linger.l_linger = 0;

   status = setsockopt(abortedSocket,
                       SOL_SOCKET,
                       SO_LINGER,
                       (char*)&linger,
                       sizeof(LINGER));
   if(status != NO_ERROR)
   {
      status = WSAGetLastError();

      HlprLogError(L"HlprWinSockSetAbortiveDisconnect : setsockopt() [status: %#x]",
                   status);
   }

   return status;
}

/**
 @helper_function="HlprWinSockSetSocketReceiveTimeout"
 
   Purpose:  Specify a timeout for blocking receive calls.                                      <br>
                                                                                                <br>
   Notes:    This function needs to be called prior to binding the socket.                      <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS740476.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprWinSockSetSocketReceiveTimeout(_In_ SOCKET receivingSocket,
                                          _In_ UINT32 timeout)         /* 5000 */
{
   ASSERT(receivingSocket != INVALID_SOCKET);

   UINT32 status = NO_ERROR;

   status = setsockopt(receivingSocket,
                       SOL_SOCKET,
                       SO_RCVTIMEO,
                       (char*)&timeout,
                       sizeof(UINT32));
   if(status != NO_ERROR)
   {
      status = WSAGetLastError();

      HlprLogError(L"HlprWinSockSetSocketReceiveTimeout : setsockopt() [status: %#x]",
                   status);
   }

   return status;
}

/**
 @helper_function="HlprWinSockSetSocketSendTimeout"
 
   Purpose:  Specify a timeout for blocking send calls.                                         <br>
                                                                                                <br>
   Notes:    This function needs to be called prior to binding the socket.                      <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS740476.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprWinSockSetSocketSendTimeout(_In_ SOCKET sendingSocket,
                                       _In_ UINT32 timeout)       /* 5000 */
{
   ASSERT(sendingSocket != INVALID_SOCKET);

   UINT32 status = NO_ERROR;

   status = setsockopt(sendingSocket,
                       SOL_SOCKET,
                       SO_SNDTIMEO,
                       (char*)&timeout,
                       sizeof(UINT32));
   if(status != NO_ERROR)
   {
      status = WSAGetLastError();

      HlprLogError(L"HlprWinSockSetSocketSendTimeout : setsockopt() [status: %#x]",
                   status);
   }

   return status;
}


/**
 @helper_function="HlprWinSockSetSocketNonBlocking"
 
   Purpose:  Enable non-blocking mode on the socket.                                            <br>
                                                                                                <br>
   Notes:    This function needs to be called prior to binding the socket.                      <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS741621.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprWinSockSetSocketNonBlocking(_In_ SOCKET unboundSocket)
{
   ASSERT(unboundSocket != INVALID_SOCKET);

   UINT32 status              = NO_ERROR;
   UINT32 isNonBlockingSocket = TRUE;
   UINT32 bytesReturned       = 0;

   status = WSAIoctl(unboundSocket,
                     FIONBIO,
                     &isNonBlockingSocket,
                     sizeof(UINT32),
                     0,
                     0,
                     (LPDWORD)&bytesReturned,
                     0,
                     0);
   if(status != NO_ERROR)
   {
      status = WSAGetLastError();

      HlprLogError(L"HlprWinSockSetSocketNonBlocking : WSAIoctl() [status: %#x]",
                   status);
   }

   return status;
}

/**
 @helper_function="HlprWinSockAcquirePortReservationForSocket"
 
   Purpose:  Request a runtime reservation for the provided ports.                              <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/GG699720.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS741621.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprWinSockAcquirePortReservationForSocket(_In_ SOCKET unboundSocket,
                                                  _In_ INET_PORT_RANGE* pPortRange,
                                                  _Inout_ UINT64* pReservationToken)
{
   ASSERT(unboundSocket != INVALID_SOCKET);
   ASSERT(pPortRange);
   ASSERT(pReservationToken);

   UINT32                         status        = NO_ERROR;
   UINT32                         bytesReturned = 0;
   INET_PORT_RESERVATION_INSTANCE reservation   = {0};

   status = WSAIoctl(unboundSocket,
                     SIO_ACQUIRE_PORT_RESERVATION,
                     pPortRange,
                     sizeof(INET_PORT_RANGE),
                     &reservation,
                     sizeof(INET_PORT_RESERVATION_INSTANCE),
                     (LPDWORD)&bytesReturned,
                     0,
                     0);
   if(status != NO_ERROR)
   {
      status = WSAGetLastError();

      HlprLogError(L"HlprWinSockAssociatePortReservationWithSocket : WSAIoctl() [status: %#x]",
                   status);
   }
   else
      *pReservationToken = reservation.Token.Token;

   return status;
}

/**
 @helper_function="HlprWinSockAssociatePortReservationWithSocket"
 
   Purpose:  Associate the socket with the persistent port(s) that were reserved previously.    <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/GG699721.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS741621.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprWinSockAssociatePortReservationWithSocket(_In_ SOCKET unboundSocket,
                                                     _In_ UINT64 reservationToken)
{
   ASSERT(unboundSocket != INVALID_SOCKET);

   UINT32 status        = NO_ERROR;
   UINT32 bytesReturned = 0;

   status = WSAIoctl(unboundSocket,
                     SIO_ASSOCIATE_PORT_RESERVATION,
                     &reservationToken,
                     sizeof(UINT64),
                     0,
                     0,
                     (LPDWORD)&bytesReturned,
                     0,
                     0);
   if(status != NO_ERROR)
   {
      status = WSAGetLastError();

      HlprLogError(L"HlprWinSockAssociatePortReservationWithSocket : WSAIoctl() [status: %#x]",
                   status);
   }

   return status;
}

/**
 @private_function="HlprWinSockQueryPortReservation"
 
   Purpose:  Find the token for the previously created port reservations.                       <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/GG696072.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/GG696063.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprWinSockQueryPortReservation(_In_ UINT8 ipProtocol,
                                       _In_ INET_PORT_RANGE* pPortRange,
                                       _Inout_ UINT64* pReservationToken)
{
   ASSERT(ipProtocol != IPPROTO_TCP &&
          ipProtocol != IPPROTO_UDP);
   ASSERT(pPortRange);
   ASSERT(pReservationToken);

   UINT32 status = NO_ERROR;

   if(ipProtocol == IPPROTO_TCP)
      status = LookupPersistentTcpPortReservation(htons(pPortRange->StartPort),
                                                  pPortRange->NumberOfPorts,
                                                  pReservationToken);
   else
      status = LookupPersistentUdpPortReservation(htons(pPortRange->StartPort),
                                                  pPortRange->NumberOfPorts,
                                                  pReservationToken);

   if(status != NO_ERROR)
   {
      if(status == ERROR_NOT_FOUND)
         HlprLogInfo(L"HlprWinSockQueryPortReservation : LookupPersistent%sPortReservation() [status: %#x]",
                     ipProtocol == IPPROTO_TCP ? L"Tcp" : L"Udp",
                     status);
      else
         HlprLogError(L"HlprWinSockQueryPortReservation : LookupPersistent%sPortReservation() [status: %#x]",
                      ipProtocol == IPPROTO_TCP ? L"Tcp" : L"Udp",
                      status);

      *pReservationToken = 0;
   }

   return status;
}

/**
 @private_function="HlprWinSockDestroyPortReservation"
 
   Purpose:  Removes the reservation of specific ports for the Proxy service's use.             <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/GG696070.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/GG696071.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprWinSockDestroyPortReservation(_In_ UINT8 ipProtocol,
                                         _In_ INET_PORT_RANGE* pPortRange,
                                         _Inout_opt_ UINT64* pReservationToken) /* 0 */
{
   ASSERT(ipProtocol != IPPROTO_TCP &&
          ipProtocol != IPPROTO_UDP);
   ASSERT(pPortRange);

   UINT32 status = NO_ERROR;

   if(ipProtocol == IPPROTO_TCP)
      status = DeletePersistentTcpPortReservation(htons(pPortRange->StartPort),
                                                  pPortRange->NumberOfPorts);
   else
      status = DeletePersistentUdpPortReservation(htons(pPortRange->StartPort),
                                                  pPortRange->NumberOfPorts);

   if(status == NO_ERROR)
   {
      if(pReservationToken)
         *pReservationToken = 0;
   }
   else if(status == ERROR_NOT_FOUND)
   {
      status = NO_ERROR;

      if(pReservationToken)
         *pReservationToken = 0;
   }
   else
      HlprLogError(L"HlprWinSockDestroyPortReservation : DeletePersistent%sPortReservation() [status: %#x]",
                   ipProtocol == IPPROTO_TCP ? L"Tcp" : L"Udp",
                   status);

   return status;
}

/**
 @private_function="HlprWinSockCreatePortReservation"
 
   Purpose:  Reserves specific ports for the Proxy service's use.                               <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/GG696068.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/GG696069.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprWinSockCreatePortReservation(_In_ UINT8 ipProtocol,
                                        _In_ INET_PORT_RANGE* pPortRange,
                                        _Inout_ UINT64* pReservationToken)
{
   ASSERT(ipProtocol != IPPROTO_TCP &&
          ipProtocol != IPPROTO_UDP);
   ASSERT(pPortRange);
   ASSERT(pReservationToken);

   UINT32 status = NO_ERROR;

   if(ipProtocol == IPPROTO_TCP)
      status = CreatePersistentTcpPortReservation(htons(pPortRange->StartPort),
                                                  pPortRange->NumberOfPorts,
                                                  pReservationToken);
   else
      status = CreatePersistentUdpPortReservation(htons(pPortRange->StartPort),
                                                  pPortRange->NumberOfPorts,
                                                  pReservationToken);

   if(status != NO_ERROR)
   {
      HlprLogError(L"HlprWinSockCreatePortReservation : CreatePersistent%sPortReservation() [status: %#x]",
                   ipProtocol == IPPROTO_TCP ? L"Tcp" : L"Udp",
                   status);

      *pReservationToken = 0;
   }

   return status;
}

#if(NTDDI_VERSION >= NTDDI_WIN8)

/**
 @helper_function="HlprWinSockAssociateConnectionRedirectRecordsAndContextWithProxySocket"
 
   Purpose:  Retrieve the original REDIRECT_RECORDS from the original socket and associate them 
             with the new (proxy) socket.                                                       <br>
                                                                                                <br>
   Notes:    This function needs to be called prior to binding the proxied socket.              <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS741621.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/HH802472.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/HH802473.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/HH802474.aspx             <br>

*/
_Success_(return == NO_ERROR)
UINT32 HlprWinSockAssociateConnectionRedirectRecordsAndContextWithProxySocket(_In_ SOCKET originalSocket,
                                                                              _Inout_ SOCKET proxySocket,
                                                                              _Inout_ SOCKADDR_STORAGE* pProxyPeerSockAddrStorage,
                                                                              _Inout_opt_ SOCKADDR_STORAGE* pOriginSockAddrStorage) /* 0 */
{
   ASSERT(originalSocket != INVALID_SOCKET);
   ASSERT(proxySocket != INVALID_SOCKET);
   ASSERT(pProxyPeerSockAddrStorage);

   UINT32       status                = NO_ERROR;
   const SIZE_T REDIRECT_RECORDS_SIZE = 8192;
   const SIZE_T REDIRECT_CONTEXT_SIZE = sizeof(SOCKADDR_STORAGE) * 2;
   BYTE*        pRedirectRecords      = 0;
   BYTE*        pRedirectContext      = 0;
   SIZE_T       redirectRecordsSize   = 0;
   SIZE_T       redirectContextSize   = 0;
   SIZE_T       outputSize            = 0;

   HLPR_NEW_ARRAY(pRedirectRecords,
                  BYTE,
                  REDIRECT_RECORDS_SIZE);
   HLPR_BAIL_ON_ALLOC_FAILURE(pRedirectRecords,
                              status);

   HLPR_NEW_ARRAY(pRedirectContext,
                  BYTE,
                  REDIRECT_CONTEXT_SIZE);
   HLPR_BAIL_ON_ALLOC_FAILURE(pRedirectContext,
                              status);

   /// Retrieve the REDIRECT_RECORDS from the client socket.
   status = WSAIoctl(originalSocket,
                     SIO_QUERY_WFP_CONNECTION_REDIRECT_RECORDS,
                     0,
                     0,
                     pRedirectRecords,
                     REDIRECT_RECORDS_SIZE,
                     (LPDWORD)&redirectRecordsSize,
                     0,
                     0);
   if(status != NO_ERROR)
   {
      status = WSAGetLastError();

      HlprLogError(L"HlprWinSockAssociateConnectionRedirectRecordsAndContextWithProxySocket : WSAIoctl() [status: %#x]",
                   status);

      HLPR_BAIL;
   }

   /// Retrieve the REDIRECT_CONTEXT from the client socket.
   status = WSAIoctl(originalSocket,
                     SIO_QUERY_WFP_CONNECTION_REDIRECT_CONTEXT,
                     0,
                     0,
                     pRedirectContext,
                     REDIRECT_CONTEXT_SIZE,
                     (LPDWORD)&redirectContextSize,
                     0,
                     0);
   if(status != NO_ERROR)
   {
      status = WSAGetLastError();

      if(status != FWP_E_NOT_FOUND)
      {
         HlprLogError(L"HlprWinSockAssociateConnectionRedirectRecordsAndContextWithProxySocket : WSAIoctl() [status: %#x]",
                      status);

         HLPR_BAIL;
      }
   }
   else
   {
      ASSERT(redirectContextSize == REDIRECT_CONTEXT_SIZE);

      /// The context passed in WFPSamplerCalloutDriver.sys is the SOCKADDR_STORAGE info of the 
      /// classified peer endpoint ...
      RtlCopyMemory(pProxyPeerSockAddrStorage,
                    pRedirectContext,
                    sizeof(SOCKADDR_STORAGE));

      /// ... and the SOCK_ADDR_STORAGE of the classified origin endpoint.
      if(pOriginSockAddrStorage)
      {
         UINT32 contextOriginOffset = sizeof(SOCKADDR_STORAGE);

         RtlCopyMemory(pOriginSockAddrStorage,
                       &(pRedirectContext[contextOriginOffset]),
                       sizeof(SOCKADDR_STORAGE));
      }
   }

   /// Attach the REDIRECT_RECORDS to the proxied socket
   status = WSAIoctl(proxySocket,
                     SIO_SET_WFP_CONNECTION_REDIRECT_RECORDS,
                     pRedirectRecords,
                     (DWORD)redirectRecordsSize,
                     0,
                     0,
                     (LPDWORD)&outputSize,
                     0,
                     0);
   if(status != NO_ERROR)
   {
      status = WSAGetLastError();

      HlprLogError(L"HlprWinSockAssociateConnectionRedirectRecordsAndContextWithProxySocket : WSAIoctl() [status: %#x]",
                   status);

      HLPR_BAIL;
   }

   HLPR_BAIL_LABEL:

   HLPR_DELETE_ARRAY(pRedirectContext);

   HLPR_DELETE_ARRAY(pRedirectRecords);

   return status;
}

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)

/**
 @helper_function="HlprWinSockBindToSocket"
 
   Purpose:  Bind a socket to a protocol, port, and network address.                            <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS737550.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprWinSockBindToSocket(_In_ SOCKET unboundSocket,
                               _In_ SOCKADDR_STORAGE* pSockAddrStorage)
{
   ASSERT(unboundSocket != INVALID_SOCKET);
   ASSERT(pSockAddrStorage);

   UINT32 status = NO_ERROR;

   status = bind(unboundSocket,
                 (SOCKADDR*)pSockAddrStorage,
                 sizeof(SOCKADDR_STORAGE));
   if(status != NO_ERROR)
   {
      status = WSAGetLastError();

      HlprLogError(L"HlprWinSockBindToSocket : bind() [status: %#x]",
                   status);
   }

   return status;
}

/**
 @helper_function="HlprWinSockListenOnSocket"
 
   Purpose:  Place the socket in a state in which it listens for incoming connections.          <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS739168.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprWinSockListenOnSocket(_In_ SOCKET boundSocket)
{
   ASSERT(boundSocket != INVALID_SOCKET);

   UINT32 status = NO_ERROR;

   status = listen(boundSocket,
                   SOMAXCONN);
   if(status != NO_ERROR)
   {
      status = WSAGetLastError();

      HlprLogError(L"HlprWinSockListenOnSocket : listen() [status: %#x]",
                   status);
   }

   return status;
}

/**
 @helper_function="HlprWinSockAcceptConnection"
 
   Purpose:  Place the socket in a state in which it listens for incoming connections.          <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS741513.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprWinSockAcceptConnection(_In_ SOCKET listeningSocket,
                                   _Inout_ SOCKET* pConnectedSocket,
                                   _Inout_ SOCKADDR_STORAGE* pPeerSockAddrStorage)
{
   ASSERT(listeningSocket != INVALID_SOCKET);
   ASSERT(pConnectedSocket);
   ASSERT(pPeerSockAddrStorage);

   UINT32 status        = NO_ERROR;
   UINT32 addressLength = sizeof(SOCKADDR_STORAGE);

   *pConnectedSocket = WSAAccept(listeningSocket,
                                 (SOCKADDR*)pPeerSockAddrStorage,
                                 (LPINT)&addressLength,
                                 0,
                                 0);
   if(*pConnectedSocket == INVALID_SOCKET)
   {
      status = WSAGetLastError();

      HlprLogError(L"HlprWinSockAcceptConnection : WSAAccept() [status: %#x]",
                   status);
   }

   return status;
}

/**
 @helper_function="HlprWinSockConnectSocket"
 
   Purpose:  Establish the TCP handshake with the remote peer.                                  <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS741559.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprWinSockConnectSocket(_In_ SOCKET unconnectedSocket,
                                _In_ SOCKADDR_STORAGE* pPeerSockAddrStorage)
{
   ASSERT(unconnectedSocket != INVALID_SOCKET);
   ASSERT(pPeerSockAddrStorage);

   UINT32 status = NO_ERROR;

   status = WSAConnect(unconnectedSocket,
                       (SOCKADDR*)pPeerSockAddrStorage,
                       sizeof(SOCKADDR_STORAGE),
                       0,
                       0,
                       0,
                       0);
   if(status != NO_ERROR)
   {
      status = WSAGetLastError();

      HlprLogError(L"HlprWinSockConnectSocket : WSAConnect() [status: %#x]",
                   status);
   }

   return status;
}

/**
 @helper_function="HlprWinSockReceiveTCPData"
 
   Purpose:  Receive TCP data from the peer.                                                    <br>
                                                                                                <br>
   Notes:    For use with TCP only.                                                             <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS741688.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprWinSockReceiveTCPData(_In_ SOCKET receivingSocket,
                                 _Inout_updates_(numDataBuffers) WSABUF* pDataBuffers,
                                 _Inout_ UINT32* pNumBytesReceived,
                                 _In_opt_ LPWSAOVERLAPPED pOverlapped,                      /* 0 */
                                 _In_opt_ LPWSAOVERLAPPED_COMPLETION_ROUTINE pCompletionFn, /* 0 */
                                 _In_ UINT32 numDataBuffers,                                /* 1 */
                                 _In_ UINT32 msgFlags)                                      /* 0 */
{
   ASSERT(receivingSocket != INVALID_SOCKET);
   ASSERT(pDataBuffers);
   ASSERT(pDataBuffers->len);
   ASSERT(pDataBuffers->buf);
   ASSERT(pNumBytesReceived);
   ASSERT(numDataBuffers);
   ASSERT((pNumBytesReceived &&
           pOverlapped == 0 &&
           pCompletionFn == 0) ||
           (pNumBytesReceived &&
           pOverlapped &&
           pCompletionFn));


   UINT32 status = NO_ERROR;
   UINT32 flags  = msgFlags;

   status = WSARecv(receivingSocket,
                    pDataBuffers,
                    numDataBuffers,
                    (LPDWORD)pNumBytesReceived,
                    (LPDWORD)&flags,
                    pOverlapped,
                    pCompletionFn);
   if(status != NO_ERROR)
   {
      status = WSAGetLastError();

      if(status == WSA_IO_PENDING &&
         pOverlapped &&
         pCompletionFn)
         status = NO_ERROR;
      else if(status != WSAEDISCON)
         HlprLogError(L"HlprWinSockReceiveData : WSARecv() [status: %#x]",
                      status);
   }

   return status;
}

/**
 @helper_function="HlprWinSockSendTCPData"
 
   Purpose:  Transmit TCP data to the peer.                                                     <br>
                                                                                                <br>
   Notes:    For use with TCP only.                                                             <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS742203.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprWinSockSendTCPData(_In_ SOCKET sendingSocket,
                              _In_reads_(numDataBuffers) WSABUF* pDataBuffers,
                              _Inout_opt_ UINT32* pNumBytesTransmitted,
                              _In_opt_ LPWSAOVERLAPPED pOverlapped,                      /* 0 */
                              _In_opt_ LPWSAOVERLAPPED_COMPLETION_ROUTINE pCompletionFn, /* 0 */
                              _In_ UINT32 numDataBuffers)                                /* 1 */
{
   ASSERT(sendingSocket != INVALID_SOCKET);
   ASSERT(pDataBuffers);
   ASSERT(pDataBuffers->len);
   ASSERT(pDataBuffers->buf);
   ASSERT(numDataBuffers);
   ASSERT((pNumBytesTransmitted &&
           pOverlapped == 0 &&
           pCompletionFn == 0) ||
           (pNumBytesTransmitted &&
           pOverlapped &&
           pCompletionFn));

   UINT32 status = NO_ERROR;

   status = WSASend(sendingSocket,
                    pDataBuffers,
                    numDataBuffers,
                    (LPDWORD)pNumBytesTransmitted,
                    0,
                    pOverlapped,
                    pCompletionFn);
   if(status != NO_ERROR)
   {
      status = WSAGetLastError();

      if(status == WSA_IO_PENDING &&
         pOverlapped &&
         pCompletionFn)
         status = NO_ERROR;
      else
         HlprLogError(L"HlprWinSockSendTCPData : WSASend() [status: %#x]",
                      status);
   }

   return status;
}
