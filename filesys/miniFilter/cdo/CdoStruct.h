/*++

Copyright (c) 1999 - 2003  Microsoft Corporation

Module Name:

    CdoStruct.h 

Abstract:

    This is the header file defining the data structures used by the kernel mode
    filter driver implementing the control device object sample.


Environment:

    Kernel mode


--*/

//
//  CDO sample filter global data
//

//
//  GLOBAL_DATA_F_xxx flags
//

//
//  Indicates that there is a open reference to the CDO
//
#define GLOBAL_DATA_F_CDO_OPEN_REF      0x00000001 

//
//  Indicates that there is a open handle to the CDO
//

#define GLOBAL_DATA_F_CDO_OPEN_HANDLE   0x00000002

//
//  Globals
//

typedef struct _CDO_GLOBAL_DATA {

    //
    //  Handle to minifilter returned from FltRegisterFilter()
    //

    PFLT_FILTER Filter;

    //
    //  Driver object for this filter
    //
    
    PDRIVER_OBJECT FilterDriverObject;

    //
    //  Control Device Object for this filter
    //
    
    PDEVICE_OBJECT FilterControlDeviceObject;

    //
    //  Flags - GLOBAL_DATA_F_xxx
    //

    ULONG Flags;

    //
    //  Resource to synchronize access to flags
    //

    ERESOURCE Resource;
    
#if DBG

    //
    // Field to control nature of debug output
    //
    
    ULONG DebugLevel;
#endif

} CDO_GLOBAL_DATA, *PCDO_GLOBAL_DATA;

extern CDO_GLOBAL_DATA Globals;

//
//  The name of the CDO created by this filter
//

#define CONTROL_DEVICE_OBJECT_NAME               L"\\FileSystem\\Filters\\CdoSample"

//
//  Macro to test if this is my control device object
//

#define IS_MY_CONTROL_DEVICE_OBJECT(_devObj) \
    (((_devObj) == Globals.FilterControlDeviceObject) ? \
            (FLT_ASSERT(((_devObj)->DriverObject == Globals.FilterDriverObject) && \
                        ((_devObj)->DeviceExtension == NULL)), TRUE) : \
            FALSE)


//
//  Debug helper functions
//
            
#if DBG
            
            
#define DEBUG_TRACE_ERROR                               0x00000001  // Errors - whenever we return a failure code
#define DEBUG_TRACE_LOAD_UNLOAD                         0x00000002  // Loading/unloading of the filter
            
#define DEBUG_TRACE_CDO_CREATE_DELETE                   0x00000004  // Creation/Deletion of CDO
#define DEBUG_TRACE_CDO_SUPPORTED_OPERATIONS            0x00000008  // Supported operations on CDO
#define DEBUG_TRACE_CDO_FASTIO_OPERATIONS               0x00000010  // FastIO operations on CDO
#define DEBUG_TRACE_CDO_ALL_OPERATIONS                  0x00000020  // All operations on CDO
                                    
#define DEBUG_TRACE_ALL                                 0xFFFFFFFF  // All flags
            
            
#define DebugTrace(Level, Data)                     \
    if ((Level) & Globals.DebugLevel) {             \
        DbgPrint Data;                              \
    }
            
            
#else
            
#define DebugTrace(Level, Data)             {NOTHING;}
            
#endif


