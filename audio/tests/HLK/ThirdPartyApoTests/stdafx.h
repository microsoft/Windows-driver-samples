//
// stdafx.h -- Copyright (c) Microsoft Corporation
//
// Author: zaneh
//
// Description:
//
//   Include file for well-known headers
//
#pragma once

#define _USE_MATH_DEFINES
#include <cmath>
#include <vector>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#define NO_MOCK_VALIDATION
#include <mock.h>
#include <Windows.Media.Devices.h>
#include <windows.h>
#include <versionhelpers.h>
#include <WexTestClass.h>

#include "wil\resource.h"
#include "wil\com.h"


// System Effect Specifiers. These help determine which type of system
// effect the device is attempting to use in order to test it properly
#define DT_GFX          0x00000002
#define DT_LFX          0x00000004
#define DT_LGFX         (DT_LFX | DT_GFX)
#define DT_SFX          0x00000008
#define DT_MFX          0x00000010
#define DT_EFX          0x00000020
#define DT_SMEFX        (DT_SFX | DT_MFX | DT_EFX)

// Reg Key definitions for disabling AudioDG crashing on an APO exception
#define PREVENT_APOTEST_CRASH_OR_REPORT_ON_APO_EXCEPTION L"PreventAPOTestCrashOrReportOnAPOException"
#define REGKEY_AUDIO_SOFTWARE  L"Software\\Microsoft\\Windows\\CurrentVersion\\Audio"
#define REGKEY_DEFAULT L"(default)"

// Test logging macros
#ifndef LOG_ERROR
#define LOG_ERROR(fmt, ...) WEX::Logging::Log::Error(WEX::Common::String().Format(fmt L"\n\t\tFunction: " \
                            __FUNCTION__ L" Line: %d", __VA_ARGS__, __LINE__))
#endif

#ifndef LOG_OUTPUT
#define LOG_OUTPUT(fmt, ...)  WEX::Logging::Log::Comment(WEX::Common::String().Format(fmt, __VA_ARGS__))
#endif

#define LOG_RETURN_IF_FAILED(hr, fmt, ...) if (FAILED(hr)) { LOG_ERROR(fmt, __VA_ARGS__); \
                            WEX::Logging::Log::Error(WEX::Common::String().Format(L"\t\tError Code: (0x%08lx)" \
                            , hr)); return hr; }
#define LOG_RETURN_VOID_IF_FAILED(hr, fmt, ...) if(FAILED(hr)) { LOG_ERROR(fmt, __VA_ARGS__); \
                            WEX::Logging::Log::Error(WEX::Common::String().Format(L"\t\tError Code: (0x%08lx)" \
                            , hr)); return; }
#define LOG_RETURN_HR_IF(condition, hr, fmt, ...) if (condition) { LOG_ERROR(fmt, __VA_ARGS__); \
                            WEX::Logging::Log::Error(WEX::Common::String().Format(L"\t\tError Code: (0x%08lx)" \
                            , hr)); return hr; }
// End Test logging macros

// Begin Metadata defines
#define COMMON_ONECORE_TEST_PROPERTIES \
    COMMON_TEST_PROPERTIES \
    TEST_METHOD_PROPERTY(L"Kits.SupportedOS", L"Windows v10.0 Client x86") \
    TEST_METHOD_PROPERTY(L"Kits.SupportedOS", L"Windows v10.0 Client x64") \
    TEST_METHOD_PROPERTY(L"Kits.SupportedOS", L"Windows v10.0 Client ARM") \
    TEST_METHOD_PROPERTY(L"Kits.SupportedOS", L"Windows v10.0 Client ARM64") \
    TEST_METHOD_PROPERTY(L"Kits.MinRelease", L"RS5") \
    TEST_METHOD_PROPERTY(L"Kits.CorePackageComposition", L"OneCoreUAP") \
    TEST_METHOD_PROPERTY(L"Kits.CorePackageComposition", L"OneCore") \
    TEST_METHOD_PROPERTY(L"Kits.CorePackageComposition", L"Full") \
    
#define COMMON_TEST_PROPERTIES \
    TEST_METHOD_PROPERTY(L"Kits.TestType", L"Development") \
    TEST_METHOD_PROPERTY(L"Kits.RunElevated", L"True") \
    TEST_METHOD_PROPERTY(L"Kits.RequiresReboot", L"False") \
    TEST_METHOD_PROPERTY(L"Kits.PublishingOrganization", L"Microsoft Corporation") \
    TEST_METHOD_PROPERTY(L"Kits.DevelopmentPhase", L"Development and Integration") \
    TEST_METHOD_PROPERTY(L"Kits.HasSupplementalContent", L"False") \

#define APO_TEST_PROPERTIES \
    TEST_METHOD_PROPERTY(L"Kits.Specification", L"Device.Audio.APO.ThirdParty") \

#define START_APPVERIFIFER \
    TEST_CLASS_PROPERTY(L"Kits.TestTask", L"AppVerifierStart") \
    TEST_CLASS_PROPERTY(L"Kits.TestTask.AppVerifierStart.Stage", L"Setup") \
    TEST_CLASS_PROPERTY(L"Kits.TestTask.AppVerifierStart.RunElevated", L"True") \
    TEST_CLASS_PROPERTY(L"Kits.TestTask.AppVerifierStart.CommandLine", L"cmd.exe /c appverif -enable exceptions handles heaps leak locks memory srwlock threadpool tls -for te.processhost.exe te.exe audiodg.exe") \
    TEST_CLASS_PROPERTY(L"Kits.TestTask.AppVerifierStart.Order", L"1") \
    TEST_CLASS_PROPERTY(L"Kits.TestTask.AppVerifierStart.ExpectedExitCode", L"0") \

#define END_APPVERIFIER \
    TEST_CLASS_PROPERTY(L"Kits.TestTask", L"AppVerifierEnd") \
    TEST_CLASS_PROPERTY(L"Kits.TestTask.AppVerifierEnd.Stage", L"Cleanup") \
    TEST_CLASS_PROPERTY(L"Kits.TestTask.AppVerifierEnd.RunElevated", L"True") \
    TEST_CLASS_PROPERTY(L"Kits.TestTask.AppVerifierEnd.CommandLine", L"cmd.exe /c appverif -disable exceptions handles heaps leak locks memory srwlock threadpool tls -for te.processhost.exe te.exe audiodg.exe") \
    TEST_CLASS_PROPERTY(L"Kits.TestTask.AppVerifierEnd.Order", L"1") \
    TEST_CLASS_PROPERTY(L"Kits.TestTask.AppVerifierEnd.ExpectedExitCode", L"0") \

#define START_OSUPGRADE \
    TEST_CLASS_PROPERTY(L"Kits.TestTask", L"OSUpgrade") \
    TEST_CLASS_PROPERTY(L"Kits.TestTask.OSUpgrade.Stage", L"Setup") \
    TEST_CLASS_PROPERTY(L"Kits.TestTask.OSUpgrade.RunElevated", L"True") \
    TEST_CLASS_PROPERTY(L"Kits.TestTask.OSUpgrade.CommandLine", L"cmd.exe /c c:\\HLK\\ISO\\setup.exe /auto:upgrade /compat IgnoreWarning /noreboot") \
    TEST_CLASS_PROPERTY(L"Kits.TestTask.OSUpgrade.Order", L"1") \
    TEST_CLASS_PROPERTY(L"Kits.TestTask.OSUpgrade.ExpectedExitCode", L"0") \
    TEST_CLASS_PROPERTY(L"Kits.TestTask.OSUpgrade.TimeoutInMinutes", L"240") \

#define START_APPVERIFIFER_UPGRADE \
    TEST_CLASS_PROPERTY(L"Kits.TestTask", L"AppVerifierStart") \
    TEST_CLASS_PROPERTY(L"Kits.TestTask.AppVerifierStart.Stage", L"Setup") \
    TEST_CLASS_PROPERTY(L"Kits.TestTask.AppVerifierStart.RunElevated", L"True") \
    TEST_CLASS_PROPERTY(L"Kits.TestTask.AppVerifierStart.CommandLine", L"cmd.exe /c appverif -enable exceptions handles heaps leak locks memory srwlock threadpool tls -for te.processhost.exe te.exe audiodg.exe") \
    TEST_CLASS_PROPERTY(L"Kits.TestTask.AppVerifierStart.Order", L"2") \
    TEST_CLASS_PROPERTY(L"Kits.TestTask.AppVerifierStart.ExpectedExitCode", L"0") \

// End Metadata defines
