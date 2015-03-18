EventTracing SystemTraceProvider control sample
====================================================================================
This sample demonstrates how to use event tracing control API's to collect events
from system trace provider. The code provided will start an ETW system trace and
enable system events with stacks. After collecting the data for 30 seconds trace
will be stopped. Resulting file (systemtrace.etl) can be processed with
inbox tracerpt.exe, programmatically (OpenTrace/ProcessTrace/CloseTrace) or using
WPT (Windows Performance Toolkit) available in the SDK.

Sample Language Implementations
===============================
C++

Files
=================================================
SystemTraceProvider.sln
SystemTraceProvider.vcxproj
SystemTraceProvider.cpp
sources
ReadMe.txt

To build the sample using the command prompt:
=============================================
     1. Open the Command Prompt window and navigate to the  directory.
     2. Type msbuild SystemTraceControl.sln.

To build the sample using Visual Studio (preferred method):
================================================
     1. Open File Explorer and navigate to the  SystemTraceControl directory.
     2. Double-click the icon for the .sln (solution) file to open the file in Visual Studio.
     3. In the Build menu, select Build Solution. The application will be built in the default \Debug or \Release directory.

To run the sample:
=================
     1. Navigate to the directory that contains the new executable, using the command prompt or File Explorer.
     2. Type SystemTraceControl.exe at the command line, or double-click the icon for SystemTraceControl.exe to launch it from File Explorer.