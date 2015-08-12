ObCallback Callback Registration Driver
=======================================

The ObCallback sample driver demonstrates the use of registered callbacks for process protection. The driver registers control callbacks which are called at process creation.


Design and Operation
--------------------

The sample exercises both the [**PsSetCreateProcessNotifyRoutineEx**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff559951) and the [**ObRegisterCallbacks**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff558692) routines. The first example uses the **ObRegisterCallbacks** routine and a callback to restrict requested access rights during a open process action. The second example uses the **PsSetCreateProcessNotifyRoutineEx** routine to reject a process creation by examining the command line.

The following is a command line usage scenario to exercise access restriction:

```
C:\> obcallbacktestctrl.exe  -?                      (for command line help)
C:\> obcallbacktestctrl.exe  -install                (installs the kernel driver)
C:\> obcallbacktestctrl.exe  -name  notepad          (specifies that the string "notepad"  will be watched as a protected executable)
                                                     (now you can start up "notepad.exe")
C:\> notepad

C:\> tlist                                           (locate the process ID of notepad.exe)

C:\> kill -f  2329                                   (attempt to kill off the notepad.exe with a PID of 2329)
process notepad.exe (2329) - 'Untitled - Notepad' could not be killed

C:\> obcallbacktestctrl.exe  -deprotect              (remove the protections on the notepad process)

C:\> kill -f  2329                                   (attempt to kill off the process - which will succeed)
C:\> obcallbacktestctrl.exe  -uninstall              (uninstall the kernel driver)
```

The following is another sample test you can run to prevent a process from being created:

```
C:\> obcallbacktestctrl.exe  -install                (installs the kernel driver)
C:\> obcallbacktestctrl.exe  -reject  notepad        (specifies that the string "notepad"  will be watched and prevented from starting as a process)

C:\> notepad                                         (now you can start up "notepad.exe")
Access is denied.
```
