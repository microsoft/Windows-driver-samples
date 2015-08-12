Scanner File System Minifilter Driver
=====================================

The Scanner minifilter is an example for developers who intend to write filters that examine data in files. Typically, antivirus products fall into this category.

## Universal Windows Driver Compliant
This sample builds a Universal Windows Driver. It uses only APIs and DDIs that are included in OneCoreUAP.

Design and Operation
--------------------

The Scanner minifilter comprises both kernel-mode and user-mode components. The kernel-mode component recognizes appropriate moments for scanning a file's data and passes it to the user-mode component for further validation. The user-mode component creates a number of threads that await validation requests and corresponding data from the kernel-mode component. After scanning the data for occurrences of a "foul" string, the user-mode component sends an appropriate response to the kernel-mode component.

The kernel-mode component scans files with specific extensions only. The file is first scanned on a successful open. If the file was opened with write access, it is scanned again before a close. Scanning is also performed on data that is about to be written to a file. Writes will be rejected if any occurrences of a "foul" string are found in the data. If a "foul" string is detected during the closing of a file, a debug message is printed.

For more information on file system minifilter design, start with the [File System Minifilter Drivers](http://msdn.microsoft.com/en-us/library/windows/hardware/ff540402) section in the Installable File Systems Design Guide.

