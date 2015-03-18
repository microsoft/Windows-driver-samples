
#define DRIVER_FUNC_INSTALL     0x01
#define DRIVER_FUNC_REMOVE      0x02
#define DRIVER_FUNC_STOP        0x03

#define DRIVER_NAME       _T("tracedrv")

BOOLEAN
ManageDriver(
    IN LPCTSTR  DriverName,
    IN LPCTSTR  ServiceName,
    IN USHORT   Function
    );

