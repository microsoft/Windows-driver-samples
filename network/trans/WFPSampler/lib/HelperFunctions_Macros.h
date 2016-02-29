////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_Macros.h
//
//   Abstract:
//      This module contains definitions for the various macros used throughout this program.
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef HELPERFUNCTIONS_MACROS_H
#define HELPERFUNCTIONS_MACROS_H

#include <Windows.h>       /// Include\UM
#include <WInternl.h>      /// Include\UM
#include <StdLib.h>        /// Inc\CRT
#include <PsAPI.h>         /// Include\UM
#include <TlHelp32.h>         /// Include\UM
#include <FWPSU.h>         /// Include\UM
#include <FWPMU.h>         /// Include\UM
#include <NTDDNDIS.h>      /// Include\Shared
#include <WinSock2.h>      /// Include\UM
#include <WS2TCPIP.h>      /// Include\UM
#include <MSTCPIP.h>       /// Include\Shared
#include <IPHlpAPI.h>      /// Include\Shared
#include <StrSafe.h>       /// Include\Shared
#include <IntSafe.h>       /// Include\Shared

#include "WFPSamplerRPC.h" /// $(OBJ_PATH)\..\idl\$(O)
#include "Identifiers.h"   /// ..\inc
#include "WFPArrays.h"     /// ..\inc
#include "ScenarioData.h"  /// ..\inc

/// <FORWARD_DECLARATIONS>

VOID HlprLogError(_In_ PCWSTR pMessage,
                  ...);

/// </FORWARD_DECLARATIONS>

/**
 @macro="HLPR_CLOSE_HANDLE"
 
   Purpose:  Close a standard handle and set to 0.                                              <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_CLOSE_HANDLE(handle) \
   if(handle)                     \
   {                              \
      CloseHandle(handle);        \
      handle = 0;                 \
   }

/**
 @macro="HLPR_CLOSE_SERVICE_HANDLE"
 
   Purpose:  Close a service handle and set to 0.                                               <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_CLOSE_SERVICE_HANDLE(handle) \
   if(handle)                             \
   {                                      \
      CloseServiceHandle(handle);         \
      handle = 0;                         \
   }

/**
 @macro="HLPR_DELETE"
 
   Purpose:  Free memory allocated with new and set the pointer to 0.                           <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_DELETE(pPtr) \
   if(pPtr)               \
   {                      \
      delete pPtr;        \
      pPtr = 0;           \
   }

/**
 @macro="HLPR_DELETE_ARRAY"
 
   Purpose:  Free memory allocated with new[] and set the pointer to 0.                         <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_DELETE_ARRAY(pPtr) \
   if(pPtr)                     \
   {                            \
      delete[] pPtr;            \
      pPtr = 0;                 \
   }

/**
 @macro="HLPR_NEW"
 
   Purpose:  Allocate memory with new and zero out its contents.                                <br>
                                                                                                <br>
   Notes:    Caller responsible for freeing allocated memory using macro HLPR_DELETE.           <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_NEW(pPtr, object)             \
   do                                      \
   {                                       \
      HLPR_DELETE(pPtr);                   \
      pPtr = new object;                   \
      if(pPtr)                             \
         SecureZeroMemory(pPtr,            \
                          sizeof(object)); \
   }while(pPtr == 0)

/**
 @macro="HLPR_NEW_ARRAY"
 
   Purpose:  Allocate memory with new[] and zero out its contents.                              <br>
                                                                                                <br>
   Notes:    Caller responsible for freeing allocated memory using macro HLPR_DELETE_ARRAY.     <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_NEW_ARRAY(pPtr, object, count)           \
   do                                                 \
   {                                                  \
      size_t SAFE_SIZE = 0;                           \
      HLPR_DELETE_ARRAY(pPtr);                        \
      if(count &&                                     \
         SizeTMult(sizeof(object),                    \
                   (size_t)count,                     \
                   &SAFE_SIZE) == S_OK &&             \
         SAFE_SIZE >= (sizeof(object) * count))       \
      {                                               \
         pPtr = new object[count];                    \
         if(pPtr)                                     \
            SecureZeroMemory(pPtr,                    \
                             SAFE_SIZE);              \
      }                                               \
      else                                            \
      {                                               \
         pPtr = 0;                                    \
         HlprLogError(L"[count: %d][objectSize: %d]", \
                      count,                          \
                      sizeof(object));                \
         break;                                       \
      }                                               \
   }while(pPtr == 0)

/**
 @macro="HLPR_NEW_CASTED_ARRAY"
 
   Purpose:  Allocate memory with new[] while casting the pointer, and zero out its contents.   <br>
                                                                                                <br>
   Notes:    Caller responsible for freeing allocated memory using macro HLPR_DELETE_ARRAY.     <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_NEW_CASTED_ARRAY(pPtr, CAST_TYPE, object, count) \
   do                                                         \
   {                                                          \
      size_t SAFE_SIZE = 0;                                   \
      HLPR_DELETE_ARRAY(pPtr);                                \
      if(count &&                                             \
         SizeTMult(sizeof(object),                            \
                   (size_t)count,                             \
                   &SAFE_SIZE) == S_OK &&                     \
         SAFE_SIZE >= (sizeof(object) * count))               \
      {                                                       \
         pPtr = (CAST_TYPE*)(new object[count]);              \
         if(pPtr)                                             \
            SecureZeroMemory(pPtr,                            \
                             SAFE_SIZE);                      \
      }                                                       \
      else                                                    \
      {                                                       \
         pPtr = 0;                                            \
         HlprLogError(L"[count: %d][objectSize: %d]",         \
                      count,                                  \
                      sizeof(object));                        \
         break;                                               \
      }                                                       \
   }while(pPtr == 0)

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

/*
 @macro="HLPR_BAIL_LABEL_2"
 
   Purpose:  Tag for the cleanup and exit portion of the function.                              <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_BAIL_LABEL_2 \
   cleanup_2

/**
 @macro="HLPR_BAIL_ON_FAILURE_WITH_LABEL"
 
   Purpose:  Jump in the code's execution to the provided tag if an error occurs.               <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_BAIL_ON_FAILURE_WITH_LABEL(status, label) \
   if(status != NO_ERROR)                              \
      goto label

/**
 @macro="HLPR_BAIL_ON_FAILURE"
 
   Purpose:  Jump in the code's execution path to the HLPR_BAIL_LABEL tag if an error occurs.   <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_BAIL_ON_FAILURE(status)                \
   HLPR_BAIL_ON_FAILURE_WITH_LABEL(status,          \
                                   HLPR_BAIL_LABEL)

/**
 @macro="HLPR_BAIL_ON_FAILURE_2"
 
   Purpose:  Jump in the code's execution path to the HLPR_BAIL_LABEL_2 tag if an error occurs. <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_BAIL_ON_FAILURE_2(status)                \
   HLPR_BAIL_ON_FAILURE_WITH_LABEL(status,            \
                                   HLPR_BAIL_LABEL_2)

/**
 @macro="HLPR_BAIL_ON_ALLOC_FAILURE_WITH_LABEL"
 
   Purpose:  Jump in the code's execution path to the provided tag if memory allocation fails.  <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_BAIL_ON_ALLOC_FAILURE_WITH_LABEL(pPtr, status, label) \
   if(pPtr == 0)                                                   \
   {                                                               \
      status = ERROR_OUTOFMEMORY;                                  \
      HlprLogError(L"Allocation Failure [status: %#x]", status);   \
      goto label;                                                  \
   }

/**
 @macro="HLPR_BAIL_ON_ALLOC_FAILURE"
 
   Purpose:  Jump in the code's execution path to the HLPR_BAIL_LABEL tag if memory allocation 
             fails.                                                                             <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_BAIL_ON_ALLOC_FAILURE(pPtr, status)          \
   HLPR_BAIL_ON_ALLOC_FAILURE_WITH_LABEL(pPtr,            \
                                         status,          \
                                         HLPR_BAIL_LABEL)

/**
 @macro="HLPR_BAIL_ON_ALLOC_FAILURE_2"
 
   Purpose:  Jump in the code's execution path to the HLPR_BAIL_LABEL_2 tag if memory allocation 
             fails.                                                                             <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_BAIL_ON_ALLOC_FAILURE_2(pPtr, status)          \
   HLPR_BAIL_ON_ALLOC_FAILURE_WITH_LABEL(pPtr,              \
                                         status,            \
                                         HLPR_BAIL_LABEL_2)

/**
 @macro="HLPR_BAIL_ON_NULL_POINTER"
 
   Purpose:  Jump in the code's execution path to the HLPR_BAIL_LABEL tag if the pointer is 
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
 @macro="HLPR_BAIL_ON_NULL_POINTER_2"
 
   Purpose:  Jump in the code's execution path to the HLPR_BAIL_LABEL_2 tag if the pointer is 
             NULL.                                                                              <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_BAIL_ON_NULL_POINTER_2(pPtr) \
      if(pPtr == 0)                       \
         HLPR_BAIL_2

/**
 @macro="HLPR_BAIL_ON_NULL_POINTER_WITH_STATUS"
 
   Purpose:  Jump in the code's execution path to the HLPR_BAIL_LABEL tag if the pointer is 
             NULL.                                                                              <br>
                                                                                                <br>
   Notes:    Status is set to ERROR_OUTOFMEMORY.                                                <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_BAIL_ON_NULL_POINTER_WITH_STATUS(pPtr, status) \
   if(pPtr == 0)                                            \
   {                                                        \
      status = ERROR_OUTOFMEMORY;                           \
      HLPR_BAIL;                                            \
   }

/**
 @macro="HLPR_BAIL_ON_NULL_POINTER_WITH_STATUS_2"
 
   Purpose:  Jump in the code's execution path to the HLPR_BAIL_LABEL_2 tag if the pointer is 
             NULL.                                                                              <br>
                                                                                                <br>
   Notes:    Status is set to ERROR_OUTOFMEMORY.                                                <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_BAIL_ON_NULL_POINTER_WITH_STATUS_2(pPtr, status) \
   if(pPtr == 0)                                              \
   {                                                          \
      status = ERROR_OUTOFMEMORY;                             \
      HLPR_BAIL_2;                                            \
   }

/**
 @macro="HLPR_BAIL_WITH_LABEL"
 
   Purpose:  Jump in the code's execution path to the provided tag.                             <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_BAIL_WITH_LABEL(label) \
   goto label

/**
 @macro="HLPR_BAIL"
 
   Purpose:  Jump in the code's execution path to the HLPR_BAIL_LABEL tag.                      <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_BAIL \
   HLPR_BAIL_WITH_LABEL(HLPR_BAIL_LABEL)

/**
 @macro="HLPR_BAIL_2"
 
   Purpose:  Jump in the code's execution path to the HLPR_BAIL_LABEL_2 tag.                    <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
#define HLPR_BAIL_2 \
   HLPR_BAIL_WITH_LABEL(HLPR_BAIL_LABEL_2)

#endif /// HELPERFUNCTIONS_MACROS_H