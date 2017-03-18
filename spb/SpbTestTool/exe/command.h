/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name: 

    command.h

Abstract:

    This module contains the class declarations for the various
    SpbTestTool commands.

Environment:

    user-mode

Revision History:

--*/

typedef struct CCommand CCommand, *PCCommand;

pair<ULONG, PBYTE>
ParseBuffer(
    _In_ list<string>           *Parameters,
    _In_ list<string>::iterator  Start
    );

VOID
PrintCommands();

struct CCommand 
{
public:

    long Index;
    string Type;
    list<string> *Parameters;

    //
    // File handle this command is being run against (if any).
    //

    HANDLE File;

    //
    // Overlapped structure for this command to use.
    //

    OVERLAPPED Overlapped;

    //
    // Handle to the thread for this command (if run asynchronously) 
    // The thread is signalled when the command is complete.
    //

    HANDLE Thread;

    //
    // Common parameters.
    //

    string Address;
    PBYTE  Buffer;

    static long s_Index;

    CCommand(
        _In_                  string        Type,
        _In_ __drv_aliasesMem list<string> *Tokens
        ) : Type(Type),
            Thread(nullptr),
            Parameters(Tokens),
            Address(""),
            Buffer(nullptr),
            File(nullptr)
    {
        ZeroMemory(&Overlapped, sizeof(OVERLAPPED));
        Index = s_Index;
        s_Index += 1;
        return;
    }

    virtual
    ~CCommand(
        void
        )
    {
        delete[] Buffer;
        delete Parameters;
    }

    virtual
    bool
    Parse(
        void
        )
    {
        File = g_Peripheral;
        return true;
    }

    void
    DetachParameter(
        void
        )
    {
        Parameters = nullptr;
    }


    typedef 
    void
    (FN_PARSE)(
        _In_ list<string>::const_iterator &Iterator
        );

public:

    static
    PCCommand
    _ParseCommand(
        _In_ __drv_when(return != nullptr, __drv_aliasesMem) list<string> *Tokens
        );

public:

    VOID
    FakeCompletion(
        _In_    DWORD        Status,
        _In_    DWORD        Information
        )
    {
        Overlapped.Internal = Status == NO_ERROR ? Status : HRESULT_FROM_WIN32(Status);
        Overlapped.InternalHigh = Information;
        SetEvent(Overlapped.hEvent);
    }

    virtual
    bool
    Execute(
        VOID
        ) = 0;

    virtual
    void
    Complete(
        _In_ DWORD        Status,
        _In_ DWORD        Information
        )
    {
        printf("%s completed with status %u, information %u\n", 
               Type.c_str(),
               Status,
               Information);
    }

    virtual
    bool
    Cancel(
        VOID
        )
    {
        if (File != nullptr)
        {
            return CancelIoEx(File, &Overlapped) ? true : false;
        }
        else
        {
            return false;
        }
    }
};

class COpenCommand : public CCommand
{
private:

public:
    COpenCommand(
        _In_ __drv_aliasesMem list<string> *Parameters,
        _In_opt_              string        Tag
        ) : CCommand("open", Parameters)
    {
        return;
    }

    bool
    Execute(
        VOID
        );

    void
    Complete(
        _In_ DWORD        Status,
        _In_ DWORD        Information
        );
};

class CCloseCommand : public CCommand
{
private:

public:
    CCloseCommand(
        _In_ __drv_aliasesMem list<string> *Parameters,
        _In_opt_              string        Tag
        ) : CCommand("close", Parameters)
    {
        return;
    }

    bool
    Execute(
        VOID
        );

    void
    Complete(
        _In_ DWORD        Status,
        _In_ DWORD        Information
        );
};

class CLockCommand : public CCommand
{
private:

public:
    CLockCommand(
        _In_ __drv_aliasesMem list<string> *Parameters,
        _In_opt_              string        Tag
        ) : CCommand("lock", Parameters)
    {
        return;
    }

    bool
    Execute(
        VOID
        );

    void
    Complete(
        _In_ DWORD        Status,
        _In_ DWORD        Information
        );
};

class CUnlockCommand : public CCommand
{
private:

public:
    CUnlockCommand(
        _In_ __drv_aliasesMem list<string> *Parameters,
        _In_opt_              string        Tag
        ) : CCommand("unlock", Parameters)
    {
        return;
    }

    bool
    Execute(
        VOID
        );

    void
    Complete(
        _In_ DWORD        Status,
        _In_ DWORD        Information
        );
};

class CLockConnectionCommand : public CCommand
{
private:

public:
    CLockConnectionCommand(
        _In_ __drv_aliasesMem list<string> *Parameters,
        _In_opt_              string        Tag
        ) : CCommand("lockconn", Parameters)
    {
        return;
    }

    bool
    Execute(
        VOID
        );

    void
    Complete(
        _In_ DWORD        Status,
        _In_ DWORD        Information
        );
};

class CUnlockConnectionCommand : public CCommand
{
private:

public:
    CUnlockConnectionCommand(
        _In_ __drv_aliasesMem list<string> *Parameters,
        _In_opt_              string        Tag
        ) : CCommand("unlockconn", Parameters)
    {
        return;
    }

    bool
    Execute(
        VOID
        );

    void
    Complete(
        _In_ DWORD        Status,
        _In_ DWORD        Information
        );
};

class CReadCommand : public CCommand
{
private:
    ULONG  Length;

public:

    CReadCommand(
        _In_ __drv_aliasesMem list<string> *Parameters,
        _In_opt_              string        Tag
        ) : CCommand("read", Parameters)
    {
        return;
    }

    bool
    Parse(
        void
        )
    {
        if (CCommand::Parse() == false)
        {
            return false;
        }
            
        if (PopNumberParameter(Parameters, 10, &Length) == false)
        {
            printf("Length required\n");
            return false;
        }

        if (Length > 0)
        {
            Buffer = new BYTE[Length];
            ZeroMemory(Buffer, Length);
        }
        else 
        {
            Buffer = nullptr;
        }

        return true;
    }

    bool
    Execute(
        VOID
        );

    void
    Complete(
        _In_ DWORD        Status,
        _In_ DWORD        Information
        );
};


class CWriteCommand : public CCommand
{
private:
    ULONG  Length;

public:

    CWriteCommand(
        _In_ __drv_aliasesMem list<string> *Parameters,
        _In_opt_              string        Tag
        ) : CCommand("write", Parameters)
    {
        return;
    }

    bool
    Parse(
        VOID
        )
    {
        if (CCommand::Parse() == false)
        {
            return false;
        }
            
        pair<ULONG, PBYTE> buf;
       
        if (PopBufferParameter(Parameters, &buf) == false)
        {
            printf("Buffer required\n");
            return false;
        }

        Length = buf.first;
        Buffer = buf.second;
        
        return true;
    }

    bool
    Execute(
        VOID
        );

    void
    Complete(
        _In_ DWORD        Status,
        _In_ DWORD        Information
        );
};

class CWriteReadCommand : public CCommand
{
private:
    ULONG  WriteLength;
    ULONG  ReadLength;

    PBYTE  WriteBuffer;

public:

    CWriteReadCommand(
        _In_ __drv_aliasesMem list<string> *Parameters,
        _In_opt_              string        Tag
        ) : CCommand("writeread", Parameters)
    {
        WriteBuffer = NULL;
        return;
    }

    ~CWriteReadCommand(
        void
        )
    {
        delete[] WriteBuffer;
    }

    bool
    Parse(
        void
        )
    {
        if (CCommand::Parse() == false)
        {
            return false;
        }
            
        pair<ULONG, PBYTE> buf;
       
        if (PopBufferParameter(Parameters, &buf) == false)
        {
            printf("Buffer required\n");
            return false;
        }

        WriteLength = buf.first;
        WriteBuffer = buf.second;
        
        if (PopNumberParameter(Parameters, 10, &ReadLength) == false)
        {
            printf("Length required\n");
            return false;
        }

        if (ReadLength > 0)
        {
            Buffer = new BYTE[ReadLength];
            ZeroMemory(Buffer, ReadLength);
        }
        else 
        {
            Buffer = nullptr;
        }

        return true;
    }

    bool
    Execute(
        VOID
        );

    void
    Complete(
        _In_ DWORD        Status,
        _In_ DWORD        Information
        );
};

class CFullDuplexCommand : public CCommand
{
private:
    ULONG  WriteLength;
    ULONG  ReadLength;

    PBYTE  WriteBuffer;

public:

    CFullDuplexCommand(
        _In_ __drv_aliasesMem list<string> *Parameters,
        _In_opt_              string        Tag
        ) : CCommand("fullduplex", Parameters)
    {
        WriteBuffer = NULL;
        return;
    }

    ~CFullDuplexCommand(
        void
        )
    {
        delete[] WriteBuffer;
    }

    bool
    Parse(
        void
        )
    {
        if (CCommand::Parse() == false)
        {
            return false;
        }
            
        pair<ULONG, PBYTE> buf;
       
        if (PopBufferParameter(Parameters, &buf) == false)
        {
            printf("Buffer required\n");
            return false;
        }

        WriteLength = buf.first;
        WriteBuffer = buf.second;
        
        if (PopNumberParameter(Parameters, 10, &ReadLength) == false)
        {
            printf("Length required\n");
            return false;
        }

        if (ReadLength > 0)
        {
            Buffer = new BYTE[ReadLength];
            ZeroMemory(Buffer, ReadLength);
        }
        else 
        {
            Buffer = nullptr;
        }

        return true;
    }

    bool
    Execute(
        VOID
        );

    void
    Complete(
        _In_ DWORD        Status,
        _In_ DWORD        Information
        );
};

class CSignalInterruptCommand : public CCommand
{
private:

public:
    CSignalInterruptCommand(
        _In_ __drv_aliasesMem list<string> *Parameters,
        _In_opt_              string        Tag
        ) : CCommand("signal", Parameters)
    {
        return;
    }

    bool
    Execute(
        VOID
        );

    void
    Complete(
        _In_ DWORD        Status,
        _In_ DWORD        Information
        );
};

class CWaitOnInterruptCommand : public CCommand
{
private:

public:
    CWaitOnInterruptCommand(
        _In_ __drv_aliasesMem list<string> *Parameters,
        _In_opt_              string        Tag
        ) : CCommand("waitoninterrupt", Parameters)
    {
        return;
    }

    bool
    Execute(
        VOID
        );

    void
    Complete(
        _In_ DWORD        Status,
        _In_ DWORD        Information
        );
};
