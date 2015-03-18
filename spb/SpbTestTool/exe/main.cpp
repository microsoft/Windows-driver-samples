/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name: 

    main.cpp

Abstract:

    This module contains the main entry point for the SpbTestTool
    app.  It facilitates accepting input, building commands, and
    printing output.

Environment:

    user-mode

Revision History:

--*/

#include "internal.h"

DWORD WINAPI InterruptNotificationThread(
    _In_ LPVOID pvData
    );

bool
ReadCommandFromStream(
    _In_    FILE*         InputStream,
    _Inout_ list<string> *Tokens
    );

VOID
PrintUsage(
    _In_ PCSTR exeName
    );

DWORD
RunCommand(
    _In_ PCCommand Command
    );

HANDLE g_Peripheral = nullptr;
HANDLE g_Event = nullptr;
HANDLE g_InterruptNotificationThread = nullptr;
PCCommand g_CurrentCommand = nullptr;
bool g_WaitOnInterrupt = true;

BOOL
WINAPI
OnControlKey(
    _In_ DWORD ControlType
    );

void 
__cdecl 
main(
    _In_                    ULONG ArgumentsCe,
    _In_reads_(ArgumentsCe) PCSTR Arguments[]
    )
{
    FILE* inputStream = stdin;
    bool prompt = true;

    PCSTR peripheralPath = nullptr;
    PCSTR inputPath = nullptr;

    //
    // Parse the command line arguments.
    //

    ULONG arg = 1;

    while (arg < ArgumentsCe)
    {
        if ((Arguments[arg][0] != '/') &&
            (Arguments[arg][0] != '-'))
        {
            PrintUsage(Arguments[0]);
            goto exit;
        }
        else
        {
            if (tolower(Arguments[arg][1]) == 'p')
            {
                arg++;
                if (arg == ArgumentsCe)
                {
                    PrintUsage(Arguments[0]);
                    goto exit;
                }
                peripheralPath = Arguments[arg];
            }
            else if (tolower(Arguments[arg][1]) == 'i')
            {
                arg++;
                if (arg == ArgumentsCe)
                {
                    PrintUsage(Arguments[0]);
                    goto exit;
                }
                inputPath = Arguments[arg];
            }
            else
            {
                PrintUsage(Arguments[0]);
                goto exit;
            }
        }

        arg++;
    }

    //
    // Open the input file if specified
    //

    if (inputPath != nullptr)
    {
        errno_t error;

        printf("Opening %s as command input file\n", inputPath);
        error = fopen_s(&inputStream, inputPath, "r");

        if (error != 0)
        {
            printf("Error opening input file %s - %d\n", inputPath, error);
            goto exit;
        }

        if (inputStream == nullptr)
        {
            printf("Error opening input file %s - %d\n", inputPath, error);
            goto exit;
        }

        prompt = false;
    }

    //
    // Open peripheral driver
    //

    if (peripheralPath == nullptr)
    {
        g_Peripheral = CreateFileW(
            SPBTESTTOOL_USERMODE_PATH,
            (GENERIC_READ | GENERIC_WRITE),
            0,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_OVERLAPPED,
            nullptr);
    }
    else
    {
        printf("Opening %s as peripheral driver path \n", peripheralPath);
        g_Peripheral = CreateFileA(
            peripheralPath,
            (GENERIC_READ | GENERIC_WRITE),
            0,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_OVERLAPPED,
            nullptr);
    }

    if (g_Peripheral == INVALID_HANDLE_VALUE)
    {
        printf("Error opening peripheral driver - %u\n", GetLastError());
        goto exit;
    }

    setvbuf(inputStream, nullptr, _IONBF, 0);
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);

    //
    // Setup a control-C handler.
    //

    if (SetConsoleCtrlHandler(OnControlKey, true) == FALSE)
    {
        printf("Error setting ctrl-C handler - %u\n", GetLastError());
        goto exit;
    }

    //
    // Setup an overlapped structure to use with each I/O.
    //

    g_Event = CreateEvent(nullptr, true, false, nullptr); 
    
    if (g_Event == nullptr)
    {
        printf("error creating I/O event - %u\n", GetLastError());
        goto exit;
    }

    //
    // Create interrupt notification thread.
    //

    g_InterruptNotificationThread = CreateThread(
        nullptr,
        0,
        &InterruptNotificationThread,
        nullptr,
        0,
        nullptr);
    
    if (g_InterruptNotificationThread == nullptr)
    {
        printf("error creating interrupt notification thread - %u\n", GetLastError());
        goto exit;
    }

    //
    // Loop reading commands off the command line and parsing them.
    // EOF causes an exit.
    //

    do 
    {
        list<string> *tokens = new list<string>();
        PCCommand command;

        if (prompt)
        {
            printf("> ");
            fflush(stdout);
        }

        if (ReadCommandFromStream(inputStream, tokens) == false)
        {
            delete tokens;
            break;
        }

        if (tokens->empty())
        {
            delete tokens;
            continue;
        }

        command = CCommand::_ParseCommand(tokens);

        if (command == nullptr) 
        {
            delete tokens;
            continue;
        }

        g_CurrentCommand = command;
            
        RunCommand(command);

        g_CurrentCommand = nullptr;
            
        //
        // A reference to the tokens list is saved by
        // the command and will be freed when the command
        // is deleted.
        //
        
        delete command;

    }
    while (feof(inputStream) == 0);

exit:

    if (prompt == false)
    {
        fclose(inputStream);
    }

    if (g_InterruptNotificationThread != nullptr)
    {
        WaitForSingleObject(g_InterruptNotificationThread, INFINITE);    
        CloseHandle(g_InterruptNotificationThread);
    }

    CloseHandle(g_Peripheral);

    if (g_Event != nullptr)
    {
        CloseHandle(g_Event);
    }

    return;
}

DWORD WINAPI InterruptNotificationThread(
    _In_ LPVOID pvData
    )
{
    DWORD status;
    DWORD bytesReturned;
    OVERLAPPED ov = {0};

    UNREFERENCED_PARAMETER(pvData);

    ov.hEvent = CreateEvent(nullptr, false, false, nullptr); 
    
    if (ov.hEvent == nullptr)
    {
        printf("error creating overlapped event for interrupt thread - %u\n", GetLastError());
        goto exit;
    }

    while (g_WaitOnInterrupt == true)
    {
        if ((DeviceIoControl(
            g_Peripheral, 
            IOCTL_SPBTESTTOOL_WAIT_ON_INTERRUPT,
            nullptr,
            0,
            nullptr,
            0,
            nullptr,
            &ov) == TRUE) || 
            (GetLastError() != ERROR_IO_PENDING))
        {
            printf("failed to pend WaitOnInterrupt IOCTL- %u\n", GetLastError());
            goto exit;
        }

        status = WaitForSingleObject(ov.hEvent, INFINITE);

        switch (status)
        {
            // DeviceIoControl completed.
            case WAIT_OBJECT_0:

                if (!GetOverlappedResult(g_Peripheral, &ov, &bytesReturned, FALSE))
                {
                    printf("GetOverlappedResult failed with status: %u\n\n", GetLastError());
                }
                else
                {
                    printf("\n\n");
                    printf("  **  Interrupt detected. Please acknowledge or disable   **\n");
                    printf(" ***  and type 'signal' to inform the SpbTestTool driver  ***\n");
                    printf("  **  that the interrupt has been handled.                **\n");
                    printf("\n");
                }
                break;

            default:
                // Error in the WaitForSingleObject; abort.
                // This indicates a problem with the OVERLAPPED 
                // structure's event handle.
                printf("WaitOnInterrupt unexpected return %u\n\n", status);
                break;
        }
    }

exit:

    return 0;
}

VOID
PrintUsage(
    _In_ PCSTR exeName
    )
{
    printf("Usage: %s [/p <driver_path>] [/i <script_name>]\n",
        exeName);
}

DWORD
RunCommand(
    _In_ PCCommand Command
    )
{

    DWORD status;
    DWORD bytesTransferred;

    Command->Overlapped.hEvent = g_Event;
    
    if (Command->Execute() == true)
    {
        if (GetOverlappedResult(Command->File, 
                                &(Command->Overlapped),
                                &bytesTransferred,
                                true) == FALSE)
        {
            status = GetLastError();
        }
        else
        {
            status = NO_ERROR;
        }

        Command->Complete(status, bytesTransferred);
    }
    else
    {
        status = GetLastError();
    }

    return status;
}


bool
ReadLine(
    _In_                    FILE* InputStream,
    _In_                    ULONG BufferCch,
    _Out_writes_(BufferCch) CHAR  Buffer[]
    )
{
    ULONG i;

    for(i = 0; i < BufferCch - 1; i += 1)
    {
        if (fread(&(Buffer[i]), sizeof(CHAR), 1, InputStream) == 0)
        {
           return false;
        } 

        fputc(Buffer[i], stdout);

        if (Buffer[i] == '\n')
        {
            Buffer[i] = '\0';
            return true;
        }
        else if (Buffer[i] == '\b')
        {
            Buffer[i] = '\0';
            i -= 2;
        }
    }

    return false;
}

bool
ReadCommandFromStream(
    _In_    FILE*         InputStream,
    _Inout_ list<string> *Tokens
    )
{   
    CHAR buffer[255];

    if (ReadLine(InputStream, 
                 sizeof(buffer) - 1,
                 buffer) == false)
    {
        return false;
    }

    string currentLine(buffer);
    string token;

    list<string> tokens;

    Tokens->clear();

    string::size_type i;
    long start;

    for(i = 0, start = -1; 
        i < currentLine.length() + 1; 
        i += 1)
    {
        wchar_t c;
        
        if (i < currentLine.length()) 
        {
            c = currentLine[i];
        }
        else
        {
            c = L'\0';
        }

        if (iswspace(c) || (c == L'\0'))
        {
            if (start == -1)
            {
                //
                // not tracking a token - whitespace is skipped.
                //
            }
            else
            {
                //
                // tracking a token - whitespace or NUL terminates it.
                // if the last character was a } then split the token.
                //

                if (currentLine[i-1] == '}')
                {
                    token.assign(currentLine, start, i - start - 1);
                    Tokens->insert(Tokens->end(), token);
                    Tokens->insert(Tokens->end(), string("}"));
                }
                else
                {
                    token.assign(currentLine, start, i - start);
                    Tokens->insert(Tokens->end(), token);
                }

                start = -1;
            }
        }
        else if (start == -1)
        {
            //
            // first character of a token.  If it's { then split the token.
            //

            if (currentLine[i] == '{')
            {
                Tokens->insert(Tokens->end(), string("{"));
            }
            else if (currentLine[i] == '}')
            {
                Tokens->insert(Tokens->end(), string("}"));
            }
            else
            {
                start = (long) i;
            }
        }
        else 
        {
            //
            // tracking a token - non whitespace is included.
            //
        }
    }

    return true;
}

VOID
PrintBytes(
    _In_                  ULONG BufferCb,
    _In_reads_bytes_(BufferCb) BYTE  Buffer[]
    )
{
    ULONG index = 0;

    for(index = 0; index < BufferCb; index += 16)
    {
        printf("  ");
        for(ULONG i = index; i < (index + 16); i += 1)
        {
            if (i < BufferCb)
            {
                if ((i != index) && (i % 8 == 0))
                {
                    printf("- ");
                }

                printf("%02x ", Buffer[i]);
            }
            else 
            {
                if ((i != index) && (i % 8 == 0))
                {
                    printf("  ");
                }
                printf("   ");
            }
        }

        printf(" : ");

        for(ULONG i = index; i < (index + 16); i += 1)
        {
            if (i < BufferCb)
            {
                if ((i != index) && (i % 8 == 0))
                {
                    printf(" ");
                }

                if (isprint(Buffer[i]))
                {
                    printf("%c", Buffer[i]);
                }
                else
                {
                    printf(".");
                }
            }
            else 
            {
                printf(" ");
            }
        }

        printf("\n");
    }
}

BOOL
WINAPI
OnControlKey(
    _In_ DWORD ControlType
    )
{
    if (ControlType == CTRL_C_EVENT)
    {
        //
        // If there's a current command then attempt to cancel it.
        //

        if (g_CurrentCommand != nullptr)
        {
            g_CurrentCommand->Cancel();
            return TRUE;
        }

        //
        // Stop the interrupt notification thread
        //

        g_WaitOnInterrupt = false;
        CancelIo(g_Peripheral);
    }

    return FALSE;
}
