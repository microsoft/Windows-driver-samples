// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
//
// File Name:
//
//    Exception.h
//
// Abstract:
//
//    Exception macro and class declarations.
//

#pragma once

//
// Macro to convert HRESULT into an exception throw
//
#ifndef THROW_ON_FAILED_HRESULT
#define THROW_ON_FAILED_HRESULT(func_)                                              \
{                                                                                   \
    HRESULT hr_ = func_;                                                            \
    if (FAILED(hr_)) { xpsrasfilter::ThrowHRException(hr_, __FILE__, __LINE__); }   \
} 
#endif // THROW_ON_FAILED_HRESULT

#ifndef THROW_LAST_ERROR
#define THROW_LAST_ERROR()                                      \
{                                                               \
        HRESULT errhr_ = HRESULT_FROM_WIN32(::GetLastError());  \
        THROW_ON_FAILED_HRESULT(errhr_);                        \
}
#endif // THROW_LAST_ERROR

//
// Macro to catch various exceptions, including
// HRESULT-turned-exceptions.
//
// Because we have defined USE_NATIVE_EH=1, the
// catch(...) block will not catch structural
// exceptions.
//
#ifndef CATCH_VARIOUS
#define CATCH_VARIOUS(hr_)                  \
    catch(std::bad_alloc const& )           \
    {                                       \
        hr_ = E_OUTOFMEMORY;                \
    }                                       \
    catch(xpsrasfilter::hr_error const& e)  \
    {                                       \
        hr_ = e.hr;                         \
    }                                       \
    catch(std::exception const& )           \
    {                                       \
        hr_ = E_FAIL;                       \
    }                                       \
    catch(...)                              \
    {                                       \
        hr_ = E_UNEXPECTED;                 \
    }
#endif // CATCH_VARIOUS

namespace xpsrasfilter
{

//
// HRESULT exception
//
struct hr_error
{
    HRESULT hr;

    hr_error(HRESULT hr_in) : hr(hr_in)
    { }
};

void ThrowHRException(
    HRESULT hr,
    char const *fileName,
    int lineNum
    );

} // namespace xpsrasfilter
