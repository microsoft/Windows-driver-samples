#pragma once

//
// Echo service
// GUID {33C7FF13-50B0-454A-8BEB-F73EED6C0AF9}
//
DEFINE_GUID(GUID_ECHO_SERVICE,
    0x33c7ff13, 0x50b0, 0x454a, 0x8b, 0xeb, 0xf7, 0x3e, 0xed, 0x6c, 0xa, 0xf9);

//
// Copies the content of input buffer to output buffer
//
#define ECHO_SERVICE_ECHO                   1

//
// Repeats the content of input buffer to output buffer
//
#define ECHO_SERVICE_REPEAT                 2

//
// Copies the bytes of input buffer to output buffer in reverse order
//
#define ECHO_SERVICE_REVERSE                3

//
// Kernel memory service
// {D28698A4-3B07-4F34-B65E-AE3DA6ACF2AC}
// 
DEFINE_GUID(GUID_KERNEL_MEMORY_SERVICE,
    0xd28698a4, 0x3b07, 0x4f34, 0xb6, 0x5e, 0xae, 0x3d, 0xa6, 0xac, 0xf2, 0xac);

//
// Get a safe range of kernel address space that can be read from
//
// Input: None
// Output: KERNEL_MEMORY_SAFE_RANGE
//

typedef struct _KERNEL_MEMORY_SAFE_RANGE {
    ULONG64 Base;
    ULONG Length;
} KERNEL_MEMORY_SAFE_RANGE, *PKERNEL_MEMORY_SAFE_RANGE;

#define KERNEL_MEMORY_SERVICE_GET_SAFE_RANGE 1

//
// Write a byte to kernel address space
//
// Input: KERNEL_MEMORY_WRITE_BYTE
// Output: Previous UCHAR value at the address
//

typedef struct _KERNEL_MEMORY_WRITE_BYTE {
    ULONG64 Address;
    UCHAR Value;
} KERNEL_MEMORY_WRITE_BYTE, *PKERNEL_MEMORY_WRITE_BYTE;

#define KERNEL_MEMORY_SERVICE_WRITE_BYTE    2

//
// Read a byte from kernel address space
//
// Input: ULONG64 address of kernel memory to read
// Output: UCHAR
//
#define KERNEL_MEMORY_SERVICE_READ_BYTE     3