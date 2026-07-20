// Copyright (C) Microsoft Corporation. All rights reserved.
#include "pch.hpp"
#include "memory.h"
#include "memory.tmh"

// for wil::make_unique_nothrow
void*
operator new(size_t s, std::nothrow_t const&)
{
    return operator new(s);
}