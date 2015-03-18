/*++

Copyright (c) 1999 - 2002  Microsoft Corporation

Module Name:

    MetadataManagerStruct.h

Abstract:

    This is the header file defining the data structures used by the kernel mode
    filter driver implementing filter metadata manager.


Environment:

    Kernel mode


--*/

//
//  If this is 1, then the filter will validate that the metadata file is indeed open 
//  whenever a create suceeds on the volume
//

#define VERIFY_METADATA_OPENED 0


//
//  Memory Pool Tags
//

#define FMM_STRING_TAG                        'tSmF'
#define FMM_INSTANCE_CONTEXT_TAG              'cImF'


//
//  Filter metadata management filter global data
//

typedef struct _FMM_GLOBAL_DATA {

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

} FMM_GLOBAL_DATA, *PFMM_GLOBAL_DATA;

extern FMM_GLOBAL_DATA Globals;




//
//  Instance context flags and data structure
//

//
//  Indicates that the instance context resource has been released 
//  before performing a file system operation that could potentially
//  cause the resource to be re-acquired and deadlock the system
//

#define INSTANCE_CONTEXT_F_TRANSITION           0x00000001


//
//  Indicates if the filter has opened the metadata file and
//  holds a reference to the metadata file object for the
//  volume
//

#define INSTANCE_CONTEXT_F_METADATA_OPENED      0x00000002


typedef struct _FMM_INSTANCE_CONTEXT {

    //
    //  Flags for this instance - defined as INSTANCE_CONTEXT_F_XXX
    //

    ULONG Flags;

    //
    //  Instance for this context.
    //

    PFLT_INSTANCE Instance;

    //
    //  File System Type for this instance.
    //

    FLT_FILESYSTEM_TYPE FilesystemType;

    //
    //  Volume associated with this instance.
    //

    PFLT_VOLUME Volume;

    //
    //  Resource for synchronizing  access to the metadata file. 
    //  This recource may also be overloaded to control access to in-memory
    //  structures that hang off the instance context of the volume.
    //

    ERESOURCE MetadataResource;

    //
    //  Handle of the metadata file.
    //

    HANDLE MetadataHandle;

    //
    //  File object of the metadata file.
    //

    PFILE_OBJECT MetadataFileObject;

    //
    //  The file object on cleanup or cancel removal of which we need to re-open
    //  our metadata file. This is basically the file object on which we received
    //  an explicit or implicit lock or a pnp query removal that caused us to
    //  drop the references to our metadata file
    //

    PFILE_OBJECT MetadataOpenTriggerFileObject;

} FMM_INSTANCE_CONTEXT, *PFMM_INSTANCE_CONTEXT;

#define FMM_INSTANCE_CONTEXT_SIZE         sizeof( FMM_INSTANCE_CONTEXT )


//
//  Name of the metadata file for this filter.
//  In this sample, we put the metadata file in the SystemVolumeInformation
//  folder so as to demonstrate creation of this folder if it does not
//  exist
//

#define FMM_METADATA_FILE_NAME                L"\\System Volume Information\\FilterMetadata.md"
#define FMM_METADATA_FILE_NAME_LENGTH         (sizeof( FMM_METADATA_FILE_NAME ) - sizeof( WCHAR ))

//
//  Default length of the volume name.
//

#define FMM_DEFAULT_VOLUME_NAME_LENGTH        64


//
//  Debug helper functions
//

#if DBG


#define DEBUG_TRACE_ERROR                           0x00000001  // Errors - whenever we return a failure code
#define DEBUG_TRACE_LOAD_UNLOAD                     0x00000002  // Loading/unloading of the filter
#define DEBUG_TRACE_INSTANCES                       0x00000004  // Attach / detatch of instances

#define DEBUG_TRACE_METADATA_OPERATIONS             0x00000008  // Operation to access / modify in memory metadata

#define DEBUG_TRACE_ALL_IO                          0x00000010  // All IO operations tracked by this filter

#define DEBUG_TRACE_INFO                            0x00000020  // Misc. information

#define DEBUG_TRACE_ALL                             0xFFFFFFFF  // All flags


#define DebugTrace(Level, Data)                     \
    if ((Level) & Globals.DebugLevel) {               \
        DbgPrint Data;                              \
    }


#else

#define DebugTrace(Level, Data)             {NOTHING;}

#endif

