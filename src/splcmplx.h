/*
**  The JAZZ++ Midi Sequencer
**
** Copyright (C) 1994-2000 Andreas Voss and Per Sigmond, all rights reserved.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
*/                                                                              

#include <math.h>
struct c_complex
  { double re, im;
  };

struct complex
  { double re, im;
    complex(double r, double i = 0.0) { re = r; im = i; }
    complex() { }					/* uninitialized complex */
    complex(c_complex z) { re = z.re; im = z.im; }	/* init from denotation */
  };

extern complex csqrt(complex), cexp(complex), expj(double);	    /* from complex.C */
extern complex evaluate(complex[], int, complex[], int, complex);   /* from complex.C */

inline double hypot(complex z)
  { return hypot(z.im, z.re);
  }

inline double atan2(complex z)
  { return atan2(z.im, z.re);
  }

inline complex cconj(complex z)
  { z.im = -z.im;
    return z;
  }

inline complex operator * (double a, complex z)
  { z.re *= a; z.im *= a;
    return z;
  }

inline complex operator / (complex z, double a)
  { z.re /= a; z.im /= a;
    return z;
  }

inline void operator /= (complex &z, double a)
  { z = z / a;
  }

extern complex operator * (complex, complex);
extern complex operator / (complex, complex);

inline complex operator + (complex z1, complex z2)
  { z1.re += z2.re;
    z1.im += z2.im;
    return z1;
  }

inline complex operator - (complex z1, complex z2)
  { z1.re -= z2.re;
    z1.im -= z2.im;
    return z1;
  }

inline complex operator - (complex z)
  { return 0.0 - z;
  }

inline bool operator == (complex z1, complex z2)
  { return (z1.re == z2.re) && (z1.im == z2.im);
  }

inline complex sqr(complex z)
  { return z*z;
  }

