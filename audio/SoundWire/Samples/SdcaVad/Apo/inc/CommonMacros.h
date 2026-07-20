//**@@@*@@@****************************************************
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//**@@@*@@@****************************************************

//
// FileName:    CommonMacros.h
//
// Abstract:    Useful macros
//
// ----------------------------------------------------------------------------


#pragma once
#include <windef.h>
#include <windows.h>


//-------------------------------------------------------------------------
// Description:
//
// If the condition evaluates to TRUE, jump to the given label.
//
// Parameters:
//
//      condition - [in] code that fits in if statement
//      label - [in] label to jump if condition is met
//
#define IF_TRUE_JUMP(condition, label)                          \
    if (condition)                                              \
    {                                                           \
        goto label;                                             \
    }

//-------------------------------------------------------------------------
// Description:
//
// If the condition evaluates to FALSE, jump to the given label.
//
// Parameters:
//
//      condition - [in] code that fits in if statement
//      label - [in] label to jump if condition is met
//
#define IF_FALSE_JUMP(condition, label)                         \
    if (!condition)                                             \
    {                                                           \
        goto label;                                             \
    }

//-------------------------------------------------------------------------
// Description:
//
// If the hresult passed FAILED, jump to the given label.
//
// Parameters:
//
//      _hresult - [in] Value to check
//      label - [in] label to jump if condition is met
//
#define IF_FAILED_JUMP(_hresult, label)                         \
    if (FAILED(_hresult))                                       \
    {                                                           \
        goto label;                                             \
    }

//-------------------------------------------------------------------------
// Description:
//
// If the hresult passed SUCCEEDED, jump to the given label.
//
// Parameters:
//
//      _hresult - [in] Value to check
//      label - [in] label to jump if condition is met
//
#define IF_SUCCEEDED_JUMP(_hresult, label)                      \
    if (SUCCEEDED(_hresult))                                    \
    {                                                           \
        goto label;                                             \
    }

//-------------------------------------------------------------------------
// Description:
//
// If the condition evaluates to TRUE, perform the given statement
// then jump to the given label.
//
// Parameters:
//
//      condition - [in] Code that fits in if statement
//      action - [in] action to perform in body of if statement
//      label - [in] label to jump if condition is met
//
#define IF_TRUE_ACTION_JUMP(condition, action, label)           \
    if (condition)                                              \
    {                                                           \
        action;                                                 \
        goto label;                                             \
    }

//-------------------------------------------------------------------------
// Description:
//
// If the hresult FAILED, perform the given statement then jump to
// the given label.
//
// Parameters:
//
//      _hresult - [in] Value to check
//      action - [in] action to perform in body of if statement
//      label - [in] label to jump if condition is met
//
#define IF_FAILED_ACTION_JUMP(_hresult, action, label)          \
    if (FAILED(_hresult))                                       \
    {                                                           \
        action;                                                 \
        goto label;                                             \
    }

//-------------------------------------------------------------------------
// Description:
//
// Closes a handle and assigns NULL.
//
// Parameters:
//
//      h - [in] handle to close
//
#define SAFE_CLOSE_HANDLE(h)                                    \
    if (NULL != h)                                              \
    {                                                           \
        CloseHandle(h);                                         \
        h = NULL;                                               \
    }

//-------------------------------------------------------------------------
// Description:
//
// Addref an interface pointer
//
// Parameters:
//
//      p - [in] object to addref
//
#define SAFE_ADDREF(p)                                          \
    if (NULL != p)                                              \
    {                                                           \
        (p)->AddRef();;                                         \
    }

//-------------------------------------------------------------------------
// Description:
//
// Releases an interface pointer and assigns NULL.
//
// Parameters:
//
//      p - [in] object to release
//
#define SAFE_RELEASE(p)                                         \
    if (NULL != p)                                              \
    {                                                           \
        (p)->Release();                                         \
        (p) = NULL;                                             \
    }

//-------------------------------------------------------------------------
// Description:
//
// Deletes a pointer and assigns NULL. Do not check for NULL because
// the default delete operator checks for it.
//
// Parameters:
//
//      p - [in] object to delete
//
#define SAFE_DELETE(p)                                          \
    delete p;                                                   \
    p = NULL;

//-------------------------------------------------------------------------
// Description:
//
// Deletes an array pointer and assigns NULL. Do not check for NULL because
// the default delete operator checks for it.
//
// Parameters:
//
//      p - [in] Array to delete
//
#define SAFE_DELETE_ARRAY(p)                                    \
    delete [] p;                                                \
    p = NULL;

//-------------------------------------------------------------------------
// Description:
//
// Frees a block of memory allocated by CoTaskMemAlloc and assigns NULL to
// the pointer
//
// Parameters:
//
//      p - [in] Pointer to memory to free
//
#define SAFE_COTASKMEMFREE(p)                                   \
    if (NULL != p)                                              \
    {                                                           \
        CoTaskMemFree(p);                                       \
        (p) = NULL;                                             \
    }

//-------------------------------------------------------------------------
// Description:
//
// Frees a DLL loaded with LoadLibrary and assigns NULL to the handle
//
// Parameters:
//
//      h - [in] Handle to DLL to free
//
#define SAFE_FREELIBRARY(h)                                     \
    if (NULL != h)                                              \
    {                                                           \
        FreeLibrary(h);                                         \
        (h) = NULL;                                             \
    }

//-------------------------------------------------------------------------
// Description:
//
//  Used to validate a read pointer
//
// Parameters:
//
//     p - [in] read pointer.
//     s - [in] size of memory in bytes pointed to by p.
//
#define IS_VALID_READ_POINTER(p, s)     ((NULL != p) || (0 == s))

//-------------------------------------------------------------------------
// Description:
//
//  Used to validate a write pointer
//
// Parameters:
//
//     p - [in] write pointer.
//     s - [in] size of memory in bytes pointed to by p.
//
#define IS_VALID_WRITE_POINTER(p, s)    ((NULL != p) || (0 == s))

//-------------------------------------------------------------------------
// Description:
//
//  Used to validate a read pointer of a particular type
//
// Parameters:
//
//     p - [in] typed read pointer
//
#define IS_VALID_TYPED_READ_POINTER(p)  IS_VALID_READ_POINTER((p), sizeof *(p))

//-------------------------------------------------------------------------
// Description:
//
//  Used to validate a write pointer of a particular type
//
// Parameters:
//
//     p - [in] typed write pointer
//
#define IS_VALID_TYPED_WRITE_POINTER(p) IS_VALID_WRITE_POINTER((p), sizeof *(p))

// ---------------------------------------------------------------------------
// Macros that wrap windows messages.  Similar to those in windowsX.h and
// commctrl.h
//
#if !defined Static_SetIcon
#define Static_SetIcon(hwnd, hi) \
            (BOOL)SNDMSG((hwnd), STM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)(hi))
#endif

#define TrackBar_SetTickFrequency(hwnd, f) \
            (BOOL)SNDMSG((hwnd), TBM_SETTICFREQ, (WPARAM)(f), 0)

#define TrackBar_SetBuddy(hwnd, f, hbud) \
            (HWND)SNDMSG((hwnd), TBM_SETBUDDY, (WPARAM)(f), (LPARAM)hbud)

#define TrackBar_GetPos(hwnd) \
            (int)SNDMSG((hwnd), TBM_GETPOS, 0, 0)

#define TrackBar_SetPos(hwnd, pos) \
            SNDMSG((hwnd), TBM_SETPOS, (WPARAM)TRUE, (LPARAM)pos)

#define TrackBar_SetRange(hwnd, min, max) \
            SNDMSG((hwnd), TBM_SETRANGE , (WPARAM)TRUE, (LPARAM) MAKELONG(min, max))

#define TrackBar_SetThumbLength(hwnd, l) \
            SNDMSG((hwnd), TBM_SETTHUMBLENGTH, (WPARAM)l, 0);

#define TrackBar_SetPageSize(hwnd, n) \
            SNDMSG((hwnd), TBM_SETPAGESIZE, 0, (LPARAM)n)

#define Window_GetFont(hwnd) \
            (HFONT)SNDMSG((hwnd), WM_GETFONT, 0, 0)

#define Window_SetFont(hwnd, font) \
            SNDMSG((hwnd), WM_SETFONT, (WPARAM)font, FALSE)


// ----------------------------------------------------------------------
// A struct for holding a rect in easier terms than a RECT struct
//
struct SRECT    
{
    int x, y, w, h;
    SRECT()
    {
        x = y = w = h = 0;
    }
    SRECT(int X, int Y, int W, int H)
    {
        x = X; y = Y; w = W; h = H;
    }
    SRECT(RECT* prc)
    {
        x = prc->left;
        y = prc->top;
        w = prc->right - prc->left;
        h = prc->bottom - prc->top;
    }
};

#define HNS_PER_SECOND (10ull * 1000ull * 1000ull)
