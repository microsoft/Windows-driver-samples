////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_Macros.h
//
//   Abstract:
//      This module contains definitions for the various macros used throughout this driver.
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Add support for multiple injectors and WPP tracing.
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef HELPERFUNCTIONS_MACROS_H
#define HELPERFUNCTIONS_MACROS_H

extern "C"
{
   #pragma warning(push)
   #pragma warning(disable: 4201) /// NAMELESS_STRUCT_UNION
   #pragma warning(disable: 4324) /// STRUCTURE_PADDED

   #include <ntifs.h>                    /// IfsKit\Inc
   #include <ntddk.h>                    /// Inc
   #include <wdf.h>                      /// Inc\WDF\KMDF\1.9
   #include <ndis.h>                     /// Inc
   #include <fwpmk.h>                    /// Inc
   #include <fwpsk.h>                    /// Inc
   #include <netioddk.h>                 /// Inc
   #include <ntintsafe.h>                /// Inc
   #include <ntstrsafe.h>                /// Inc
   #include <stdlib.h>                   /// SDK\Inc\CRT

   #pragma warning(pop)
}

#include "Identifiers.h"                 /// ..\Inc
#include "ProviderContexts.h"            /// ..\Inc

/// Macros

#define WFPSAMPLER_SYSLIB_TAG         (UINT32)'LSSW'
#define WFPSAMPLER_NDIS_POOL_TAG      (UINT32)'PNSW'
#define WFPSAMPLER_CALLOUT_DRIVER_TAG (UINT32)'DCSW'

#define WFPSAMPLER_INDEX 0
#define UNIVERSAL_INDEX  1
#define MAX_INDEX        2

#define FWPM_SUBLAYER_UNIVERSAL_WEIGHT 0x8000

#define WPP_COMPID_LEVEL_LOGGER(COMPID,LEVEL)  (WPP_CONTROL(WPP_BIT_Error).Logger),
#define WPP_COMPID_LEVEL_ENABLED(COMPID,LEVEL) (WPP_CONTROL(WPP_BIT_Error).Level >= LEVEL)

#define SID_LENGTH(pSID)                        \
    (8 + (4 * ((SID*)pSID)->SubAuthorityCount))

/**
 @macro="htonl"
 
   Purpose:  Convert ULONG in Host Byte Order to Network Byte Order.                            <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define htonl(l)                  \
   ((((l) & 0xFF000000) >> 24) | \
   (((l) & 0x00FF0000) >> 8)  |  \
   (((l) & 0x0000FF00) << 8)  |  \
   (((l) & 0x000000FF) << 24))

/**
 @macro="htons"
 
   Purpose:  Convert USHORT in Host Byte Order to Network Byte Order.                           <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define htons(s) \
   ((((s) >> 8) & 0x00FF) | \
   (((s) << 8) & 0xFF00))

/**
 @macro="ntohl"
 
   Purpose:  Convert ULONG in Network Byte Order to Host Byte Order.                            <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define ntohl(l)                   \
   ((((l) >> 24) & 0x000000FFL) | \
   (((l) >>  8) & 0x0000FF00L) |  \
   (((l) <<  8) & 0x00FF0000L) |  \
   (((l) << 24) & 0xFF000000L))

/**
 @macro="ntohs"
 
   Purpose:  Convert USHORT in Network Byte Order to Host Byte Order.                           <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define ntohs(s)                     \
   ((USHORT)((((s) & 0x00ff) << 8) | \
   (((s) & 0xff00) >> 8)))


/**
 @macro="HLPR_CLOSE_HANDLE"
 
   Purpose:  Close a standard handle and set to 0.                                              <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_CLOSE_HANDLE(handle)\
   if(handle)                    \
   {                             \
      CloseHandle(handle);       \
      handle = 0;                \
   }

/**
 @macro="HLPR_REG_CLOSE_KEY"
 
   Purpose:  Close a registry handle and set to 0.                                              <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_REG_CLOSE_KEY(keyHandle)\
   if(keyHandle)                     \
   {                                 \
      RegCloseKey(keyHandle);        \
      keyHandle = 0;                 \
   }

/**
 @macro="HLPR_DELETE"
 
   Purpose:  Free memory allocated with ExAllocatePoolWithTag and set the pointer to 0.         <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_DELETE(pPtr, tag)       \
   if(pPtr)                          \
   {                                 \
      ExFreePoolWithTag((VOID*)pPtr, \
                        tag);        \
      pPtr = 0;                      \
   }

/**
 @macro="HLPR_DELETE_ARRAY"
 
   Purpose:  Free memory allocated with ExAllocatePoolWithTag and set the pointer to 0.         <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_DELETE_ARRAY(pPtr, tag) \
   HLPR_DELETE(pPtr, tag)

/**
 @macro="HLPR_NEW"
 
   Purpose:  Allocate memory from NonPaged Pool with ExAllocatePoolWithTag and initialize it's 
             contents with 0's.                                                                 <br>
                                                                                                <br>
   Notes:    Caller responsible for freeing allocated memory using macro HLPR_DELETE.           <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_NEW(pPtr, object, tag)                         \
   for(;                                                    \
       pPtr == 0;                                           \
      )                                                     \
   {                                                        \
      pPtr = (object*)ExAllocatePoolWithTag(NonPagedPoolNx, \
                                            sizeof(object), \
                                            tag);           \
      if(pPtr)                                              \
         RtlSecureZeroMemory(pPtr,                          \
                             sizeof(object));               \
   }

/**
 @macro="HLPR_NEW_ARRAY"
 
   Purpose:  Allocate memory from NonPaged Pool with ExAllocatePoolWithTag and initialize it's 
             contents with 0's.                                                                 <br>
                                                                                                <br>
   Notes:    Caller responsible for freeing allocated memory using macro HLPR_DELETE_ARRAY.     <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_NEW_ARRAY(pPtr, object, count, tag)               \
   for(;                                                       \
       pPtr == 0;                                              \
      )                                                        \
   {                                                           \
      size_t SAFE_SIZE = 0;                                    \
      if(count &&                                              \
         RtlSizeTMult(sizeof(object),                          \
                      (size_t)count,                           \
                      &SAFE_SIZE) == STATUS_SUCCESS &&         \
         SAFE_SIZE >= (sizeof(object) * count))                \
      {                                                        \
         pPtr = (object*)ExAllocatePoolWithTag(NonPagedPoolNx, \
                                               SAFE_SIZE,      \
                                               tag);           \
         if(pPtr)                                              \
            RtlZeroMemory(pPtr,                                \
                          SAFE_SIZE);                          \
      }                                                        \
      else                                                     \
      {                                                        \
         pPtr = 0;                                             \
         break;                                                \
      }                                                        \
   }

/**
 @macro="HLPR_NEW_CASTED_ARRAY"
 
   Purpose:  Allocate memory from NonPaged Pool with ExAllocatePoolWithTag and initialize it's 
             contents with 0's.                                                                 <br>
                                                                                                <br>
   Notes:    Caller responsible for freeing allocated memory using macro HLPR_DELETE_ARRAY.     <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_NEW_CASTED_ARRAY(pPtr, CAST_TYPE, object, count, tag) \
   for(;                                                           \
       pPtr == 0;                                                  \
      )                                                            \
   {                                                               \
      size_t SAFE_SIZE = 0;                                        \
      if(count &&                                                  \
         RtlSizeTMult(sizeof(object),                              \
                      (size_t)count,                               \
                      &SAFE_SIZE) == STATUS_SUCCESS &&             \
         SAFE_SIZE >= (sizeof(object) * count))                    \
      {                                                            \
         pPtr = (CAST_TYPE*)ExAllocatePoolWithTag(NonPagedPoolNx,  \
                                                  SAFE_SIZE,       \
                                                  tag);            \
         if(pPtr)                                                  \
            RtlZeroMemory(pPtr,                                    \
                          SAFE_SIZE);                              \
      }                                                            \
      else                                                         \
      {                                                            \
         pPtr = 0;                                                 \
         break;                                                    \
      }                                                            \
   }

/**
 @macro="HLPR_NEW"
 
   Purpose:  Allocate memory from NonPaged Pool with ExAllocatePoolWithTag and leave it's 
             contents as is.                                                                    <br>
                                                                                                <br>
   Notes:    Caller responsible for freeing allocated memory using macro HLPR_DELETE.           <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_NEW_POPULATED(pPtr, object, tag)               \
   for(;                                                    \
       pPtr == 0;                                           \
      )                                                     \
   {                                                        \
      pPtr = (object*)ExAllocatePoolWithTag(NonPagedPoolNx, \
                                            sizeof(object), \
                                            tag);           \
   }

/**
 @macro="HLPR_BAIL_LABEL"
 
   Purpose:  Tag for the cleanup and exit portion of the function.                              <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_BAIL_LABEL \
      cleanup

/**
 @macro="HLPR_BAIL_LABEL"
 
   Purpose:  Tag for the cleanup and exit portion of the function.                              <br>
                                                                                                <br>
   Notes:    Used when there can be more than 1 jump in code.                                   <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_BAIL_LABEL_2 \
      cleanup_2

/**
 @macro="HLPR_BAIL_ON_FAILURE_WITH_LABEL"
 
   Purpose:  Jump in the code's execution to the provided label if an error occurs.             <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_BAIL_ON_FAILURE_WITH_LABEL(status, label) \
   if(status != STATUS_SUCCESS)                        \
      goto label


#define HLPR_BAIL_ON_ALLOC_FAILURE_WITH_LABEL(pPtr, status, label) \
   if(pPtr == 0)                                                   \
   {                                                               \
      status = (UINT32)STATUS_NO_MEMORY;                           \
      goto label;                                                  \
   }

/**
 @macro="HLPR_BAIL_ON_FAILURE"
 
   Purpose:  Jump in the code's execution path to the HLPR_BAIL_LABEL label if an error occurs. <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_BAIL_ON_FAILURE(status)                        \
   HLPR_BAIL_ON_FAILURE_WITH_LABEL(status, HLPR_BAIL_LABEL)

/**
 @macro="HLPR_BAIL_ON_FAILURE_2"
 
   Purpose:  Jump in the code's execution path to the HLPR_BAIL_LABEL_2 label if an error 
             occurs.                                                                            <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_BAIL_ON_FAILURE_2(status)                        \
   HLPR_BAIL_ON_FAILURE_WITH_LABEL(status, HLPR_BAIL_LABEL_2)

/**
 @macro="HLPR_BAIL_ON_ALLOC_FAILURE"
 
   Purpose:  Jump in the code's execution path to the HLPR_BAIL_LABEL label if memory allocation 
             fails.                                                                             <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_BAIL_ON_ALLOC_FAILURE(pPtr, status)                        \
   HLPR_BAIL_ON_ALLOC_FAILURE_WITH_LABEL(pPtr, status, HLPR_BAIL_LABEL)

/**
 @macro="HLPR_BAIL_ON_ALLOC_FAILURE_2"
 
   Purpose:  Jump in the code's execution path to the HLPR_BAIL_LABEL_2 label if memory 
             allocation fails.                                                                  <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_BAIL_ON_ALLOC_FAILURE_2(pPtr, status)                        \
   HLPR_BAIL_ON_ALLOC_FAILURE_WITH_LABEL(pPtr, status, HLPR_BAIL_LABEL_2)

/**
 @macro="HLPR_BAIL_WITH_LABEL"
 
   Purpose:  Jump in the code's execution path to the provided label.                           <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_BAIL_WITH_LABEL(label) \
   goto label

/**
 @macro="HLPR_BAIL"
 
   Purpose:  Jump in the code's execution path to the HLPR_BAIL_LABEL label.                    <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_BAIL                         \
   HLPR_BAIL_WITH_LABEL(HLPR_BAIL_LABEL)

/**
 @macro="HLPR_BAIL_2"
 
   Purpose:  Jump in the code's execution path to the HLPR_BAIL_LABEL_2 label.                  <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_BAIL_2                        \
   HLPR_BAIL_WITH_LABEL(HLPR_BAIL_LABEL_2)

/**
 @macro="HLPR_BAIL_ON_NULL_POINTER"
 
   Purpose:  Jump in the code's execution path to the HLPR_BAIL_LABEL label if the pointer is 
             NULL.                                                                              <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_BAIL_ON_NULL_POINTER(pPtr) \
   if(pPtr == 0)                        \
      HLPR_BAIL

/**
 @macro="HLPR_BAIL_ON_NULL_POINTER_WITH_STATUS"
 
   Purpose:  Jump in the code's execution path to the HLPR_BAIL_LABEL label if the pointer is 
             NULL.                                                                              <br>
                                                                                                <br>
   Notes:    Status is set to STATUS_INVALID_ADDRESS.                                           <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_BAIL_ON_NULL_POINTER_WITH_STATUS(pPtr, status) \
   if(pPtr == 0)                                            \
   {                                                        \
      status = STATUS_INVALID_ADDRESS;                      \
      HLPR_BAIL;                                            \
   }

#endif /// HELPERFUNCTIONS_MACROS_H
