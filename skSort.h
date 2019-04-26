/*
-------------------------------------------------------------------------------

    Copyright (c) Charles Carley.

    Contributor(s): none yet.

-------------------------------------------------------------------------------
  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
-------------------------------------------------------------------------------
*/
#ifndef _skSort_h_
#define _skSort_h_

#include "Config/skConfig.h"
#include "skTraits.h"
#include "skMinMax.h"


template <typename T, typename C>
struct skSort {
    typedef int (*Function)(T a, T b);
    typedef typename C::Iterator Iterator;
    typedef typename C::ReferenceType ReferenceType;

    skSort(Function fnc = 0)
        :   m_sort(fnc)
    {
    }

    Function m_sort;
    void sort(C& container)
    {
        SKsize size = container.size();

        if (!m_sort || size < 2)
            return;

        bool swapped = false;
        SKsize i;

        do {
            if (size - 1 == SK_NPOS)
                break;

            Iterator it = container.iterator();
            for (i = 0; i < (size - 1); i++) {
                ReferenceType a = it.getNext();
                ReferenceType b = it.peekNext();

                if (!m_sort(a, b))
                {
                    swapped = true;
                    skSwap<T>(a, b);
                }
            }
            size -= 1;
        } while (swapped && size != SK_NPOS);
    }
};

template <typename T, typename C>
class skQSort {
public:
    typedef int(*Function)(T a, T b);
    typedef typename C::ReferenceType ReferenceType;


private:
    Function m_sort;

public:
    skQSort(Function fnc = 0)
        : m_sort(fnc)
    {
    }

    
    void sort(C& container)
    {
        SKsize size = container.size();
        if (!m_sort || size < 2)
            return;

        _sort(container, 0, size - 1);
    }

private:
    void _sort(C& container, SKsize first, SKsize last)
    {
        SKsize pivot;

        if (first < last && last != SK_NPOS)
        {
            pivot = _partition(container, first, last);
            _sort(container, first, pivot -1);
            _sort(container, pivot+1, last);
        }
    }

    SKsize _partition(C& container, SKsize first, SKsize last)
    {
        SKsize lo;

        skSwap(container[first], container[(first + last) / 2]);

        ReferenceType pivot = container[first];
        lo = first;
        for (SKsize i = first + 1; i <= last; ++i)
        {
            if (m_sort(container[i], pivot) != 0)
            {
                ++lo;
                if (lo != i)
                    skSwap(container[lo], container[i]);
            }
        }
        skSwap(container[first], container[lo]);
        return lo;
    }
};


#endif//_skSort_h_
