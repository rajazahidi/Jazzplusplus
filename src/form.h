/*
 * wxForm Replacement
 *
 * Copyright (C) 2003 Patrick Earl
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef form_h
#define form_h

class tForm {
  /*
   public:
      tForm();
      ~tForm();
  */
};

class tFormItem {
};

enum {
  TFORM_CONSTRAINT_NUMERIC,
  TFORM_CONSTRAINT_STRINGLIST
};

class tFormItemConstraint {
 private:
  int type;
  double lower;
  double upper;
  wxArrayString items;
 public:
  tFormItemConstraint(tFormItemConstraint &x);
  tFormItemConstraint(double start, double end);
  tFormItemConstraint(char *first, ...);
  tFormItemConstraint(wxList *list);
  ~tFormItemConstraint();
};

tFormItemConstraint tMakeConstraintStrings(wxList *list);
tFormItemConstraint tMakeConstraintStrings(char *first, ...);
tFormItemConstraint tMakeConstraintRange(double lo, double hi);

#endif
