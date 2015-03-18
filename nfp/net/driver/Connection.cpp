/*++

Copyright (c) Microsoft Corporation.  All rights reserved.
      
Author:

    Travis Martin (TravM) 06-24-2010
    
--*/
#include "internal.h"

#include "Connection.tmh"

HRESULT SetSocketIpv6Only(_In_ SOCKET socket, _In_ BOOL Ipv6Only);

HRESULT SynchronousReadSocket(_In_ SOCKET Socket, _In_reads_bytes_(cbBuffer) PVOID pBuffer, _In_ DWORD cbBuffer)
{
    HRESULT hr = S_OK;
    DWORD dwIgnore;
    OVERLAPPED Overlapped;
    ZeroMemory(&Overlapped, sizeof(Overlapped));
    if (!ReadFile((HANDLE)Socket, pBuffer, cbBuffer, &dwIgnore, &Overlapped))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        if (hr == HRESULT_FROM_WIN32(ERROR_IO_PENDING))
        {
            if (!GetOverlappedResult((HANDLE)Socket, &Overlapped, &dwIgnore, TRUE))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
            else
            {
                hr = S_OK;
            }
        }
    }
    return hr;
}

//CConnection
// static
HRESULT CConnection::Create(_In_ IConnectionCallback* pCallback, _Outptr_ CConnection** ppConnection)
{
    CConnection* pConnection;
    pConnection = new CConnection(pCallback);
    HRESULT hr = (pConnection != NULL ? S_OK : E_OUTOFMEMORY);
    if (SUCCEEDED(hr))
    {
        *ppConnection = pConnection;
    }

    return hr;
}

void CConnection::Terminate()
{
    MethodEntry("void");

    // Only want to terminate once
    STATE PriorState = (STATE)(InterlockedExchange((long*)&_State, (long)TERMINATED));
    if (PriorState != TERMINATED)
    {
        // Graceful shutdown
        shutdown(_Socket, SD_SEND);
        
        // Don't wait for threadpool callbacks when this thread is actually the threadpool callback
        if (_ThreadpoolThreadId != GetCurrentThreadId())
        {
            // Let the ReceiveThreadProc gracefully shutdown
            WaitForThreadpoolWorkCallbacks(_ThreadpoolWork, false);
        }
        
        SOCKET Socket = (SOCKET)InterlockedExchangePointer((PVOID*)&_Socket, (PVOID)INVALID_SOCKET);
        if (Socket != INVALID_SOCKET)
        {
            closesocket(Socket);
        }
    }

    MethodReturnVoid();
}

/* 9C7D2C68-5AD8-4A14-BE20-F8741D60D100 */
const GUID MAGIC_PACKET = 
  {0x9C7D2C68, 0x5AD8, 0x4A14, {0xBE, 0x20, 0xF8, 0x74, 0x1D, 0x60, 0xD1, 0x00}};

HRESULT CConnection::InitializeAsClient(_In_ BEGIN_PROXIMITY_ARGS* pArgs)
{
    pArgs->szName[MAX_PATH-1] = L'\0';
    
    MethodEntry("pArgs->szName = '%S'",
                 pArgs->szName);

    // Open a TCP/IP Socket to the remote Network NearFieldProximity device
    HRESULT hr = S_OK;

    _Socket = socket(AF_INET6, SOCK_STREAM, 0);
    if (_Socket == INVALID_SOCKET)
    {
        hr = HRESULT_FROM_WIN32(WSAGetLastError());
    }
    
    if (SUCCEEDED(hr))
    {
        hr = SetSocketIpv6Only(_Socket, FALSE);
    }
    
    if (SUCCEEDED(hr))
    {
        SOCKADDR_STORAGE LocalAddress = {};
        SOCKADDR_STORAGE RemoteAddress = {};
        DWORD cbLocalAddress = sizeof(LocalAddress);
        DWORD cbRemoteAddress = sizeof(RemoteAddress);
        timeval Timeout = {8, 0};
        if (!WSAConnectByName(_Socket, pArgs->szName, L"9299", 
                              &cbLocalAddress, (SOCKADDR*)&LocalAddress, 
                              &cbRemoteAddress, (SOCKADDR*)&RemoteAddress, &Timeout, NULL))
        {
            hr = HRESULT_FROM_WIN32(WSAGetLastError());
            TraceErrorHR(hr, L"WSAConnectByName FAILED");
        }
    }

    if (SUCCEEDED(hr))
    {
        if (setsockopt(_Socket, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0) == SOCKET_ERROR)
        {
            hr = HRESULT_FROM_WIN32(WSAGetLastError());
        }
    }

    if (SUCCEEDED(hr))
    {
        // Send the Magic Packet
        if (send(_Socket, (char*)&MAGIC_PACKET, sizeof(MAGIC_PACKET), 0) == SOCKET_ERROR)
        {
            hr = HRESULT_FROM_WIN32(WSAGetLastError());
        }
        
        if (SUCCEEDED(hr))
        {
            GUID MagicPacket = {};
            hr = SynchronousReadSocket(_Socket, &MagicPacket, sizeof(MagicPacket));
            if (SUCCEEDED(hr))
            {
                if (memcmp(&MagicPacket, &MAGIC_PACKET, sizeof(MAGIC_PACKET)) != 0)
                {
                    hr = E_FAIL;
                }
            }
        }
        
        if (SUCCEEDED(hr))
        {
            // This doesn't take the socket, because we're the client and already
            // have the socket in _Socket.
            hr = FinalizeEstablish(INVALID_SOCKET);
        }
    }
    
    
    if (FAILED(hr))
    {
        if (_Socket != INVALID_SOCKET)
        {
            // Abortive shutdown of the socket
            closesocket(_Socket);
            _Socket = INVALID_SOCKET;
        }
    }

    MethodReturnHR(hr);
}

void CConnection::ValidateAccept(_In_ SOCKET Socket, _In_ GUID* pMagicPacket)
{
    MethodEntry("...");
    
    TraceASSERT(Socket != INVALID_SOCKET);
    
    HRESULT hr = S_OK;
    if (memcmp(pMagicPacket, &MAGIC_PACKET, sizeof(MAGIC_PACKET)) != 0)
    {
        hr = E_FAIL;
    }
    
    if (SUCCEEDED(hr))
    {
        // Send the MAGIC_PACKET
        if (send(Socket, (char*)&MAGIC_PACKET, sizeof(MAGIC_PACKET), 0) == SOCKET_ERROR)
        {
            hr = HRESULT_FROM_WIN32(WSAGetLastError());
        }
    }
    
    if (SUCCEEDED(hr))
    {
        hr = FinalizeEstablish(Socket);
    }
    
    if (FAILED(hr))
    {
        // Abortive shutdown of the socket
        closesocket(Socket);
    }
    
    MethodReturnVoid();
}

HRESULT CConnection::FinalizeEstablish(_In_ SOCKET Socket)
{
    MethodEntry("...");

    HRESULT hr = S_OK;
    STATE PriorState = (STATE)(InterlockedCompareExchange((long*)&_State, (long)ESTABLISHED, (long)INITIAL));
    if (PriorState != INITIAL)
    {
        // Already established (or terminated), drop this
        hr = HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
    }
    
    if (SUCCEEDED(hr))
    {
        // Init Threadpool work item for the receieve thread proc.
        _ThreadpoolWork = CreateThreadpoolWork(s_ReceiveThreadProc, this, NULL);
        if (_ThreadpoolWork == NULL)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        else
        {
            if (Socket != INVALID_SOCKET)
            {
                // Take ownership of the socket
                _Socket = Socket;
            }
            SubmitThreadpoolWork(_ThreadpoolWork);
            
            _pCallback->ConnectionEstablished(this);
        }
    }
    
    MethodReturnHR(hr);
}

BOOL CConnection::ReceiveThreadProc()
{
    MethodEntry("void");

    MESSAGE* pMessage = new MESSAGE();
    if (pMessage == NULL)
    {
        Terminate();
    }
    else
    {
        while (_Socket != INVALID_SOCKET)
        {
            if (recv(_Socket, (char*)pMessage, sizeof(*pMessage), MSG_WAITALL) == sizeof(*pMessage))
            {
                _pCallback->HandleReceivedMessage(pMessage);
            }
            else
            {
                Terminate();
                break;
            }
        }
        delete pMessage;
    }

    // The connection is now terminated
    BOOL fConnectionDeleted = _pCallback->ConnectionTerminated(this);
    
    MethodReturnBool(fConnectionDeleted);
}

HRESULT CConnection::TransmitMessage(_In_ MESSAGE* pMessage)
{
    HRESULT hr = S_OK;
    if (_Socket == INVALID_SOCKET)
    {
        hr = HRESULT_FROM_WIN32(WSAENOTSOCK);
    }
    else
    {
        if (send(_Socket, (char*)pMessage, sizeof(*pMessage), 0) == SOCKET_ERROR)
        {
            hr = HRESULT_FROM_WIN32(WSAGetLastError());
            Terminate();
        }
    }
    
    return hr;
}

