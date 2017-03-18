/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name: 

    command.cpp

Abstract:

    This module contains the class definitions for the various
    SpbTestTool commands.

Environment:

    user-mode

Revision History:

--*/

#include "internal.h"

long CCommand::s_Index = 1;

VOID
PrintCommands()
{
    printf("\n");
    printf("Commands:\n");
    printf("  open                     open handle to SPB controller\n");
    printf("  close                    close handle to SPB controller\n");
    printf("  lock                     lock the bus for exclusive access\n");
    printf("  unlock                   unlock the bus\n");
    printf("  lockconn                 lock the shared connection for exclusive access -\n");
    printf("                           this primitive is used to synchronize with op-region\n");
    printf("                           accesses by firmware\n");
    printf("  unlockconn               unlock the shared connection\n");
    printf("  write {}                 write byte array to peripheral\n");
    printf("                            > write {01 02 03}\n");
    printf("  read <numBytes>          read <numBytes> from peripheral\n");
    printf("                            > read 5\n");
    printf("  writeread {} <numBytes>  atomically write byte array to peripheral\n");
    printf("                           and read <numBytes> back\n");
    printf("                            > writeread {01 02 03} 5\n");
    printf("  fullduplex {} <numBytes> simultaneously write byte array to peripheral\n");
    printf("                           and read <numBytes> back\n");
    printf("                            > full duplex {01 02 03} 5\n");
    printf("  signal                   inform the SpbTestTool driver that the\n");
    printf("                           interrupt has been handled\n");
    printf("  help                     print command list\n");
    printf("\n");
}

PCCommand
CCommand::_ParseCommand(
    _In_ __drv_when(return != nullptr, __drv_aliasesMem) list<string> *Parameters
    )
{
    string tag;
    string name;
    
    PCCommand command = nullptr;

    if (Parameters->front()[0] == L'@')
    {
        if (PopStringParameter(Parameters, &tag) == false)
        {
            printf("Error - could not pop tag\n");
            return nullptr;
        }
    }

    if (PopStringParameter(Parameters, &name) == false)
    {
        return nullptr;
    }

    if(_stricmp(name.c_str(), "open") == 0)
    {
        command = new COpenCommand(Parameters, tag);
    }
    else if(_stricmp(name.c_str(), "close") == 0)
    {
        command = new CCloseCommand(Parameters, tag);
    }
    else if(_stricmp(name.c_str(), "lock") == 0)
    {
        command = new CLockCommand(Parameters, tag);
    }
    else if(_stricmp(name.c_str(), "unlock") == 0)
    {
        command = new CUnlockCommand(Parameters, tag);
    }
    else if(_stricmp(name.c_str(), "lockconn") == 0)
    {
        command = new CLockConnectionCommand(Parameters, tag);
    }
    else if(_stricmp(name.c_str(), "unlockconn") == 0)
    {
        command = new CUnlockConnectionCommand(Parameters, tag);
    }
    else if(_stricmp(name.c_str(), "read") == 0)
    {
        command = new CReadCommand(Parameters, tag);
    }
    else if(_stricmp(name.c_str(), "write") == 0)
    {
        command = new CWriteCommand(Parameters, tag);
    }
    else if(_stricmp(name.c_str(), "writeread") == 0)
    {
        command = new CWriteReadCommand(Parameters, tag);
    }
    else if(_stricmp(name.c_str(), "fullduplex") == 0)
    {
        command = new CFullDuplexCommand(Parameters, tag);
    }
    else if(_stricmp(name.c_str(), "signal") == 0)
    {
        command = new CSignalInterruptCommand(Parameters, tag);
    }
    else if(_stricmp(name.c_str(), "help") == 0)
    {
        PrintCommands();
        return nullptr;
    }
    else
    {
        printf("unrecognized command %s\n", name.c_str());
        PrintCommands();
        return nullptr;
    }

    if (command->Parse() == false)
    {
        command->DetachParameter(); //avoid double deletion
        delete command;
        return nullptr;
    }

    return command;
}

bool
COpenCommand::Execute(
    VOID
    )
{
    //
    // Open peripheral target
    //

    ULONG bytesReturned;

    if (File == nullptr)
    {
        return false;
    }
    
    if ((DeviceIoControl(
        File, 
        IOCTL_SPBTESTTOOL_OPEN,
        nullptr,
        0,
        nullptr,
        0,
        &bytesReturned,
        &Overlapped) == TRUE) || 
        (GetLastError() != ERROR_IO_PENDING))
    {
        FakeCompletion(GetLastError(), bytesReturned);
    }

    return true;
}

void
COpenCommand::Complete(
    _In_ DWORD        Status,
    _In_ DWORD        /* Information */
    )
{
    if (Status != NO_ERROR)
    {
        printf("Error %u\n", GetLastError());
    }
    else
    {
        printf("Peripheral target opened\n");
    }
}

bool
CCloseCommand::Execute(
    VOID
    )
{
    //
    // Close peripheral target
    //

    ULONG bytesReturned;

    if (File == nullptr)
    {
        return false;
    }
    
    if ((DeviceIoControl(
        File, 
        IOCTL_SPBTESTTOOL_CLOSE,
        nullptr,
        0,
        nullptr,
        0,
        &bytesReturned,
        &Overlapped) == TRUE) || 
        (GetLastError() != ERROR_IO_PENDING))
    {
        FakeCompletion(GetLastError(), bytesReturned);
    }

    return true;
}

void
CCloseCommand::Complete(
    _In_ DWORD        Status,
    _In_ DWORD        /* Information */
    )
{
    if (Status != NO_ERROR)
    {
        printf("Error %u\n", GetLastError());
    }
    else
    {
        printf("Peripheral target closed\n");
    }
}

bool
CLockCommand::Execute(
    VOID
    )
{
    //
    // Lock controller for peripheral target
    //

    ULONG bytesReturned;

    if (File == nullptr)
    {
        return false;
    }
    
    if ((DeviceIoControl(
        File, 
        IOCTL_SPBTESTTOOL_LOCK,
        nullptr,
        0,
        nullptr,
        0,
        &bytesReturned,
        &Overlapped) == TRUE) || 
        (GetLastError() != ERROR_IO_PENDING))
    {
        FakeCompletion(GetLastError(), bytesReturned);
    }

    return true;
}

void
CLockCommand::Complete(
    _In_ DWORD        Status,
    _In_ DWORD        /* Information */
    )
{
    if (Status != NO_ERROR)
    {
        printf("Error %u\n", GetLastError());
    }
    else
    {
        printf("Controller locked\n");
    }
}

bool
CUnlockCommand::Execute(
    VOID
    )
{
    //
    // Unlock controller
    //

    ULONG bytesReturned;

    if (File == nullptr)
    {
        return false;
    }
    
    if ((DeviceIoControl(
        File, 
        IOCTL_SPBTESTTOOL_UNLOCK,
        nullptr,
        0,
        nullptr,
        0,
        &bytesReturned,
        &Overlapped) == TRUE) || 
        (GetLastError() != ERROR_IO_PENDING))
    {
        FakeCompletion(GetLastError(), bytesReturned);
    }

    return true;
}

void
CUnlockCommand::Complete(
    _In_ DWORD        Status,
    _In_ DWORD        /* Information */
    )
{
    if (Status != NO_ERROR)
    {
        printf("Error %u\n", GetLastError());
    }
    else
    {
        printf("Controller unlocked\n");
    }
}

bool
CLockConnectionCommand::Execute(
    VOID
    )
{
    //
    // Lock connection for peripheral target
    //

    ULONG bytesReturned;

    if (File == nullptr)
    {
        return false;
    }
    
    if ((DeviceIoControl(
        File, 
        IOCTL_SPBTESTTOOL_LOCK_CONNECTION,
        nullptr,
        0,
        nullptr,
        0,
        &bytesReturned,
        &Overlapped) == TRUE) || 
        (GetLastError() != ERROR_IO_PENDING))
    {
        FakeCompletion(GetLastError(), bytesReturned);
    }

    return true;
}

void
CLockConnectionCommand::Complete(
    _In_ DWORD        Status,
    _In_ DWORD        /* Information */
    )
{
    if (Status != NO_ERROR)
    {
        printf("Error %u\n", GetLastError());
    }
    else
    {
        printf("Peripheral target locked\n");
    }
}

bool
CUnlockConnectionCommand::Execute(
    VOID
    )
{
    //
    // Unlock connection
    //

    ULONG bytesReturned;

    if (File == nullptr)
    {
        return false;
    }
    
    if ((DeviceIoControl(
        File, 
        IOCTL_SPBTESTTOOL_UNLOCK_CONNECTION,
        nullptr,
        0,
        nullptr,
        0,
        &bytesReturned,
        &Overlapped) == TRUE) || 
        (GetLastError() != ERROR_IO_PENDING))
    {
        FakeCompletion(GetLastError(), bytesReturned);
    }

    return true;
}

void
CUnlockConnectionCommand::Complete(
    _In_ DWORD        Status,
    _In_ DWORD        /* Information */
    )
{
    if (Status != NO_ERROR)
    {
        printf("Error %u\n", GetLastError());
    }
    else
    {
        printf("Peripheral target unlocked\n");
    }
}

bool
CReadCommand::Execute(
    VOID
    )
{
    ULONG bytesRead;

    if (File == nullptr)
    {
        return false;
    }

    if ((ReadFile(File, 
                  Buffer,
                  Length,
                  &bytesRead,
                  &Overlapped) == TRUE) || 
        (GetLastError() != ERROR_IO_PENDING))
    {
        FakeCompletion(GetLastError(), bytesRead);
    }

    return true;
}

void
CReadCommand::Complete(
    _In_ DWORD        Status,
    _In_ DWORD        Information
    )
{
    if (Status != NO_ERROR)
    {
        printf("Error %u\n", Status);
    }
    else
    {
        printf("%u bytes read\n", Information);
        PrintBytes(Information, Buffer);
    }
}

bool
CWriteCommand::Execute(
    VOID
    )
{
    ULONG bytesWritten;

    if (File == nullptr)
    {
        return false;
    }

    if ((WriteFile(File, 
                   Buffer,
                   Length,
                   &bytesWritten,
                   &Overlapped) == TRUE) || 
        (GetLastError() != ERROR_IO_PENDING))
    {
        FakeCompletion(GetLastError(), bytesWritten);
    }

    return true;
}


void
CWriteCommand::Complete(
    _In_ DWORD        Status,
    _In_ DWORD        Information
    )
{
    if (Status != NO_ERROR)
    {
        printf("Error %u\n", Status);
    }
    else
    {
        printf("%u bytes written\n", Information);
    }
}

bool
CWriteReadCommand::Execute(
    VOID
    )
{
    ULONG bytesTransferred;

    if (File == nullptr)
    {
        return false;
    }

    if ((DeviceIoControl(File, 
                         IOCTL_SPBTESTTOOL_WRITEREAD,
                         WriteBuffer,
                         WriteLength,
                         Buffer,
                         ReadLength,
                         &bytesTransferred,
                         &Overlapped) == TRUE) || 
        (GetLastError() != ERROR_IO_PENDING))
    {
        FakeCompletion(GetLastError(), bytesTransferred);
    }

    return true;
}

void
CWriteReadCommand::Complete(
    _In_ DWORD        Status,
    _In_ DWORD        Information
    )
{
    if (Status != NO_ERROR)
    {
        printf("Error %u\n", GetLastError());
    }
    else
    {
        printf("%u bytes returned\n", Information);
        PrintBytes(Information, Buffer);
    }
}

bool
CFullDuplexCommand::Execute(
    VOID
    )
{
    ULONG bytesTransferred;

    if (File == nullptr)
    {
        return false;
    }

    if ((DeviceIoControl(File, 
                         IOCTL_SPBTESTTOOL_FULL_DUPLEX,
                         WriteBuffer,
                         WriteLength,
                         Buffer,
                         ReadLength,
                         &bytesTransferred,
                         &Overlapped) == TRUE) || 
        (GetLastError() != ERROR_IO_PENDING))
    {
        FakeCompletion(GetLastError(), bytesTransferred);
    }

    return true;
}

void
CFullDuplexCommand::Complete(
    _In_ DWORD        Status,
    _In_ DWORD        Information
    )
{
    if (Status != NO_ERROR)
    {
        printf("Error %u\n", GetLastError());
    }
    else
    {
        printf("%u bytes returned\n", Information);
        PrintBytes(Information, Buffer);
    }
}

bool
CSignalInterruptCommand::Execute(
    VOID
    )
{
    //
    // Signal interrupt
    //

    ULONG bytesReturned;

    if (File == nullptr)
    {
        return false;
    }
    
    if ((DeviceIoControl(
        File, 
        IOCTL_SPBTESTTOOL_SIGNAL_INTERRUPT,
        nullptr,
        0,
        nullptr,
        0,
        &bytesReturned,
        &Overlapped) == TRUE) || 
        (GetLastError() != ERROR_IO_PENDING))
    {
        FakeCompletion(GetLastError(), bytesReturned);
    }

    return true;
}

void
CSignalInterruptCommand::Complete(
    _In_ DWORD        Status,
    _In_ DWORD        /* Information */
    )
{
    if (Status != NO_ERROR)
    {
        printf("Error %u\n", GetLastError());
    }
    else
    {
        printf("Interrupt signalled\n");
    }
}
