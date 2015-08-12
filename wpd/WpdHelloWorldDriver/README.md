WPDHelloWorld sample driver for portable devices
================================================

The WpdHelloWorld sample driver supports four objects: a device object, a storage object, a folder object, and a file object. Each object supports corresponding properties. These properties are defined in the file WpdObjectProperties.h.

The sample driver supports a device object that exposes ten read-only properties. These properties, their types, and their values are listed in the following table.

Property name | Property type | Value 
--------------|---------------|------
DEVICE_PROTOCOL | String | "Hello World Protocol ver 1.00" 
DEVICE_FIRMWARE_VERSION | String | "1.0.0.0" 
DEVICE_POWER_LEVEL | Integer | 100 
DEVICE_MODEL | String | "Hello World!" 
DEVICE_MANUFACTURER | String | "Windows Portable Devices Group" 
DEVICE_FRIENDLY | String | "Hello World!" 
DEVICE_SERIAL_NUMBER | String | "01234567890123-45676890123456" 
DEVICE_SUPPORTS_NONCONSUMABLE | Bool | True 
WPD_DEVICE_TYPE | Integer | WPD_DEVICE_TYPE_GENERIC 
WPD_FUNCTIONAL_OBJECT_CATEGORY | GUID | WPD_FUNCTIONAL_CATEGORY_STORAGE 

The driver supports a storage object that exposes seven read-only properties. These properties, their types, and their values are listed in the following table.

Property name | Property type | Value 
--------------|---------------|------
STORAGE_CAPACITY | 64-bit Integer | 1024 * 1024  
STORAGE_FREE_SPACE_IN_BYTES | 64-bit Integer | 1024 * 1024 
STORAGE_SERIAL_NUMBER | String | 98765432109876-54321098765432  
STORAGE_FILE_SYSTEM_TYPE | String | FAT32  
STORAGE_DESCRIPTION | String | Hello World! Memory Storage System  
WPD_STORAGE_TYPE | Integer | WPD_STORAGE_TYPE_FIXED_ROM 
WPD_FUNCTIONAL_OBJECT_CATEGORY | GUID | WPD_FUNCTIONAL_CATEGORY_STORAGE 

The driver supports a folder object that exposes three read-only properties. These properties, their types, and their values are listed in the following table.

Property name | Property type | Value 
--------------|---------------|------
WPD_OBJECT_DATE_MODIFIED | Date | 2006/6/26 5:0:0.0 
WPD_OBJECT_DATE_CREATED | Date | 2006/1/25 12:0:0.0 
WPD_OBJECT_ORIGINAL_FILE_NAME_VALUE | String | Documents  

The driver supports a file object that exposes three read-only properties. These properties, their types, and their values are listed in the following table.

Property name | Property type | Value 
--------------|---------------|------
WPD_OBJECT_DATE_MODIFIED | Date | 2006/6/26 5:0:0.0 
WPD_OBJECT_DATE_CREATED | Date | 2006/1/25 12:0:0.0  
WPD_OBJECT_ORIGINAL_FILE_NAME | String | Readme.txt 

In addition to the above properties, every object (for example, device, storage, folder, or file) also supports seven common WPD object properties. These are read-only properties that contain object-specific values for the most part. These properties, their types, and their values are listed in the following table.

Property name | Property type | Value 
--------------|---------------|------
WPD_OBJECT_ID | String | Object-specific  
WPD_OBJECT_PERSISTENT_UNIQUE_ID | String | Object-specific 
WPD_OBJECT_PARENT_ID | String | Object-specific  
WPD_OBJECT_NAME | String | Object-specific  
WPD_OBJECT_FORMAT | GUID | Object-specific  
WPD_OBJECT_CONTENT_TYPE | GUID | Object-specific 
WPD_OBJECT_CAN_DELETE | Bool | False 

For a complete description of this sample and its underlying code and functionality, refer to the [WPD HelloWorld Driver](http://msdn.microsoft.com/en-us/library/windows/hardware/) description in the Windows Driver Kit documentation.

Related topics
--------------

[WPD Design Guide](http://msdn.microsoft.com/en-us/library/windows/hardware/ff597864)

[WPD Driver Development Tools](http://msdn.microsoft.com/en-us/library/windows/hardware/ff597568)

[WPD Programming Guide](https://msdn.microsoft.com/en-us/library/windows/hardware/ff597898)
