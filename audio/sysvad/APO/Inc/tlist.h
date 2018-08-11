//**@@@*@@@****************************************************
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//**@@@*@@@****************************************************

//
// FileName:    tlist.h
//
// Abstract:    This is the header and implementation file for a template 
//              list class
//
// ----------------------------------------------------------------------------

#pragma once


typedef void* LISTPOS;

template <class T>
class TItem
{
public:
    TItem() throw();
    ~TItem() throw();
    TItem<T> * pNext;
    TItem<T> * pPrev;
    T* pData;
};

template <class T>
TItem<T>::TItem() :
   pNext(NULL),
   pPrev(NULL),
   pData(NULL)
{
    return;
}

template <class T>
TItem<T>::~TItem()
{
    return;
}


template <class T>
class TList
{
 public:
    TList() throw();
    ~TList() throw();

    BOOL    GetHead( T** ppHead )  const throw();
    LISTPOS GetHeadPosition()   const throw();
    LISTPOS GetTailPosition() const throw();
    LISTPOS AddHead(__drv_aliasesMem T* pNewHead) throw();
    LISTPOS AddTail(__drv_aliasesMem T* pNewTail) throw();
    BOOL    RemoveTail(T** ppOldTail) throw();
    BOOL    RemoveHead(T** ppOldHead) throw();
    void    RemoveAll() throw();
    LISTPOS Find( T* pSearchValue ) const throw();
    void    RemoveAt(LISTPOS pos) throw();
    BOOL    GetNext(LISTPOS& rPos, T**ppThisData) const throw();
    BOOL    IsEmpty() const throw();
    DWORD   GetCount() const throw();
    HRESULT Initialize(DWORD dwValue) throw();
    BOOL    GetAt( LONG index, T** ppElement) const throw();
    BOOL    GetAt( LISTPOS pos, T** ppElement) const throw();
    LISTPOS InsertBefore(LISTPOS pos, T* pNewElement) throw();
    LISTPOS InsertAfter(LISTPOS pos, T* pNewElement) throw();
    void    MoveHeadList(TList<T> *pFromList) throw();
private:
    TItem<T>*   m_pHead;
    TItem<T>*   m_pTail;
    LONG        m_lCount;
};

template <class T>
TList<T>::TList() :
    m_pHead(NULL),
    m_pTail(NULL),
    m_lCount(0)
{
    return;
}

template <class T>
TList<T>::~TList()
{
    ATLASSERT(NULL == m_pHead);
    ATLASSERT(NULL == m_pTail);
    ATLASSERT(0 == m_lCount);
    return;
}

template <class T>
BOOL TList<T>::GetHead(T** ppHead) const
{
    if (!m_pHead)
    {
        *ppHead = NULL;
        return FALSE;
    }

    *ppHead = m_pHead->pData;
    return TRUE;
}

template <class T>
LISTPOS TList<T>::GetHeadPosition() const
{
    return reinterpret_cast<LISTPOS>(m_pHead);
}

template <class T>
LISTPOS TList<T>::GetTailPosition() const
{
    return reinterpret_cast<LISTPOS>(m_pTail);
}

template <class T>
LISTPOS TList<T>::AddTail(__drv_aliasesMem T* pNewTail)
{
    TItem<T>* pNewItem;

    pNewItem = new TItem<T>;
    if (NULL == pNewItem)
    {
        return( reinterpret_cast<LISTPOS>( NULL ));
    }

    pNewItem->pData = pNewTail;
    pNewItem->pPrev = m_pTail;
    pNewItem->pNext = NULL;

    if (NULL == m_pTail)
    {
        m_pHead = pNewItem;
    }
    else
    {
        m_pTail->pNext = pNewItem;
    }

    m_pTail = pNewItem;
    m_lCount++;

    return( reinterpret_cast<LISTPOS>( pNewItem ) );
}

template <class T>
LISTPOS TList<T>::AddHead(__drv_aliasesMem T* pNewHead)
{
    TItem<T> *pNewItem;
    pNewItem = new TItem<T>;
    if ( !pNewItem )
    {
        return( reinterpret_cast<LISTPOS>( NULL ) );
    }

    pNewItem->pData = pNewHead;
    pNewItem->pNext = m_pHead;
    pNewItem->pPrev = NULL;

    if ( NULL != m_pHead )
    {
        m_pHead->pPrev = pNewItem;
    }
    else
    {
        m_pTail = pNewItem;
    }

    m_pHead = pNewItem;
    m_lCount++;

    return( reinterpret_cast<LISTPOS>( pNewItem ) );
}

template <class T>
void TList<T>::MoveHeadList(TList<T> *pFromList)
{
    // Link head item of this list to tail item of other list
    if (NULL != m_pHead)
        m_pHead->pPrev = pFromList->m_pTail;
    if (NULL != pFromList->m_pTail)
        pFromList->m_pTail->pNext = m_pHead;

    // Adjust this list's head and tail pointers
    if (pFromList->m_pHead)
        m_pHead = pFromList->m_pHead;
    if (NULL == m_pTail)
        m_pTail = pFromList->m_pTail;

    // Adjust this list's count
    m_lCount += pFromList->m_lCount;

    // Reset other list's head and tail pointers and count
    pFromList->m_pHead = NULL;
    pFromList->m_pTail = NULL;
    pFromList->m_lCount = 0;
}

template <class T>
BOOL TList<T>::RemoveTail( T** ppOldTail)
{

    if (!ppOldTail || !m_pTail)
    {
        return FALSE;
    }

    TItem<T> *pOldItem = m_pTail;
    *ppOldTail = pOldItem->pData;

    m_pTail = pOldItem->pPrev;

    if ( NULL != m_pTail )
    {
        m_pTail->pNext = NULL;
    }
    else
    {
        m_pHead = NULL;
    }

    delete pOldItem;
    m_lCount--;
    ATLASSERT(m_lCount >= 0);

    return( TRUE );
}

template <class T>
BOOL TList<T>::RemoveHead(T** ppOldHead)
{
    if ( !ppOldHead || !m_pHead )
    {
        return( FALSE );
    }

    TItem<T> *pOldItem = m_pHead;
    *ppOldHead = pOldItem->pData;

    m_pHead = pOldItem->pNext;

    if ( NULL != m_pHead )
    {
        m_pHead->pPrev = NULL;
    }
    else
    {
        m_pTail = NULL;
    }

    delete pOldItem;
    m_lCount--;
    ATLASSERT(m_lCount >= 0);

    return( TRUE );
}

template <class T>
void TList<T>::RemoveAll()
{
    TItem<T> *pNext;

    while( NULL != m_pHead )
    {
        pNext = m_pHead->pNext;
        delete m_pHead;
        m_pHead = pNext;
    }

    m_lCount = 0;
    m_pTail = NULL;

    return;
}

template <class T>
LISTPOS TList<T>::Find( T* pSearchValue ) const
{
    TItem<T> *pItem = m_pHead;

    for ( ; NULL != pItem; pItem = pItem->pNext )
    {
        if ( pItem->pData == pSearchValue )
        {
            return( reinterpret_cast<LISTPOS>(pItem) );
        }
    }

    return( reinterpret_cast<LISTPOS>(NULL) );
}

template <class T>
void TList<T>::RemoveAt(LISTPOS pos)
{
    // ATLASSERT( NULL != pos )
    TItem<T> *pOldItem = reinterpret_cast< TItem<T>* >(pos);

    // remove pOldItem from list
    if ( pOldItem == m_pHead )
    {
        m_pHead = pOldItem->pNext;
    }
    else
    {
        ATLASSERT( pOldItem->pPrev );
        pOldItem->pPrev->pNext = pOldItem->pNext;
    }

    if (pOldItem == m_pTail)
    {
        m_pTail = pOldItem->pPrev;
    }
    else
    {
        ATLASSERT( pOldItem->pNext );
        pOldItem->pNext->pPrev = pOldItem->pPrev;
    }

    delete pOldItem;
    m_lCount--;
    ATLASSERT(m_lCount >= 0);
    return;
}

template <class T>
BOOL TList<T>::GetNext(LISTPOS& rPos, T**ppThisData) const
{
    TItem<T> *pItem = reinterpret_cast< TItem<T>* >(rPos);

    if ( !pItem )
    {
        return( FALSE );
    }

    rPos = reinterpret_cast<LISTPOS>(pItem->pNext);
    if (ppThisData)
    {
        *ppThisData = pItem->pData;
    }

    return( TRUE );
}

template <class T>
BOOL TList<T>::IsEmpty() const
{
    return (0 == m_lCount);
}

template <class T>
HRESULT TList<T>::Initialize(DWORD dwValue)
{
    HRESULT hr = S_OK;
    return hr;
}

template <class T>
DWORD TList<T>::GetCount() const
{
    return static_cast<DWORD>(m_lCount);
}


template <class T>
LISTPOS TList<T>::InsertBefore(LISTPOS pos, T* pNewElement)
{
    TItem<T>* pOldItem = reinterpret_cast<TItem<T>*>(pos);
    TItem<T>* pNewItem;

    if ((pOldItem == NULL) || (pOldItem->pPrev == NULL))
    {
        return (AddHead(pNewElement));  // insert before nothing -> head of the list
    }

    // Insert it before pos
    pNewItem = new TItem<T>;
    if (!pNewItem)
    {
        return (reinterpret_cast<LISTPOS>(NULL));
    }

    pNewItem->pData = pNewElement;
    pNewItem->pPrev = pOldItem->pPrev;
    pNewItem->pNext = pOldItem;

    pOldItem->pPrev->pNext = pNewItem;
    pOldItem->pPrev = pNewItem;

    m_lCount++;

    return (reinterpret_cast<LISTPOS>(pNewItem));
}

template <class T>
LISTPOS TList<T>::InsertAfter(LISTPOS pos, T* pNewElement)
{
    TItem<T>* pOldItem = reinterpret_cast<TItem<T>*>(pos);
    TItem<T>* pNewItem;

    if ((pOldItem == NULL) || (pOldItem->pNext == NULL))
    {
        return (AddTail(pNewElement));  // insert after nothing -> tail of the list
    }

    // Insert it after pos
    pNewItem = new TItem<T>;
    if (!pNewItem)
    {
        return (reinterpret_cast<LISTPOS>(NULL));
    }

    pNewItem->pData = pNewElement;
    pNewItem->pPrev = pOldItem;
    pNewItem->pNext = pOldItem->pNext;

    pOldItem->pNext->pPrev = pNewItem;
    pOldItem->pNext = pNewItem;

    m_lCount++;

    return (reinterpret_cast<LISTPOS>(pNewItem));
}

template <class T>
BOOL TList<T>::GetAt( LONG index, T** ppElement) const
{
    if (ppElement == NULL)
    {
        return false;
    }

    *ppElement = NULL;

    if ((index < 0) || (index > (m_lCount - 1)))
    {
        return false;
    }

    T*      pElement = NULL;
    BOOL    bRes = TRUE;
    LISTPOS pos = GetHeadPosition();

    for (int i = 0; bRes && (i <= index); i++)
    {
        bRes = GetNext(pos, &pElement);
    }

    if (bRes)
    {
        *ppElement = pElement;
    }

    return bRes;
}

template <class T>
BOOL TList<T>::GetAt(LISTPOS pos, T**ppThisData) const
{
    TItem<T> *pItem = reinterpret_cast< TItem<T>* >(pos);

    if ( !pItem )
    {
        return( FALSE );
    }

    *ppThisData = pItem->pData;

    return( TRUE );
}
