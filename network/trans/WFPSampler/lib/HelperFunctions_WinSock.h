////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_WinSock.h
//
//   Abstract:
//      This module contains prototypes of functions which assist in operations pertaining to 
//         Windows Sockets.
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

#ifndef HELPERFUNCTIONS_WINSOCK_H
#define HELPERFUNCTIONS_WINSOCK_H

_Success_(return == NO_ERROR)
UINT32 HlprWinSockCleanup();

_Success_(return == NO_ERROR)
UINT32 HlprWinSockInitialize();

_Success_(return == NO_ERROR)
UINT32 HlprWinSockDestroyWSAEvent(_Inout_ WSAEVENT* pWSAEvent);

_Success_(return == NO_ERROR)
UINT32 HlprWinSockSetWSAEvent(_In_ WSAEVENT wsaEvent);

_Success_(return == NO_ERROR)
UINT32 HlprWinSockCreateWSAEvent(_Inout_ WSAEVENT* pWSAEvent);

_Success_(return == NO_ERROR)
UINT32 HlprWinSockDestroySocket(_Inout_ SOCKET* pSocket);

_Success_(return == NO_ERROR)
UINT32 HlprWinSockCreateSocket(_In_ SOCKADDR_STORAGE* pSockAddr,
                               _In_ UINT8 protocol,
                               _Inout_ SOCKET* pSocket);

_Success_(return == NO_ERROR)
UINT32 HlprWinSockSetAbortiveDisconnect(_In_ SOCKET abortedSocket);

_Success_(return == NO_ERROR)
UINT32 HlprWinSockSetSocketReceiveTimeout(_In_ SOCKET receivingSocket,
                                          _In_ UINT32 timeout = 5000);

_Success_(return == NO_ERROR)
UINT32 HlprWinSockSetSocketSendTimeout(_In_ SOCKET sendingSocket,
                                       _In_ UINT32 timeout = 5000);

_Success_(return == NO_ERROR)
UINT32 HlprWinSockSetSocketNonBlocking(_In_ SOCKET unboundSocket);

_Success_(return == NO_ERROR)
UINT32 HlprWinSockAcquirePortReservationForSocket(_In_ SOCKET unboundSocket,
                                                  _In_ INET_PORT_RANGE* pPortRange,
                                                  _Inout_ UINT64* pReservationToken);

_Success_(return == NO_ERROR)
UINT32 HlprWinSockAssociatePortReservationWithSocket(_In_ SOCKET unboundSocket,
                                                     _In_ UINT64 reservationToken);

_Success_(return == NO_ERROR)
UINT32 HlprWinSockQueryPortReservation(_In_ UINT8 ipProtocol,
                                       _In_ INET_PORT_RANGE* pPortRange,
                                       _Inout_ UINT64* pReservationToken);

_Success_(return == NO_ERROR)
UINT32 HlprWinSockDestroyPortReservation(_In_ UINT8 ipProtocol,
                                         _In_ INET_PORT_RANGE* pPortRange,
                                         _Inout_opt_ UINT64* pReservationToken = 0);

_Success_(return == NO_ERROR)
UINT32 HlprWinSockCreatePortReservation(_In_ UINT8 ipProtocol,
                                        _In_ INET_PORT_RANGE* pPortRange,
                                        _Inout_ UINT64* pReservationToken);

#if(NTDDI_VERSION >= NTDDI_WIN8)

_Success_(return == NO_ERROR)
UINT32 HlprWinSockAssociateConnectionRedirectRecordsAndContextWithProxySocket(_In_ SOCKET originalSocket,
                                                                              _Inout_ SOCKET proxySocket,
                                                                              _Inout_ SOCKADDR_STORAGE* pRemoteSockAddrStorage,
                                                                              _Inout_opt_ SOCKADDR_STORAGE* pOriginSockAddrStorage = 0);

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)

_Success_(return == NO_ERROR)
UINT32 HlprWinSockBindToSocket(_In_ SOCKET unboundSocket,
                               _In_ SOCKADDR_STORAGE* pSockAddrStorage);

_Success_(return == NO_ERROR)
UINT32 HlprWinSockListenOnSocket(_In_ SOCKET boundSocket);

_Success_(return == NO_ERROR)
UINT32 HlprWinSockAcceptConnection(_In_ SOCKET listeningSocket,
                                   _Inout_ SOCKET* pConnectedSocket,
                                   _Inout_ SOCKADDR_STORAGE* pPeerSockAddrStorage);

_Success_(return == NO_ERROR)
UINT32 HlprWinSockConnectSocket(_In_ SOCKET unconnectedSocket,
                                _In_ SOCKADDR_STORAGE* pPeerSockAddrStorage);

_Success_(return == NO_ERROR)
UINT32 HlprWinSockReceiveTCPData(_In_ SOCKET receivingSocket,
                                 _Inout_updates_(numDataBuffers) WSABUF* pDataBuffers,
                                 _Inout_ UINT32* pNumBytesReceived,
                                 _In_opt_ LPWSAOVERLAPPED pOverlapped = 0,
                                 _In_opt_ LPWSAOVERLAPPED_COMPLETION_ROUTINE pCompletionFn = 0,
                                 _In_ UINT32 numDataBuffers = 1,
                                 _In_ UINT32 msgFlags = 0);

_Success_(return == NO_ERROR)
UINT32 HlprWinSockSendTCPData(_In_ SOCKET sendingSocket,
                              _In_reads_(numDataBuffers) WSABUF* pDataBuffers,
                              _Inout_opt_ UINT32* pNumBytesTransmitted,
                              _In_opt_ LPWSAOVERLAPPED pOverlapped = 0,
                              _In_opt_ LPWSAOVERLAPPED_COMPLETION_ROUTINE pCompletionFn = 0,
                              _In_ UINT32 numDataBuffers = 1);

#endif /// HELPERFUNCTIONS_WINSOCK_H
