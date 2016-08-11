/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  Copyright (c) 2003 Microsoft Corporation.  All Rights Reserved.
 *
 *  DESCRIPTION: Dynamic array template class
 *
 *******************************************************************************/
#ifndef __SIMARRAY_H_INCLUDED
#define __SIMARRAY_H_INCLUDED

template<class T>
class CBasicDynamicArray
{
private:
    int m_nSize;
    int m_nMaxSize;
    int m_nGrowSize;
    T *m_pArray;
    enum
    {
        eGrowSize = 10  // The number of items to add each time the array grows.
    };
public:
    CBasicDynamicArray(void)
      : m_nSize(0),
        m_nMaxSize(0),
        m_nGrowSize(eGrowSize),
        m_pArray(NULL)
    {
    }
    CBasicDynamicArray( int nInitialSize, int nGrowSize=0 )
      : m_nSize(0),
        m_nMaxSize(0),
        m_nGrowSize(nGrowSize ? nGrowSize : eGrowSize),
        m_pArray(NULL)
    {
        GrowTo(nInitialSize);
    }
    CBasicDynamicArray( const CBasicDynamicArray<T> &other )
      : m_nSize(0),
        m_nMaxSize(0),
        m_nGrowSize(eGrowSize),
        m_pArray(NULL)
    {
        Append(other);
    }
    virtual ~CBasicDynamicArray(void)
    {
        Destroy();
    }
    CBasicDynamicArray &operator=( const CBasicDynamicArray &other )
    {
        if (this != &other)
        {
            Destroy();
            Append(other);
        }
        return *this;
    }
    void Destroy(void)
    {
        if (m_pArray)
        {
            delete[] m_pArray;
            m_pArray = NULL;
        }
        m_nSize = m_nMaxSize = 0;
    }
    void Append( const CBasicDynamicArray &other )
    {
        if (GrowTo( m_nSize + other.Size() ))
        {
            for (int i=0;i<other.Size();i++)
            {
                Append(other[i]);
            }
        }
    }
    int Append( const T &element )
    {
        if (GrowTo( m_nSize + 1 ))
        {
            m_pArray[m_nSize] = element;
            int nResult = m_nSize;
            m_nSize++;
            return nResult;
        }
        else return -1;
    }
    int Insert( const T &element, int nIndex )
    {
        //
        // Make sure we can accomodate this new item
        //
        if (GrowTo( m_nSize + 1 ))
        {
            //
            // Make sure the item is within the range we've allocated
            //
            if (nIndex >= 0 && nIndex <= m_nSize)
            {
                //
                // Make room for the new item by moving all items above up by one slot
                //
                for (int i=Size();i>nIndex;i--)
                {
                    m_pArray[i] = m_pArray[i-1];
                }

                //
                // Save the new item
                //
                m_pArray[nIndex] = element;

                //
                // We're now one larger
                //
                m_nSize++;

                //
                // Return the index of the slot we used
                //
                return nIndex;
            }
        }

        //
        // Return an error
        //
        return -1;
    }
    void Delete( int nItem )
    {
        if (nItem >= 0 && nItem < m_nSize && m_pArray)
        {
            T *pTmpArray = new T[m_nMaxSize];
            if (pTmpArray)
            {
                T *pSrc, *pTgt;
                pSrc = m_pArray;
                pTgt = pTmpArray;
                for (int i=0;i<m_nSize;i++)
                {
                    if (i != nItem)
                    {
                        *pTgt = *pSrc;
                        pTgt++;
                    }
                    pSrc++;
                }
                delete[] m_pArray;
                m_pArray = pTmpArray;
                m_nSize--;
            }
        }
    }
    bool GrowTo( int nSize )
    {
        //
        // If the array is already large enough, just return true
        //
        if (nSize < m_nMaxSize)
        {
            return true;
        }

        //
        // Save old size, in case we can't allocate a new array
        //
        int nOldMaxSize = m_nMaxSize;

        //
        // Find the correct size to grow to
        //
        while (m_nMaxSize < nSize)
        {
            m_nMaxSize += m_nGrowSize;
        }

        //
        // Allocate the array
        //
        T *pTmpArray = new T[m_nMaxSize];
        if (pTmpArray)
        {
            //
            // Copy the old array over
            //
            for (int i=0;i<m_nSize;i++)
            {
                pTmpArray[i] = m_pArray[i];
            }

            //
            // Delete the old array
            //
            if (m_pArray)
            {
                delete[] m_pArray;
            }

            //
            // Assign the new array to the old one and return true
            //
            m_pArray = pTmpArray;
            return true;
        }
        else
        {
            //
            // If we couldn't allocate the new array, restore the maximum size
            // and return false
            //
            m_nMaxSize = nOldMaxSize;
            return false;
        }
    }
    
    //
    // Simple swap
    //
    void Swap( T &a, T &b )
    {
        T t = a;
        a = b;
        b = t;
    }

    int Find( const T& element ) const
    {
        for (int i=0;i<m_nSize;i++)
        {
            if (m_pArray[i] == element)
            {
                return i;
            }
        }
        return -1;
    }
    bool operator==( const CBasicDynamicArray &other )
    {
        if (Size() != other.Size())
            return false;
        for (int i=0;i<Size();i++)
            if (!(m_pArray[i] == other[i]))
                return false;
        return true;
    }
    bool Contains( const T& element )     { return(Find(element) >= 0);}
    void Size( int nSize )                { m_nSize = nSize;}
    void MaxSize( int nMaxSize )          { m_nMaxSize = nMaxSize;}
    void GrowSize( int nGrowSize )        { m_nGrowSize = nGrowSize;}
    int Size(void) const                  { return m_nSize;}
    int MaxSize(void) const               { return m_nMaxSize;}
    int GrowSize(void) const              { return m_nGrowSize;}
    T *GetBuffer( int nSize )             { return GrowTo(nSize) ? m_pArray : NULL;}
    const T *Array(void) const            { return m_pArray;}
    const T &operator[](int nIndex) const { return m_pArray[nIndex];}
    T &operator[](int nIndex)             { return m_pArray[nIndex];}
};

#endif

