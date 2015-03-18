The sample code exercises both PsSetCreateProcessNotifyRoutineEx() and ObRegisterCallbacks().
These routines were introduced in Vista SP1 and are present in Windows7.  They are available in both 32bit OS and 64bit OS.
The first example uses ObRegisterCallbacks() and a callback to restrict requested access rights during a open process action.
The second example uses PsSetCreateProcessNotifyRoutineEx() to reject a process creation by examining the command line.

The code once compiled produces two files:   ObCallbackTest.sys and ObCallbackTestCtrl.exe

It is important to change the names of the binaries in the sample code to be unique for your own use.
#define TD_DRIVER_NAME             L"ObCallbackTest"
#define TD_DRIVER_NAME_WITH_EXT    L"ObCallbackTest.sys"

#define TD_NT_DEVICE_NAME          L"\\Device\\ObCallbackTest"
#define TD_DOS_DEVICES_LINK_NAME   L"\\DosDevices\\ObCallbackTest"
#define TD_WIN32_DEVICE_NAME       L"\\\\.\\ObCallbackTest"




For running the code you can use (run as administrator):

  C:\> obcallbacktest.exe  -?          (for command line help)
  C:\> obcallbacktest.exe  -install          (installs the kernel driver)
  C:\> obcallbacktest.exe  -name  notepad          (specifies that the string “notepad”  will be watched as a protected executable)

  (now you can start up  “notepad.exe”)
  C:\> notepad

  (locate the process ID of notepad.exe)
  C:\>  tlist

  (attempt to kill off the notepad.exe with a PID of 2329)
  C:\>  kill –f  2329
  process notepad.exe (2329) – ‘Untitled – Notepad’ could not be killed

  (remove the protections on the notepad process)
  C:\> obcallbacktest.exe  -deprotect          

  (attempt to kill off the process – which will succeed)
  C:\>  kill –f  2329

   (uninstall the kernel driver)
 C:\> obcallbacktest.exe  -uninstall          
 
Another sample test you can run is to prevent a process from being created

  C:\> obcallbacktest.exe  -install          (installs the kernel driver)
  C:\> obcallbacktest.exe  -reject  notepad          (specifies that the string “notepad”  will be watched and prevented from starting as a process)

  (now you can start up  “notepad.exe”)
  C:\> notepad
  Access is denied.  


Use this sample code at your own risk; there is no support from Microsoft for the sample code.  In addition, this sample code is licensed to you under the terms of the Microsoft Public License (http://www.microsoft.com/opensource/licenses.mspx).

May 2009

