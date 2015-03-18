/*++

Copyright (c) 1999 - 2002  Microsoft Corporation

Module Name:

    CtxStruct.h

Abstract:

    This is the header file defining the data structures used by the kernel mode
    filter driver implementing the context sample.


Environment:

    Kernel mode


--*/

//
//  Memory Pool Tags
//

#define CTX_STRING_TAG                        'tSxC'
#define CTX_RESOURCE_TAG                      'cRxC'
#define CTX_INSTANCE_CONTEXT_TAG              'cIxC'
#define CTX_FILE_CONTEXT_TAG                  'cFxC'
#define CTX_STREAM_CONTEXT_TAG                'cSxC'
#define CTX_STREAMHANDLE_CONTEXT_TAG          'cHxC'


//
//  Context sample filter global data
//

typedef struct _CTX_GLOBAL_DATA {

    //
    // Handle to minifilter returned from FltRegisterFilter()
    //

    PFLT_FILTER Filter;
    
#if DBG

    //
    // Field to control nature of debug output
    //
    
    ULONG DebugLevel;
#endif

} CTX_GLOBAL_DATA, *PCTX_GLOBAL_DATA;

extern CTX_GLOBAL_DATA Globals;




//
//  Instance context data structure
//

typedef struct _CTX_INSTANCE_CONTEXT {

    //
    //  Instance for this context.
    //

    PFLT_INSTANCE Instance;

    //
    //  Volume associated with this instance.
    //

    PFLT_VOLUME Volume;

    //
    //  Name of the volume associated with this instance.
    //

    UNICODE_STRING VolumeName;

} CTX_INSTANCE_CONTEXT, *PCTX_INSTANCE_CONTEXT;

#define CTX_INSTANCE_CONTEXT_SIZE         sizeof( CTX_INSTANCE_CONTEXT )


//
//  File context data structure
//

typedef struct _CTX_FILE_CONTEXT {

    //
    //  Name of the file associated with this context.
    //

    UNICODE_STRING FileName;

    //
    //  There is no resource to protect the context since the
    //  filename in the context is never modified. The filename 
    //  is put in when the context is created and then freed 
    //  with context is cleaned-up
    //
    
} CTX_FILE_CONTEXT, *PCTX_FILE_CONTEXT;

#define CTX_FILE_CONTEXT_SIZE         sizeof( CTX_FILE_CONTEXT )



//
//  Stream context data structure
//

typedef struct _CTX_STREAM_CONTEXT {

    //
    //  Name of the file associated with this context.
    //

    UNICODE_STRING FileName;

    //
    //  Number of times we saw a create on this stream
    //

    ULONG CreateCount;

    //
    //  Number of times we saw a cleanup on this stream
    //

    ULONG CleanupCount;

    //
    //  Number of times we saw a close on this stream
    //

    ULONG CloseCount;

    //
    //  Lock used to protect this context.
    //

    PERESOURCE Resource;

} CTX_STREAM_CONTEXT, *PCTX_STREAM_CONTEXT;

#define CTX_STREAM_CONTEXT_SIZE         sizeof( CTX_STREAM_CONTEXT )



//
//  Stream handle context data structure
//

typedef struct _CTX_STREAMHANDLE_CONTEXT {

    //
    //  Name of the file associated with this context.
    //

    UNICODE_STRING FileName;

    //
    //  Lock used to protect this context.
    //

    PERESOURCE Resource;

} CTX_STREAMHANDLE_CONTEXT, *PCTX_STREAMHANDLE_CONTEXT;

#define CTX_STREAMHANDLE_CONTEXT_SIZE         sizeof( CTX_STREAMHANDLE_CONTEXT )


//
//  Debug helper functions
//

#if DBG


#define DEBUG_TRACE_ERROR                               0x00000001  // Errors - whenever we return a failure code
#define DEBUG_TRACE_LOAD_UNLOAD                         0x00000002  // Loading/unloading of the filter
#define DEBUG_TRACE_INSTANCES                           0x00000004  // Attach / detatch of instances

#define DEBUG_TRACE_INSTANCE_CONTEXT_OPERATIONS         0x00000008  // Operation on instance context
#define DEBUG_TRACE_FILE_CONTEXT_OPERATIONS             0x00000010  // Operation on file context
#define DEBUG_TRACE_STREAM_CONTEXT_OPERATIONS           0x00000020  // Operation on stream context
#define DEBUG_TRACE_STREAMHANDLE_CONTEXT_OPERATIONS     0x00000040  // Operation on stream handle context

#define DEBUG_TRACE_ALL_IO                              0x00000080  // All IO operations tracked by this filter

#define DEBUG_TRACE_ALL                                 0xFFFFFFFF  // All flags


#define DebugTrace(Level, Data)                     \
    if ((Level) & Globals.DebugLevel) {               \
        DbgPrint Data;                              \
    }


#else

#define DebugTrace(Level, Data)             {NOTHING;}

#endif

