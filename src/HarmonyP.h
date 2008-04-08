//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Copyright (C) 1994-2000 Andreas Voss and Per Sigmond, all rights reserved.
// Modifications Copyright (C) 2004 Patrick Earl
// Modifications Copyright (C) 2008 Peter J. Stieber
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

#ifndef JZ_HARMONYP_H
#define JZ_HARMONYP_H

#include <iostream>
#include <string>

// ------------------------------------------------------------------
// HBChord
// ------------------------------------------------------------------

enum TEScaleType
{
  Major,
  Harmon,
  Melod,
  Ionb13,
  nScaleTypes
};


class HBChord
{
    friend std::ostream& operator << (std::ostream& os, HBChord const &a);
    friend std::istream& operator >> (std::istream& is, HBChord &a);

  public:

    HBChord()
    {
      keys = 0;
    }

    HBChord(int k)
    {
      keys = k;
    }

    HBChord(
      int a,
      int b,
      int c = -1,
      int d = -1,
      int e = -1,
      int f = -1,
      int g = -1,
      int h = -1,
      int i = -1,
      int k = -1,
      int l = -1);

    void Rotate(int semis);
    int Fit(int key);                // nearest key in chord
    int Count() const;                // # keys in chord
    int Iter(int key) const;        // next key in chord

    void Clear()
    {
      keys = 0;
    }

    int Keys() const
    {
      return keys;
    }

    bool Contains(int key) const        // key in chord?
    {
      return ((1L << ((key + 240) % 12)) & keys) != 0;
    }

    bool Contains(HBChord const &o) const
    {
      return (keys & o.keys) == o.keys;
    }

    HBChord & operator += (int key)
    {
      keys |= (1L << ((key + 240) % 12));
      return *this;
    }

    HBChord & operator -= (int key)
    {
      keys &= ~(1L << ((key + 240) % 12));
      return *this;
    }

    HBChord & operator -= (const HBChord &o)
    {
      keys &= ~o.keys;
      return *this;
    }

    HBChord operator - (const HBChord &o) const
    {
      HBChord ret(*this);
      ret.keys &= ~o.keys;
      return ret;
    }

    HBChord & operator &= (const HBChord & sc)
    {
      keys &= sc.keys;
      return *this;
    }

    HBChord & operator |= (const HBChord &sc)
    {
      keys |= sc.keys;
      return *this;
    }

    HBChord & operator ^= (const HBChord &sc)
    {
      keys ^= sc.keys;
      return *this;
    }

    HBChord operator & (const HBChord &sc) const
    {
      HBChord ret(*this);
      ret &= sc;
      return ret;
    }

    HBChord operator | (const HBChord &sc) const
    {
      HBChord ret(*this);
      ret |= sc;
      return ret;
    }

    HBChord operator ^ (const HBChord &sc) const
    {
      HBChord ret(*this);
      ret ^= sc;
      return ret;
    }

    HBChord operator + (int key)
    {
      HBChord ret(*this);
      ret += key;
      return ret;
    }

    HBChord operator - (int key)
    {
      HBChord ret(*this);
      ret -= key;
      return ret;
    }

    bool operator == (const HBChord &o) const
    {
      return (keys == o.keys);
    }

    bool operator != (const HBChord &o) const
    {
      return (keys != o.keys);
    }

    void flat(int key)
    {
      *this -= key;
      *this += key-1;
    }

    void sharp(int key)
    {
      *this -= key;
      *this += key+1;
    }

    void CreateName(std::string& CordName, int key, int flat = 0);

    static const std::string ScaleName(int key, int flat = 0)
    {
      return mScaleNames[flat != 0][(key + 240) % 12];
    }

  protected:
    int keys;

  private:

    static const std::string const mScaleNames[2][12];
};

inline std::ostream& operator << (std::ostream& os, HBChord const &a)
{
  os << a.keys << std::endl;
  return os;
}

inline std::istream& operator >> (std::istream& is, HBChord &a)
{
  is >> a.keys;
  return is;
}


// scales, msb = highest note
const HBChord major_scale       (0xab5L);
const HBChord harmonic_scale         (0x9adL);
const HBChord melodic_scale         (0xaadL);
const HBChord ionb13_scale         (0x9b5L);

const HBChord altered_scale        (0, 1, 3, 4, 6, 8, 10);
const HBChord dimin_scale       (0xb6dL);
const HBChord chromatic_scale         (0xfffL);

// chords, based to C
const HBChord Cj7        (0,4,7,11);
const HBChord Cj7b5        (0,4,6,11);
const HBChord Cj7s5        (0,4,8,11);

const HBChord C7        (0,4,7,10);
const HBChord C7b5        (0,4,6,10);
const HBChord C7s5        (0,4,8,10);

const HBChord Cm7        (0,3,7,10);
const HBChord Cm7b5        (0,3,6,10);
const HBChord Cm7s5        (0,3,8,10);

const HBChord Cmj7        (0,3,7,11);
const HBChord Cmj7b5        (0,3,6,11);
const HBChord Cmj7s5        (0,3,8,11);

const HBChord C0        (0,3,6,9);

// ------------------------------------------------------------------
// HBContext
// ------------------------------------------------------------------

#define NAME_TABLE 0

class HBContext
{
    friend class HBContextIterator;
    friend std::ostream& operator << (std::ostream& os, HBContext const &a);
    friend std::istream& operator >> (std::istream& is, HBContext &a);

  public:

    HBContext(int sn, int cn = 0, TEScaleType st = Major);

    HBContext();

    HBChord *PScale()
    {
      return &scale;
    }

    HBChord Scale() const
    {
      return scale;
    }

    HBChord *PChord()
    {
      return &chord;
    }

    HBChord Chord() const
    {
      return chord;
    }

    int ScaleKey() const
    {
      return scale_nr;
    }

    int ChordKey() const
    {
      return chord_key;
    }

    int ChordNr() const
    {
      return chord_nr;
    }

    int ScaleNr() const
    {
      return scale_nr;
    }

    TEScaleType ScaleType() const
    {
      return scale_type;
    }

    int SeqNr() const
    {
      return seq_nr;
    }

    void SetSeqNr(int n = 0)
    {
      seq_nr = n;
    }

    // For example "Dm75-"
    std::string GetChordName() const;

    // For example "IV"
    const char* ChordNrName() const;

    const char* ContextName() const               // "mixo#11"
    {
      return context_names[scale_type][chord_nr];
    }

    // For example "C#"
    const std::string& GetScaleName() const;

    const char* ScaleTypeName() const;            // "major"

    int operator == (const HBContext& Rhs) const
    {
      return
        scale_type == Rhs.scale_type &&
        scale_nr   == Rhs.scale_nr &&
        chord_nr   == Rhs.chord_nr;
    }

    int operator != (const HBContext& Rhs) const
    {
      return !operator == (Rhs);
    }

  private:

    void Initialize();

    HBChord MakeScale() const;

    HBChord MakeChord() const;

    int MakeChordKey() const;

    TEScaleType scale_type;

    int scale_nr;

    int chord_nr;

    int seq_nr;

    HBChord chord;

    HBChord scale;

    int chord_key;

#if NAME_TABLE
    static const char *const chord_names[nScaleTypes][7];
#endif

    static const char* const chord_nr_names[7];
    static const char* const scale_type_names[nScaleTypes];
    static const int         flat_keys[12];
    static const char* const context_names[nScaleTypes][7];
};


// ------------------------------------------------------------------
// HBContextIterator
// ------------------------------------------------------------------

class HBMatch
{
  public:

    virtual ~HBMatch()
    {
    }

    virtual bool operator()(const HBContext &)
    {
      return true;
    }
};

class HBMatchContains : public HBMatch
{
  public:

    HBMatchContains(HBChord c)
      : chord(c)
    {
    }

    virtual bool operator()(const HBContext &iter);

  private:

    HBChord chord;
};


class HBContextIterator
{
  public:

    HBContextIterator();

    HBContextIterator(HBMatch &);

    void SetSequence(HBContext *s[], int n)
    {
      seq = s;
      n_seq = n;
    }

    void SetScaleType(TEScaleType st)
    {
      scale_type = st;
    }

    bool operator()();

    const HBContext *operator->() const
    {
      return &context;
    }

    const HBContext& Context() const
    {
      return context;
    }

  private:

    HBContext context;

    HBMatch& match;

    HBMatch def_match;

    HBContext** seq;

    int i_seq, n_seq;

    TEScaleType scale_type;
};

#endif // !defined(JZ_HARMONYP_H)
