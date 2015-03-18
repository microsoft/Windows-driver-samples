/*++

Copyright (c) Microsoft Corporation. All rights reserved

Abstract:

    Monitor Sample callout driver IOCTL header

Environment:

    Kernel mode
    
--*/

#pragma once

// b3241f1d-7cd2-4e7a-8721-2e97d07702e5
DEFINE_GUID(
    MONITOR_SAMPLE_SUBLAYER,
    0xb3241f1d,
    0x7cd2,
    0x4e7a,
    0x87, 0x21, 0x2e, 0x97, 0xd0, 0x77, 0x02, 0xe5
);

// 3aaccbc0-2c29-455f-bb91-0e801c8994a4
DEFINE_GUID(
    MONITOR_SAMPLE_FLOW_ESTABLISHED_CALLOUT_V4,
    0x3aaccbc0,
    0x2c29,
    0x455f,
    0xbb, 0x91, 0x0e, 0x80, 0x1c, 0x89, 0x94, 0xa4
);

// cea0131a-6ed3-4ed6-b40c-8a8fe8434b0a
DEFINE_GUID(
    MONITOR_SAMPLE_STREAM_CALLOUT_V4,
    0xcea0131a,
    0x6ed3,
    0x4ed6,
    0xb4, 0x0c, 0x8a, 0x8f, 0xe8, 0x43, 0x4b, 0x0a
);


