#pragma once

//
// Sample test service
// {4AFA2AF5-0912-407B-8B12-AFFF4047672A}
//
DEFINE_GUID(GUID_SAMPLE_TEST_SERVICE,
    0x4afa2af5, 0x912, 0x407b, 0x8b, 0x12, 0xaf, 0xff, 0x40, 0x47, 0x67, 0x2a);

//
// Writes classic L"Hello, world!" message to output buffer.
//
#define TEST_SERVICE_HELLO_WORLD            1

//
// Writes 64-bit timestamp returned from KeQueryInterruptTimePrecise to output
// buffer.
//
#define TEST_SERVICE_GET_INTERRUPT_TIME     2

//
// A request that is only available from kernel mode
//
// Input: None
// Output: None
#define TEST_SERVICE_KERNEL_ONLY            3

//
// The request will be completed asynchronously after given delay
//
// Input: ULONG (delay in msec)
// Output: ULONG (0x12345678)
#define TEST_SERVICE_DELAYED_COMPLETION     4

//
// Sample test service consuming sample OS service
// {D69482F9-7347-431A-8409-F625BEB3469D}
//
DEFINE_GUID(GUID_SAMPLE_TEST2_SERVICE, 
    0xd69482f9, 0x7347, 0x431a, 0x84, 0x9, 0xf6, 0x25, 0xbe, 0xb3, 0x46, 0x9d);

//
// Copies contents of input buffer to output buffer
//
#define TEST2_SERVICE_ECHO                  1

//
// Copies contents of input buffer to output buffer twice, reversed
//
#define TEST2_SERVICE_ECHO_TWICE_REVERSED   2

//
// Other service I/O example
//

//
// The request will be complete asynchronously after 1 second
// Input : NULL-terminated WCHAR[], size includes the terminating NULL character
// Output : None
//
#define IOCTL_TEST_DELAYED_COMPLETION   CTL_CODE(FILE_DEVICE_TRUST_ENV, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// Other device I/O example
//

//
// Prints a string to debugger through DbgPrintEx
// Input : NULL-terminated WCHAR[], size includes the terminating NULL character
// Output : None
//
#define IOCTL_SAMPLE_DBGPRINT           CTL_CODE(FILE_DEVICE_TRUST_ENV, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)