// ------------------------------------------------------------------------------
//
// Copyright (C) Microsoft. All rights reserved.
//
// Module Name:
//
//  tests.h
//
// Abstract:
//
//  Header for testcases
//
// -------------------------------------------------------------------------------
#pragma once

// Stream test
void Test_StreamStateControl(void);
void Test_LoopbackStreamStateControl(void);
void Test_StreamMinBufferSize(void);
void Test_StreamMaxBufferSize(void);
void Test_StreamBufferSize(HNSTIME requestedPeriodicity);
void Test_StreamDifferentFormat(void);
void Test_LoopbackStreamDifferentFormat(void);
void Test_StreamMultipleModes(void);

// Compliance test
void Test_VerifyPinWaveRTConformance(void);