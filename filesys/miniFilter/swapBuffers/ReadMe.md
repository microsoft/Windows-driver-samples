SwapBuffer File System Minifilter Driver
========================================

The SwapBuffers minifilter demonstrates how to switch buffers between reads and writes of data. This technique is particularly useful for encryption filters because they have to encrypt data before writing it to disk and decrypt it after reading it from disk. Because encryption/decryption has to be done transparently, you cannot use system-supplied buffers directly, so intermediate buffers have to be introduced.

## Universal Windows Driver Compliant
This sample builds a Universal Windows Driver. It uses only APIs and DDIs that are included in OneCoreUAP.

Design and Operation
--------------------

The *SwapBuffers* minifilter introduces a new buffer before a read/write or directory control operations. The corresponding operation is then performed on the new buffer instead of the buffer that was originally provided. After the operation completes, the contents of the new buffer are copied back in to the original buffer.

For more information on file system minifilter design, start with the [File System Minifilter Drivers](http://msdn.microsoft.com/en-us/library/windows/hardware/ff540402) section in the Installable File Systems Design Guide.

