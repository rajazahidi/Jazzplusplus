//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Copyright (C) 1994-2000 Andreas Voss and Per Sigmond, all rights reserved.
// Modifications Copyright (C) 2004 Patrick Earl
// Modifications Copyright (C) 2008-2010 Peter J. Stieber
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//*****************************************************************************

#ifndef JZ_DYNAMICARRAY_H
#define JZ_DYNAMICARRAY_H

#include <assert.h>

#define DECLARE_ARRAY(CLASS, TYPE)                      \
                                                        \
class CLASS                                             \
{                                                       \
  public:                                               \
    CLASS(TYPE initVal, int initSize = 0);              \
    CLASS();                                            \
    virtual ~CLASS();                                   \
    CLASS(const CLASS &);                               \
    CLASS & operator=(const CLASS &);                   \
                                                        \
    TYPE & operator[](int i);                           \
    const TYPE operator[](int i) const;                 \
    int GetSize() const;                                \
  protected:                                            \
    void Resize(int newSize);                           \
    int  nArrays;     /* Anzahl Arrays */               \
    int  block_size;  /* Anzahl Elemente je Array */    \
    void Clear();     /* Loescht alle Arrays */         \
    TYPE initVal;                                       \
    TYPE** ppArray;                                     \
};


#define DEFINE_ARRAY(CLASS, TYPE)                              \
                                                               \
CLASS::CLASS(TYPE val, int s) : initVal(val)                   \
{                                                              \
  nArrays = 0;                                                 \
  ppArray = 0;                                                 \
  block_size = 16;                                             \
  if (s) Resize(s);                                            \
}                                                              \
                                                               \
CLASS::CLASS() : initVal(0)                                    \
{                                                              \
  nArrays = 0;                                                 \
  ppArray = 0;                                                 \
  block_size = 16;                                             \
}                                                              \
                                                               \
CLASS::CLASS(const CLASS &X) : initVal(X.initVal)              \
{                                                              \
  int i;                                                       \
  nArrays = 0;                                                 \
  ppArray = 0;                                                 \
  block_size = 16;                                             \
  for (i = 0; i < X.nArrays * block_size; i++)                 \
    (*this)[i] = (TYPE)X[i];                                   \
}                                                              \
                                                               \
void CLASS::Clear()                                            \
{                                                              \
  int i;                                                       \
  for (i = 0; i < nArrays; i++)                                \
  {                                                            \
    delete [] ppArray[i];                                      \
  }                                                            \
  delete [] ppArray;                                           \
  nArrays = 0;                                                 \
  ppArray = 0;                                                 \
}                                                              \
                                                               \
CLASS::~CLASS()                                                \
{                                                              \
  Clear();                                                     \
}                                                              \
                                                               \
CLASS& CLASS::operator=(const CLASS &X)                        \
{                                                              \
  int i;                                                       \
  if (&X == this)                                              \
    return *this;                                              \
  Clear();                                                     \
  initVal = X.initVal;                                         \
  block_size = X.block_size;                                   \
  for (i = 0; i < X.nArrays * X.block_size; i++)               \
  {                                                            \
    (*this)[i] = (TYPE)X[i];                                   \
  }                                                            \
  return *this;                                                \
}                                                              \
                                                               \
TYPE& CLASS::operator[](int i)                                 \
{                                                              \
  assert(i >= 0);                                              \
  Resize(i);                                                   \
  return ppArray[i / block_size][i % block_size];              \
}                                                              \
                                                               \
const TYPE CLASS::operator[](int i) const                      \
{                                                              \
  assert(i >= 0);                                              \
  int k = i / block_size;                                      \
  if (k >= nArrays || ppArray[k] == 0)                         \
  {                                                            \
    return initVal;                                            \
  }                                                            \
  return ppArray[k][i % block_size];                           \
}                                                              \
                                                               \
void CLASS::Resize(int newSize)                                \
{                                                              \
  int k = newSize / block_size;                                \
  if (k >= nArrays)                                            \
  {                                                            \
    int i, n = k + 1;                                          \
    TYPE **tmp = new TYPE * [n];                               \
    for (i = 0; i < nArrays; i++)                              \
    {                                                          \
      tmp[i] = ppArray[i];                                     \
    }                                                          \
    for (; i < n; i++)                                         \
    {                                                          \
      tmp[i] = 0;                                              \
    }                                                          \
    delete [] ppArray;                                         \
    ppArray = tmp;                                             \
    nArrays = n;                                               \
  }                                                            \
                                                               \
  if (ppArray[k] == 0)                                         \
  {                                                            \
    int i;                                                     \
    ppArray[k] = new TYPE [block_size];                        \
    for (i = 0; i < block_size; i++)                           \
    {                                                          \
      ppArray[k][i] = initVal;                                 \
    }                                                          \
  }                                                            \
}                                                              \
                                                               \
int CLASS::GetSize() const                                     \
{                                                              \
  return nArrays * block_size;                                 \
}


DECLARE_ARRAY(JZIntArray, int)



class JZUniqIds
{
  public:

    JZUniqIds();
    int Get();
    void Get(int id);

    // Returns the no of pending references to id.
    int Put(int id);

  private:

    JZIntArray mArray;
};


class JZBitset
{
  public:
    int operator()(int i)
    {
      return (mArray[index(i)] & mask(i)) != 0;
    }
    void set(int i, int b)
    {
      if (b)
      {
        mArray[index(i)] |= mask(i);
      }
      else
      {
        mArray[index(i)] &= ~mask(i);
      }
    }
    void operator += (int i)
    {
      mArray[index(i)] |= mask(i);
    }
    void operator -= (int i)
    {
      mArray[index(i)] &= ~mask(i);
    }

  private:

    JZIntArray mArray;

    // this works for sizeof(int) >= 4
    int index(int i)
    {
      return i >> 5;
    }
    int mask(int i)
    {
      return 1 << (i & 31);
    }
};


DECLARE_ARRAY(JZVoidPtrArray, void *)

#endif // !defined(JZ_DYNAMICARRAY_H)
