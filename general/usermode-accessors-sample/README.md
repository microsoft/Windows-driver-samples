---
page_type: sample
description: "A KMDF driver demonstrating safe user-mode memory access using the usermode_accessors.h DDI family."
languages:
- c
products:
- windows-wdk
---

# Usermode Accessors Sample Driver

This sample demonstrates how to safely access user-mode memory from a kernel-mode driver using the `usermode_accessors.h` header. These APIs provide type-safe, mode-aware wrappers for reading, writing, copying, and performing atomic operations on user-mode memory buffers.

## DDI Functions Exercised

### Copy Operations
| DDI | Description |
|-----|-------------|
| `CopyFromUser` | Safely copy data from user-mode to kernel memory |
| `CopyToUser` | Safely copy data from kernel to user-mode memory |
| `CopyFromMode` | Copy from mode-dependent memory (user or kernel) |
| `CopyToMode` | Copy to mode-dependent memory |

### Typed Read Operations (user-mode)
| DDI | Description |
|-----|-------------|
| `ReadUCharFromUser` | Safely read a UCHAR from user memory |
| `ReadUShortFromUser` | Safely read a USHORT from user memory |
| `ReadULongFromUser` | Safely read a ULONG from user memory |
| `ReadULong64FromUser` | Safely read a ULONG64 from user memory |
| `ReadBooleanFromUser` | Safely read a BOOLEAN from user memory |
| `ReadHandleFromUser` | Safely read a HANDLE from user memory |

### Typed Write Operations (user-mode)
| DDI | Description |
|-----|-------------|
| `WriteUCharToUser` | Safely write a UCHAR to user memory |
| `WriteUShortToUser` | Safely write a USHORT to user memory |
| `WriteULongToUser` | Safely write a ULONG to user memory |
| `WriteULong64ToUser` | Safely write a ULONG64 to user memory |
| `WriteBooleanToUserRelease` | Safely write a BOOLEAN with release semantics |

### Typed Mode Operations
| DDI | Description |
|-----|-------------|
| `ReadULongFromMode` | Read ULONG based on processor mode |
| `WriteULongToMode` | Write ULONG based on processor mode |

### Structure Operations
| DDI | Description |
|-----|-------------|
| `ReadStructFromUser` | Safely read a complete struct from user memory |
| `WriteStructToUser` | Safely write a complete struct to user memory |

### Memory Fill Operations
| DDI | Description |
|-----|-------------|
| `FillUserMemory` | Fill user-mode memory with a byte value |

### Interlocked Operations (user-mode)
| DDI | Description |
|-----|-------------|
| `InterlockedAndToUser` | Atomic AND on 32-bit user-mode value |
| `InterlockedOrToUser` | Atomic OR on 32-bit user-mode value |
| `InterlockedCompareExchangeToUser` | Atomic CAS on 32-bit user-mode value |
| `InterlockedAnd64ToUser` | Atomic AND on 64-bit user-mode value |
| `InterlockedOr64ToUser` | Atomic OR on 64-bit user-mode value |
| `InterlockedCompareExchange64ToUser` | Atomic CAS on 64-bit user-mode value |

### String Operations
| DDI | Description |
|-----|-------------|
| `StringLengthFromUser` | Safely compute ANSI string length in user memory |
| `WideStringLengthFromUser` | Safely compute wide string length in user memory |

### Exception Handling
| DDI | Description |
|-----|-------------|
| `UmaExceptionFilter` | Mode-dependent exception filtering for SEH |

## Why Use usermode_accessors.h?

Traditional approaches to accessing user-mode memory in kernel drivers involve manual `ProbeForRead`/`ProbeForWrite` calls wrapped in `__try/__except` blocks. The `usermode_accessors.h` APIs provide:

1. **Type safety**: Each Read/Write function is typed for a specific data type, preventing size mismatches.
2. **Mode awareness**: The `XxxFromMode`/`XxxToMode` variants automatically validate pointers based on the processor mode, useful for drivers that handle both kernel and user-mode callers.
3. **Acquire/Release semantics**: Variants like `ReadULongFromUserAcquire` and `WriteULongToUserRelease` provide memory ordering guarantees for lock-free data sharing.
4. **Non-temporal access**: `CopyFromUserNonTemporal` and `CopyToUserNonTemporal` use non-temporal store instructions for large buffer copies, avoiding cache pollution.
5. **Built-in exception handling**: All functions include proper structured exception handling internally.

## Building the Sample

1. Open the solution in Visual Studio with the WDK installed.
2. Select the target configuration (Debug/Release) and platform (x64/ARM64).
3. Build the solution.

## Installing the Sample

```cmd
devcon install usermode_accessors_sample.inf Root\UmaAccessorsSample
```

## Testing

Use a user-mode test application that opens the device interface `{B4E6A7C8-3D2F-4A1E-9F5B-8C7D6E5F4A3B}` and sends the defined IOCTLs with appropriate input/output buffers.

## Documentation

- [usermode_accessors.h reference](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/usermode_accessors/)
