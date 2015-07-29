AVStream filter-centric simulated capture sample driver (Avssamp)
=================================================================

The AVStream filter-centric simulated capture sample driver (Avssamp) provides a filter-centric [AVStream](http://msdn.microsoft.com/en-us/library/windows/hardware/ff554240) capture driver with functional audio. This streaming media driver performs video captures at 320 x 240 pixel resolution in RGB24 or YUV422 format while playing a user-provided Pulse Code Modulation (PCM) wave audio file in a loop. The sample demonstrates how to write a filter-centric AVStream minidriver.


Installation instructions
-------------------------

1.  Copy AVssamp.inf to a directory, for example, C:\\Avstream\\.
2.  In this directory, create a new subdirectory named objfre\_x86 if the target operating system is x86-based, or objfre\_amd64 for an x64-based target operating system, for example, C:\\AVstream\\objfre\_x86\\.
3.  Copy the processor-appropriate Avssamp.sys file to the objfre\_\* directory.
4.  Start a command prompt with administrator privilege and run the processor-specific WDK tool Devcon.exe to launch the installation. For example:

    `C:\WinDDK\7600.16384.0\tools\devcon\i386\devcon.exe install C:\AVstream\avssamp.inf SW\{20698827-7099-4c4e-861A-4879D639A35F}`

Programming Tour
----------------

[**DriverEntry**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff558717) in Avssamp.cpp is the initial point of entry into the driver. This routine passes control to AVStream by calling [**KsInitializeDriver**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff562683). In this call, the minidriver passes the device descriptor, an AVStream structure that recursively defines the AVStream object hierarchy for a driver. This is common behavior for an AVStream minidriver.

Filter.cpp is where the sample lays out the [**KSPIN\_DESCRIPTOR\_EX**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff563534) structure for the single capture pin. Audio.cpp contains the **KSPIN\_DESCRIPTOR\_EX** structure for the audio capture pin. This pin is dynamically created only if C:\\avssamp.wav exists and is a valid and readable PCM format wave file.

The filter dispatch structure [**KSFILTER\_DISPATCH**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff562554) in Filter.cpp provides dispatches to create and process data. The **DispatchProcess** method is defined inline in Filter.h. It calls the **Process** method in Filter.cpp in the context of the **CCaptureFilter** class. Be aware that the process dispatch is provided in **KSFILTER\_DISPATCH** because this sample is filter-centric.

Audio.cpp lays out a [**KSPIN\_DISPATCH**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff563535) pin dispatch structure, which contains the dispatch table for the audio pin. Be aware that the **Process** member of this structure is **NULL** because the sample is filter-centric. Similarly, Video.cpp contains the **KSPIN\_DISPATCH** structure for the video capture pin, again with the **Process** member set to **NULL**.

For more information, see the comments in all .cpp files.

Code tour
---------

**File manifest**

<table>
<colgroup>
<col width="50%" />
<col width="50%" />
</colgroup>
<thead>
<tr class="header">
<th align="left">File
Description</th>
</tr>
</thead>
<tbody>
<tr class="odd">
<td align="left">Audio.cpp
<p>Audio capture pin implementation</p></td>
<td align="left">Audio.h
<p>Header file for Audio.cpp</p></td>
</tr>
</tbody>
</table>


