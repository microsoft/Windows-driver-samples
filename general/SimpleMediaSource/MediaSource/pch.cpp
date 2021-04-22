/// <copyright file="stdafx.cpp" company="Microsoft">
///    Copyright (c) Microsoft Corporation. All rights reserved.
/// </copyright>
// stdafx.cpp : source file that includes just the standard includes
// KinectMFMediaSource.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "pch.h"

// TODO: reference any additional headers you need in pch.h
// and not in this file

namespace winrt
{
    template<> bool is_guid_of<IMFMediaSourceEx>(guid const& id) noexcept
    {
        return is_guid_of<IMFMediaSourceEx, IMFMediaSource, IMFMediaEventGenerator>(id);
    }

    template<> bool is_guid_of<IMFMediaStream2>(guid const& id) noexcept
    {
        return is_guid_of<IMFMediaStream2, IMFMediaStream, IMFMediaEventGenerator>(id);
    }

    template<> bool is_guid_of<IMFActivate>(guid const& id) noexcept
    {
        return is_guid_of<IMFActivate, IMFAttributes>(id);
    }
}