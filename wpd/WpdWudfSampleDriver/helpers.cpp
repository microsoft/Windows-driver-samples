#include "stdafx.h"
#include "helpers.tmh"

// Define the value for the registered WAVE format this driver uses
#ifndef WAVE_FORMAT_MSAUDIO3
  #define WAVE_FORMAT_MSAUDIO3 0x0162
#endif

#define VCARD_FORMAT "BEGIN:VCARD\r\nVERSION:2.1\r\nN:%ws;%ws\r\nFN:%ws\r\nORG:%ws\r\nTITLE:%ws\r\nTEL;HOME;VOICE:%ws\r\nTEL;WORK;VOICE:%ws\r\nTEL;CELL;VOICE:%ws\r\nTEL;WORK;FAX:%ws\r\nADR;HOME;ENCODING=QUOTED-PRINTABLE:;;%ws=0D=0A%ws;%ws;,;%ws;;REV:20051206T185151Z\r\nEND:VCARD\r\n"

const PROPERTYKEY* g_SupportedPropertiesForFormatAll[] =
{
    &WPD_OBJECT_ID,
    &WPD_OBJECT_PERSISTENT_UNIQUE_ID,
    &WPD_OBJECT_PARENT_ID,
    &WPD_OBJECT_NAME,
    &WPD_OBJECT_CONTENT_TYPE,
    &WPD_OBJECT_FORMAT,
    &WPD_OBJECT_CAN_DELETE,
    &WPD_OBJECT_ISHIDDEN,
    &WPD_OBJECT_ISSYSTEM,
    &WPD_OBJECT_NON_CONSUMABLE,
};

const PROPERTYKEY* g_SupportedPropertiesForFakeContentFormat[] =
{
    &WPD_OBJECT_ID,
    &WPD_OBJECT_PERSISTENT_UNIQUE_ID,
    &WPD_OBJECT_PARENT_ID,
    &WPD_OBJECT_NAME,
    &WPD_OBJECT_CONTENT_TYPE,
    &WPD_OBJECT_FORMAT,
    &WPD_OBJECT_CAN_DELETE,
    &WPD_OBJECT_ISHIDDEN,
    &WPD_OBJECT_ISSYSTEM,
    &WPD_FOLDER_CONTENT_TYPES_ALLOWED,
    &WPD_OBJECT_ORIGINAL_FILE_NAME,
    &WPD_OBJECT_NON_CONSUMABLE,
};

const PROPERTYKEY* g_SupportedPropertiesForFakeDeviceContentFormat[] =
{
    &WPD_OBJECT_ID,
    &WPD_OBJECT_PERSISTENT_UNIQUE_ID,
    &WPD_OBJECT_PARENT_ID,
    &WPD_OBJECT_NAME,
    &WPD_OBJECT_CONTENT_TYPE,
    &WPD_OBJECT_FORMAT,
    &WPD_OBJECT_CAN_DELETE,
    &WPD_OBJECT_ISHIDDEN,
    &WPD_OBJECT_ISSYSTEM,
    &WPD_DEVICE_SUPPORTS_NON_CONSUMABLE,
    &WPD_OBJECT_NON_CONSUMABLE,
    &WPD_FUNCTIONAL_OBJECT_CATEGORY,
    &WPD_DEVICE_FIRMWARE_VERSION,
    &WPD_DEVICE_POWER_LEVEL,
    &WPD_DEVICE_POWER_SOURCE,
    &WPD_DEVICE_PROTOCOL,
    &WPD_DEVICE_MODEL,
    &WPD_DEVICE_SERIAL_NUMBER,
    &WPD_DEVICE_MANUFACTURER,
    &WPD_DEVICE_TYPE,
    &WPD_DEVICE_FRIENDLY_NAME,
};

const PROPERTYKEY* g_SupportedPropertiesForFakeStorageContentFormat[] =
{
    &WPD_OBJECT_ID,
    &WPD_OBJECT_PERSISTENT_UNIQUE_ID,
    &WPD_OBJECT_PARENT_ID,
    &WPD_OBJECT_NAME,
    &WPD_OBJECT_CONTENT_TYPE,
    &WPD_OBJECT_FORMAT,
    &WPD_OBJECT_CAN_DELETE,
    &WPD_OBJECT_ISHIDDEN,
    &WPD_OBJECT_ISSYSTEM,
    &WPD_FOLDER_CONTENT_TYPES_ALLOWED,
    &WPD_OBJECT_NON_CONSUMABLE,
    &WPD_STORAGE_CAPACITY,
    &WPD_STORAGE_FREE_SPACE_IN_BYTES,
    &WPD_FUNCTIONAL_OBJECT_CATEGORY,
    &WPD_STORAGE_TYPE,
};

const PROPERTYKEY* g_SupportedPropertiesForFakeImageContentFormat[] =
{
    &WPD_OBJECT_ID,
    &WPD_OBJECT_PERSISTENT_UNIQUE_ID,
    &WPD_OBJECT_PARENT_ID,
    &WPD_OBJECT_NAME,
    &WPD_OBJECT_CONTENT_TYPE,
    &WPD_OBJECT_FORMAT,
    &WPD_OBJECT_CAN_DELETE,
    &WPD_OBJECT_ISHIDDEN,
    &WPD_OBJECT_ISSYSTEM,
    &WPD_OBJECT_NON_CONSUMABLE,
    &WPD_MEDIA_HEIGHT,
    &WPD_MEDIA_WIDTH,
    &WPD_OBJECT_DATE_CREATED,
    &WPD_OBJECT_ORIGINAL_FILE_NAME,
    &WPD_OBJECT_SIZE,
};

const PROPERTYKEY* g_SupportedPropertiesForFakeMusicContentFormat[] =
{
    &WPD_OBJECT_ID,
    &WPD_OBJECT_PERSISTENT_UNIQUE_ID,
    &WPD_OBJECT_PARENT_ID,
    &WPD_OBJECT_NAME,
    &WPD_OBJECT_CONTENT_TYPE,
    &WPD_OBJECT_FORMAT,
    &WPD_OBJECT_CAN_DELETE,
    &WPD_OBJECT_ISHIDDEN,
    &WPD_OBJECT_ISSYSTEM,
    &WPD_OBJECT_NON_CONSUMABLE,
    &WPD_MEDIA_TITLE,
    &WPD_MEDIA_ARTIST,
    &WPD_MEDIA_DURATION,
    &WPD_OBJECT_SIZE,
    &WPD_OBJECT_DATE_AUTHORED,
    &WPD_OBJECT_DATE_MODIFIED,
    &WPD_MUSIC_ALBUM,
    &WPD_MEDIA_GENRE,
    &WPD_MUSIC_TRACK,
    &WPD_OBJECT_ORIGINAL_FILE_NAME,
};

const PROPERTYKEY* g_SupportedPropertiesForFakeVideoContentFormat[] =
{
    &WPD_OBJECT_ID,
    &WPD_OBJECT_PERSISTENT_UNIQUE_ID,
    &WPD_OBJECT_PARENT_ID,
    &WPD_OBJECT_NAME,
    &WPD_OBJECT_CONTENT_TYPE,
    &WPD_OBJECT_FORMAT,
    &WPD_OBJECT_CAN_DELETE,
    &WPD_OBJECT_ISHIDDEN,
    &WPD_OBJECT_ISSYSTEM,
    &WPD_OBJECT_NON_CONSUMABLE,
    &WPD_MEDIA_TITLE,
    &WPD_MEDIA_DURATION,
    &WPD_OBJECT_SIZE,
    &WPD_MEDIA_HEIGHT,
    &WPD_MEDIA_WIDTH,
    &WPD_OBJECT_DATE_AUTHORED,
    &WPD_OBJECT_DATE_MODIFIED,
    &WPD_OBJECT_ORIGINAL_FILE_NAME,
    &WPD_VIDEO_SCAN_TYPE,
    &WPD_VIDEO_BITRATE,
    &WPD_VIDEO_FOURCC_CODE,
    &WPD_OBJECT_GENERATE_THUMBNAIL_FROM_RESOURCE,
};

const PROPERTYKEY* g_SupportedPropertiesForFakeContactContentFormat[] =
{
    &WPD_OBJECT_ID,
    &WPD_OBJECT_PERSISTENT_UNIQUE_ID,
    &WPD_OBJECT_PARENT_ID,
    &WPD_OBJECT_NAME,
    &WPD_OBJECT_CONTENT_TYPE,
    &WPD_OBJECT_FORMAT,
    &WPD_OBJECT_CAN_DELETE,
    &WPD_OBJECT_ISHIDDEN,
    &WPD_OBJECT_ISSYSTEM,
    &WPD_OBJECT_NON_CONSUMABLE,
    &WPD_CONTACT_DISPLAY_NAME,
    &WPD_CONTACT_PRIMARY_PHONE,
    &WPD_CONTACT_MOBILE_PHONE,
    &WPD_CONTACT_BUSINESS_PHONE,
    &WPD_OBJECT_ORIGINAL_FILE_NAME,
    &WPD_OBJECT_SIZE,
};

const PROPERTYKEY* g_SupportedPropertiesForRenderingInformation[] =
{
    &WPD_OBJECT_ID,
    &WPD_OBJECT_PERSISTENT_UNIQUE_ID,
    &WPD_OBJECT_PARENT_ID,
    &WPD_OBJECT_NAME,
    &WPD_OBJECT_CONTENT_TYPE,
    &WPD_OBJECT_FORMAT,
    &WPD_OBJECT_CAN_DELETE,
    &WPD_OBJECT_ISHIDDEN,
    &WPD_OBJECT_ISSYSTEM,
    &WPD_OBJECT_NON_CONSUMABLE,
    &WPD_FUNCTIONAL_OBJECT_CATEGORY,
    &WPD_RENDERING_INFORMATION_PROFILES,
};

const PROPERTYKEY* g_SupportedPropertiesForNetworkConfiguration[] =
{
    &WPD_OBJECT_ID,
    &WPD_OBJECT_PERSISTENT_UNIQUE_ID,
    &WPD_OBJECT_PARENT_ID,
    &WPD_OBJECT_NAME,
    &WPD_OBJECT_CONTENT_TYPE,
    &WPD_OBJECT_FORMAT,
    &WPD_OBJECT_CAN_DELETE,
    &WPD_OBJECT_ISHIDDEN,
    &WPD_OBJECT_ISSYSTEM,
    &WPD_FOLDER_CONTENT_TYPES_ALLOWED,
    &WPD_OBJECT_NON_CONSUMABLE,
    &WPD_FUNCTIONAL_OBJECT_CATEGORY,
};

const PROPERTYKEY* g_SupportedPropertiesForNetworkAssociation[] =
{
    &WPD_OBJECT_ID,
    &WPD_OBJECT_PERSISTENT_UNIQUE_ID,
    &WPD_OBJECT_PARENT_ID,
    &WPD_OBJECT_NAME,
    &WPD_OBJECT_CONTENT_TYPE,
    &WPD_OBJECT_FORMAT,
    &WPD_OBJECT_CAN_DELETE,
    &WPD_OBJECT_ISHIDDEN,
    &WPD_OBJECT_ISSYSTEM,
    &WPD_OBJECT_NON_CONSUMABLE,
    &WPD_NETWORK_ASSOCIATION_HOST_NETWORK_IDENTIFIERS,
};


const PROPERTYKEY* g_SupportedPropertiesForMicrosoftWFC[] =
{
    &WPD_OBJECT_ID,
    &WPD_OBJECT_PERSISTENT_UNIQUE_ID,
    &WPD_OBJECT_PARENT_ID,
    &WPD_OBJECT_NAME,
    &WPD_OBJECT_CONTENT_TYPE,
    &WPD_OBJECT_FORMAT,
    &WPD_OBJECT_CAN_DELETE,
    &WPD_OBJECT_ISHIDDEN,
    &WPD_OBJECT_ISSYSTEM,
    &WPD_OBJECT_NON_CONSUMABLE,
};

const PROPERTYKEY* g_SupportedPropertiesForFakeMemoContentFormat[] =
{
    &WPD_OBJECT_ID,
    &WPD_OBJECT_PERSISTENT_UNIQUE_ID,
    &WPD_OBJECT_PARENT_ID,
    &WPD_OBJECT_NAME,
    &WPD_OBJECT_CONTENT_TYPE,
    &WPD_OBJECT_FORMAT,
    &WPD_OBJECT_CAN_DELETE,
    &WPD_OBJECT_ISHIDDEN,
    &WPD_OBJECT_ISSYSTEM,
    &WPD_OBJECT_NON_CONSUMABLE,
    &WPD_OBJECT_SIZE,
    &WPD_OBJECT_DATE_AUTHORED,
    &WPD_OBJECT_DATE_MODIFIED,
    &WPD_OBJECT_ORIGINAL_FILE_NAME,
};

KeyAndAttributesEntry g_FixedAttributesTable[] =
{
    // Properties for all objects, regardless of format
    {&WPD_OBJECT_FORMAT_ALL, &WPD_OBJECT_ID,                           UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_ALL, &WPD_OBJECT_PERSISTENT_UNIQUE_ID,         UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_ALL, &WPD_OBJECT_PARENT_ID,                    UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_ALL, &WPD_OBJECT_FORMAT,                       UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_ALL, &WPD_OBJECT_CONTENT_TYPE,                 UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_ALL, &WPD_OBJECT_CAN_DELETE,                   UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_ALL, &WPD_OBJECT_ISHIDDEN,                     UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_ALL, &WPD_OBJECT_ISSYSTEM,                     UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_ALL, &WPD_OBJECT_NON_CONSUMABLE,               UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    // Properties for generic objects
    {&FakeContent_Format, &WPD_OBJECT_ID,                           UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeContent_Format, &WPD_OBJECT_PERSISTENT_UNIQUE_ID,         UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeContent_Format, &WPD_OBJECT_PARENT_ID,                    UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeContent_Format, &WPD_OBJECT_NAME,                         UnspecifiedForm_CanRead_CanWrite_CannotDelete_Fast},
    {&FakeContent_Format, &WPD_OBJECT_FORMAT,                       UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeContent_Format, &WPD_OBJECT_CONTENT_TYPE,                 UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeContent_Format, &WPD_OBJECT_CAN_DELETE,                   UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeContent_Format, &WPD_OBJECT_ISHIDDEN,                     UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeContent_Format, &WPD_OBJECT_ISSYSTEM,                     UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeContent_Format, &WPD_FOLDER_CONTENT_TYPES_ALLOWED,        UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeContent_Format, &WPD_OBJECT_NON_CONSUMABLE,               UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeContent_Format, &WPD_OBJECT_ORIGINAL_FILE_NAME,           UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    // Properties for the device object
    {&FakeDeviceContent_Format, &WPD_OBJECT_ID,                           UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeDeviceContent_Format, &WPD_OBJECT_PERSISTENT_UNIQUE_ID,         UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeDeviceContent_Format, &WPD_OBJECT_PARENT_ID,                    UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeDeviceContent_Format, &WPD_OBJECT_NAME,                         UnspecifiedForm_CanRead_CanWrite_CannotDelete_Fast},
    {&FakeDeviceContent_Format, &WPD_OBJECT_FORMAT,                       UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeDeviceContent_Format, &WPD_OBJECT_CONTENT_TYPE,                 UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeDeviceContent_Format, &WPD_OBJECT_CAN_DELETE,                   UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeDeviceContent_Format, &WPD_OBJECT_ISHIDDEN,                     UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeDeviceContent_Format, &WPD_OBJECT_ISSYSTEM,                     UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeDeviceContent_Format, &WPD_DEVICE_SUPPORTS_NON_CONSUMABLE,      UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeDeviceContent_Format, &WPD_OBJECT_NON_CONSUMABLE,               UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeDeviceContent_Format, &WPD_FUNCTIONAL_OBJECT_CATEGORY,          UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeDeviceContent_Format, &WPD_DEVICE_FIRMWARE_VERSION,             UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeDeviceContent_Format, &WPD_DEVICE_POWER_LEVEL,                  UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeDeviceContent_Format, &WPD_DEVICE_POWER_SOURCE,                 UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeDeviceContent_Format, &WPD_DEVICE_PROTOCOL,                     UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeDeviceContent_Format, &WPD_DEVICE_MODEL,                        UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeDeviceContent_Format, &WPD_DEVICE_SERIAL_NUMBER,                UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeDeviceContent_Format, &WPD_DEVICE_MANUFACTURER,                 UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeDeviceContent_Format, &WPD_DEVICE_TYPE,                         UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeDeviceContent_Format, &WPD_DEVICE_NETWORK_IDENTIFIER,           UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeDeviceContent_Format, &WPD_DEVICE_FRIENDLY_NAME,                UnspecifiedForm_CanRead_CanWrite_CannotDelete_Fast},
    {&FakeDeviceContent_Format, &WPD_DEVICE_SYNC_PARTNER,                 UnspecifiedForm_CanRead_CanWrite_CannotDelete_Fast},
    // Properties for storage objects
    {&FakeStorageContent_Format, &WPD_OBJECT_ID,                           UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeStorageContent_Format, &WPD_OBJECT_PERSISTENT_UNIQUE_ID,         UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeStorageContent_Format, &WPD_OBJECT_PARENT_ID,                    UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeStorageContent_Format, &WPD_OBJECT_NAME,                         UnspecifiedForm_CanRead_CanWrite_CannotDelete_Fast},
    {&FakeStorageContent_Format, &WPD_OBJECT_FORMAT,                       UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeStorageContent_Format, &WPD_OBJECT_CONTENT_TYPE,                 UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeStorageContent_Format, &WPD_OBJECT_CAN_DELETE,                   UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeStorageContent_Format, &WPD_OBJECT_ISHIDDEN,                     UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeStorageContent_Format, &WPD_OBJECT_ISSYSTEM,                     UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeStorageContent_Format, &WPD_FOLDER_CONTENT_TYPES_ALLOWED,        UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeStorageContent_Format, &WPD_OBJECT_NON_CONSUMABLE,               UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeStorageContent_Format, &WPD_DEVICE_SUPPORTS_NON_CONSUMABLE,      UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeStorageContent_Format, &WPD_STORAGE_CAPACITY,                    UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeStorageContent_Format, &WPD_STORAGE_FREE_SPACE_IN_BYTES,         UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeStorageContent_Format, &WPD_FUNCTIONAL_OBJECT_CATEGORY,          UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeStorageContent_Format, &WPD_STORAGE_TYPE,                        UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    // Properties for Image objects
    {&WPD_OBJECT_FORMAT_EXIF, &WPD_OBJECT_ID,                       UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_EXIF, &WPD_OBJECT_PERSISTENT_UNIQUE_ID,     UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_EXIF, &WPD_OBJECT_PARENT_ID,                UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_EXIF, &WPD_OBJECT_NAME,                     UnspecifiedForm_CanRead_CanWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_EXIF, &WPD_OBJECT_CONTENT_TYPE,             UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_EXIF, &WPD_OBJECT_FORMAT,                   UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_EXIF, &WPD_OBJECT_CAN_DELETE,               UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_EXIF, &WPD_OBJECT_ISHIDDEN,                 UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_EXIF, &WPD_OBJECT_ISSYSTEM,                 UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_EXIF, &WPD_OBJECT_NON_CONSUMABLE,           UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_EXIF, &WPD_MEDIA_HEIGHT,                    UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_EXIF, &WPD_MEDIA_WIDTH,                     UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_EXIF, &WPD_OBJECT_DATE_CREATED,             UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_EXIF, &WPD_OBJECT_ORIGINAL_FILE_NAME,       UnspecifiedForm_CanRead_CanWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_EXIF, &WPD_OBJECT_SIZE,                     UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    // Properties for Music objects
    {&WPD_OBJECT_FORMAT_WMA, &WPD_OBJECT_ID,                        UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMA, &WPD_OBJECT_PERSISTENT_UNIQUE_ID,      UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMA, &WPD_OBJECT_PARENT_ID,                 UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMA, &WPD_OBJECT_NAME,                      UnspecifiedForm_CanRead_CanWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMA, &WPD_OBJECT_CONTENT_TYPE,              UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMA, &WPD_OBJECT_FORMAT,                    UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMA, &WPD_OBJECT_CAN_DELETE,                UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMA, &WPD_OBJECT_ISHIDDEN,                  UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMA, &WPD_OBJECT_ISSYSTEM,                  UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMA, &WPD_OBJECT_NON_CONSUMABLE,            UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMA, &WPD_MEDIA_TITLE,                      UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMA, &WPD_MEDIA_ARTIST,                     UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMA, &WPD_MEDIA_DURATION,                   UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMA, &WPD_OBJECT_SIZE,                      UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMA, &WPD_OBJECT_DATE_AUTHORED,             UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMA, &WPD_OBJECT_DATE_MODIFIED,             UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMA, &WPD_MUSIC_ALBUM,                      UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMA, &WPD_MEDIA_GENRE,                      UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMA, &WPD_MUSIC_TRACK,                      UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMA, &WPD_OBJECT_ORIGINAL_FILE_NAME,        UnspecifiedForm_CanRead_CanWrite_CannotDelete_Fast},
    // Properties for video objects
    {&WPD_OBJECT_FORMAT_WMV, &WPD_OBJECT_ID,                        UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMV, &WPD_OBJECT_PERSISTENT_UNIQUE_ID,      UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMV, &WPD_OBJECT_PARENT_ID,                 UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMV, &WPD_OBJECT_NAME,                      UnspecifiedForm_CanRead_CanWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMV, &WPD_OBJECT_CONTENT_TYPE,              UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMV, &WPD_OBJECT_FORMAT,                    UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMV, &WPD_OBJECT_CAN_DELETE,                UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMV, &WPD_OBJECT_ISHIDDEN,                  UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMV, &WPD_OBJECT_ISSYSTEM,                  UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMV, &WPD_OBJECT_NON_CONSUMABLE,            UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMV, &WPD_MEDIA_TITLE,                      UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMV, &WPD_MEDIA_DURATION,                   UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMV, &WPD_OBJECT_SIZE,                      UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMV, &WPD_MEDIA_HEIGHT,                     UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMV, &WPD_MEDIA_WIDTH,                      UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMV, &WPD_OBJECT_DATE_AUTHORED,             UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMV, &WPD_OBJECT_DATE_MODIFIED,             UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMV, &WPD_OBJECT_ORIGINAL_FILE_NAME,        UnspecifiedForm_CanRead_CanWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMV, &WPD_VIDEO_SCAN_TYPE,                  UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMV, &WPD_VIDEO_BITRATE,                    UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMV, &WPD_VIDEO_FOURCC_CODE,                UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_WMV, &WPD_OBJECT_GENERATE_THUMBNAIL_FROM_RESOURCE, UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    // Properties for contact objects
    {&WPD_OBJECT_FORMAT_VCARD2, &WPD_OBJECT_ID,                    UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_VCARD2, &WPD_OBJECT_PERSISTENT_UNIQUE_ID,  UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_VCARD2, &WPD_OBJECT_PARENT_ID,             UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_VCARD2, &WPD_OBJECT_NAME,                  UnspecifiedForm_CanRead_CanWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_VCARD2, &WPD_OBJECT_FORMAT,                UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_VCARD2, &WPD_OBJECT_CONTENT_TYPE,          UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_VCARD2, &WPD_OBJECT_CAN_DELETE,            UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_VCARD2, &WPD_OBJECT_ISHIDDEN,              UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_VCARD2, &WPD_OBJECT_ISSYSTEM,              UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_VCARD2, &WPD_OBJECT_NON_CONSUMABLE,        UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_VCARD2, &WPD_CONTACT_DISPLAY_NAME,         UnspecifiedForm_CanRead_CanWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_VCARD2, &WPD_CONTACT_PRIMARY_PHONE,        UnspecifiedForm_CanRead_CanWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_VCARD2, &WPD_CONTACT_MOBILE_PHONE,         UnspecifiedForm_CanRead_CanWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_VCARD2, &WPD_CONTACT_BUSINESS_PHONE,       UnspecifiedForm_CanRead_CanWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_VCARD2, &WPD_OBJECT_ORIGINAL_FILE_NAME,    UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_VCARD2, &WPD_OBJECT_SIZE,                  UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    // Properties for Rendering Information object
    {&WPD_FUNCTIONAL_CATEGORY_RENDERING_INFORMATION, &WPD_OBJECT_ID,                        UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_FUNCTIONAL_CATEGORY_RENDERING_INFORMATION, &WPD_OBJECT_PERSISTENT_UNIQUE_ID,      UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_FUNCTIONAL_CATEGORY_RENDERING_INFORMATION, &WPD_OBJECT_PARENT_ID,                 UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_FUNCTIONAL_CATEGORY_RENDERING_INFORMATION, &WPD_OBJECT_NAME,                      UnspecifiedForm_CanRead_CanWrite_CannotDelete_Fast},
    {&WPD_FUNCTIONAL_CATEGORY_RENDERING_INFORMATION, &WPD_OBJECT_FORMAT,                    UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_FUNCTIONAL_CATEGORY_RENDERING_INFORMATION, &WPD_OBJECT_CONTENT_TYPE,              UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_FUNCTIONAL_CATEGORY_RENDERING_INFORMATION, &WPD_OBJECT_CAN_DELETE,                UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_FUNCTIONAL_CATEGORY_RENDERING_INFORMATION, &WPD_OBJECT_ISHIDDEN,                  UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_FUNCTIONAL_CATEGORY_RENDERING_INFORMATION, &WPD_OBJECT_ISSYSTEM,                  UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_FUNCTIONAL_CATEGORY_RENDERING_INFORMATION, &WPD_OBJECT_NON_CONSUMABLE,            UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_FUNCTIONAL_CATEGORY_RENDERING_INFORMATION, &WPD_FUNCTIONAL_OBJECT_CATEGORY,       UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_FUNCTIONAL_CATEGORY_RENDERING_INFORMATION, &WPD_RENDERING_INFORMATION_PROFILES,   UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    // Properties for Network Configuration object
    {&WPD_FUNCTIONAL_CATEGORY_NETWORK_CONFIGURATION, &WPD_OBJECT_ID,                        UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_FUNCTIONAL_CATEGORY_NETWORK_CONFIGURATION, &WPD_OBJECT_PERSISTENT_UNIQUE_ID,      UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_FUNCTIONAL_CATEGORY_NETWORK_CONFIGURATION, &WPD_OBJECT_PARENT_ID,                 UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_FUNCTIONAL_CATEGORY_NETWORK_CONFIGURATION, &WPD_OBJECT_NAME,                      UnspecifiedForm_CanRead_CanWrite_CannotDelete_Fast},
    {&WPD_FUNCTIONAL_CATEGORY_NETWORK_CONFIGURATION, &WPD_OBJECT_FORMAT,                    UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_FUNCTIONAL_CATEGORY_NETWORK_CONFIGURATION, &WPD_OBJECT_CONTENT_TYPE,              UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_FUNCTIONAL_CATEGORY_NETWORK_CONFIGURATION, &WPD_OBJECT_CAN_DELETE,                UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_FUNCTIONAL_CATEGORY_NETWORK_CONFIGURATION, &WPD_OBJECT_ISHIDDEN,                  UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_FUNCTIONAL_CATEGORY_NETWORK_CONFIGURATION, &WPD_OBJECT_ISSYSTEM,                  UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_FUNCTIONAL_CATEGORY_NETWORK_CONFIGURATION, &WPD_FOLDER_CONTENT_TYPES_ALLOWED,     UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_FUNCTIONAL_CATEGORY_NETWORK_CONFIGURATION, &WPD_OBJECT_NON_CONSUMABLE,            UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_FUNCTIONAL_CATEGORY_NETWORK_CONFIGURATION, &WPD_FUNCTIONAL_OBJECT_CATEGORY,       UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    // Properties for Network Association object
    {&WPD_OBJECT_FORMAT_NETWORK_ASSOCIATION, &WPD_OBJECT_ID,                                    UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_NETWORK_ASSOCIATION, &WPD_OBJECT_PERSISTENT_UNIQUE_ID,                  UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_NETWORK_ASSOCIATION, &WPD_OBJECT_PARENT_ID,                             UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_NETWORK_ASSOCIATION, &WPD_OBJECT_NAME,                                  UnspecifiedForm_CanRead_CanWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_NETWORK_ASSOCIATION, &WPD_OBJECT_FORMAT,                                UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_NETWORK_ASSOCIATION, &WPD_OBJECT_CONTENT_TYPE,                          UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_NETWORK_ASSOCIATION, &WPD_OBJECT_CAN_DELETE,                            UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_NETWORK_ASSOCIATION, &WPD_OBJECT_ISHIDDEN,                              UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_NETWORK_ASSOCIATION, &WPD_OBJECT_ISSYSTEM,                              UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_NETWORK_ASSOCIATION, &WPD_OBJECT_NON_CONSUMABLE,                        UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_NETWORK_ASSOCIATION, &WPD_NETWORK_ASSOCIATION_HOST_NETWORK_IDENTIFIERS, UnspecifiedForm_CanRead_CanWrite_CannotDelete_Fast},
    // Properties for Microsoft WFC object
    {&WPD_OBJECT_FORMAT_MICROSOFT_WFC, &WPD_OBJECT_ID,                      UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_MICROSOFT_WFC, &WPD_OBJECT_PERSISTENT_UNIQUE_ID,    UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_MICROSOFT_WFC, &WPD_OBJECT_PARENT_ID,               UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_MICROSOFT_WFC, &WPD_OBJECT_NAME,                    UnspecifiedForm_CanRead_CanWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_MICROSOFT_WFC, &WPD_OBJECT_FORMAT,                  UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_MICROSOFT_WFC, &WPD_OBJECT_CONTENT_TYPE,            UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_MICROSOFT_WFC, &WPD_OBJECT_CAN_DELETE,              UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_MICROSOFT_WFC, &WPD_OBJECT_ISHIDDEN,                UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_MICROSOFT_WFC, &WPD_OBJECT_ISSYSTEM,                UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&WPD_OBJECT_FORMAT_MICROSOFT_WFC, &WPD_OBJECT_NON_CONSUMABLE,          UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    // Properties for Memo object
    {&FakeMemoContent_Format, &WPD_OBJECT_ID,                   UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeMemoContent_Format, &WPD_OBJECT_PERSISTENT_UNIQUE_ID, UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeMemoContent_Format, &WPD_OBJECT_PARENT_ID,            UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeMemoContent_Format, &WPD_OBJECT_NAME,                 UnspecifiedForm_CanRead_CanWrite_CannotDelete_Fast},
    {&FakeMemoContent_Format, &WPD_OBJECT_FORMAT,               UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeMemoContent_Format, &WPD_OBJECT_CONTENT_TYPE,         UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeMemoContent_Format, &WPD_OBJECT_CAN_DELETE,           UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeMemoContent_Format, &WPD_OBJECT_ISHIDDEN,             UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeMemoContent_Format, &WPD_OBJECT_ISSYSTEM,             UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeMemoContent_Format, &WPD_OBJECT_NON_CONSUMABLE,       UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeMemoContent_Format, &WPD_OBJECT_SIZE,                 UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeMemoContent_Format, &WPD_OBJECT_DATE_AUTHORED,        UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeMemoContent_Format, &WPD_OBJECT_DATE_MODIFIED,        UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast},
    {&FakeMemoContent_Format, &WPD_OBJECT_ORIGINAL_FILE_NAME,   UnspecifiedForm_CanRead_CanWrite_CannotDelete_Fast},
};

HRESULT AddPropertyKeyArrayToCollection(
    _In_reads_(cKeys) const PROPERTYKEY**             ppKeys,
                      const DWORD                     cKeys,
    _In_              IPortableDeviceKeyCollection*   pCollection)
{

    HRESULT hr = S_OK;

    if (hr == S_OK)
    {
        // Add the keys
        for (DWORD dwIndex = 0; dwIndex < cKeys; dwIndex++)
        {
            hr = pCollection->Add(*ppKeys[dwIndex]);
            CHECK_HR(hr, "Failed to add key at index %d", dwIndex);
            if (FAILED(hr))
            {
                break;
            }
        }
    }

    return hr;
}

HRESULT AddFixedAttributesByType(
         FakeDevicePropertyAttributesType AttributesType,
    _In_ IPortableDeviceValues*           pAttributes)
{

    HRESULT hr = S_OK;

    if(pAttributes == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, ("Cannot have NULL parameter"));
        return hr;
    }

    // Initialize our default values for the static attributes
    DWORD   dwForm          = WPD_PROPERTY_ATTRIBUTE_FORM_UNSPECIFIED;
    BOOL    bCanRead        = TRUE;
    BOOL    bCanWrite       = FALSE;
    BOOL    bCanDelete      = FALSE;
    BOOL    bFastProperty   = TRUE;

    // Adjust the attributes for the specific property type if needed
    if(AttributesType == UnspecifiedForm_CanRead_CanWrite_CannotDelete_Fast)
    {
        bCanWrite = TRUE;
    }

    // Add the static attributes for this property.
    if(hr == S_OK)
    {
        if (hr == S_OK)
        {
            hr = pAttributes->SetUnsignedIntegerValue(WPD_PROPERTY_ATTRIBUTE_FORM, dwForm);
            CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_FORM");
        }
        if (hr == S_OK)
        {
            hr = pAttributes->SetBoolValue(WPD_PROPERTY_ATTRIBUTE_CAN_READ, bCanRead);
            CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_CAN_READ");
        }
        if (hr == S_OK)
        {
            hr = pAttributes->SetBoolValue(WPD_PROPERTY_ATTRIBUTE_CAN_WRITE, bCanWrite);
            CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_CAN_WRITE");
        }
        if (hr == S_OK)
        {
            hr = pAttributes->SetBoolValue(WPD_PROPERTY_ATTRIBUTE_CAN_DELETE, bCanDelete);
            CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_CAN_DELETE");
        }
        if (hr == S_OK)
        {
            hr = pAttributes->SetBoolValue(WPD_PROPERTY_ATTRIBUTE_FAST_PROPERTY, bFastProperty);
            CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_FAST_PROPERTY");
        }
    }

    return hr;
}

HRESULT AddFixedPropertyAttributes(
    _In_    REFGUID                         guidObjectFormat,
    _In_    REFPROPERTYKEY                  key,
    _In_    IPortableDeviceValues*          pAttributes)
{
    HRESULT hr = S_OK;

    if(pAttributes == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, ("Cannot have NULL parameter"));
        return hr;
    }

    if (hr == S_OK)
    {
        for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(g_FixedAttributesTable); dwIndex++)
        {
            if((*g_FixedAttributesTable[dwIndex].pFormat == guidObjectFormat) &&
               (IsEqualPropertyKey(*g_FixedAttributesTable[dwIndex].pKey, key)))
            {
                hr = AddFixedAttributesByType(g_FixedAttributesTable[dwIndex].type, pAttributes);
                CHECK_HR(hr, "Failed to add fixed attributes for %ws.%d on format %ws", (LPWSTR)CComBSTR(key.fmtid), key.pid, (LPWSTR)CComBSTR(guidObjectFormat));
                break;
            }
        }
    }

    return hr;
}

HRESULT AddSupportedProperties(
    _In_            REFGUID                         guidObjectFormatOrCategory,
    _COM_Outptr_    IPortableDeviceKeyCollection**  ppKeys)
{
    HRESULT hr = S_OK;

    if(ppKeys == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    *ppKeys = NULL;
    CComPtr<IPortableDeviceKeyCollection> pCollection;

    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_PortableDeviceKeyCollection,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceKeyCollection,
                              (VOID**) &pCollection);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceKeyCollection");
    }

    if (SUCCEEDED(hr))
    {
        hr = AddSupportedProperties(guidObjectFormatOrCategory, pCollection);
        CHECK_HR(hr, "Failed to add supported properties");
    }

    if (SUCCEEDED(hr))
    {
        hr = pCollection->QueryInterface(IID_PPV_ARGS(ppKeys));
        CHECK_HR(hr, "Failed to QI IPortableDeviceKeyCollection for IPortableDeviceKeyCollection");
    }

    return hr;
}

HRESULT AddSupportedProperties(
    _In_    REFGUID                        guidObjectFormatOrCategory,
    _In_    IPortableDeviceKeyCollection*  pKeys)
{
    HRESULT hr = S_OK;

    if(pKeys == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, ("Cannot have NULL parameter"));
        return hr;
    }

    if (guidObjectFormatOrCategory  == WPD_OBJECT_FORMAT_ALL)
    {
        hr = AddPropertyKeyArrayToCollection(g_SupportedPropertiesForFormatAll,
                                             ARRAYSIZE(g_SupportedPropertiesForFormatAll),
                                             pKeys);
    } else if (guidObjectFormatOrCategory  == FakeContent_Format)
    {
        hr = AddPropertyKeyArrayToCollection(g_SupportedPropertiesForFakeContentFormat,
                                             ARRAYSIZE(g_SupportedPropertiesForFakeContentFormat),
                                             pKeys);
    }
    else if (guidObjectFormatOrCategory  == WPD_OBJECT_FORMAT_EXIF)
    {
        hr = AddPropertyKeyArrayToCollection(g_SupportedPropertiesForFakeImageContentFormat,
                                             ARRAYSIZE(g_SupportedPropertiesForFakeImageContentFormat),
                                             pKeys);
    }
    else if (guidObjectFormatOrCategory  == WPD_OBJECT_FORMAT_WMA)
    {
        hr = AddPropertyKeyArrayToCollection(g_SupportedPropertiesForFakeMusicContentFormat,
                                             ARRAYSIZE(g_SupportedPropertiesForFakeMusicContentFormat),
                                             pKeys);
    }
    else if (guidObjectFormatOrCategory  == WPD_OBJECT_FORMAT_WMV)
    {
        hr = AddPropertyKeyArrayToCollection(g_SupportedPropertiesForFakeVideoContentFormat,
                                             ARRAYSIZE(g_SupportedPropertiesForFakeVideoContentFormat),
                                             pKeys);
    }
    else  if (guidObjectFormatOrCategory  == WPD_OBJECT_FORMAT_VCARD2)
    {
        hr = AddPropertyKeyArrayToCollection(g_SupportedPropertiesForFakeContactContentFormat,
                                             ARRAYSIZE(g_SupportedPropertiesForFakeContactContentFormat),
                                             pKeys);
    }
    else  if (guidObjectFormatOrCategory  == WPD_FUNCTIONAL_CATEGORY_RENDERING_INFORMATION)
    {
        hr = AddPropertyKeyArrayToCollection(g_SupportedPropertiesForRenderingInformation,
                                             ARRAYSIZE(g_SupportedPropertiesForRenderingInformation),
                                             pKeys);
    }
    else  if (guidObjectFormatOrCategory  == WPD_FUNCTIONAL_CATEGORY_NETWORK_CONFIGURATION)
    {
        hr = AddPropertyKeyArrayToCollection(g_SupportedPropertiesForNetworkConfiguration,
                                             ARRAYSIZE(g_SupportedPropertiesForNetworkConfiguration),
                                             pKeys);
    }
    else  if (guidObjectFormatOrCategory  == WPD_OBJECT_FORMAT_NETWORK_ASSOCIATION)
    {
        hr = AddPropertyKeyArrayToCollection(g_SupportedPropertiesForNetworkAssociation,
                                             ARRAYSIZE(g_SupportedPropertiesForNetworkAssociation),
                                             pKeys);
    }
    else  if (guidObjectFormatOrCategory  == WPD_OBJECT_FORMAT_MICROSOFT_WFC)
    {
        hr = AddPropertyKeyArrayToCollection(g_SupportedPropertiesForMicrosoftWFC,
                                             ARRAYSIZE(g_SupportedPropertiesForMicrosoftWFC),
                                             pKeys);
    }
    else  if (guidObjectFormatOrCategory  == WPD_FUNCTIONAL_CATEGORY_STORAGE)
    {
        hr = AddPropertyKeyArrayToCollection(g_SupportedPropertiesForFakeStorageContentFormat,
                                             ARRAYSIZE(g_SupportedPropertiesForFakeStorageContentFormat),
                                             pKeys);
    }
    else  if (guidObjectFormatOrCategory  == WPD_FUNCTIONAL_CATEGORY_DEVICE)
    {
        hr = AddPropertyKeyArrayToCollection(g_SupportedPropertiesForFakeDeviceContentFormat,
                                             ARRAYSIZE(g_SupportedPropertiesForFakeDeviceContentFormat),
                                             pKeys);
    }
    else if (guidObjectFormatOrCategory  == FakeMemoContent_Format)
    {
        hr = AddPropertyKeyArrayToCollection(g_SupportedPropertiesForFakeMemoContentFormat,
                                             ARRAYSIZE(g_SupportedPropertiesForFakeMemoContentFormat),
                                             pKeys);
    }

    return hr;
}

HRESULT GetPreferredAudioProfile(
    _COM_Outptr_ IPortableDeviceValues** ppProfile)
{
    HRESULT hr = S_OK;
    CComPtr<IPortableDeviceValues> pProfile;

    if(ppProfile == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, ("Cannot have NULL parameter"));
        return hr;
    }

    *ppProfile = NULL;

    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_PortableDeviceValues,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceValues,
                              (VOID**) &pProfile);
        CHECK_HR(hr, "Failed to CoCreateInstance CLSID_PortableDeviceValues");
    }

    // Set the value for WPD_OBJECT_FORMAT to indicate this profile applies to WMA objects
    if (SUCCEEDED(hr))
    {
        hr = pProfile->SetGuidValue(WPD_OBJECT_FORMAT, WPD_OBJECT_FORMAT_WMA);
        CHECK_HR(hr, "Failed to set WPD_OBJECT_FORMAT");
    }

    // Set the preferred value for WPD_MEDIA_TOTAL_BITRATE
    if (SUCCEEDED(hr))
    {
        hr = pProfile->SetUnsignedIntegerValue(WPD_MEDIA_TOTAL_BITRATE, 192000);
        CHECK_HR(hr, "Failed to set WPD_MEDIA_TOTAL_BITRATE");
    }

    // Set the preferred value for WPD_AUDIO_CHANNEL_COUNT
    if (SUCCEEDED(hr))
    {
        hr = pProfile->SetUnsignedIntegerValue(WPD_AUDIO_CHANNEL_COUNT, 2);
        CHECK_HR(hr, "Failed to set WPD_AUDIO_CHANNEL_COUNT");
    }

    // Set the preferred value for WPD_AUDIO_FORMAT_CODE
    if (SUCCEEDED(hr))
    {
        hr = pProfile->SetUnsignedIntegerValue(WPD_AUDIO_FORMAT_CODE, WAVE_FORMAT_MSAUDIO3);
        CHECK_HR(hr, "Failed to set WPD_AUDIO_FORMAT_CODE");
    }

    // Set the output result
    if (SUCCEEDED(hr))
    {
        hr = pProfile->QueryInterface(IID_PPV_ARGS(ppProfile));
        CHECK_HR(hr, "Failed to QI for IPortableDeviceValues");
    }

    return hr;
}

HRESULT GetAudioProfile2(
    _COM_Outptr_ IPortableDeviceValues** ppProfile)
{
    HRESULT hr = S_OK;
    CComPtr<IPortableDeviceValues> pProfile;
    CComPtr<IPortableDeviceValues> pTotalBitRate;

    if(ppProfile == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, ("Cannot have NULL parameter"));
        return hr;
    }

    *ppProfile = NULL;

    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_PortableDeviceValues,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceValues,
                              (VOID**) &pProfile);
        CHECK_HR(hr, "Failed to CoCreateInstance CLSID_PortableDeviceValues");
    }

    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_PortableDeviceValues,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceValues,
                              (VOID**) &pTotalBitRate);
        CHECK_HR(hr, "Failed to CoCreateInstance CLSID_PortableDeviceValues");
    }

    // Set the value for WPD_OBJECT_FORMAT to indicate this profile applies to WMA objects
    if (SUCCEEDED(hr))
    {
        hr = pProfile->SetGuidValue(WPD_OBJECT_FORMAT, WPD_OBJECT_FORMAT_WMA);
        CHECK_HR(hr, "Failed to set WPD_OBJECT_FORMAT");
    }

    // Set the value for WPD_MEDIA_TOTAL_BITRATE
    if (SUCCEEDED(hr))
    {
        // First, set the values for the range which will be contained in pTotalBitRate
        hr = pTotalBitRate->SetUnsignedIntegerValue(WPD_PROPERTY_ATTRIBUTE_FORM, WPD_PROPERTY_ATTRIBUTE_FORM_RANGE);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_FORM for WPD_MEDIA_TOTAL_BITRATE");

        if (SUCCEEDED(hr))
        {
            hr = pTotalBitRate->SetUnsignedIntegerValue(WPD_PROPERTY_ATTRIBUTE_RANGE_MIN, 64000);
            CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_RANGE_MIN for WPD_MEDIA_TOTAL_BITRATE");
        }
        if (SUCCEEDED(hr))
        {
            hr = pTotalBitRate->SetUnsignedIntegerValue(WPD_PROPERTY_ATTRIBUTE_RANGE_MAX, 256000);
            CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_RANGE_MAX for WPD_MEDIA_TOTAL_BITRATE");
        }
        if (SUCCEEDED(hr))
        {
            hr = pTotalBitRate->SetUnsignedIntegerValue(WPD_PROPERTY_ATTRIBUTE_RANGE_STEP, 1000);
            CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_RANGE_STEP for WPD_MEDIA_TOTAL_BITRATE");
        }

        // Now set the bit rate property to be pTotalBitRate
        if (SUCCEEDED(hr))
        {
            hr = pProfile->SetIPortableDeviceValuesValue(WPD_MEDIA_TOTAL_BITRATE, pTotalBitRate);
            CHECK_HR(hr, "Failed to set WPD_MEDIA_TOTAL_BITRATE");
        }
    }

    // Set the value for WPD_AUDIO_CHANNEL_COUNT
    if (SUCCEEDED(hr))
    {
        hr = pProfile->SetUnsignedIntegerValue(WPD_AUDIO_CHANNEL_COUNT, 2);
        CHECK_HR(hr, "Failed to set WPD_AUDIO_CHANNEL_COUNT");
    }

    // Set the value for WPD_AUDIO_FORMAT_CODE
    if (SUCCEEDED(hr))
    {
        hr = pProfile->SetUnsignedIntegerValue(WPD_AUDIO_FORMAT_CODE, WAVE_FORMAT_MSAUDIO3);
        CHECK_HR(hr, "Failed to set WPD_AUDIO_FORMAT_CODE");
    }

    // Set the output result
    if (SUCCEEDED(hr))
    {
        hr = pProfile->QueryInterface(IID_PPV_ARGS(ppProfile));
        CHECK_HR(hr, "Failed to QI for IPortableDeviceValues");
    }

    return hr;
}

HRESULT GetVideoProfile(
    _COM_Outptr_ IPortableDeviceValues** ppProfile)
{
    HRESULT hr = S_OK;
    CComPtr<IPortableDeviceValues> pProfile;
    CComPtr<IPortableDeviceValues> pFourCCCode;

    if(ppProfile == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, ("Cannot have NULL parameter"));
        return hr;
    }

    *ppProfile = NULL;

    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_PortableDeviceValues,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceValues,
                              (VOID**) &pProfile);
        CHECK_HR(hr, "Failed to CoCreateInstance CLSID_PortableDeviceValues");
    }

    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_PortableDeviceValues,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceValues,
                              (VOID**) &pFourCCCode);
        CHECK_HR(hr, "Failed to CoCreateInstance CLSID_PortableDeviceValues");
    }

    // Set the value for WPD_OBJECT_FORMAT to indicate this profile applies to WMV objects
    if (SUCCEEDED(hr))
    {
        hr = pProfile->SetGuidValue(WPD_OBJECT_FORMAT, WPD_OBJECT_FORMAT_WMV);
        CHECK_HR(hr, "Failed to set WPD_OBJECT_FORMAT to WPD_OBJECT_FORMAT_WMV for the rendering profile");
    }


    // Set the value for WPD_VIDEO_FOURCC_CODE
    if (SUCCEEDED(hr))
    {
        CComPtr<IPortableDevicePropVariantCollection> pFourCCCodeEnumElements;

        hr = CoCreateInstance(CLSID_PortableDevicePropVariantCollection,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_IPortableDevicePropVariantCollection,
                                (VOID**) &pFourCCCodeEnumElements);
        CHECK_HR(hr, "Failed to CoCreateInstance CLSID_PortableDevicePropVariantCollection");

        if (SUCCEEDED(hr))
        {
            hr = pFourCCCode->SetUnsignedIntegerValue(WPD_PROPERTY_ATTRIBUTE_FORM, WPD_PROPERTY_ATTRIBUTE_FORM_ENUMERATION);
            CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_FORM for WPD_VIDEO_FOURCC_CODE");
        }

        if (SUCCEEDED(hr))
        {
            // Only 1 sample value is set here, add more as appropriate for your device
            PROPVARIANT pvValue;
            PropVariantInit(&pvValue);
            pvValue.vt = VT_UI4;  
            pvValue.ulVal = MAKEFOURCC('W', 'M', 'V', '3');  // No need to PropVariantClear as we are assigning a value
            hr = pFourCCCodeEnumElements->Add(&pvValue);
            CHECK_HR(hr, "Failed to populate the FourCC Code Enumeration Elements");
        }

        if (SUCCEEDED(hr))
        {
            hr = pFourCCCode->SetIPortableDevicePropVariantCollectionValue(WPD_PROPERTY_ATTRIBUTE_ENUMERATION_ELEMENTS, pFourCCCodeEnumElements);
            CHECK_HR(hr, "Failed to set WPD_PROPERTY_ATTRIBUTE_ENUMERATION_ELEMENTS for WPD_VIDEO_FOURCC_CODE");
        }

        // Now set the Video FourCC Code property to be pFourCCCode
        if (SUCCEEDED(hr))
        {
            hr = pProfile->SetIPortableDeviceValuesValue(WPD_VIDEO_FOURCC_CODE, pFourCCCode);
            CHECK_HR(hr, "Failed to add the WPD_VIDEO_FOURCC_CODE attributes to the WPD_OBJECT_FORMAT_WMV rendering profile");
        }
    }

    // Set the output result
    if (SUCCEEDED(hr))
    {
        hr = pProfile->QueryInterface(IID_PPV_ARGS(ppProfile));
        CHECK_HR(hr, "Failed to QI for IPortableDeviceValues");
    }

    return hr;
}

HRESULT SetRenderingProfiles(
    _In_    IPortableDeviceValues*          pValues)
{
    HRESULT hr = S_OK;
    CComPtr<IPortableDeviceValues> pPreferredAudioProfile;
    CComPtr<IPortableDeviceValues> pAudioProfile2;
    CComPtr<IPortableDeviceValues>  pVideoProfile;

    CComPtr<IPortableDeviceValuesCollection> pProfiles;

    if(pValues == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, ("Cannot have NULL parameter"));
        return hr;
    }

    // Create the collection to hold the profiles
    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDeviceValuesCollection,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceValuesCollection,
                              (VOID**) &pProfiles);
        CHECK_HR(hr, "Failed to CoCreateInstance CLSID_PortableDeviceValuesCollection");
    }

    // Get the preferred audio profile
    if (hr == S_OK)
    {
        hr = GetPreferredAudioProfile(&pPreferredAudioProfile);
        CHECK_HR(hr, "Failed to get preferred audio profile properties");
    }

    // Add the profile
    if (hr == S_OK)
    {
        hr = pProfiles->Add(pPreferredAudioProfile);
        CHECK_HR(hr, "Failed to add preferred audio profile to profile collection");
    }

    // Get the second audio profile
    if (hr == S_OK)
    {
        hr = GetAudioProfile2(&pAudioProfile2);
        CHECK_HR(hr, "Failed to get second audio profile properties");
    }

    // Add the profile
    if (hr == S_OK)
    {
        hr = pProfiles->Add(pAudioProfile2);
        CHECK_HR(hr, "Failed to add second audio profile to profile collection");
    }

    // Get the video profile
    if (hr == S_OK)
    {
        hr = GetVideoProfile(&pVideoProfile);
        CHECK_HR(hr, "Failed to get video profile properties");
    }

    // Add the profile
    if (hr == S_OK)
    {
        hr = pProfiles->Add(pVideoProfile);
        CHECK_HR(hr, "Failed to add second audio profile to profile collection");
    }

    // Set the WPD_RENDERING_INFORMATION_PROFILES
    if (hr == S_OK)
    {
        hr = pValues->SetIPortableDeviceValuesCollectionValue(WPD_RENDERING_INFORMATION_PROFILES, pProfiles);
        CHECK_HR(hr, "Failed to set WPD_RENDERING_INFORMATION_PROFILES");
    }

    return hr;
}

DWORD GetResourceSize(
    UINT uiResource)
{
    HRESULT hr             = S_OK;
    LONG    lError         = ERROR_SUCCESS;
    DWORD   dwResourceSize = 0;

    HRSRC hResource = FindResource(g_hInstance, MAKEINTRESOURCE(uiResource), TEXT("DATA_FILE"));
    if (hResource)
    {
        HGLOBAL hGlobal = LoadResource(g_hInstance, hResource);
        if (hGlobal)
        {
            dwResourceSize = SizeofResource(g_hInstance, hResource);
        }
        else
        {
            lError = GetLastError();
            hr = HRESULT_FROM_WIN32(lError);
        }
    }
    else
    {
        lError = GetLastError();
        hr = HRESULT_FROM_WIN32(lError);
    }

    if (FAILED(hr))
    {
        CHECK_HR(hr, "Failed to get resource size for '%d'", uiResource);
    }

    return dwResourceSize;
}

PBYTE GetResourceData(
    UINT uiResource)
{
    HRESULT hr     = S_OK;
    LONG    lError = ERROR_SUCCESS;
    PBYTE   pData  = NULL;

    HRSRC hResource = FindResource(g_hInstance, MAKEINTRESOURCE(uiResource), TEXT("DATA_FILE"));
    if (hResource)
    {
        HGLOBAL hGlobal = LoadResource(g_hInstance, hResource);
        if (hGlobal)
        {
            pData = static_cast<BYTE*>(LockResource(hGlobal));
        }
        else
        {
            lError = GetLastError();
            hr = HRESULT_FROM_WIN32(lError);
        }
    }
    else
    {
        lError = GetLastError();
        hr = HRESULT_FROM_WIN32(lError);
    }

    if (FAILED(hr))
    {
        CHECK_HR(hr, "Failed to get resource data pointer for '%d'", uiResource);
    }

    return pData;
}

HRESULT IsValidContentType(
    _In_    REFGUID             guidObjectContentType,
    _In_    CAtlArray<GUID>&    RestrictedTypes)
{
    HRESULT hr = S_OK;

    size_t numElems = RestrictedTypes.GetCount();
    if(numElems > 0)
    {
        BOOL bContentTypeAllowed = FALSE;
        for(size_t typeIndex = 0; typeIndex < numElems; typeIndex++)
        {
            if(RestrictedTypes[typeIndex] == guidObjectContentType)
            {
                bContentTypeAllowed = TRUE;
            }
        }
        if(!bContentTypeAllowed)
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "Parent Object does not allow creation of content type %ws", CComBSTR(guidObjectContentType));
        }
    }

    return hr;
}

HRESULT GetClientContext(
    _In_            IPortableDeviceValues*  pParams,
    _In_            LPCWSTR                 pszContextKey,
    _COM_Outptr_    IUnknown**              ppContext)
{
    HRESULT      hr             = S_OK;
    ContextMap*  pContextMap    = NULL;

    if(ppContext == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, ("Cannot have NULL parameter"));
        return hr;
    }

    *ppContext = NULL;

    if (SUCCEEDED(hr))
    {
        hr = pParams->GetIUnknownValue(PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP, (IUnknown**) &pContextMap);
        CHECK_HR(hr, "Failed to get PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP");
    }

    if (SUCCEEDED(hr) && pContextMap == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Client context map is NULL");
    }

    if (SUCCEEDED(hr))
    {
        *ppContext = pContextMap->GetContext(pszContextKey);
        if(*ppContext == NULL)
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
            CHECK_HR(hr, "Failed to find context %ws for this client", pszContextKey);
        }
    }

    SAFE_RELEASE(pContextMap);

    return hr;
}

HRESULT GetClientEventCookie(
    _In_                      IPortableDeviceValues*  pParams,
    _Outptr_result_maybenull_ LPWSTR*                 ppszEventCookie)
{
    HRESULT        hr               = S_OK;
    LPWSTR         pszClientContext = NULL;
    ClientContext* pClientContext   = NULL;

    if ((pParams         == NULL) || 
        (ppszEventCookie == NULL))
    {
        return E_POINTER;
    }

    *ppszEventCookie = NULL;

    hr = pParams->GetStringValue(WPD_PROPERTY_COMMON_CLIENT_INFORMATION_CONTEXT, &pszClientContext);
    CHECK_HR(hr, "Missing value for WPD_PROPERTY_COMMON_CLIENT_INFORMATION_CONTEXT");

    if (SUCCEEDED(hr))
    {
        // Get the client context for this request.
        hr = GetClientContext(pParams, pszClientContext, (IUnknown**)&pClientContext);
        CHECK_HR(hr, "Failed to get the client context");
    }

    if (SUCCEEDED(hr) && (pClientContext->EventCookie.GetLength() > 0))
    {
        // Get the event cookie only if it has been set
        *ppszEventCookie = AtlAllocTaskWideString(pClientContext->EventCookie);
        if (*ppszEventCookie == NULL)
        {
            hr = E_OUTOFMEMORY;
            CHECK_HR(hr, "Failed to allocate the client event cookie");
        }
    }

    // We're done with the context
    SAFE_RELEASE(pClientContext);

    CoTaskMemFree(pszClientContext);
    pszClientContext = NULL;

    return hr;
}

HRESULT PostWpdEvent(
    _In_    IPortableDeviceValues*  pCommandParams,
    _In_    IPortableDeviceValues*  pEventParams)
{
    HRESULT hr             = S_OK;
    BYTE*   pBuffer        = NULL;
    DWORD   cbBuffer       = 0;
    LPWSTR  pszEventCookie = NULL;

    CComPtr<IWDFDevice>     pDevice;
    CComPtr<IWpdSerializer> pSerializer;

    // Get the WUDF Device Object
    hr = pCommandParams->GetIUnknownValue(PRIVATE_SAMPLE_DRIVER_WUDF_DEVICE_OBJECT, (IUnknown**) &pDevice);
    CHECK_HR(hr, "Failed to get PRIVATE_SAMPLE_DRIVER_WUDF_DEVICE_OBJECT");

    // Get the WpdSerializer Object
    if (hr == S_OK)
    {
        hr = pCommandParams->GetIUnknownValue(PRIVATE_SAMPLE_DRIVER_WPD_SERIALIZER_OBJECT, (IUnknown**) &pSerializer);
        CHECK_HR(hr, "Failed to get PRIVATE_SAMPLE_DRIVER_WPD_SERIALIZER_OBJECT");
    }

    if (hr == S_OK)
    {
        // Set the client event cookie if available.  This is benign, as some clients may not provide a cookie.
        HRESULT hrEventCookie = GetClientEventCookie(pCommandParams, &pszEventCookie);
        if ((hrEventCookie == S_OK) && (pszEventCookie != NULL))
        {
            hrEventCookie = pEventParams->SetStringValue(WPD_CLIENT_EVENT_COOKIE, pszEventCookie);
            CHECK_HR(hrEventCookie, "Failed to set WPD_CLIENT_EVENT_COOKIE (error ignored)");
        }
    }

    if (hr == S_OK)
    {
        // Create a buffer with the serialized parameters
        hr = pSerializer->GetBufferFromIPortableDeviceValues(pEventParams, &pBuffer, &cbBuffer);
        CHECK_HR(hr, "Failed to get buffer from IPortableDeviceValues");
    }

    // Send the event
    if (hr == S_OK && pBuffer != NULL)
    {
        hr = pDevice->PostEvent(WPD_EVENT_NOTIFICATION, WdfEventBroadcast, pBuffer, cbBuffer);
        CHECK_HR(hr, "Failed to post WPD (broadcast) event");
    }

    // Free the memory
    CoTaskMemFree(pBuffer);
    pBuffer = NULL;

    CoTaskMemFree(pszEventCookie);
    pszEventCookie = NULL;

    return hr;
}

HRESULT PostWpdEventWithProgress(
    _In_    IPortableDeviceValues*  pCommandParams,
    _In_    IPortableDeviceValues*  pEventParams,
    _In_    REFGUID                 guidEvent,
            const DWORD             dwOperationState,
            const DWORD             dwOperationProgress)
{
    HRESULT hr = S_OK;

    if((pCommandParams == NULL) || (pEventParams == NULL))
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    hr = pEventParams->SetGuidValue(WPD_EVENT_PARAMETER_EVENT_ID, guidEvent);
    CHECK_HR(hr, "Failed to set WPD_EVENT_PARAMETER_EVENT_ID");

    if (hr == S_OK)
    {
        hr = pEventParams->SetUnsignedIntegerValue(WPD_EVENT_PARAMETER_OPERATION_STATE, dwOperationState);
        CHECK_HR(hr, "Failed to set WPD_EVENT_PARAMETER_OPERATION_STATE");
    }

    if (hr == S_OK)
    {
        hr = pEventParams->SetUnsignedIntegerValue(WPD_EVENT_PARAMETER_OPERATION_PROGRESS, dwOperationProgress);
        CHECK_HR(hr, "Failed to set WPD_EVENT_PARAMETER_OPERATION_PROGRESS");
    }

    if (hr == S_OK)
    {
        hr = PostWpdEvent(pCommandParams, pEventParams);
        CHECK_HR(hr, "Failed to post event with progress");
    }

    return hr;
}


BOOL ExistsInCollection(
    _In_    REFGUID                               guid,
    _In_    IPortableDevicePropVariantCollection* pCollection)
{
    HRESULT hr = S_OK;

    BOOL    bFound     = FALSE;
    DWORD   dwNumGuids = 0;

    if(pCollection != NULL)
    {
        hr = pCollection->GetCount(&dwNumGuids);
        if (SUCCEEDED(hr))
        {
            // Loop through each guid in the collection
            for (DWORD dwIndex = 0; dwIndex < dwNumGuids; dwIndex++)
            {
                PROPVARIANT pv = {0};
                PropVariantInit(&pv);
                hr = pCollection->GetAt(dwIndex, &pv);
                if (SUCCEEDED(hr))
                {
                    if ((pv.puuid != NULL) && (pv.vt == VT_CLSID))
                    {
                        bFound = IsEqualGUID(guid, *pv.puuid);
                    }
                }

                PropVariantClear(&pv);

                if (bFound == TRUE)
                {
                    break;
                }
            }
        }
    }

    return bFound;
}

HRESULT GetAtlStringValue(
    _In_    REFPROPERTYKEY          Key,
    _In_    IPortableDeviceValues*  pValues,
    _Out_   CAtlStringW&            strValue)
{
    HRESULT hr       = S_OK;
    LPWSTR  wszValue = NULL;
    strValue = L"";

    if (pValues == NULL)
    {
        hr = E_POINTER;
        return hr;
    }

    hr = pValues->GetStringValue(Key, &wszValue);
    if (hr == S_OK)
    {
        strValue = wszValue;
    }

    if (wszValue != NULL)
    {
        CoTaskMemFree(wszValue);
        wszValue = NULL;
    }

    return hr;
}

HRESULT CreateVCard(
    _In_    IPortableDeviceValues* pValues,
    _Out_   CAtlStringA& strVCard)
{
    CAtlStringW strLastName;              // WPD_CONTACT_LAST_NAME
    CAtlStringW strFirstName;             // WPD_CONTACT_FIRST_NAME
    CAtlStringW strDisplayName;           // WPD_CONTACT_DISPLAY_NAME
    CAtlStringW strCompanyName;           // WPD_CONTACT_COMPANY_NAME
    CAtlStringW strRole;                  // WPD_CONTACT_ROLE
    CAtlStringW strPrimaryPhoneNumber;    // WPD_CONTACT_PRIMARY_PHONE or WPD_CONTACT_PERSONAL_PHONE
    CAtlStringW strBusinessPhoneNumber;   // WPD_CONTACT_BUSINESS_PHONE
    CAtlStringW strMobilePhoneNumber;     // WPD_CONTACT_MOBILE_PHONE
    CAtlStringW strPrimaryFaxPhoneNumber; // WPD_CONTACT_PRIMARY_FAX
    CAtlStringW strAddressLine1;          // WPD_CONTACT_PERSONAL_POSTAL_ADDRESS_LINE1
    CAtlStringW strAddressLine2;          // WPD_CONTACT_PERSONAL_POSTAL_ADDRESS_LINE2
    CAtlStringW strAddressCity;           // WPD_CONTACT_PERSONAL_POSTAL_ADDRESS_CITY
    CAtlStringW strAddressPostalCode;     // WPD_CONTACT_PERSONAL_POSTAL_ADDRESS_POSTAL_CODE

    if (pValues != NULL)
    {
        // Read the contact property values from IPortableDeviceValues
        // NOTE: ALL values are not required to be present to create a valid VCARD file.  If no properties are
        //       found then a blank VCARD will be created.
        GetAtlStringValue(WPD_CONTACT_LAST_NAME, pValues, strLastName);                         // WPD_CONTACT_LAST_NAME
        GetAtlStringValue(WPD_CONTACT_FIRST_NAME, pValues, strFirstName);                       // WPD_CONTACT_FIRST_NAME
        GetAtlStringValue(WPD_CONTACT_DISPLAY_NAME, pValues, strDisplayName);                   // WPD_CONTACT_DISPLAY_NAME
        GetAtlStringValue(WPD_CONTACT_COMPANY_NAME, pValues, strCompanyName);                   // WPD_CONTACT_COMPANY_NAME
        GetAtlStringValue(WPD_CONTACT_ROLE, pValues, strRole);                                  // WPD_CONTACT_ROLE
        GetAtlStringValue(WPD_CONTACT_PRIMARY_PHONE, pValues, strPrimaryPhoneNumber);           // WPD_CONTACT_PRIMARY_PHONE or WPD_CONTACT_PERSONAL_PHONE
        GetAtlStringValue(WPD_CONTACT_BUSINESS_PHONE, pValues, strBusinessPhoneNumber);         // WPD_CONTACT_BUSINESS_PHONE
        GetAtlStringValue(WPD_CONTACT_MOBILE_PHONE, pValues, strMobilePhoneNumber);             // WPD_CONTACT_MOBILE_PHONE
        GetAtlStringValue(WPD_CONTACT_PRIMARY_FAX, pValues, strPrimaryFaxPhoneNumber);          // WPD_CONTACT_PRIMARY_FAX
        GetAtlStringValue(WPD_CONTACT_PERSONAL_POSTAL_ADDRESS_LINE1, pValues, strAddressLine1); // WPD_CONTACT_PERSONAL_POSTAL_ADDRESS_LINE1
        GetAtlStringValue(WPD_CONTACT_PERSONAL_POSTAL_ADDRESS_LINE2, pValues, strAddressLine2); // WPD_CONTACT_PERSONAL_POSTAL_ADDRESS_LINE2
        GetAtlStringValue(WPD_CONTACT_PERSONAL_POSTAL_ADDRESS_CITY, pValues, strAddressCity);   // WPD_CONTACT_PERSONAL_POSTAL_ADDRESS_CITY
        GetAtlStringValue(WPD_CONTACT_PERSONAL_POSTAL_ADDRESS_POSTAL_CODE, pValues, strAddressPostalCode); // WPD_CONTACT_PERSONAL_POSTAL_ADDRESS_POSTAL_CODE
    }

    // Create the VCARD from the properties found in the IPortableDeviceValues
    strVCard.Format(VCARD_FORMAT, strLastName.GetString(), strFirstName.GetString(),
                                  strDisplayName.GetString(),
                                  strCompanyName.GetString(),
                                  strRole.GetString(),
                                  strPrimaryPhoneNumber.GetString(),
                                  strBusinessPhoneNumber.GetString(),
                                  strMobilePhoneNumber.GetString(),
                                  strPrimaryFaxPhoneNumber.GetString(),
                                  strAddressLine1.GetString(), strAddressLine2.GetString(),
                                  strAddressCity.GetString(), strAddressPostalCode.GetString());
    return S_OK;
}

HRESULT UpdateDeviceFriendlyName(
    _In_ IPortableDeviceClassExtension*  pPortableDeviceClassExtension,
    _In_ LPCWSTR                         wszDeviceFriendlyName)
{
    HRESULT hr = S_OK;

    // If we were passed NULL parameters we have nothing to do, return S_OK.
    if ((pPortableDeviceClassExtension == NULL) || (wszDeviceFriendlyName == NULL))
    {
        return S_OK;
    }

    CComPtr<IPortableDeviceValues>  pParams;
    CComPtr<IPortableDeviceValues>  pResults;
    CComPtr<IPortableDeviceValues>  pValues;

    // Prepare to make a call to set the device information
    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDeviceValues, NULL, CLSCTX_INPROC_SERVER, IID_IPortableDeviceValues, (VOID**)&pParams);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceValues");
    }

    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDeviceValues, NULL, CLSCTX_INPROC_SERVER, IID_IPortableDeviceValues, (VOID**)&pResults);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceValues for results");
    }

    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDeviceValues, NULL, CLSCTX_INPROC_SERVER, IID_IPortableDeviceValues, (VOID**)&pValues);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceValues for results");
    }

    // Get the information values to update and set them in WPD_PROPERTY_CLASS_EXTENSION_DEVICE_INFORMATION_VALUES
    if (hr == S_OK)
    {
        hr = pValues->SetStringValue(WPD_DEVICE_FRIENDLY_NAME, wszDeviceFriendlyName);
        CHECK_HR(hr, ("Failed to set WPD_DEVICE_FRIENDLY_NAME"));
    }

    // Set the params
    if (hr == S_OK)
    {
        hr = pParams->SetGuidValue(WPD_PROPERTY_COMMON_COMMAND_CATEGORY, WPD_COMMAND_CLASS_EXTENSION_WRITE_DEVICE_INFORMATION.fmtid);
        CHECK_HR(hr, ("Failed to set WPD_PROPERTY_COMMON_COMMAND_CATEGORY"));
    }
    if (hr == S_OK)
    {
        hr = pParams->SetUnsignedIntegerValue(WPD_PROPERTY_COMMON_COMMAND_ID, WPD_COMMAND_CLASS_EXTENSION_WRITE_DEVICE_INFORMATION.pid);
        CHECK_HR(hr, ("Failed to set WPD_PROPERTY_COMMON_COMMAND_ID"));
    }
    if (hr == S_OK)
    {
        hr = pParams->SetIPortableDeviceValuesValue(WPD_PROPERTY_CLASS_EXTENSION_DEVICE_INFORMATION_VALUES, pValues);
        CHECK_HR(hr, ("Failed to set WPD_PROPERTY_CLASS_EXTENSION_DEVICE_INFORMATION_VALUES"));
    }

    // Make the call
    if (hr == S_OK)
    {
        hr = pPortableDeviceClassExtension->ProcessLibraryMessage(pParams, pResults);
        CHECK_HR(hr, ("Failed to process update device information message"));
    }

    // A Failed ProcessLibraryMessage operation for updating this value is not considered
    // fatal and should return S_OK.

    return S_OK;
}

HRESULT GetCommonResourceAttributes(
    _COM_Outptr_    IPortableDeviceValues** ppAttributes)
{
    HRESULT                         hr      = S_OK;
    CComPtr<IPortableDeviceValues>  pAttributes;

    if(ppAttributes == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL attributes parameter");
        return hr;
    }

    *ppAttributes = NULL;

    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_PortableDeviceValues,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceValues,
                              (VOID**) &pAttributes);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceValues");
    }

    // Add the attributes that are common to all our resources.
    if (SUCCEEDED(hr))
    {
        // Add a default value for size.  This will be overridden by the content objects with the actual value.
        hr = pAttributes->SetUnsignedIntegerValue(WPD_RESOURCE_ATTRIBUTE_TOTAL_SIZE, FAKE_DATA_SIZE);
        CHECK_HR(hr, "Failed to set WPD_RESOURCE_ATTRIBUTE_TOTAL_SIZE");
    }
    if (SUCCEEDED(hr))
    {
        hr = pAttributes->SetBoolValue(WPD_RESOURCE_ATTRIBUTE_CAN_READ, TRUE);
        CHECK_HR(hr, "Failed to set WPD_RESOURCE_ATTRIBUTE_CAN_READ");
    }
    if (SUCCEEDED(hr))
    {
        hr = pAttributes->SetBoolValue(WPD_RESOURCE_ATTRIBUTE_CAN_WRITE, FALSE);
        CHECK_HR(hr, "Failed to set WPD_RESOURCE_ATTRIBUTE_CAN_WRITE");
    }
    if (SUCCEEDED(hr))
    {
        hr = pAttributes->SetBoolValue(WPD_RESOURCE_ATTRIBUTE_CAN_DELETE, FALSE);
        CHECK_HR(hr, "Failed to set WPD_RESOURCE_ATTRIBUTE_CAN_DELETE");
    }
    if (SUCCEEDED(hr))
    {
        hr = pAttributes->SetUnsignedIntegerValue(WPD_RESOURCE_ATTRIBUTE_OPTIMAL_READ_BUFFER_SIZE, OPTIMAL_BUFFER_SIZE);
        CHECK_HR(hr, "Failed to set WPD_RESOURCE_ATTRIBUTE_OPTIMAL_READ_BUFFER_SIZE");
    }
    if (SUCCEEDED(hr))
    {
        hr = pAttributes->SetUnsignedIntegerValue(WPD_RESOURCE_ATTRIBUTE_OPTIMAL_WRITE_BUFFER_SIZE, OPTIMAL_BUFFER_SIZE);
        CHECK_HR(hr, "Failed to set WPD_RESOURCE_ATTRIBUTE_OPTIMAL_WRITE_BUFFER_SIZE");
    }

    // Return the resource attributes
    if (SUCCEEDED(hr))
    {
        hr = pAttributes->QueryInterface(IID_IPortableDeviceValues, (VOID**) ppAttributes);
        CHECK_HR(hr, "Failed to QI for IPortableDeviceValues on Wpd IPortableDeviceValues");
    }
    return hr;
}


