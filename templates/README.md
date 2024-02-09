Windows Driver Kit Templates.

The WDK.vsix contains 17 templates.  They are listed beneath.  

After installation of VSIX they can be found under: 
  C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\Extensions\mjmwh1sj.nyr\ProjectTemplates\Windows Drivers\

This folder is made by manually inspecting all these templates, then running following to remove .vs folder: 
  for /F %i in ('dir /s /b /A:DH .vs') do rmdir /s /q %i
  
Applications\EmptyApplication: Empty Desktop Application for Drivers (Universal)
Applications\EmptyDll: Empty DLL for Drivers (Universal) <-- This is not marked in VS as "Project Type=Driver".
Applications\EmptyStatic: Empty Static Library for Drivers (Universal)
Applications\WinUsbApp: WinUSB Application (Universal)
Devices\KmdfUsb: Kernel Mode Driver, USB (KMDF)
Devices\NdisFilter: Filter Driver: NDIS
Devices\Umdf2Usb: User Mode Driver, USB (UMDF V2)
Devices\v4PrintDriver: Printer Driver V4
Devices\XpsRenderFilter: Printer XPS Render Filter
Legacy\WDMEmpty: Empty WDM Driver
Package\DriverPackage: Driver Install Package
Package\PrinterDriverPackage: Printer Driver Install Package <-- This is listed but does not show in VS New Project... dialog
Package\WinUsbInfPackage: WinUSB INF Driver Package
WDF\KMDF: Kernel Mode Driver (KMDF)
WDF\KMDFEmpty: Kernel Mode Driver, Empty (KMDF)
WDF\UMDF2: User Mode Driver (UMDF V2)
WDF\UMDF2Empty: User Mode Driver, Empty (UMDF V2)
