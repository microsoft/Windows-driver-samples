//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992-2001.
//
//  File:       LIST . H
//
//  Contents:   
//
//  Notes: List manipulation functions.
//
//----------------------------------------------------------------------------

#ifndef LIST_H_INCLUDED

#define LIST_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>


template<class X, class Y> class List { 
     struct Node {
        X    item;
        Y    key;
        Node *next;
     };

     Node  *m_head;
     DWORD m_dwNodeCount;

  public:

              List ();

      virtual ~List();

     HRESULT  Insert (X item,
                      Y key);

     HRESULT  Remove (X *item);

     HRESULT  RemoveThis (X item);

     HRESULT  RemoveByKey (Y key,
                           X *item);

     VOID     RemoveAll(VOID);

     HRESULT  Find (DWORD dwIndex,
                    X *item);

     HRESULT  FindByKey (Y key,
                         X *item);

     DWORD    ListCount (VOID);
};

template<class X, class Y> List<X, Y>::List ()
{
  m_head = NULL;
  m_dwNodeCount = 0;
}

template<class X, class Y> List<X, Y>::~List ()
{
  RemoveAll();
}

template<class X, class Y> HRESULT List<X, Y>::Insert (X item,
                                                       Y key)
{
  Node *pNewNode;

  pNewNode = new Node;

  if ( pNewNode ) {
     pNewNode->item = item;
     pNewNode->key = key;
     pNewNode->next = NULL;

     if ( m_dwNodeCount ) {
        pNewNode->next = m_head;
     }

     m_head = pNewNode;
     m_dwNodeCount++;
  }

  return ( pNewNode ) ? S_OK : HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
}

template<class X, class Y> HRESULT List<X, Y>::Remove (X *item)
{
  Node *temp;

  if ( m_dwNodeCount == 0 ) {
     return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
  }

  *item = m_head->item;
  temp = m_head;
  m_head = m_head->next;
  delete temp;
  m_dwNodeCount--;

  return S_OK;
}
  
template<class X, class Y> HRESULT List<X, Y>::RemoveThis (X item)
{
  Node *temp;
  Node *nodeToFind;

  if ( m_dwNodeCount == 0 ) {
     return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
  }

  if ( m_head->item == item ) {

     nodeToFind = m_head;
     m_head = m_head->next;
  }
  else {
     for (temp = m_head; temp->next && (temp->next->item != item); ) {
        temp = temp->next;
     }

     if ( temp->next ) {
        nodeToFind = temp->next;
        temp->next = temp->next->next;
     }
     else
        nodeToFind = NULL;
  }

  if ( nodeToFind ) {
     delete nodeToFind;
     m_dwNodeCount--;
     return S_OK;
  }
  else
     return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
}

template<class X, class Y> HRESULT List<X, Y>::RemoveByKey (Y key,
                                                            X *item)
{
  Node *temp;
  Node *nodeToFind;

  if ( m_dwNodeCount == 0 ) {
     return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
  }

  if ( m_head->key == key ) {

     nodeToFind = m_head;
     m_head = m_head->next;
  }
  else {
     for (temp = m_head; temp->next && (temp->next->key != key); ) {
        temp = temp->next;
     }

     if ( temp->next ) {
        nodeToFind = temp->next;
        temp->next = temp->next->next;
     }
     else
        nodeToFind = NULL;
  }

  if ( nodeToFind ) {
     *item = nodeToFind->item;

     delete nodeToFind;
     m_dwNodeCount--;

     return S_OK;
  }
  else
     return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
}

template<class X, class Y> VOID List<X, Y>::RemoveAll (VOID)
{
  Node *temp;

  for (; m_dwNodeCount; --m_dwNodeCount) {
     temp = m_head;
     m_head = m_head->next;
     delete temp;
  }

  return;
}

template<class X, class Y> HRESULT List<X, Y>::Find (DWORD dwIndex,
                                                     X *item)
{
  Node  *temp;
  DWORD i;

  if ( (m_dwNodeCount == 0) || (dwIndex > m_dwNodeCount) ) {

     return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
  }

  for (i=0, temp = m_head; i < dwIndex; ++i) {
     temp = temp->next;
  }

  *item = temp->item;
  return S_OK;
}

template<class X, class Y> HRESULT List<X, Y>::FindByKey (Y key,
                                                          X *item)
{
  Node *temp;

  for (temp = m_head; temp && (temp->key != key); )
     temp = temp->next;

  if ( temp ) {
     *item = temp->item;
     return S_OK;
  }
  else {
     return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
  }
}

template<class X, class Y> DWORD List<X, Y>::ListCount (VOID)
{
  return m_dwNodeCount;
}

#endif
