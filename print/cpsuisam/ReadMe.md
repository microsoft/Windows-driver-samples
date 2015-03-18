Common Property Sheet UI Sample
===============================

The CPSUISAM application causes the Common Property Sheet User Interface (CPSUI) to call the Windows print spooler to create property sheet pages for the system's default printer.

Printer interface DLLs must not perform this action and this sample shows how create property sheet pages for a printer.

The application then creates an additional property sheet page to illustrate some of the techniques that you can use when you are using CPSUI to create a new page.

CPSUI is a user-mode DLL that enables you to create property sheet pages that have a standard appearance.

CPSUIAM causes CPSUI to call the Windows print spooler to create property sheet pages for the system's default printer. The application then creates an additional property sheet page to illustrate some of the techniques that you can use when you are using CPSUI to create a new page. For more information, see [Common Property Sheet User Interface](http://msdn.microsoft.com/en-us/library/windows/hardware/ff546163(v=vs.85).aspx).

