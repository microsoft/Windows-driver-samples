/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    sockechoserver.h

Abstract:

    Header file for the socket  server module of the socketecho application 
    
Environment:

    User mode only

--*/

#pragma once


#define MAX_CONNECTIONS             5
#define DEFAULT_PORT_ADDRESS        6000
#define DATA_LENGTH                 1024*40
 
void 
SocketServerMain(
        _In_ unsigned short uPort
        );

    //
    //  Class definition for CEchoServer Class 
    //
class CEchoServer 
{

        public:

        SOCKET m_socket;
        HANDLE m_NetworkEvent;



        CEchoServer(SOCKET socketclient);
        void Start();
      
};

