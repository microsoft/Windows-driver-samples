#include "FakeDevice.h.tmh"

#define NUM_VERTICAL_OBJECTS    7
#define NUM_IMAGE_OBJECTS       31
#define NUM_MUSIC_OBJECTS       8
#define NUM_VIDEO_OBJECTS       4
#define NUM_MEMO_OBJECTS        2
#define NUM_CONTACT_OBJECTS     5
#define STORAGE1_OBJECT_ID                      L"Storage1"
#define STORAGE2_OBJECT_ID                      L"Storage2"
#define RENDERING_INFORMATION_OBJECT_ID         L"RenderingInformation"
#define NETWORK_CONFIG_OBJECT_ID                L"NetworkConfig"
#define UNTYPEDDATA_FOLDER_OBJECT_ID            L"UntypedData"
#define CONTACT_FOLDER_OBJECT_ID                L"Phonebook"
#define MEDIA_FOLDER_OBJECT_ID                  L"Media folder"
#define MEMO_FOLDER_OBJECT_ID                   L"Memo folder"
#define IMAGE_FOLDER_OBJECT_ID                  L"Picture folder"
#define MUSIC_FOLDER_OBJECT_ID                  L"Music folder"
#define VIDEO_FOLDER_OBJECT_ID                  L"Video folder"

/**
 * This class represents an abstraction of a real device.
 * Driver implementors should replace this with their own
 * device I/O classes/libraries.
 */
class FakeDevice
{
public:
    FakeDevice() :
        m_dwLastObjectID(0)
    {

    }

    ~FakeDevice()
    {
        for(size_t index = 0; index < m_Content.GetCount(); index++)
        {
            delete m_Content[index];
            m_Content[index] = NULL;
        }
    }

    HRESULT InitializeContent(
        _In_ IPortableDeviceClassExtension *pPortableDeviceClassExtension)
    {
        HRESULT hr = S_OK;

        if(pPortableDeviceClassExtension == NULL)
        {
            hr = E_POINTER;
            CHECK_HR(hr, "Cannot have NULL parameter");
            return hr;
        }
        
        // Add device object
        CAutoPtr<DeviceObjectFakeContent> pDeviceObjectContent(new DeviceObjectFakeContent(pPortableDeviceClassExtension));
        if (pDeviceObjectContent)
        {
            pDeviceObjectContent->Name          = L"Fake Device";
            pDeviceObjectContent->ParentID      = L"";
            pDeviceObjectContent->ObjectID      = WPD_DEVICE_OBJECT_ID;
            pDeviceObjectContent->PersistentUniqueID.Format(L"PersistentUniqueID_%ws", pDeviceObjectContent->ObjectID.GetString());
            pDeviceObjectContent->CanDelete     = FALSE;
            pDeviceObjectContent->IsHidden      = TRUE;
            pDeviceObjectContent->ContentType   = WPD_CONTENT_TYPE_FUNCTIONAL_OBJECT;

            _ATLTRY
            {
                m_Content.Add(pDeviceObjectContent);
            }
            _ATLCATCH(e)
            {
                hr = e;
                CHECK_HR(hr, "ATL Exception when adding DeviceObjectFakeContent");
                return hr;
            }
            pDeviceObjectContent.Detach();
        }
        else
        {
            hr = E_OUTOFMEMORY;
            return hr;
        }

        // Add Toplevel object: Storage1
        CAutoPtr<StorageObjectFakeContent> pStorageContent(new StorageObjectFakeContent());
        if (pStorageContent)
        {
            pStorageContent->Name                  = STORAGE1_OBJECT_ID;
            pStorageContent->ParentID              = WPD_DEVICE_OBJECT_ID;
            pStorageContent->ObjectID              = pStorageContent->Name;
            pStorageContent->PersistentUniqueID.Format(L"PersistentUniqueID_%ws", pStorageContent->ObjectID.GetString());
            pStorageContent->CanDelete             = FALSE;
            pStorageContent->ContentType           = WPD_CONTENT_TYPE_FUNCTIONAL_OBJECT;
            pStorageContent->IsExternalStorage     = FALSE;
            pStorageContent->Capacity              = 120034890000;
            pStorageContent->FreeSpaceInBytes      = 60017445000;

            _ATLTRY
            {
                m_Content.Add(pStorageContent);
            }
            _ATLCATCH(e)
            {
                hr = e;
                CHECK_HR(hr, "ATL Exception when adding StorageObjectFakeContent");
                return hr;
            }
            pStorageContent.Detach();
        }
        else
        {
            hr = E_OUTOFMEMORY;
            return hr;
        }

        // Add Toplevel object: Storage2
        pStorageContent.Attach(new StorageObjectFakeContent());
        if (pStorageContent)
        {
            pStorageContent->Name                  = STORAGE2_OBJECT_ID;
            pStorageContent->ParentID              = WPD_DEVICE_OBJECT_ID;
            pStorageContent->ObjectID              = pStorageContent->Name;
            pStorageContent->PersistentUniqueID.Format(L"PersistentUniqueID_%ws", pStorageContent->ObjectID.GetString());
            pStorageContent->CanDelete             = FALSE;
            pStorageContent->ContentType           = WPD_CONTENT_TYPE_FUNCTIONAL_OBJECT;
            pStorageContent->IsExternalStorage     = TRUE;
            pStorageContent->Capacity              = 80000000000;
            pStorageContent->FreeSpaceInBytes      = 75000000000;

            _ATLTRY
            {
                m_Content.Add(pStorageContent);
            }
            _ATLCATCH(e)
            {
                hr = e;
                CHECK_HR(hr, "ATL Exception when adding StorageObjectFakeContent");
                return hr;
            }
            pStorageContent.Detach();
        }
        else
        {
            hr = E_OUTOFMEMORY;
            return hr;
        }

        // Add Toplevel object: RenderingInformation
        CAutoPtr<RenderingInformationFakeContent> pRenderingInformationContent(new RenderingInformationFakeContent());
        if (pRenderingInformationContent)
        {
            pRenderingInformationContent->Name                  = RENDERING_INFORMATION_OBJECT_ID;
            pRenderingInformationContent->ParentID              = WPD_DEVICE_OBJECT_ID;
            pRenderingInformationContent->ObjectID              = pRenderingInformationContent->Name;
            pRenderingInformationContent->PersistentUniqueID.Format(L"PersistentUniqueID_%ws", pRenderingInformationContent->ObjectID.GetString());
            pRenderingInformationContent->CanDelete             = FALSE;
            pRenderingInformationContent->IsHidden              = TRUE;
            pRenderingInformationContent->ContentType           = WPD_CONTENT_TYPE_FUNCTIONAL_OBJECT;

            _ATLTRY
            {
                m_Content.Add(pRenderingInformationContent);
            }
            _ATLCATCH(e)
            {
                hr = e;
                CHECK_HR(hr, "ATL Exception when adding RenderingInformationFakeContent");
                return hr;
            }
            pRenderingInformationContent.Detach();
        }
        else
        {
            hr = E_OUTOFMEMORY;
            return hr;
        }

        // Add Toplevel object: NetworkConfig
        CAutoPtr<NetworkConfigFakeContent> pNetworkConfigContent(new NetworkConfigFakeContent());
        if (pNetworkConfigContent)
        {
            pNetworkConfigContent->Name                  = NETWORK_CONFIG_OBJECT_ID;
            pNetworkConfigContent->ParentID              = WPD_DEVICE_OBJECT_ID;
            pNetworkConfigContent->ObjectID              = pNetworkConfigContent->Name;
            pNetworkConfigContent->PersistentUniqueID.Format(L"PersistentUniqueID_%ws", pNetworkConfigContent->ObjectID.GetString());
            pNetworkConfigContent->CanDelete             = FALSE;
            pNetworkConfigContent->IsHidden              = TRUE;
            pNetworkConfigContent->ContentType           = WPD_CONTENT_TYPE_FUNCTIONAL_OBJECT;
            pNetworkConfigContent->RestrictToContentTypes.Add(WPD_CONTENT_TYPE_NETWORK_ASSOCIATION);
            pNetworkConfigContent->RestrictToContentTypes.Add(WPD_CONTENT_TYPE_WIRELESS_PROFILE);

            _ATLTRY
            {
                m_Content.Add(pNetworkConfigContent);
            }
            _ATLCATCH(e)
            {
                hr = e;
                CHECK_HR(hr, "ATL Exception when adding NetworkConfigFakeContent");
                return hr;
            }
            pNetworkConfigContent.Detach();
        }
        else
        {
            hr = E_OUTOFMEMORY;
            return hr;
        }

        // Add Toplevel object: Untyped data folder
        CAutoPtr<FakeFolderContent> pFolderContent(new FakeFolderContent());
        if (pFolderContent)
        {
            pFolderContent->Name                  = UNTYPEDDATA_FOLDER_OBJECT_ID;
            pFolderContent->ParentID              = STORAGE2_OBJECT_ID;
            pFolderContent->ObjectID              = pFolderContent->Name;
            pFolderContent->FileName              = pFolderContent->Name;
            pFolderContent->PersistentUniqueID.Format(L"PersistentUniqueID_%ws", pFolderContent->ObjectID.GetString());
            pFolderContent->CanDelete             = FALSE;
            pFolderContent->ContentType           = WPD_CONTENT_TYPE_FOLDER;

            _ATLTRY
            {
                m_Content.Add(pFolderContent);
            }
            _ATLCATCH(e)
            {
                hr = e;
                CHECK_HR(hr, "ATL Exception when adding FakeFolderContent");
                return hr;
            }
            pFolderContent.Detach();
        }
        else
        {
            hr = E_OUTOFMEMORY;
            return hr;
        }

        // Add Toplevel object: Phonebook folder
        pFolderContent.Attach(new FakeFolderContent());
        if (pFolderContent)
        {
            pFolderContent->Name                  = CONTACT_FOLDER_OBJECT_ID;
            pFolderContent->ParentID              = STORAGE1_OBJECT_ID;
            pFolderContent->ObjectID              = pFolderContent->Name;
            pFolderContent->FileName              = pFolderContent->Name;
            pFolderContent->PersistentUniqueID.Format(L"PersistentUniqueID_%ws", pFolderContent->ObjectID.GetString());
            pFolderContent->CanDelete             = FALSE;
            pFolderContent->ContentType           = WPD_CONTENT_TYPE_FOLDER;
            pFolderContent->RestrictToContentTypes.Add(WPD_CONTENT_TYPE_CONTACT);

            _ATLTRY
            {
                m_Content.Add(pFolderContent);
            }
            _ATLCATCH(e)
            {
                hr = e;
                CHECK_HR(hr, "ATL Exception when adding FakeFolderContent");
                return hr;
            }
            pFolderContent.Detach();
        }
        else
        {
            hr = E_OUTOFMEMORY;
            return hr;
        }

        // Add Toplevel folder object with custom icon: Memos
        CAutoPtr<FakeMemoFolderContent> pMemoFolderContent(new FakeMemoFolderContent());
        if (pMemoFolderContent)
        {
            pMemoFolderContent->Name                  = MEMO_FOLDER_OBJECT_ID;
            pMemoFolderContent->ParentID              = STORAGE1_OBJECT_ID;
            pMemoFolderContent->ObjectID              = pMemoFolderContent->Name;
            pMemoFolderContent->FileName              = pMemoFolderContent->Name;
            pMemoFolderContent->PersistentUniqueID.Format(L"PersistentUniqueID_%ws", pMemoFolderContent->ObjectID.GetString());
            pMemoFolderContent->CanDelete             = FALSE;
            pMemoFolderContent->ContentType           = WPD_CONTENT_TYPE_FOLDER;
            pMemoFolderContent->RestrictToContentTypes.Add(WPD_CONTENT_TYPE_MEMO);

            _ATLTRY
            {
                m_Content.Add(pMemoFolderContent);
            }
            _ATLCATCH(e)
            {
                hr = e;
                CHECK_HR(hr, "ATL Exception when adding FakeMemoFolderContent");
                return hr;
            }
            pMemoFolderContent.Detach();
        }
        else
        {
            hr = E_OUTOFMEMORY;
            return hr;
        }

        // Add Toplevel object: Media folder
        pFolderContent.Attach(new FakeFolderContent());
        if (pFolderContent)
        {
            pFolderContent->Name                  = MEDIA_FOLDER_OBJECT_ID;
            pFolderContent->ParentID              = STORAGE1_OBJECT_ID;
            pFolderContent->ObjectID              = pFolderContent->Name;
            pFolderContent->FileName              = pFolderContent->Name;
            pFolderContent->PersistentUniqueID.Format(L"PersistentUniqueID_%ws", pFolderContent->ObjectID.GetString());
            pFolderContent->CanDelete             = FALSE;
            pFolderContent->ContentType           = WPD_CONTENT_TYPE_FOLDER;
            pFolderContent->RestrictToContentTypes.Add(WPD_CONTENT_TYPE_FOLDER);

            _ATLTRY
            {
                m_Content.Add(pFolderContent);
            }
            _ATLCATCH(e)
            {
                hr = e;
                CHECK_HR(hr, "ATL Exception when adding FakeFolderContent");
                return hr;
            }
            pFolderContent.Detach();
        }
        else
        {
            hr = E_OUTOFMEMORY;
            return hr;
        }

        // Add Media folder: Picture folder
        pFolderContent.Attach(new FakeFolderContent());
        if (pFolderContent)
        {
            pFolderContent->Name                  = IMAGE_FOLDER_OBJECT_ID;
            pFolderContent->ParentID              = MEDIA_FOLDER_OBJECT_ID;
            pFolderContent->ObjectID              = pFolderContent->Name;
            pFolderContent->FileName              = pFolderContent->Name;
            pFolderContent->PersistentUniqueID.Format(L"PersistentUniqueID_%ws", pFolderContent->ObjectID.GetString());
            pFolderContent->CanDelete             = FALSE;
            pFolderContent->ContentType           = WPD_CONTENT_TYPE_FOLDER;
            pFolderContent->RestrictToContentTypes.Add(WPD_CONTENT_TYPE_IMAGE);

            _ATLTRY
            {
                m_Content.Add(pFolderContent);
            }
            _ATLCATCH(e)
            {
                hr = e;
                CHECK_HR(hr, "ATL Exception when adding FakeFolderContent");
                return hr;
            }
            pFolderContent.Detach();
        }
        else
        {
            hr = E_OUTOFMEMORY;
            return hr;
        }

        // Add Media folder: Music folder
        pFolderContent.Attach(new FakeFolderContent());
        if (pFolderContent)
        {
            pFolderContent->Name                  = MUSIC_FOLDER_OBJECT_ID;
            pFolderContent->ParentID              = MEDIA_FOLDER_OBJECT_ID;
            pFolderContent->ObjectID              = pFolderContent->Name;
            pFolderContent->FileName              = pFolderContent->Name;
            pFolderContent->PersistentUniqueID.Format(L"PersistentUniqueID_%ws", pFolderContent->ObjectID.GetString());
            pFolderContent->CanDelete             = FALSE;
            pFolderContent->ContentType           = WPD_CONTENT_TYPE_FOLDER;
            pFolderContent->RestrictToContentTypes.Add(WPD_CONTENT_TYPE_AUDIO);

            _ATLTRY
            {
                m_Content.Add(pFolderContent);
            }
            _ATLCATCH(e)
            {
                hr = e;
                CHECK_HR(hr, "ATL Exception when adding FakeFolderContent");
                return hr;
            }
            pFolderContent.Detach();
        }
        else
        {
            hr = E_OUTOFMEMORY;
            return hr;
        }

        // Add Media folder: Video folder
        pFolderContent.Attach(new FakeFolderContent());
        if (pFolderContent)
        {
            pFolderContent->Name                  = VIDEO_FOLDER_OBJECT_ID;
            pFolderContent->ParentID              = MEDIA_FOLDER_OBJECT_ID;
            pFolderContent->ObjectID              = pFolderContent->Name;
            pFolderContent->FileName              = pFolderContent->Name;
            pFolderContent->PersistentUniqueID.Format(L"PersistentUniqueID_%ws", pFolderContent->ObjectID.GetString());
            pFolderContent->CanDelete             = FALSE;
            pFolderContent->ContentType           = WPD_CONTENT_TYPE_FOLDER;
            pFolderContent->RestrictToContentTypes.Add(WPD_CONTENT_TYPE_VIDEO);

            _ATLTRY
            {
                m_Content.Add(pFolderContent);
            }
            _ATLCATCH(e)
            {
                hr = e;
                CHECK_HR(hr, "ATL Exception when adding FakeFolderContent");
                return hr;
            }
            pFolderContent.Detach();
        }
        else
        {
            hr = E_OUTOFMEMORY;
            return hr;
        }

        // Add generic file content objects to storage 1
        for(DWORD dwIndex = 0; dwIndex < NUM_VERTICAL_OBJECTS; dwIndex++)
        {
            m_dwLastObjectID++;

            CAutoPtr<FakeGenericFileContent> pGenericFileContent(new FakeGenericFileContent());
            if (pGenericFileContent)
            {
                pGenericFileContent->Name.Format(L"Name%d", m_dwLastObjectID);
                pGenericFileContent->ParentID      = STORAGE1_OBJECT_ID;
                pGenericFileContent->ObjectID.Format(L"%d", m_dwLastObjectID);
                pGenericFileContent->PersistentUniqueID.Format(L"PersistentUniqueID_%ws", pGenericFileContent->ObjectID.GetString());
                pGenericFileContent->ContentType           = WPD_CONTENT_TYPE_UNSPECIFIED;
                pGenericFileContent->CanDelete             = FALSE;
                pGenericFileContent->IsHidden              = TRUE;
                pGenericFileContent->IsSystem              = TRUE;
                pGenericFileContent->FileName              = pGenericFileContent->Name;

                _ATLTRY
                {
                    m_Content.Add(pGenericFileContent);
                }
                _ATLCATCH(e)
                {
                    hr = e;
                    CHECK_HR(hr, "ATL Exception when adding FakeGenericFileContent");
                    return hr;
                }
                pGenericFileContent.Detach();
            }
            else
            {
                hr = E_OUTOFMEMORY;
                return hr;
            }
        }

        // Add image objects to the image folder
        for(DWORD dwImageIndex = 0; dwImageIndex < NUM_IMAGE_OBJECTS; dwImageIndex++)
        {
            m_dwLastObjectID++;

            CAutoPtr<FakeImageContent> pImageContent(new FakeImageContent());
            if (pImageContent)
            {
                pImageContent->Name.Format(L"Image%d", m_dwLastObjectID);
                pImageContent->ParentID      = IMAGE_FOLDER_OBJECT_ID;
                pImageContent->ObjectID.Format(L"%d", m_dwLastObjectID);
                pImageContent->PersistentUniqueID.Format(L"PersistentUniqueID_%ws", pImageContent->ObjectID.GetString());
                pImageContent->ContentType   = WPD_CONTENT_TYPE_IMAGE;
                pImageContent->FileName.Format(L"ImageFile_%d.jpg", m_dwLastObjectID);

                _ATLTRY
                {
                    m_Content.Add(pImageContent);
                }
                _ATLCATCH(e)
                {
                    hr = e;
                    CHECK_HR(hr, "ATL Exception when adding FakeImageContent");
                    return hr;
                }
                pImageContent.Detach();
            }
            else
            {
                hr = E_OUTOFMEMORY;
                return hr;
            }
        }

        // Add music objects to the music folder
        for(DWORD dwMusicIndex = 0; dwMusicIndex < NUM_MUSIC_OBJECTS; dwMusicIndex++)
        {
            m_dwLastObjectID++;

            CAutoPtr<FakeMusicContent> pMusicContent(new FakeMusicContent());
            if (pMusicContent)
            {
                pMusicContent->Name.Format(L"Music%d", m_dwLastObjectID);
                pMusicContent->ParentID      = MUSIC_FOLDER_OBJECT_ID;
                pMusicContent->ObjectID.Format(L"%d", m_dwLastObjectID);
                pMusicContent->PersistentUniqueID.Format(L"PersistentUniqueID_%ws", pMusicContent->ObjectID.GetString());
                pMusicContent->ContentType   = WPD_CONTENT_TYPE_AUDIO;
                pMusicContent->FileName.Format(L"MusicFile_%d.wma", m_dwLastObjectID);

                _ATLTRY
                {
                    m_Content.Add(pMusicContent);
                }
                _ATLCATCH(e)
                {
                    hr = e;
                    CHECK_HR(hr, "ATL Exception when adding FakeMusicContent");
                    return hr;
                }
                pMusicContent.Detach();
            }
            else
            {
                hr = E_OUTOFMEMORY;
                return hr;
            }
        }

        // Add video objects to the video folder
        for(DWORD dwVideoIndex = 0; dwVideoIndex < NUM_VIDEO_OBJECTS; dwVideoIndex++)
        {
            m_dwLastObjectID++;

            CAutoPtr<FakeVideoContent> pVideoContent(new FakeVideoContent());
            if (pVideoContent)
            {
                pVideoContent->Name.Format(L"Video%d", m_dwLastObjectID);
                pVideoContent->ParentID      = VIDEO_FOLDER_OBJECT_ID;
                pVideoContent->ObjectID.Format(L"%d", m_dwLastObjectID);
                pVideoContent->PersistentUniqueID.Format(L"PersistentUniqueID_%ws", pVideoContent->ObjectID.GetString());
                pVideoContent->ContentType   = WPD_CONTENT_TYPE_VIDEO;
                pVideoContent->FileName.Format(L"VideoFile_%d.wmv", m_dwLastObjectID);

                _ATLTRY
                {
                    m_Content.Add(pVideoContent);
                }
                _ATLCATCH(e)
                {
                    hr = e;
                    CHECK_HR(hr, "ATL Exception when adding FakeVideoContent");
                    return hr;
                }
                pVideoContent.Detach();
            }
            else
            {
                hr = E_OUTOFMEMORY;
                return hr;
            }
        }

        // Add contact objects to the contact folder
        for(DWORD dwContactIndex = 0; dwContactIndex < NUM_CONTACT_OBJECTS; dwContactIndex++)
        {
            m_dwLastObjectID++;

            CAutoPtr<FakeContactContent> pContactContent(new FakeContactContent());
            if (pContactContent)
            {
                if ((m_dwLastObjectID % 2) == 0) // Mark every second contact as having a photo
                {
                    pContactContent->bHasContactPhoto = TRUE;
                }
                pContactContent->Name.Format(L"Contact%d", m_dwLastObjectID);
                pContactContent->ParentID       = CONTACT_FOLDER_OBJECT_ID;
                pContactContent->ObjectID.Format(L"%d", m_dwLastObjectID);
                pContactContent->PersistentUniqueID.Format(L"PersistentUniqueID_%ws", pContactContent->ObjectID.GetString());
                pContactContent->ContentType    = WPD_CONTENT_TYPE_CONTACT;
                pContactContent->DisplayName.Format(L"Surname%d, FirstName%d", dwContactIndex, dwContactIndex);
                pContactContent->PrimaryPhone   = L"(425) 555 0821";
                pContactContent->WorkPhone      = L"(425) 556 6010";
                pContactContent->CellPhone      = L"(206) 557 1441";

                _ATLTRY
                {
                    m_Content.Add(pContactContent);
                }
                _ATLCATCH(e)
                {
                    hr = e;
                    CHECK_HR(hr, "ATL Exception when adding FakeContactContent");
                    return hr;
                }
                pContactContent.Detach();
            }
            else
            {
                hr = E_OUTOFMEMORY;
                return hr;
            }
        }

        // Add memo objects to the memo folder
        for(DWORD dwMemoIndex = 0; dwMemoIndex < NUM_MEMO_OBJECTS; dwMemoIndex++)
        {
            m_dwLastObjectID++;

            CAutoPtr<FakeMemoContent> pMemoContent(new FakeMemoContent());
            if (pMemoContent)
            {
                pMemoContent->Name.Format(L"Memo%d", m_dwLastObjectID);
                pMemoContent->ParentID      = MEMO_FOLDER_OBJECT_ID;
                pMemoContent->ObjectID.Format(L"%d", m_dwLastObjectID);
                pMemoContent->PersistentUniqueID.Format(L"PersistentUniqueID_%ws", pMemoContent->ObjectID.GetString());
                pMemoContent->ContentType   = WPD_CONTENT_TYPE_MEMO;
                pMemoContent->FileName.Format(L"MemoFile_%d.mem", m_dwLastObjectID);

                _ATLTRY
                {
                    m_Content.Add(pMemoContent);
                }
                _ATLCATCH(e)
                {
                    hr = e;
                    CHECK_HR(hr, "ATL Exception when adding FakeMemoContent");
                    return hr;
                }
                pMemoContent.Detach();
            }
            else
            {
                hr = E_OUTOFMEMORY;
                return hr;
            }
        }
        
        return hr;
    }

    bool FindNext(      const DWORD           dwStartIndex,
                  _In_  const CAtlStringW&    strParentID,
                  _Out_ CAtlStringW&          strObjectID,
                  _Out_ DWORD*                pdwNextStartIndex)
    {
        DWORD curIndex = 0;
        bool  bFound   = false;
        strObjectID = L"";
        *pdwNextStartIndex = 0;

        if(dwStartIndex >= m_Content.GetCount())
        {
            return false;
        }

        for(curIndex = dwStartIndex; curIndex < m_Content.GetCount(); curIndex++)
        {
            if(m_Content[curIndex]->ParentID == strParentID)
            {
                strObjectID = m_Content[curIndex]->ObjectID;
                bFound = true;
                break;
            }
        }

        if(bFound)
        {
            *pdwNextStartIndex = curIndex + 1;
        }
        else
        {
            *pdwNextStartIndex = (DWORD) m_Content.GetCount();
        }

        return bFound;
    }

    /**
     * Returns a pointer to the fake content in pElement corresponding to
     * the specified ObjectID.
     * Return value is true if found, otherwise false.
     */
    _Success_(return)
    bool GetContent(_In_ LPCWSTR pszObjectID, _Outptr_result_nullonfailure_ FakeContent** ppElement)
    {
        bool    bFound  = false;

        if((pszObjectID == NULL) ||
           (ppElement   == NULL))
        {
            return false;
        }

        *ppElement = NULL;

        for(DWORD Index = 0; Index < m_Content.GetCount(); Index++)
        {
            if(m_Content[Index]->ObjectID == pszObjectID)
            {
                *ppElement = m_Content[Index];
                bFound = true;
                break;
            }
        }

        return bFound;
    }

    /**
     * Returns the index of the fake content corresponding to the
     * specified ObjectID.
     * Return value is true if found, otherwise false.
     */
    bool GetContentIndex(_In_ LPCWSTR pszObjectID, _Out_ DWORD* pIndex)
    {
        bool bFound  = false;

        if((pszObjectID == NULL) ||
           (pIndex      == NULL))
        {
            return false;
        }

        *pIndex = 0;

        for(DWORD Index = 0; Index < m_Content.GetCount(); Index++)
        {
            if(m_Content[Index]->ObjectID == pszObjectID)
            {
                *pIndex = Index;
                bFound = true;
                break;
            }
        }

        return bFound;
    }

    HRESULT GetSupportedProperties(
        _In_         LPCWSTR                         pszObjectID,
        _COM_Outptr_ IPortableDeviceKeyCollection**  ppCollection)
    {
        HRESULT         hr          = S_OK;
        FakeContent*    pElement    = NULL;

        *ppCollection = NULL;

        if(GetContent(pszObjectID, &pElement))
        {
            hr = pElement->GetSupportedProperties(ppCollection);
            CHECK_HR(hr, "Failed to get supported properties on element [%ws]", pszObjectID ? pszObjectID : L"NULL");
        }
        else
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "Invalid ObjectID [%ws]", pszObjectID ? pszObjectID : L"NULL");
        }

        return hr;
    }

    HRESULT GetAllValues(
        _In_         LPCWSTR                 pszObjectID,
        _COM_Outptr_ IPortableDeviceValues** ppValues)
    {
        HRESULT                 hr          = S_OK;
        FakeContent*            pElement    = NULL;

        if(ppValues == NULL)
        {
            hr = E_POINTER;
            CHECK_HR(hr, "Cannot have NULL attributes parameter");
            return hr;
        }

        *ppValues = NULL;

        if(GetContent(pszObjectID, &pElement))
        {
            CComPtr<IPortableDeviceValues> pValues;

            hr = CoCreateInstance(CLSID_PortableDeviceValues,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IPortableDeviceValues,
                                  (VOID**) &pValues);
            CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceValues");

            if (SUCCEEDED(hr))
            {
                hr = pElement->GetAllValues(pValues);
                CHECK_HR(hr, "Failed to fill property values");

                if (SUCCEEDED(hr))
                {
                    // Keep hr intact in case it is S_FALSE
                    HRESULT hrTemp = pValues->QueryInterface(IID_IPortableDeviceValues, (VOID**) ppValues);

                    if (FAILED(hrTemp))
                    {
                        hr = hrTemp;
                        CHECK_HR(hr, "Failed to QI for IPortableDeviceValues on Wpd IPortableDeviceValues");
                    }
                }
            }
        }
        else
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "Invalid ObjectID [%ws]", pszObjectID ? pszObjectID : L"NULL");
        }

        return hr;
    }

    /**
     * This fake device is an unrealistic example from a performance standpoint, since
     * requesting some values is more costly than requesting all values.
     * The reason for this is that is is easier for the underlying objects to simply return all the
     * values, and for us to filter out only the ones we want here.
     * It is not expected that real devices function this way.
     */
    HRESULT GetValues(
        _In_            LPCWSTR                         pszObjectID,
        _In_            IPortableDeviceKeyCollection*   pKeys,
        _COM_Outptr_    IPortableDeviceValues**         ppValues)
    {
        HRESULT                 hr      = S_OK;
        BOOL                    bError  = FALSE;
        CComPtr<IPortableDeviceValues>  pAllValues;
        CComPtr<IPortableDeviceValues>  pValues;

        if((pszObjectID == NULL) ||
           (pKeys       == NULL) ||
           (ppValues    == NULL))
        {
            hr = E_POINTER;
            CHECK_HR(hr, ("Cannot have NULL parameter"));
            return hr;
        }

        *ppValues = NULL;

        if (SUCCEEDED(hr))
        {
            hr = CoCreateInstance(CLSID_PortableDeviceValues,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IPortableDeviceValues,
                                  (VOID**) &pValues);
            CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDevicePropVariantCollection");
        }

        // Get ALL values (NOTE: This is inefficient)
        if (SUCCEEDED(hr))
        {
            hr = GetAllValues(pszObjectID, &pAllValues);
            CHECK_HR(hr, "Failed to get property values [%ws]", pszObjectID);
        }

        // Return only the requested values
        if (SUCCEEDED(hr))
        {
            PROPVARIANT pv      = {0};
            DWORD       dwIndex = 0;
            PROPERTYKEY Key     = WPD_PROPERTY_NULL;
            while(SUCCEEDED(hr) && pKeys->GetAt(dwIndex, &Key) == S_OK)
            {
                hr = pAllValues->GetValue(Key, &pv);
                if(FAILED(hr))
                {
                    bError = TRUE;
                    hr = pValues->SetErrorValue(Key, hr);
                    CHECK_HR(hr, "Failed to set error value for %ws.%d", (LPWSTR)CComBSTR(Key.fmtid),Key.pid);
                }
                else
                {
                    hr = pValues->SetValue(Key, &pv);
                    CHECK_HR(hr, "Failed to set property value %ws.%d", (LPWSTR)CComBSTR(Key.fmtid),Key.pid);
                }

                PropVariantClear(&pv);
                dwIndex++;
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = pValues->QueryInterface(IID_IPortableDeviceValues, (VOID**) ppValues);
            CHECK_HR(hr, "Failed to QI for IPortableDeviceValues on Wpd IPortableDeviceValues");
        }

        if (SUCCEEDED(hr) &&
           (bError == TRUE))
        {
            hr = S_FALSE;
        }

        return hr;
    }

    HRESULT WriteProperty(
        _In_    LPCWSTR         pszObjectID,
        _In_    REFPROPERTYKEY  key,
        _In_    REFPROPVARIANT  Value)
    {
        HRESULT         hr          = S_OK;
        FakeContent*    pElement    = NULL;

        if(GetContent(pszObjectID, &pElement))
        {
            hr = pElement->WriteValue(key, Value);
            CHECK_HR(hr, "Failed to write property value");
        }
        else
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "Invalid ObjectID [%ws]", pszObjectID ? pszObjectID : L"NULL");
        }

        return hr;
    }

    HRESULT WritePropertiesOnObject(
        _In_            LPCWSTR                 pszObjectID,
        _In_            IPortableDeviceValues*  pValues,
        _COM_Outptr_    IPortableDeviceValues** ppWriteResults)
    {
        HRESULT                 hr              = S_OK;
        bool                    bErrorOnWrite   = false;
        DWORD                   cValues         = 0;
        CComPtr<IPortableDeviceValues> pWriteResults;

        *ppWriteResults = NULL;

        // Create property store  for values
        if (SUCCEEDED(hr))
        {
            hr = CoCreateInstance(CLSID_PortableDeviceValues,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IPortableDeviceValues,
                                  (VOID**)&pWriteResults);
            CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceValues");
        }

        if (SUCCEEDED(hr))
        {
            hr = pValues->GetCount(&cValues);
            CHECK_HR(hr, "Failed to get property count");
        }

        if (SUCCEEDED(hr))
        {
            for(DWORD dwIndex = 0; dwIndex < cValues; dwIndex++)
            {
                HRESULT             hrWrite = S_OK;
                PROPERTYKEY         Key     = {0};
                PropVariantWrapper  pvValue;
                PropVariantWrapper  pvWriteResult;

                hrWrite = pValues->GetAt(dwIndex, &Key, &pvValue);
                CHECK_HR(hrWrite, "Failed to get property key/value at index %d", dwIndex);

                if (SUCCEEDED(hrWrite))
                {
                    FakeContent*    pElement    = NULL;
                    if(GetContent(pszObjectID, &pElement))
                    {
                        hr = pElement->WriteValue(Key, pvValue);
                        CHECK_HR(hr, "Failed to write property value");
                    }
                    else
                    {
                        hr = E_INVALIDARG;
                        CHECK_HR(hr, "Invalid ObjectID [%ws]", pszObjectID ? pszObjectID : L"NULL");
                    }
                }

                // Always set the write result for this property
                pvWriteResult.SetErrorValue(hrWrite);
                if(FAILED(hrWrite))
                {
                    bErrorOnWrite = true;
                }

                hrWrite  = pWriteResults->SetValue(Key, &pvWriteResult);
                CHECK_HR(hrWrite, "Failed to set write result for property %ws.%d", CComBSTR(Key.fmtid), Key.pid);
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = pWriteResults->QueryInterface(IID_IPortableDeviceValues, (VOID**)ppWriteResults);
            CHECK_HR(hr, "Failed to QI for IPortableDeviceValues on IPortableDeviceValues");
        }

        // Remember to set the hr to S_FALSE if there were any failures writing a property.
        if((hr            == S_OK) &&
           (bErrorOnWrite == true))
        {
            hr = S_FALSE;
        }

        return hr;
    }

    HRESULT GetAttributes(
        _In_            LPCWSTR                 pszObjectID,
        _In_            REFPROPERTYKEY          key,
        _COM_Outptr_    IPortableDeviceValues** ppAttributes)
    {
        HRESULT         hr          = S_OK;
        FakeContent*    pElement    = NULL;

        if(ppAttributes == NULL)
        {
            hr = E_POINTER;
            CHECK_HR(hr, "Cannot have NULL attributes parameter");
            return hr;
        }

        *ppAttributes = NULL;

        if(GetContent(pszObjectID, &pElement))
        {
            hr = pElement->GetAttributes(key, ppAttributes);
            CHECK_HR(hr, "Failed to get attributes for %ws.%d", CComBSTR(key.fmtid), key.pid);
        }
        else
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "Invalid ObjectID [%ws]", pszObjectID);
        }
        return hr;
    }

    HRESULT GetSupportedResources(
        _In_            LPCWSTR                         pszObjectID,
        _COM_Outptr_    IPortableDeviceKeyCollection**  ppCollection)
    {
        HRESULT         hr          = S_OK;
        FakeContent*    pElement    = NULL;

        *ppCollection = NULL;

        if(GetContent(pszObjectID, &pElement))
        {
            hr = pElement->GetSupportedResources(ppCollection);
            CHECK_HR(hr, "Failed to get supported resources on element [%ws]", pszObjectID ? pszObjectID : L"NULL");
        }
        else
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "Invalid ObjectID [%ws]", pszObjectID ? pszObjectID : L"NULL");
        }

        return hr;
    }

    HRESULT GetResourceAttributes(
        _In_            LPCWSTR                 pszObjectID,
        _In_            REFPROPERTYKEY          key,
        _COM_Outptr_    IPortableDeviceValues** ppAttributes)
    {
        HRESULT         hr          = S_OK;
        FakeContent*    pElement    = NULL;

        if(ppAttributes == NULL)
        {
            hr = E_POINTER;
            CHECK_HR(hr, "Cannot have NULL attributes parameter");
            return hr;
        }

        *ppAttributes = NULL;

        if(GetContent(pszObjectID, &pElement))
        {
            hr = pElement->GetResourceAttributes(key, ppAttributes);
            CHECK_HR(hr, "Failed to get resource attributes for %ws.%d", CComBSTR(key.fmtid), key.pid);
        }
        else
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "Invalid ObjectID [%ws]", pszObjectID);
        }
        return hr;
    }

    HRESULT ReadData(
        _In_    LPCWSTR         pszObjectID,
        _In_    REFPROPERTYKEY  ResourceKey,
                DWORD           dwStartByte,
        _Out_writes_to_(dwNumBytesToRead, *pdwNumBytesRead) BYTE*   pBuffer,
                DWORD           dwNumBytesToRead,
        _Out_   DWORD*          pdwNumBytesRead)
    {
        HRESULT         hr          = S_OK;
        FakeContent*    pElement    = NULL;

        if((pBuffer         == NULL) ||
           (pdwNumBytesRead == NULL))
        {
            hr = E_POINTER;
            CHECK_HR(hr, "Cannot have NULL parameter");
            return hr;
        }

        *pdwNumBytesRead = 0;

        if(GetContent(pszObjectID, &pElement))
        {
            hr = pElement->ReadData(ResourceKey, dwStartByte, pBuffer, dwNumBytesToRead, pdwNumBytesRead);
            CHECK_HR(hr, "Failed to read resource data for %ws.%d", CComBSTR(ResourceKey.fmtid), ResourceKey.pid);
        }
        else
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "Invalid ObjectID [%ws]", pszObjectID);
        }
        return hr;
    }

    HRESULT WriteData(
        _In_    LPCWSTR         pszObjectID,
        _In_    REFPROPERTYKEY  ResourceKey,
                DWORD           dwStartByte,
        _In_reads_(dwNumBytesToWrite)    BYTE* pBuffer,
                DWORD           dwNumBytesToWrite,
        _Out_   DWORD*          pdwNumBytesWritten)
    {
        HRESULT         hr          = S_OK;
        FakeContent*    pElement    = NULL;

        if((pBuffer             == NULL) ||
           (pdwNumBytesWritten  == NULL))
        {
            hr = E_POINTER;
            CHECK_HR(hr, "Cannot have NULL parameter");
            return hr;
        }

        *pdwNumBytesWritten = 0;

        if(GetContent(pszObjectID, &pElement))
        {
            hr = pElement->WriteData(ResourceKey, dwStartByte, pBuffer, dwNumBytesToWrite, pdwNumBytesWritten);
            CHECK_HR(hr, "Failed to write resource data for %ws.%d", CComBSTR(ResourceKey.fmtid), ResourceKey.pid);
        }
        else
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "Invalid ObjectID [%ws]", pszObjectID);
        }
        return hr;
    }

    HRESULT MarkChildrenForDeletion(
        _In_    LPCWSTR pszParentObjectID)
    {
        HRESULT hr      = S_OK;

        CAtlStringW strParentObjectID = pszParentObjectID;
        for (size_t Index = 0; Index < m_Content.GetCount(); Index++)
        {
            if (m_Content[Index]->ParentID == strParentObjectID)
            {
                // Mark this object
                m_Content[Index]->MarkedForDeletion = TRUE;
                hr = MarkChildrenForDeletion(m_Content[Index]->ObjectID);
                if (FAILED(hr))
                {
                    break;
                }
            }
        }
        return hr;
    }

    HRESULT RemoveObjectsMarkedForDeletion()
    {
        HRESULT hr      = S_OK;

        for (size_t Index = m_Content.GetCount(); Index > 0; Index--)
        {
            if (m_Content[Index - 1]->MarkedForDeletion == TRUE)
            {
                // Delete this object
                FakeContent* pContent = m_Content[Index - 1];
                m_Content.RemoveAt(Index - 1);
                delete pContent;
            }
        }

        // Removes empty elements
        m_Content.FreeExtra();

        return hr;
    }

    BOOL CanDeleteObject(
        _In_    LPCWSTR pszObjectID)
    {
        BOOL bCanDelete = FALSE;

        for (size_t Index = 0; Index < m_Content.GetCount(); Index++)
        {
            if (m_Content[Index]->ObjectID == pszObjectID)
            {
                bCanDelete = m_Content[Index]->CanDelete;
                break;
            }
        }
        return bCanDelete;
    }

    HRESULT DeleteObject(
                DWORD   dwOptions,
        _In_    LPCWSTR pszObjectID)
    {
        HRESULT hr      = S_OK;
        DWORD   dwIndex = 0;

        if(pszObjectID == NULL)
        {
            hr = E_POINTER;
            CHECK_HR(hr, "Cannot have NULL parameter");
            return hr;
        }

        // Do an access check to verify whether the object may be deleted
        if(CanDeleteObject(pszObjectID) == FALSE)
        {
            hr = E_ACCESSDENIED;
            CHECK_HR(hr, "Failed to delete read-only object %ws", pszObjectID);
        }

        // Objects can only be deleted with the no recursion flag
        // if they have no children.
        if ((dwOptions == PORTABLE_DEVICE_DELETE_NO_RECURSION) &&
            (HasChildren(pszObjectID) == TRUE))
        {
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_OPERATION);
            return hr;
        }

        // Loop through the objects and delete the children only if
        // the recursive option is specified.
        if (dwOptions == PORTABLE_DEVICE_DELETE_WITH_RECURSION)
        {
            hr = MarkChildrenForDeletion(pszObjectID);
            CHECK_HR(hr, "Error attempting to mark objects for (recursive) deletion");
        }

        // Mark this object for deletion
        if (hr == S_OK)
        {
            if(GetContentIndex(pszObjectID, &dwIndex))
            {
                m_Content[dwIndex]->MarkedForDeletion = TRUE;
            }
            else
            {
                hr = E_INVALIDARG;
                CHECK_HR(hr, "Invalid ObjectID [%ws]", pszObjectID);
            }
        }

        // Delete the objects
        if (hr == S_OK)
        {
            hr = RemoveObjectsMarkedForDeletion();
            CHECK_HR(hr, "Failed to remove objects marked for deletion");
        }

        return hr;
    }

    BOOL HasChildren(
        _In_    LPCWSTR pszObjectID)
    {
        BOOL bHasChild = FALSE;

        if (pszObjectID == NULL)
        {
            bHasChild = FALSE;
            return bHasChild;
        }

        CAtlStringW strObjectID = pszObjectID;
        for (size_t Index = 0; Index < m_Content.GetCount(); Index++)
        {
            if (m_Content[Index]->ParentID == strObjectID)
            {
                bHasChild = TRUE;
                break;
            }
        }

        return bHasChild;
    }

    HRESULT SaveNewObject(
        _In_                            IPortableDeviceValues*  pObjectProperties,
        _Outptr_result_nullonfailure_   LPWSTR*                 ppszObjectID)
    {
        HRESULT     hr              = S_OK;
        DWORD       dwParentIndex   = 0;
        LPWSTR      pszObjectName   = NULL;
        LPWSTR      pszParentID     = NULL;
        GUID        guidContentType = WPD_CONTENT_TYPE_UNSPECIFIED;

        if(pObjectProperties == NULL || ppszObjectID == NULL)
        {
            hr = E_POINTER;
            CHECK_HR(hr, ("Cannot have NULL parameter"));
            return hr;
        }

        *ppszObjectID = NULL;

        // Get the content type
        hr = pObjectProperties->GetGuidValue(WPD_OBJECT_CONTENT_TYPE, &guidContentType);
        CHECK_HR(hr, "Failed to get WPD_OBJECT_CONTENT_TYPE");

        // Get ParentID from pValues
        if (SUCCEEDED(hr))
        {
            hr = pObjectProperties->GetStringValue(WPD_OBJECT_PARENT_ID, &pszParentID);
            CHECK_HR(hr, "Failed to get WPD_OBJECT_PARENT_ID");
        }

        // Get Object Name from pValues
        if (SUCCEEDED(hr))
        {
            hr = pObjectProperties->GetStringValue(WPD_OBJECT_NAME, &pszObjectName);
            CHECK_HR(hr, "Failed to get WPD_OBJECT_NAME");
        }

        if (SUCCEEDED(hr) && !GetContentIndex(pszParentID, &dwParentIndex))
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "Invalid Parent ObjectID [%ws]", pszParentID);
        }

        // Check whether the parent can hold objects of this content type
        if (SUCCEEDED(hr))
        {
            hr = IsValidContentType(guidContentType, m_Content[dwParentIndex]->RestrictToContentTypes);
            CHECK_HR(hr, "Object named [%ws] could not be created,  because parent [%ws] does not support this content type", pszObjectName, pszParentID);
        }

        if (SUCCEEDED(hr))
        {
            FakeContent* pContent;

            // Create the object
            hr = CreateContentObject(pszObjectName, pszParentID, guidContentType, pObjectProperties, &pContent);
            CHECK_HR(hr, "Failed to create object named %ws", pszObjectName);

            // Add it to the sample driver's internal list of content objects
            if (SUCCEEDED(hr))
            {
                m_Content.Add(pContent);
                *ppszObjectID = AtlAllocTaskWideString(pContent->ObjectID);
                if(*ppszObjectID == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    CHECK_HR(hr, "Faield to allocate memory for newly created object's ID");
                }
            }
        }

        // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
        CoTaskMemFree(pszParentID);
        // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
        CoTaskMemFree(pszObjectName);

        return hr;
    }

    const CAtlStringW GetParentID(
        _In_    LPCWSTR pszObjectID)
    {
        CAtlStringW strParent = L"";

        for (size_t Index = 0; Index < m_Content.GetCount(); Index++)
        {
            if(m_Content[Index]->ObjectID.CompareNoCase(pszObjectID) == 0)
            {
                strParent = m_Content[Index]->ParentID;
                break;
            }
        }

        return strParent;
    }

    BOOL GetDepthFromParent(
        _In_    LPCWSTR pszObjectID,
        _In_    LPCWSTR pszParentObjectID,
        _Out_   DWORD*  pdwDepth)
    {
        HRESULT hr          = S_OK;
        BOOL    bDescendant = FALSE;

        if((pszObjectID         == NULL) ||
           (pszParentObjectID   == NULL) ||
           (pdwDepth            == NULL))
        {
            hr = E_POINTER;
            CHECK_HR(hr, ("Cannot have NULL parameter"));
            return bDescendant;
        }

        *pdwDepth = 0;

        DWORD       dwDepth         = 0;
        CAtlStringW strCurObject    = pszObjectID;

        while((strCurObject.CompareNoCase(pszParentObjectID) != 0) &&
              (strCurObject.GetLength() > 0))
        {
            strCurObject = GetParentID(strCurObject);
            dwDepth++;
        }

        if(strCurObject.CompareNoCase(pszParentObjectID) == 0)
        {
            bDescendant = TRUE;
            *pdwDepth   = dwDepth;
        }

        return bDescendant;
    }

#pragma warning(suppress: 6388) // PREFast bug means here there is a false positive for the call to CComPtr<>::QueryInterface().
    HRESULT GetObjectIDsByFormat(
        _In_            REFGUID                                 guidObjectFormat,
        _In_            LPCWSTR                                 pszParentObjectID,
                        DWORD                                   dwDepth,
        _COM_Outptr_    IPortableDevicePropVariantCollection**  ppObjectIDs)
    {
        HRESULT hr = S_OK;
        CComPtr<IPortableDevicePropVariantCollection> pObjectIDs;

        if(ppObjectIDs == NULL)
        {
            hr = E_POINTER;
            CHECK_HR(hr, ("Cannot have NULL parameter"));
            return hr;
        }

        *ppObjectIDs = NULL;

        // Create the ObjectID collection.  Since this sample driver does not
        // have any real hardware, it cannot take advantage of the devices
        // 'bulk' operations, and so we must simulate it here.
        // For simplicity, this sample driver simply walks the list of
        // objects, finds out whether they are of the specified format and depth, and
        // if the are then adds them to an object list.  This is
        // easy to do, but not the most efficient way to do it.
        if (SUCCEEDED(hr))
        {
            hr = CoCreateInstance(CLSID_PortableDevicePropVariantCollection,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IPortableDevicePropVariantCollection,
                                  (VOID**) &pObjectIDs);
            CHECK_HR(hr, "Failed to CoCreate CLSID_IPortableDevicePropVariantCollection");
        }

        if (SUCCEEDED(hr))
        {
            // Walk the list of objects
            PROPVARIANT pv = {0};
            PropVariantInit(&pv);
            pv.vt = VT_LPWSTR;
            for (size_t Index = 0; Index < m_Content.GetCount(); Index++)
            {
                // Check whether the object is of the correct format
                if((m_Content[Index]->GetObjectFormat() == guidObjectFormat) || (guidObjectFormat == WPD_OBJECT_FORMAT_ALL))
                {
                    DWORD dwObjectDepth = 0;
                    // find out the depth of this object from the parent
                    if(GetDepthFromParent(m_Content[Index]->ObjectID, pszParentObjectID, &dwObjectDepth))
                    {
                        // If it is within the appropriate depth, add it
                        if(dwObjectDepth <= dwDepth)
                        {
                            pv.pwszVal = (LPWSTR) m_Content[Index]->ObjectID.GetString();
                            hr = pObjectIDs->Add(&pv);
                            CHECK_HR(hr, "Failed to add next ObjectID to collection");
                            if(FAILED(hr))
                            {
                                break;
                            }
                        }
                    }
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = pObjectIDs->QueryInterface(IID_PPV_ARGS(ppObjectIDs));
            CHECK_HR(hr, "Failed to QI IPortableDevicePropVariantCollection for IPortableDevicePropVariantCollection");
        }

        return hr;
    }

    HRESULT GetObjectIDFromPersistentID(
        _In_                          LPCWSTR     pszPersistentID,
        _Outptr_result_nullonfailure_ LPWSTR*     ppszObjectID)
    {
        HRESULT hr = S_OK;

        if((pszPersistentID == NULL) ||
           (ppszObjectID    == NULL))
        {
            hr = E_POINTER;
            CHECK_HR(hr, ("Cannot have NULL parameter"));
            return hr;
        }

        *ppszObjectID = NULL;

        for (size_t Index = 0; Index < m_Content.GetCount(); Index++)
        {
            if(m_Content[Index]->PersistentUniqueID.CompareNoCase(pszPersistentID) == 0)
            {
                *ppszObjectID = AtlAllocTaskWideString(m_Content[Index]->ObjectID);
                if(*ppszObjectID == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    CHECK_HR(hr, "Could not allocate memory for ObjectID");
                }
                break;
            }
        }

        if ((hr == S_OK) && (*ppszObjectID == NULL))
        {
            // We reached the end of the content but did not find the element
            hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        }

        return hr;
    }

    /**
     * Since this is a fake device with no hardware or real contents, we simulate
     * a format by deleting the contents of a storage object.
     **/
    HRESULT FormatStorage(
        _In_    LPCWSTR                 pszObjectID,
        _In_    IPortableDeviceValues*  pCommandParams)
    {
        HRESULT     hr          = S_OK;
        CAtlStringW strObjectID = pszObjectID;

        // Validate that the object specified is one of our storage objects
        if((strObjectID.CompareNoCase(STORAGE1_OBJECT_ID) == 0) ||
           (strObjectID.CompareNoCase(STORAGE2_OBJECT_ID) == 0))
        {
            CComPtr<IPortableDeviceValues> pEventParams;

            hr = GetObjectPropertiesForEvent(strObjectID.GetString(), &pEventParams);
            CHECK_HR(hr, "Failed to get storage object properties for event");

            // Indicate that format has started
            hr = PostWpdEventWithProgress(pCommandParams, pEventParams, WPD_EVENT_STORAGE_FORMAT, WPD_OPERATION_STATE_STARTED, 0);
            CHECK_HR(hr, "Failed to send format event progress: WPD_OPERATION_STATE_STARTED");

            if (hr == S_OK)
            {
                hr = MarkChildrenForDeletion(pszObjectID);
                CHECK_HR(hr, "Failed to mark children of storage for deletion");

                // Indicate that format is halfway there
                hr = PostWpdEventWithProgress(pCommandParams, pEventParams, WPD_EVENT_STORAGE_FORMAT, WPD_OPERATION_STATE_RUNNING, 50);
                CHECK_HR(hr, "Failed to send format progress notification");

                // Delete the objects
                if (hr == S_OK)
                {
                    hr = RemoveObjectsMarkedForDeletion();
                    CHECK_HR(hr, "Failed to remove objects marked for deletion");
                }

            }

            // Indicate that format is done
            hr = PostWpdEventWithProgress(pCommandParams, pEventParams, WPD_EVENT_STORAGE_FORMAT, WPD_OPERATION_STATE_FINISHED, 100);
            CHECK_HR(hr, "Failed to send format event progress: WPD_OPERATION_STATE_FINISHED");
        }
        else
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "Cannot format object [%ws] because it is not a storage", pszObjectID);
        }

        return hr;
    }

    HRESULT MoveObject(
        _In_    LPCWSTR     pszObjectID,
        _In_    LPCWSTR     pszDestinationID)
    {
        HRESULT         hr          = S_OK;
        FakeContent*    pSource     = NULL;
        FakeContent*    pDestFolder = NULL;

        if(GetContent(pszObjectID, &pSource))
        {
            // Verify that this is a content object.  Functional objects cannot be moved.
            if(pSource->ContentType != WPD_CONTENT_TYPE_FUNCTIONAL_OBJECT)
            {
                // Verify that the source is not the same as the destination
                if(pSource->ObjectID.CompareNoCase(pszDestinationID) != 0)
                {
                    // Verify that the destination is a folder
                    if(GetContent(pszDestinationID, &pDestFolder))
                    {
                        if(pDestFolder->ContentType == WPD_CONTENT_TYPE_FOLDER)
                        {
                            pSource->ParentID = pszDestinationID;
                        }
                        else
                        {
                            hr = E_INVALIDARG;
                            CHECK_HR(hr, "Destination [%ws] for move object is not a folder", pszDestinationID);
                        }
                    }
                    else
                    {
                        hr = E_INVALIDARG;
                        CHECK_HR(hr, "Invalid ObjectID [%ws]", pszDestinationID ? pszDestinationID : L"NULL");
                    }
                }
                else
                {
                    hr = E_INVALIDARG;
                    CHECK_HR(hr, "Object [%ws] cannot be moved to itself", pszObjectID);
                }
            }
            else
            {
                hr = E_INVALIDARG;
                CHECK_HR(hr, "Attempting to move functional object %ws", pszObjectID);
            }
        }
        else
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "Invalid ObjectID [%ws]", pszObjectID ? pszObjectID : L"NULL");
        }

        return hr;
    }

    HRESULT CopyObject(
        _In_                          LPCWSTR     pszObjectID,
        _In_                          LPCWSTR     pszDestinationID,
        _Outptr_result_nullonfailure_ LPWSTR*     ppNewObjectID)
    {
        HRESULT         hr          = S_OK;
        FakeContent*    pSource     = NULL;
        FakeContent*    pDestFolder = NULL;
        CComPtr<IPortableDeviceValues> pNewContentProperties;
        LPWSTR          pwszNewObjectID = NULL;

        if(ppNewObjectID == NULL)
        {
            hr = E_POINTER;
            CHECK_HR(hr, ("Cannot have NULL parameter"));
            return hr;
        }

        *ppNewObjectID = NULL;

        if(GetContent(pszObjectID, &pSource))
        {
            // Verify that this is a content object.  Functional objects cannot be copied.
            if(pSource->ContentType != WPD_CONTENT_TYPE_FUNCTIONAL_OBJECT)
            {
                // Verify that the source is not the same as the destination
                if(pSource->ObjectID.CompareNoCase(pszDestinationID) != 0)
                {
                    // Verify that the destination is a folder
                    if(GetContent(pszDestinationID, &pDestFolder))
                    {
                        if(pDestFolder->ContentType == WPD_CONTENT_TYPE_FOLDER)
                        {
                            hr = GetAllValues(pSource->ObjectID, &pNewContentProperties);
                            CHECK_HR(hr, "Failed to get ALL values for [%ws]", pSource->ObjectID);

                            if (SUCCEEDED(hr))
                            {
                                hr = pNewContentProperties->SetStringValue(WPD_OBJECT_PARENT_ID, pDestFolder->ObjectID);
                                CHECK_HR(hr, "Failed to update WPD_OBJECT_PARENT_ID value to content object[%ws] on new copied content", pDestFolder->ObjectID);
                            }

                            if (SUCCEEDED(hr))
                            {
                                hr = SaveNewObject(pNewContentProperties, &pwszNewObjectID);
                                CHECK_HR(hr, "Failed Create new content from object[%ws]", pSource->ObjectID);
                            }

                            // If the object we just copied was a folder and it contained child objects we need to copy the contents as well.
                            if (SUCCEEDED(hr) && (pSource->ContentType == WPD_CONTENT_TYPE_FOLDER) && (HasChildren(pSource->ObjectID) == TRUE))
                            {
                                hr = CopyChildrenToNewParent(pSource->ObjectID, pwszNewObjectID);
                            }
                        }
                        else
                        {
                            hr = E_INVALIDARG;
                            CHECK_HR(hr, "Destination [%ws] for move object is not a folder", pszDestinationID);
                        }
                    }
                    else
                    {
                        hr = E_INVALIDARG;
                        CHECK_HR(hr, "Invalid ObjectID [%ws]", pszDestinationID ? pszDestinationID : L"NULL");
                    }
                }
                else
                {
                    hr = E_INVALIDARG;
                    CHECK_HR(hr, "Object [%ws] cannot be copied to itself", pszObjectID);
                }
            }
            else
            {
                hr = E_INVALIDARG;
                CHECK_HR(hr, "Attempting to move functional object %ws", pszObjectID);
            }
        }
        else
        {
            hr = E_INVALIDARG;
            CHECK_HR(hr, "Invalid ObjectID [%ws]", pszObjectID ? pszObjectID : L"NULL");
        }

        // If we failed, free the allocated object ID.
        if (FAILED(hr))
        {
            if (pwszNewObjectID != NULL)
            {
                CoTaskMemFree(pwszNewObjectID);
                pwszNewObjectID = NULL;
            }
        }

        if (pwszNewObjectID != NULL)
        {
            // The newly created object ID is given to the caller to manage and free
            *ppNewObjectID = pwszNewObjectID;
        }

        return hr;
    }

    HRESULT GetObjectPropertiesForEvent(
        _In_            LPCWSTR                 pszObjectID,
        _COM_Outptr_    IPortableDeviceValues** ppValues)
    {
        HRESULT hr = S_OK;
        CComPtr<IPortableDeviceKeyCollection> pKeys;
        CComPtr<IPortableDeviceValues> pValues;

        *ppValues = NULL;

        if (SUCCEEDED(hr))
        {
            hr = CoCreateInstance(CLSID_PortableDeviceKeyCollection,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IPortableDeviceKeyCollection,
                                  (VOID**) &pKeys);
            CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceKeyCollection");
        }

        // Add the keys we need in order to fill out event parameters
        if (SUCCEEDED(hr))
        {
            hr = pKeys->Add(WPD_OBJECT_PERSISTENT_UNIQUE_ID);
            CHECK_HR(hr, "Failed to add WPD_OBJECT_PERSISTENT_UNIQUE_ID");

            if (SUCCEEDED(hr))
            {
                hr = pKeys->Add(WPD_OBJECT_NAME);
                CHECK_HR(hr, "Failed to add WPD_OBJECT_NAME");
            }
            if (SUCCEEDED(hr))
            {
                hr = pKeys->Add(WPD_OBJECT_CONTENT_TYPE);
                CHECK_HR(hr, "Failed to add WPD_OBJECT_CONTENT_TYPE");
            }

            //These properties may or may not exist on the object
            if (SUCCEEDED(hr))
            {
                hr = pKeys->Add(WPD_FUNCTIONAL_OBJECT_CATEGORY);
                CHECK_HR(hr, "Failed to add WPD_FUNCTIONAL_OBJECT_CATEGORY");
            }
            if (SUCCEEDED(hr))
            {
                hr = pKeys->Add(WPD_OBJECT_ORIGINAL_FILE_NAME);
                CHECK_HR(hr, "Failed to add WPD_OBJECT_ORIGINAL_FILE_NAME");
            }
            if (SUCCEEDED(hr))
            {
                hr = pKeys->Add(WPD_OBJECT_PARENT_ID);
                CHECK_HR(hr, "Failed to add WPD_OBJECT_PARENT_ID");
            }
        }

        // Get the values
        if (SUCCEEDED(hr))
        {
            hr = GetValues(pszObjectID, pKeys, &pValues);
            if (hr == S_FALSE)
            {
                hr = S_OK;
            }

            // Some objects supported by this driver don't support WPD_OBJECT_ORIGINAL_FILE_NAME
            // (e.g. functional objects) so remove it if there was an error reading it.
            if (SUCCEEDED(hr))
            {
                PROPVARIANT pv = {0};
                PropVariantInit(&pv);

                hr = pValues->GetValue(WPD_OBJECT_ORIGINAL_FILE_NAME, &pv);
                CHECK_HR(hr, "Failed to get WPD_OBJECT_ORIGINAL_FILE_NAME");

                if(SUCCEEDED(hr))
                {
                    if(pv.vt == VT_ERROR)
                    {
                        hr = pValues->RemoveValue(WPD_OBJECT_ORIGINAL_FILE_NAME);
                        CHECK_HR(hr, "Failed to remove WPD_OBJECT_ORIGINAL_FILE_NAME");
                    }
                }

                PropVariantClear(&pv);

                // None of the above is fatal
                hr = S_OK;
            }

            // Add the WPD_EVENT_PARAMETER_OBJECT_PARENT_PERSISTENT_UNIQUE_ID
            if (SUCCEEDED(hr))
            {
                CAtlStringW strDevice = WPD_DEVICE_OBJECT_ID;
                // If this is the device object, set the parent's persistent unique ID to to the empty string since it has no parent.
                // Esle, get the persistent unique ID from the parent.
                if(strDevice.CompareNoCase(pszObjectID) == 0)
                {
                    pValues->SetStringValue(WPD_EVENT_PARAMETER_OBJECT_PARENT_PERSISTENT_UNIQUE_ID, L"");
                }
                else
                {
                    CComPtr<IPortableDeviceValues> pParentValues;
                    HRESULT hrTemp = S_OK;
                    LPWSTR  pszParentID = NULL;
                    LPWSTR  pszParentUID = NULL;

                    hrTemp = pValues->GetStringValue(WPD_OBJECT_PARENT_ID, &pszParentID);
                    if (SUCCEEDED(hrTemp))
                    {
                        hrTemp = GetValues(pszParentID, pKeys, &pParentValues);
                        if (SUCCEEDED(hrTemp))
                        {
                            hrTemp = pParentValues->GetStringValue(WPD_OBJECT_PERSISTENT_UNIQUE_ID, &pszParentUID);
                            if (SUCCEEDED(hrTemp))
                            {
                                pValues->SetStringValue(WPD_EVENT_PARAMETER_OBJECT_PARENT_PERSISTENT_UNIQUE_ID, pszParentUID);
                                CoTaskMemFree(pszParentUID);
                            }
                            CHECK_HR(hrTemp, "Failed to get parent unique id");
                        }
                        CoTaskMemFree(pszParentID);
                        CHECK_HR(hrTemp, "Failed to get values for parent id");
                    }
                    CHECK_HR(hrTemp, "Failed to get parent id");
                }
            }
            CHECK_HR(hr, "Failed to get properties to use in event information");
        }

        if (SUCCEEDED(hr))
        {
            *ppValues = pValues.Detach();
        }

        return hr;
    }


    HRESULT CreateContentObject(
        _In_    LPCWSTR                 pszObjectName,
        _In_    LPCWSTR                 pszParentID,
        _In_    REFGUID                 guidContentType,
        _In_    IPortableDeviceValues*  pObjectProperties,
        _Outptr_result_nullonfailure_   FakeContent**   ppContent)
    {
        HRESULT hr      = S_OK;
        HRESULT hrTemp  = S_OK;
        LPWSTR  wszOriginalFileName = NULL;

        *ppContent = NULL;

        if(guidContentType == WPD_CONTENT_TYPE_FOLDER)
        {
            FakeContent* pContent = NULL;

            pContent = new FakeContent();
            if (pContent)
            {
                pContent->Name          = pszObjectName;
                pContent->ParentID      = pszParentID;
                m_dwLastObjectID++;
                pContent->ObjectID.Format(L"%d", m_dwLastObjectID);
                pContent->ContentType   = guidContentType;
                pContent->FileName      = pszObjectName;
                pContent->PersistentUniqueID.Format(L"PersistentUniqueID_%ws", pContent->ObjectID.GetString());

                BOOL bTemp = FALSE;
                // Get next optional property WPD_OBJECT_CAN_DELETE
                hrTemp = pObjectProperties->GetBoolValue(WPD_OBJECT_CAN_DELETE, &bTemp);
                if(hrTemp == S_OK)
                {
                    pContent->CanDelete = bTemp;
                }
                // Get next optional property WPD_OBJECT_ISHIDDEN
                hrTemp = pObjectProperties->GetBoolValue(WPD_OBJECT_ISHIDDEN, &bTemp);
                if(hrTemp == S_OK)
                {
                    pContent->IsHidden = bTemp;
                }
                // Get next optional property WPD_OBJECT_NON_CONSUMABLE
                hrTemp = pObjectProperties->GetBoolValue(WPD_OBJECT_NON_CONSUMABLE, &bTemp);
                if(hrTemp == S_OK)
                {
                    pContent->NonConsumable = bTemp;
                }

                *ppContent = pContent;
            }
            else
            {
                hr = E_OUTOFMEMORY;
                CHECK_HR(hr, "Failed to allocate new FakeContent object");
            }
        }
        else if(guidContentType == WPD_CONTENT_TYPE_CONTACT)
        {
            FakeContactContent* pContent = NULL;

            pContent = new FakeContactContent();
            if (pContent)
            {
                pContent->Name          = pszObjectName;
                pContent->ParentID      = pszParentID;
                m_dwLastObjectID++;
                pContent->ObjectID.Format(L"%d", m_dwLastObjectID);
                pContent->ContentType   = guidContentType;
                pContent->PersistentUniqueID.Format(L"PersistentUniqueID_%ws", pContent->ObjectID.GetString());

                // Get the other contact properties.
                LPWSTR  pszTempString   = NULL;

                hr = pObjectProperties->GetStringValue(WPD_CONTACT_DISPLAY_NAME, &pszTempString);
                CHECK_HR(hr, "Failed to get required contact property: WPD_CONTACT_DISPLAY_NAME");
                if (SUCCEEDED(hr))
                {
                    pContent->DisplayName = pszTempString;
                    // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
                    CoTaskMemFree(pszTempString);

                    // Get next optional property WPD_CONTACT_PRIMARY_PHONE
                    hrTemp = pObjectProperties->GetStringValue(WPD_CONTACT_PRIMARY_PHONE, &pszTempString);
                    if(hrTemp == S_OK)
                    {
                        pContent->PrimaryPhone = pszTempString;
                        // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
                        CoTaskMemFree(pszTempString);
                    }

                    // Get next optional property WPD_CONTACT_MOBILE_PHONE
                    hrTemp = pObjectProperties->GetStringValue(WPD_CONTACT_MOBILE_PHONE, &pszTempString);
                    if(hrTemp == S_OK)
                    {
                        pContent->CellPhone = pszTempString;
                        // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
                        CoTaskMemFree(pszTempString);
                    }

                    // Get next optional property WPD_CONTACT_BUSINESS_PHONE
                    hrTemp = pObjectProperties->GetStringValue(WPD_CONTACT_BUSINESS_PHONE, &pszTempString);
                    if(hrTemp == S_OK)
                    {
                        pContent->WorkPhone = pszTempString;
                        // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
                        CoTaskMemFree(pszTempString);
                    }

                    // Get next optional property WPD_OBJECT_ORIGINAL_FILE_NAME
                    hrTemp = pObjectProperties->GetStringValue(WPD_OBJECT_ORIGINAL_FILE_NAME, &pszTempString);
                    if(hrTemp == S_OK)
                    {
                        pContent->FileName = pszTempString;
                        // Free the memory.  CoTaskMemFree ignores NULLs so no need to check.
                        CoTaskMemFree(pszTempString);
                    }

                    *ppContent = pContent;
                }
                else
                {
                    delete pContent;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
                CHECK_HR(hr, "Failed to allocate new FakeContactContent object");
            }
        }
        else if(guidContentType == WPD_CONTENT_TYPE_AUDIO)
        {
            FakeMusicContent* pContent = NULL;

            pContent = new FakeMusicContent();
            if (pContent)
            {
                pContent->Name          = pszObjectName;
                pContent->ParentID      = pszParentID;
                m_dwLastObjectID++;
                pContent->ObjectID.Format(L"%d", m_dwLastObjectID);
                pContent->ContentType   = guidContentType;
                pContent->PersistentUniqueID.Format(L"PersistentUniqueID_%ws", pContent->ObjectID.GetString());

                hrTemp = pObjectProperties->GetStringValue(WPD_OBJECT_ORIGINAL_FILE_NAME, &wszOriginalFileName);
                if (hrTemp == S_OK)
                {
                    pContent->FileName = wszOriginalFileName;
                }

                *ppContent = pContent;
            }
            else
            {
                hr = E_OUTOFMEMORY;
                CHECK_HR(hr, "Failed to allocate new FakeMusicContent object");
            }
        }
        else if(guidContentType == WPD_CONTENT_TYPE_VIDEO)
        {
            FakeVideoContent* pContent = NULL;

            pContent = new FakeVideoContent();
            if (pContent)
            {
                pContent->Name          = pszObjectName;
                pContent->ParentID      = pszParentID;
                m_dwLastObjectID++;
                pContent->ObjectID.Format(L"%d", m_dwLastObjectID);
                pContent->ContentType   = guidContentType;
                pContent->PersistentUniqueID.Format(L"PersistentUniqueID_%ws", pContent->ObjectID.GetString());

                hrTemp = pObjectProperties->GetStringValue(WPD_OBJECT_ORIGINAL_FILE_NAME, &wszOriginalFileName);
                if (hrTemp == S_OK)
                {
                    pContent->FileName = wszOriginalFileName;
                }

                *ppContent = pContent;
            }
            else
            {
                hr = E_OUTOFMEMORY;
                CHECK_HR(hr, "Failed to allocate new FakeVideoContent object");
            }
        }
        else if(guidContentType == WPD_CONTENT_TYPE_IMAGE)
        {
            FakeImageContent* pContent = NULL;

            pContent = new FakeImageContent();
            if (pContent)
            {
                pContent->Name          = pszObjectName;
                pContent->ParentID      = pszParentID;
                m_dwLastObjectID++;
                pContent->ObjectID.Format(L"%d", m_dwLastObjectID);
                pContent->ContentType   = guidContentType;
                pContent->PersistentUniqueID.Format(L"PersistentUniqueID_%ws", pContent->ObjectID.GetString());

                hrTemp = pObjectProperties->GetStringValue(WPD_OBJECT_ORIGINAL_FILE_NAME, &wszOriginalFileName);
                if (hrTemp == S_OK)
                {
                    pContent->FileName = wszOriginalFileName;
                }

                *ppContent = pContent;
            }
            else
            {
                hr = E_OUTOFMEMORY;
                CHECK_HR(hr, "Failed to allocate new FakeImageContent object");
            }
        }
        else if(guidContentType == WPD_CONTENT_TYPE_NETWORK_ASSOCIATION)
        {
            FakeNetworkAssociationContent* pContent = NULL;

            pContent = new FakeNetworkAssociationContent();
            if (pContent)
            {
                BYTE *pValue = NULL;
                DWORD cbValue = 0;

                pContent->Name          = pszObjectName;
                pContent->ParentID      = pszParentID;
                m_dwLastObjectID++;
                pContent->ObjectID.Format(L"%d", m_dwLastObjectID);
                pContent->ContentType   = guidContentType;
                pContent->PersistentUniqueID.Format(L"PersistentUniqueID_%ws", pContent->ObjectID.GetString());

                // Get required property WPD_NETWORK_ASSOCIATION_HOST_NETWORK_IDENTIFIERS
                hr = pObjectProperties->GetBufferValue(WPD_NETWORK_ASSOCIATION_HOST_NETWORK_IDENTIFIERS, &pValue, &cbValue);
                if (SUCCEEDED(hr))
                {
                    // Make sure the HostEUI64Array is the correct length
                    if ((cbValue % 8) == 0)
                    {
                        // Avoid copy by manually updating the PROPVARIANT
                        pContent->HostEUI64Array.vt = VT_VECTOR | VT_UI1;
                        pContent->HostEUI64Array.caub.cElems  = cbValue;
                        pContent->HostEUI64Array.caub.pElems  = pValue;
                    }
                    else
                    {
                        CoTaskMemFree(pValue);
                        pValue = NULL;

                        hr = E_INVALIDARG;
                        CHECK_HR(hr, "Invalid value for HostEUI64Array");
                    }
                }

                if (SUCCEEDED(hr))
                {
                    *ppContent = pContent;
                }
                else
                {
                    delete pContent;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
                CHECK_HR(hr, "Failed to allocate new FakeNetworkAssociationContent object");
            }
        }
        else if(guidContentType == WPD_CONTENT_TYPE_WIRELESS_PROFILE)
        {
            FakeWirelessProfileContent* pContent = NULL;

            pContent = new FakeWirelessProfileContent();
            if (pContent)
            {
                pContent->Name          = pszObjectName;
                pContent->ParentID      = pszParentID;
                m_dwLastObjectID++;
                pContent->ObjectID.Format(L"%d", m_dwLastObjectID);
                pContent->ContentType   = guidContentType;
                pContent->PersistentUniqueID.Format(L"PersistentUniqueID_%ws", pContent->ObjectID.GetString());

                hrTemp = pObjectProperties->GetStringValue(WPD_OBJECT_ORIGINAL_FILE_NAME, &wszOriginalFileName);
                if (hrTemp == S_OK)
                {
                    pContent->FileName = wszOriginalFileName;
                }

                *ppContent = pContent;
            }
            else
            {
                hr = E_OUTOFMEMORY;
                CHECK_HR(hr, "Failed to allocate new FakeWirelessProfileContent object");
            }
        }
        else
        {
            FakeContent* pContent;

            // Add generic file object
            pContent = new FakeGenericFileContent();
            if (pContent)
            {
                pContent->Name          = pszObjectName;
                pContent->ParentID      = pszParentID;
                m_dwLastObjectID++;
                pContent->ObjectID.Format(L"%d", m_dwLastObjectID);
                pContent->ContentType   = guidContentType;
                pContent->PersistentUniqueID.Format(L"PersistentUniqueID_%ws", pContent->ObjectID.GetString());

                BOOL bTemp = FALSE;
                // Get next optional property WPD_OBJECT_CAN_DELETE
                hrTemp = pObjectProperties->GetBoolValue(WPD_OBJECT_CAN_DELETE, &bTemp);
                if(hrTemp == S_OK)
                {
                    pContent->CanDelete = bTemp;
                }
                // Get next optional property WPD_OBJECT_ISHIDDEN
                hrTemp = pObjectProperties->GetBoolValue(WPD_OBJECT_ISHIDDEN, &bTemp);
                if(hrTemp == S_OK)
                {
                    pContent->IsHidden = bTemp;
                }
                // Get next optional property WPD_OBJECT_NON_CONSUMABLE
                hrTemp = pObjectProperties->GetBoolValue(WPD_OBJECT_NON_CONSUMABLE, &bTemp);
                if(hrTemp == S_OK)
                {
                    pContent->NonConsumable = bTemp;
                }

                hrTemp = pObjectProperties->GetStringValue(WPD_OBJECT_ORIGINAL_FILE_NAME, &wszOriginalFileName);
                if (hrTemp == S_OK)
                {
                    pContent->FileName = wszOriginalFileName;
                }

                *ppContent = pContent;
            }
            else
            {
                hr = E_OUTOFMEMORY;
                CHECK_HR(hr, "Failed to allocate new FakeContent object");
            }
        }

        if (wszOriginalFileName != NULL)
        {
            CoTaskMemFree(wszOriginalFileName);
            wszOriginalFileName = NULL;
        }

        return hr;
    }

    HRESULT GetContentFormat(
        _In_  LPCWSTR pszObjectID,
        _Out_ GUID&   guidObjectFormat)
    {
        HRESULT hr = S_OK;
        FakeContent* pContent = NULL;
        guidObjectFormat = GUID_NULL;

        if (GetContent(pszObjectID, &pContent) == true)
        {
            guidObjectFormat = pContent->GetObjectFormat();
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        }

        return hr;
    }

    HRESULT EnableResource(
        _In_    LPCWSTR pszObjectID,
        _In_    REFPROPERTYKEY ResourceKey)
    {
        HRESULT hr = S_OK;
        FakeContent* pContent = NULL;

        if (GetContent(pszObjectID, &pContent) == true)
        {
            hr = pContent->EnableResource(ResourceKey);
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        }

        return hr;
    }

    HRESULT CopyChildrenToNewParent(
        _In_    LPCWSTR pszParentObjectID,
        _In_    LPCWSTR pszNewParentObjectID)
    {
        HRESULT hr = S_OK;

        CAtlStringW strParentObjectID = pszParentObjectID;
        for (size_t Index = 0; Index < m_Content.GetCount(); Index++)
        {
            if (m_Content[Index]->ParentID == strParentObjectID)
            {
                LPWSTR pwszNewObjectID = NULL;
                // Copy this object
                hr = CopyObject(m_Content[Index]->ObjectID, pszNewParentObjectID, &pwszNewObjectID);

                // Free allocate new object ID
                if (pwszNewObjectID != NULL)
                {
                    CoTaskMemFree(pwszNewObjectID);
                    pwszNewObjectID = NULL;
                }

                if (FAILED(hr))
                {
                    break;
                }
            }
        }
        return hr;
    }

    HRESULT SupportsResource(
        _In_    LPCWSTR         pszObjectID,
        _In_    REFPROPERTYKEY  ResourceKey,
        _Out_   BOOL*           pbSupportsResource)
    {
        HRESULT hr = S_OK;
        CComPtr<IPortableDeviceKeyCollection> pKeys;

        if((pszObjectID == NULL) || (pbSupportsResource == NULL))
        {
            hr = E_POINTER;
            CHECK_HR(hr, "Cannot have NULL parameter");
            return hr;
        }

        *pbSupportsResource = FALSE;
        hr = GetSupportedResources(pszObjectID, &pKeys);
        CHECK_HR(hr, "Failed to get supported resources for validation");

        if (hr == S_OK)
        {
            DWORD        dwIndex = 0;
            PROPERTYKEY  TempKey = WPD_PROPERTY_NULL;
            while(pKeys->GetAt(dwIndex++, &TempKey) == S_OK)
            {
                if(IsEqualPropertyKey(ResourceKey, TempKey))
                {
                    *pbSupportsResource = TRUE;
                }
            }
        }

        return hr;
    }

    HRESULT UpdateContentObject(
        _In_    LPCWSTR                 pszObjectID,
        _In_    IPortableDeviceValues*  pObjectProperties)
    {
        HRESULT         hr       = S_OK;
        FakeContent*    pElement = NULL;

        if (GetContent(pszObjectID, &pElement))
        {
            HRESULT hrTemp              = S_OK;
            LPWSTR  wszOriginalFileName = NULL;
            LPWSTR  wszObjectName       = NULL;

            // Update selected properties; other properties will be discarded.
            hrTemp = pObjectProperties->GetStringValue(WPD_OBJECT_ORIGINAL_FILE_NAME, &wszOriginalFileName);
            if(hrTemp == S_OK)
            {
                pElement->FileName = wszOriginalFileName;
                CoTaskMemFree(wszOriginalFileName);
            }

            hrTemp = pObjectProperties->GetStringValue(WPD_OBJECT_NAME, &wszObjectName);
            if(hrTemp == S_OK)
            {
                pElement->Name = wszObjectName;
                CoTaskMemFree(wszObjectName);
            }

            // Note: This fake driver does nothing with the data.  The WriteData method is simply
            // a dummy one, and the data size is a constant.
            // Normally, a driver would also update the WPD_OBJECT_SIZE to the new data size.
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
            CHECK_HR(hr, "Failed to find the object '%ws'", pszObjectID);
        }

        return hr;
    }

private:
    CAtlArray<FakeContent*> m_Content;
    DWORD                   m_dwLastObjectID;
};


