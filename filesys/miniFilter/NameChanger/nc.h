#ifndef __NC_H__
#define __NC_H__

#pragma warning( disable: 4701 )  // 'Potentially' uninitialized local variable.
                                  // Hopefully one day the compiler will be a little
                                  // more factual about this and we can use it.  It
                                  // would make a great warning.

//
//  Allocation Tags
//

#define NC_TAG                        'NC__' // Generic NameChanger Tag
#define NC_MAPPING_TAG                'NCMP' // Mapping Buffer Tag
#define NC_INSTANCE_CONTEXT_TAG       'NCin' // Instance Context Tag
#define NC_LOCK_TAG                   'NClk' // Tag for nonpaged locks
#define NC_GENERATE_NAME_TAG          'NCgn' // Tag for strings which are allocated for name generation
#define NC_NORMALIZE_NAME_TAG         'NCnn' // Tag for strings which are allocated for name normalization
#define NC_FILE_NAME_TAG              'NCfn' // Tag for strings which are allocated for file names in file objects
#define NC_DIR_QRY_CACHE_TAG          'NCqc' // Tag for buffers which are allocated for directory enumeration cache and injection entries
#define NC_DIR_QRY_SEARCH_STRING      'NCqs' // Tag for strings which are allocated for directory search strings
#define NC_SET_LINK_BUFFER_TAG        'NCsl' // Tag for munge buffer in Set Link operations
#define NC_RENAME_BUFFER_TAG          'NCrn' // Tag for munge buffer in Rename operations


#define NC_STREAM_HANDLE__CONTEXT_TAG 'NCsh'

//
//  Macros to make math easier.
//

#define NC_SEPARATOR ((WCHAR) L'\\')
#define EMPTY_UNICODE_STRING {0, 0, NULL}

#define AlignToSize(_length, _alignment)   \
    (((_length) + ((_alignment) - 1)) & ~((_alignment) - 1))

#define Max(a,b) ((a) >= (b) ? (a) : (b))

#define NcFreeUnicodeString( UCS )               \
    ExFreePoolWithTag( (UCS)->Buffer, NC_TAG );  \
    (UCS)->Buffer = NULL;                        \
    (UCS)->MaximumLength =                       \
    (UCS)->Length = 0;

//
//  We need to supress a few warnings in order to enable compilation with
//  the 2003 IFS Kit.  WDK does not require this.
//

#pragma warning( push )
#pragma warning( disable: 4201 4214 )
#include <fltkernel.h>
#pragma warning( pop )

#include <ntintsafe.h>

#define INVALID_HANDLE_VALUE ((HANDLE) -1)

//
//  Typedefs for dynamically imported routines
//  

typedef
NTSTATUS
(* NC_REPLACE_FILEOBJECT_NAME_TYPE) (
    _In_ PFILE_OBJECT FileObject,
    _In_reads_bytes_(FileNameLength) PWSTR NewFileName,
    _In_ USHORT FileNameLength
    );

typedef
NTSTATUS
( * NC_QUERY_DIRECTORY_FILE_TYPE) (
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _Out_writes_bytes_(Length) PVOID FileInformation,
    _In_ ULONG Length,
    _In_ FILE_INFORMATION_CLASS FileInformationClass,
    _In_ BOOLEAN ReturnSingleEntry,
    _In_opt_ PUNICODE_STRING FileName,
    _In_ BOOLEAN RestartScan,
    _Out_opt_ PULONG LengthReturned
    );

typedef
NTSTATUS
( * NC_CREATE_FILE_EX_TYPE) (
    _In_ PFLT_FILTER Filter,
    _In_opt_ PFLT_INSTANCE Instance,
    _Out_ PHANDLE FileHandle,
    _Outptr_opt_ PFILE_OBJECT *FileObject,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_opt_ PLARGE_INTEGER AllocationSize,
    _In_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions,
    _In_reads_bytes_opt_(EaLength) PVOID EaBuffer,
    _In_ ULONG EaLength,
    _In_ ULONG Flags
    );

typedef
NTSTATUS
( * NC_CREATE_FILE_EX2_TYPE) (
    _In_ PFLT_FILTER Filter,
    _In_opt_ PFLT_INSTANCE Instance,
    _Out_ PHANDLE FileHandle,
    _Outptr_opt_ PFILE_OBJECT *FileObject,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_opt_ PLARGE_INTEGER AllocationSize,
    _In_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions,
    _In_reads_bytes_opt_(EaLength) PVOID EaBuffer,
    _In_ ULONG EaLength,
    _In_ ULONG Flags,
    _In_opt_ PIO_DRIVER_CREATE_CONTEXT DriverContext
    );

typedef
PVOID
( * NC_GET_NEW_SYSTEM_BUFFER_ADDRESS ) (
    _In_ PFLT_CALLBACK_DATA Data
    );

//
//  Common Structures
//

//
//  Struct which provides a single buffer and offset for data caching.
//

typedef struct __NC_CACHE_ENTRY {

    char *Buffer;
    ULONG CurrentOffset;

} NC_CACHE_ENTRY, *PNC_CACHE_ENTRY;

//
//  Structure which manages a single path.  These are used to precompute
//  various paths for the mapping for easy comparison.  We do not use
//  this structure for user-supplied paths (eg. on open.)
//

typedef struct _NC_MAPPING_PATH {

    UNICODE_STRING FullPath;              // \volume_name\parent_path\final_component
    UNICODE_STRING VolumePath;            // \volume_name
    UNICODE_STRING ParentPath;            // \volume_name\parent_name
    UNICODE_STRING FinalComponentName;    // final_component or \ (for volume root open)
    UNICODE_STRING VolumelessName;        // \parent_name\final_component
    USHORT NumberComponentsInFullPath;    // \volume_name\parent_name\final_component == 3
    USHORT NumberComponentsInVolumePath;  // \volume_name == 1

} NC_MAPPING_PATH, *PNC_MAPPING_PATH;

//
//  Structure which manages a long and short name pairing.
//

typedef struct _NC_MAPPING_ENTRY {

    //Strings which manage the long name.
    NC_MAPPING_PATH LongNamePath;

    //Strings which manage the short name.
    NC_MAPPING_PATH ShortNamePath;

} NC_MAPPING_ENTRY, *PNC_MAPPING_ENTRY;

//
//  Structure which manages with user and real mapping.
//

typedef struct _NC_MAPPING {

    NC_MAPPING_ENTRY RealMapping;
    NC_MAPPING_ENTRY UserMapping;

} NC_MAPPING, *PNC_MAPPING;

//
//  NC_PATH_OVERLAP is a series of flags which determine a name's relation to a mapping.
//
//  Ancestor means that the path is a directory along the mapping path.
//  For example \a\b is an ancestor of \a\b\c, but \a\b\x is not an ancestor of \a\b\c.
//
//  Parent means that the path is the parent directory of the mapping path.
//
//  Match means that the path and the mapping are equal.
//
//  InMapping means that the path is either the mapping itself, or a decendent of the mapping.
//
//  Peer means that the object is directly next to the mapping (a\b\x is a peer of a\b\y.)
//
//  If no flags are set, the path is entirely outside the mapping.
//

//
//  TODO: Should we use a different representation of this structure which
//  does not require warning suppressions?
//
#pragma warning( push )
#pragma warning( disable: 4201 )
typedef union {
    ULONG EntireFlags;
    struct {
        int Ancestor:1;
        int Parent:1;
        int Match:1;
        int InMapping:1;
        int Peer:1;
    };
} NC_PATH_OVERLAP, *PNC_PATH_OVERLAP;
#pragma warning( pop )

//
//  Instance Context Defines
//

typedef struct _NC_INSTANCE_CONTEXT {

    // The mapping is defined as a property of the instance.
    NC_MAPPING Mapping;

    // The file system we're attached to
    FLT_FILESYSTEM_TYPE VolumeFilesystemType;

} NC_INSTANCE_CONTEXT, *PNC_INSTANCE_CONTEXT;


//
//  This context is used when a file is enumerating
//  a directory and we need to cache entries in our context.
//

typedef struct _NC_DIR_QRY_CONTEXT {

    BOOLEAN EnumerationOutstanding;

    // Determines if the structure is in use. If it is,
    // then there is no need to initialize the cache or injection entry.
    BOOLEAN InUse;

    // Pointer to list of entries which we must drain from.
    NC_CACHE_ENTRY Cache;

    // Pointer to entry which we want to inject.
    NC_CACHE_ENTRY InjectionEntry;

    // The user provided search string which can only be set up on
    // the first query.
    UNICODE_STRING SearchString;

    // The information class which the user requested.
    FILE_INFORMATION_CLASS InformationClass;

} NC_DIR_QRY_CONTEXT, *PNC_DIR_QRY_CONTEXT;

//
//  This context is used when a file is being used for
//  directory notification forwarding.
//

typedef struct _NC_DIR_NOT_IO_CACHE {

    //
    //  Stores the state of the cache.
    //  If the pointer is NULL, there is nothing in cache.
    //
    
    NC_CACHE_ENTRY Cache;

    //
    //  If ReadyToIssue is TRUE, then its safe to fire off the request.
    //
    
    BOOLEAN ReadyToIssue;

    //
    //  Io is a pointer to the callback data.
    //  When ReadyToUse new io can be issued.
    //
    
    PFLT_CALLBACK_DATA Io;

    //
    //  SystemBuffer is a pointer to a system buffer which will
    //  be used to issue the IO.
    //
    
    char * SystemBuffer;
    ULONG SystemBufferLength;

} NC_DIR_NOT_IO_CACHE, * PNC_DIR_NOT_IO_CACHE;

typedef struct _NC_DIR_NOT_CONTEXT {

    //
    //  We save off some parameters used to send down the user
    //  request so we can look them up in our completion callback.
    //  In some cases, this is an optimization.  In others, it's
    //  because we need access to state when the user request may
    //  be completed.
    //

    enum {
        Uninitialized,
        Filter,
        Munge,
        Merge
    } Mode;
    BOOLEAN CancelSeen;
    BOOLEAN CleanupSeen;
    BOOLEAN IgnoreCase;
    BOOLEAN InsufficientBufferSeen;
    BOOLEAN WatchTree;
    UCHAR OperationFlags;
    ULONG CompletionFilter;

    //
    //  These names allow us to re-build full path names for each
    //  notification.  We need two strings to represent paths to
    //  the handle the user opened, or the handle we opened to
    //  the mapping parent when merging notifications.  After
    //  building the full name, we'll need to re-translate back
    //  to the user's path for returning.  These names are saved
    //  so we don't need to construct them again in our completion
    //  routine.
    //

    UNICODE_STRING UserRequestName;
    UNICODE_STRING MappingParentName;

    //
    //  Our three potential requests.
    //
    //  UserRequest - the request the user sent to us on this handle.
    //    We may have sent this to the filesystem (Munge case), or we
    //    may hang on to it ourselves (Filter and Merge cases.)
    //
    //  ShadowRequest - the request sent to the filesystem on the
    //    user's handle.  This is used in the Filter and Merge cases.
    //    In the Filter case, we do this to allow us to reissue
    //    the IO if we have filtered out all events so have nothing
    //    to return to the user.  In the merge case, we require two
    //    requests that we control, and will complete the user's
    //    request when either of our requests complete.
    //
    //  MappingRequest - the request sent to the filesystem on the
    //    handle we opened to the parent of the mapping.  Only valid
    //    when in Merge mode.  We use the parent so as to capture
    //    notifications about changes to the mapping itself.
    //

    PFLT_CALLBACK_DATA UserRequest;
    PFLT_CALLBACK_DATA ShadowRequest;
    PFLT_CALLBACK_DATA MappingRequest;

    PNC_INSTANCE_CONTEXT InstanceContext;

    //
    //  Specifies a handle to the parent of the real mapping.
    //  This can be NULL, and is only used if Mode == Merge.
    //  This occurs when we are watching the parent of the
    //  mapping, or if we are watching the subtree of the user
    //  mapping but not the subtree of the real mapping.  Keep
    //  a workitem so that we can reliably clean up this object,
    //  which we can't do inline at APC level.
    //

    HANDLE RealParentHandle;
    PFILE_OBJECT RealParentFileObject;
    PFLT_GENERIC_WORKITEM RealParentCloseWorkItem;

    PFILE_NOTIFY_INFORMATION BufferToFree;
    ULONG BufferLength;

} NC_DIR_NOT_CONTEXT, *PNC_DIR_NOT_CONTEXT;

typedef struct _NC_FIND_BY_SID_CONTEXT {

    //
    //  Specifies a handle to the real mapping.  This can
    //  be NULL, and is only required if finding objects
    //  under an ancestor of the user mapping which is not
    //  an ancestor of the real mapping (so we need to
    //  inject entries.)
    //

    HANDLE RealHandle;
    PFILE_OBJECT RealFileObject;

    //
    //  If the filesystem returns more results than can
    //  be returned in the current call, we need to buffer
    //  them and return them on a subsequent call.
    //  BufferToFree refers to the beginning of the allocation,
    //  which we can tear down when all entries are consumed.
    //  BufferSize is the size of this buffer, in bytes.
    //  CurrentEntry is the offset into this buffer where
    //  we should begin returning result on the subsequent
    //  call.
    //

    PFILE_NAME_INFORMATION BufferToFree;
    ULONG BufferSize;
    ULONG CurrentEntry;

    //
    //  Count of the number of outstanding requests.  We need
    //  to know this to ensure we only tear down state when
    //  a) holding the lock and b) no other requests are
    //  outstanding.
    //

    ULONG OutstandingRequests;

} NC_FIND_BY_SID_CONTEXT, *PNC_FIND_BY_SID_CONTEXT;

//
//  Stream Handle Context
//

typedef struct _NC_STREAM_HANDLE_CONTEXT {

    PERESOURCE Lock;

    NC_DIR_NOT_CONTEXT DirectoryNotificationContext;
    NC_DIR_QRY_CONTEXT DirectoryQueryContext;
    NC_FIND_BY_SID_CONTEXT FindBySidContext;

} NC_STREAM_HANDLE_CONTEXT, *PNC_STREAM_HANDLE_CONTEXT;

#define NcLockStreamHandleContext( C ) \
    FltAcquireResourceExclusive( (C)->Lock );

#define NcUnlockStreamHandleContext( C ) \
    FltReleaseResource( (C)->Lock );

//
//  Structure which gives us offsets to find values within directory
//  query buffers.
//

typedef struct _DIRECTORY_CONTROL_OFFSETS {

    ULONG NextEntryOffsetDist;
    ULONG FileNameLengthDist;
    ULONG FileNameDist;

    BOOLEAN ShortNamePresent;
    ULONG ShortNameLengthDist;
    ULONG ShortNameDist;

} DIRECTORY_CONTROL_OFFSETS, *PDIRECTORY_CONTROL_OFFSETS;

typedef struct _NC_GLOBAL_DATA {
    UNICODE_STRING UserMappingPath;
    UNICODE_STRING UserMappingFinalComponentShort;
    UNICODE_STRING UserMappingFinalComponentLong;

    UNICODE_STRING RealMappingPath;
    UNICODE_STRING RealMappingFinalComponent;

    PFLT_FILTER    FilterHandle;
} NC_GLOBAL_DATA, *PNC_GLOBAL_DATA;

extern NC_GLOBAL_DATA NcGlobalData;

//
//  Function declarations
//  

//
//  The following functions exist in nchelper.c
//

_When_(Data == NULL, _Pre_satisfies_(FileObject != NULL && Instance != NULL))
_When_(FileObject == NULL || Instance == NULL, _Pre_satisfies_(Data != NULL))
NTSTATUS 
NcGetFileNameInformation(
    _In_opt_ PFLT_CALLBACK_DATA Data,
    _In_opt_ PFILE_OBJECT FileObject,
    _In_opt_ PFLT_INSTANCE Instance,
    _In_ FLT_FILE_NAME_OPTIONS NameOptions,
    _Outptr_ PFLT_FILE_NAME_INFORMATION *FileNameInformation 
    );

NTSTATUS
NcAllocateEResource(
    _Out_ PERESOURCE * OutputLock 
    );

VOID
NcFreeEResource(
    _In_ PERESOURCE Lock
    );

NTSTATUS
NcCreateFileHelper (
    _In_ PFLT_FILTER Filter,
    _In_opt_ PFLT_INSTANCE Instance,
    _Out_ PHANDLE FileHandle,
    _Outptr_opt_ PFILE_OBJECT *FileObject,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_opt_ PLARGE_INTEGER AllocationSize,
    _In_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions,
    _In_reads_bytes_opt_(EaLength) PVOID EaBuffer,
    _In_ ULONG EaLength,
    _In_ ULONG Flags,
    _In_opt_ PFILE_OBJECT ParentFileObject
    );

NTSTATUS
NcSetCancelCompletion (
    _In_ PFLT_CALLBACK_DATA Data,
    _In_ PFLT_COMPLETE_CANCELED_CALLBACK CanceledCallback
    );

LONG
NcExceptionFilter (
    _In_ PEXCEPTION_POINTERS ExceptionPointer,
    _In_ BOOLEAN AccessingUserBuffer
    );

//
//  The following functions exist in ncmapping.c
//

VOID
NcInitMappingPath (
    _Out_ PNC_MAPPING_PATH Path
    );

VOID
NcTeardownMappingPath (
    _Inout_ PNC_MAPPING_PATH Path
    );

NTSTATUS
NcBuildMappingPathFromVolume (
    _In_ PFLT_VOLUME CONST Volume,
    _In_ PUNICODE_STRING ParentPath,
    _In_ PUNICODE_STRING FinalComponentName,
    _Inout_ PNC_MAPPING_PATH Entry
    );

BOOLEAN
NcIsMappingZeroed (
    PNC_MAPPING Mapping
    );

VOID
NcInitMapping (
    PNC_MAPPING Mapping
    );

BOOLEAN
NcIsMappingZeroed (
    PNC_MAPPING Mapping
    );

VOID
NcInitMapping (
    PNC_MAPPING Mapping
    );

VOID
NcTeardownMapping (
    _Inout_ PNC_MAPPING Mapping
    );

NTSTATUS
NcBuildMapping (
    _In_ PFILE_OBJECT UserParent,
    _In_ PFILE_OBJECT RealParent,
    _In_ PUNICODE_STRING UserFinalComponentShortName,
    _In_ PUNICODE_STRING UserFinalComponentLongName,
    _In_ PUNICODE_STRING RealFinalComponentName,
    _In_ PFLT_INSTANCE Instance,
    _Out_ PNC_MAPPING Mapping
    );

//
//  The following functions exist in ncinit.c
//

NTSTATUS 
NcInitializeMapping(
    _In_ PUNICODE_STRING RegistryPath
    );

//
//  The following functions exist in ncpath.c
//

BOOLEAN
NcComparePath (
    _In_ PCUNICODE_STRING Name,
    _In_ PNC_MAPPING_ENTRY Mapping,
    _Out_opt_ PUNICODE_STRING Remainder,
    _In_ BOOLEAN IgnoreCase,
    _In_ BOOLEAN ContainsDevice,
    _Out_ PNC_PATH_OVERLAP Overlap
    );

_Post_satisfies_(NewName->MaximumLength < MAXUSHORT)
_Post_satisfies_(NewName->Length <= NewName->MaximumLength)
_Must_inspect_result_
NTSTATUS
NcConstructPath (
    _In_ PNC_MAPPING_ENTRY RealPath,
    _In_ PUNICODE_STRING Remainder,
    _In_ BOOLEAN IncludeVolume,
    _Out_ _At_(NewName->Buffer, __drv_allocatesMem(Mem)) PUNICODE_STRING NewName
    );

NTSTATUS
NcParseFinalComponent (
    _In_ PUNICODE_STRING EntirePath,
    _Out_ PUNICODE_STRING ParentPath,
    _Out_ PUNICODE_STRING FinalComponent
    );

//
//  The following functions exist in nccontext.c
//

NTSTATUS
NcStreamHandleContextAllocAndAttach( 
    _In_ PFLT_FILTER Filter,
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _Out_ PNC_STREAM_HANDLE_CONTEXT * Context
    );

VOID
NcInstanceContextClose (
    _In_ PFLT_CONTEXT Context,
    _In_ FLT_CONTEXT_TYPE ContextType
    );

VOID
NcStreamHandleContextClose (
    _In_ PFLT_CONTEXT Context,
    _In_ FLT_CONTEXT_TYPE ContextType
    );

//
//  The following functions exist in nccreate.c
//

FLT_PREOP_CALLBACK_STATUS
NcPreCreate (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

//
//  The following functions exist in ncnameprov.c
//

NTSTATUS
NcGenerateFileName (
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_opt_ PFLT_CALLBACK_DATA Data,
    _In_ FLT_FILE_NAME_OPTIONS NameOptions,
    _Out_ PBOOLEAN CacheFileNameInformation,
    _Inout_ PFLT_NAME_CONTROL FileName
    );

NTSTATUS
NcNormalizeNameComponentEx (
    _In_ PFLT_INSTANCE Instance,
    _In_opt_ PFILE_OBJECT FileObject,
    _In_ PCUNICODE_STRING ParentDirectory,
    _In_ USHORT DeviceNameLength,
    _In_ PCUNICODE_STRING Component,
    _Out_writes_bytes_(ExpandComponentNameLength) PFILE_NAMES_INFORMATION ExpandComponentName,
    _In_ ULONG ExpandComponentNameLength,
    _In_ FLT_NORMALIZE_NAME_FLAGS Flags,
    _Inout_ PVOID *NormalizationContext
    );

//
//  The following functions exist in ncoffsets.c
//

BOOLEAN
NcDetermineStructureOffsets (
    _Out_ PDIRECTORY_CONTROL_OFFSETS Offsets,
    _In_ FILE_INFORMATION_CLASS Information
    );

VOID 
NcCalculateDirectoryNotificationOffsets (
    PDIRECTORY_CONTROL_OFFSETS Offsets
    );

ULONG
NcGetNextEntryOffset (
    _In_ CONST PVOID Buffer,
    _In_ CONST PDIRECTORY_CONTROL_OFFSETS Offsets
    );

PVOID
NcGetNextEntry (
    _In_ CONST PVOID Buffer,
    _In_ CONST PDIRECTORY_CONTROL_OFFSETS Offsets
    );

ULONG
NcGetFileNameLength (
    _In_ CONST PVOID Buffer,
    _In_ CONST PDIRECTORY_CONTROL_OFFSETS Offsets
    );

ULONG
NcGetEntrySize (
    _In_ CONST PVOID Buffer,
    _In_ CONST PDIRECTORY_CONTROL_OFFSETS Offsets
    );

PWSTR
NcGetFileName (
    _In_ CONST PVOID Buffer,
    _In_ CONST PDIRECTORY_CONTROL_OFFSETS Offsets
    );

PWSTR
NcGetShortName (
    _In_ CONST PVOID Buffer,
    _In_ CONST PDIRECTORY_CONTROL_OFFSETS Offsets
    );

CCHAR
NcGetShortNameLength (
    _In_ CONST PVOID Buffer,
    _In_ CONST PDIRECTORY_CONTROL_OFFSETS Offsets
    );

VOID
NcSetNextEntryOffset (
    _Inout_ PVOID Buffer,
    _In_ CONST PDIRECTORY_CONTROL_OFFSETS Offsets,
    _In_ BOOLEAN ForceLast);

VOID 
NcSetFileName (
    _In_ PVOID Entry,
    _In_ PWSTR NewName,
    _In_ ULONG Length,
    _In_ CONST PDIRECTORY_CONTROL_OFFSETS Offsets,
    _In_ BOOLEAN ForceLast
    );

VOID 
NcSetShortName (
    _In_ PVOID Entry,
    _In_ PWSTR NewShortName,
    _In_ USHORT Length,
    _In_ CONST PDIRECTORY_CONTROL_OFFSETS Offsets
    );

//
//  The following functions exist in ncdirenum.c
//

FLT_PREOP_CALLBACK_STATUS
NcEnumerateDirectory (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

NTSTATUS
NcEnumerateDirectorySetupInjection (
    _Inout_ PNC_DIR_QRY_CONTEXT DirQryCtx,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PNC_INSTANCE_CONTEXT InstanceContext,
    _In_ PDIRECTORY_CONTROL_OFFSETS Offsets,
    _In_ FILE_INFORMATION_CLASS InformationClass
    );

VOID
NcEnumerateDirectoryReset ( 
    _Inout_ PNC_DIR_QRY_CONTEXT dirCtx
    );

NTSTATUS
NcPerformEnumeration();

NTSTATUS
NcPopulateCacheEntry (
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_ ULONG BufferLength,
    _In_ FILE_INFORMATION_CLASS FileInfoClass,
    _In_ PUNICODE_STRING SearchString,
    _In_ BOOLEAN RestartScan,
    _Out_ PNC_CACHE_ENTRY Cache
    );

PNC_CACHE_ENTRY 
NcDirEnumSelectNextEntry ( 
    _Inout_ PNC_DIR_QRY_CONTEXT Context, 
    _In_ PDIRECTORY_CONTROL_OFFSETS Offsets,
    _In_ BOOLEAN IgnoreCase
    );

BOOLEAN
NcSkipName (
    _In_ PDIRECTORY_CONTROL_OFFSETS Offsets,
    _In_ PNC_DIR_QRY_CONTEXT Context, 
    _In_ NC_PATH_OVERLAP RealOverlap,
    _In_ PNC_MAPPING Mapping,
    _In_ BOOLEAN IgnoreCase
    );

_Success_(*Copied)
ULONG
NcCopyDirEnumEntry ( 
    _Out_ PVOID UserBuffer, 
    _In_ ULONG UserOffset, 
    _In_ ULONG UserSize, 
    _Inout_ PNC_CACHE_ENTRY Entry,
    _In_ PDIRECTORY_CONTROL_OFFSETS Offsets,
    _Out_ PBOOLEAN Copied 
    );

NTSTATUS
NcStreamHandleContextDirEnumCreate (
    _Out_ PNC_DIR_QRY_CONTEXT Context 
    );

NTSTATUS
NcStreamHandleContextEnumSetup ( 
    _Inout_ PNC_DIR_QRY_CONTEXT DirContext,
    _In_ PNC_INSTANCE_CONTEXT InstanceContext,
    _In_ PDIRECTORY_CONTROL_OFFSETS Offsets,
    _In_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ NC_PATH_OVERLAP UserMappingOverlap,
    _Out_ PBOOLEAN  FirstUsage
    );

VOID 
NcStreamHandleContextEnumClose (
    _In_ PNC_DIR_QRY_CONTEXT DirContext 
    );

//
//  The following functions exist in ncdirnotify.c
//

FLT_PREOP_CALLBACK_STATUS
NcPreNotifyDirectory (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
NcPostNotifyDirectory (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );

NTSTATUS 
NcStreamHandleContextNotCreate ( 
    _Out_ PNC_DIR_NOT_CONTEXT Context
    );

VOID 
NcStreamHandleContextNotCleanup(
    _In_ PNC_STREAM_HANDLE_CONTEXT HandleContext 
    );

VOID 
NcStreamHandleContextNotClose (
    _In_ PNC_DIR_NOT_CONTEXT DirContext 
    );

//
//  The following functions exist in ncfileinfo.c
//

FLT_PREOP_CALLBACK_STATUS
NcPreQueryAlternateName (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
NcPostQueryHardLinks (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );

FLT_POSTOP_CALLBACK_STATUS
NcPostQueryName (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
NcPreRename (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

FLT_PREOP_CALLBACK_STATUS
NcPreSetDisposition (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

FLT_PREOP_CALLBACK_STATUS
NcPreSetLinkInformation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

FLT_PREOP_CALLBACK_STATUS
NcPreSetShortName (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

//
//  The following functions exist in ncfsctrl.c
//


FLT_POSTOP_CALLBACK_STATUS
NcPostEnumUsnData (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );

NTSTATUS
NcStreamHandleContextFindBySidCreate (
    _Out_ PNC_FIND_BY_SID_CONTEXT Context 
    );

VOID 
NcStreamHandleContextFindBySidClose (
    _In_ PNC_FIND_BY_SID_CONTEXT Context
    );

FLT_PREOP_CALLBACK_STATUS
NcPreFindFilesBySid (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
NcPostFindFilesBySid (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );

#ifdef FLT_MGR_WIN7
FLT_POSTOP_CALLBACK_STATUS
NcPostLookupStreamFromCluster (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );
#endif

FLT_POSTOP_CALLBACK_STATUS
NcPostReadFileUsnData (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );

FLT_POSTOP_CALLBACK_STATUS
NcPostReadUsnJournal (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );

//
//  The following functions exist in nccompat.c
//

VOID
NcCompatInit();

//
//  Global variables
//

extern PFLT_FILTER gFilterHandle;
extern NC_REPLACE_FILEOBJECT_NAME_TYPE NcReplaceFileObjectName;
extern NC_QUERY_DIRECTORY_FILE_TYPE NcQueryDirectoryFile;
extern NC_CREATE_FILE_EX2_TYPE NcCreateFileEx2;
extern NC_GET_NEW_SYSTEM_BUFFER_ADDRESS NcGetNewSystemBufferAddress;

#endif /* __NC_H__ */

