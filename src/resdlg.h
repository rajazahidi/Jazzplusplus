/*
 * Copyright (C) 2004, Patrick Earl
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

#ifndef resdlg_h
#define resdlg_h

#include <wx/wx.h>


/* This class links together a named resource and a pointer to a data item.
   It will be used to retrieve the data from the named resource and store that
   data into the memory location represented by the pointer.  The pointer
   fields should all be set to zero, except the one that needs to be filled.
   The constructor will set all the fields to zero. */

class tResourceElement {
 public:
  wxString *string;
  bool *boolptr;
  long *longptr;

  wxString resource;
  wxArrayLong longarr;

  tResourceElement();
};


/* Create a list that can contain tResourceElements. */

WX_DECLARE_LIST(tResourceElement, tResourceElementList);


/* This class is used to create a dialog from a WRC XML file.  The dialog
   should be configured to return wxID_OK when the dialog contents are
   accepted.  Normally, you would place OK and Cancel buttons on the dialog.
   Those buttons would have IDs of wxID_OK and wxID_CANCEL.

   The LoadResource function is used to initialize the system and to load
   the various resource files.  It should be called once for each resource
   file that needs loaded from the filesystem.  Generally, this would be
   done at application init time.

   A new dialog based on a resource can be created using the tResourceDialog
   constructor.  Pass in the parent of the dialog and the ID of the dialog
   from the XRC file.  If the tResourceDialog creation is too slow, try
   pre-loading any dialog on application init.  This appears to load stuff
   needed by the system, making subsequent dialog displays much faster.

   Once the dialog is created, you will likely want to run one or more of the
   Attach methods to attach certain named WRC resources with your own
   variables.  Once a variable is attached, any accepted changes to the dialog
   will cause the appropriate change in the variable.

   ShowModal() is used to spawn the dialog in a modal fashion, updating the
   variables before it returns.  Just as wxDialog::ShowModal(), it will return
   the result of the dialog box.
*/

class tResourceDialog {
 public:
  static void LoadResource(const wxString& xrcfile);

  tResourceDialog(wxWindow* parent, const wxString& name);
  ~tResourceDialog();

  void Attach(const wxString& name, wxString *data);
  void Attach(const wxString& name, bool *data);
  void Attach(const wxString& name, long *data, wxArrayLong a);
  void Attach(const wxString& name, long *data, long *a);

  int ShowModal();

 private:
  bool LoadData(tResourceElement *elem, wxWindow *win);
  bool StoreData(tResourceElement *elem, wxWindow *win);

  static bool initialized;    // If the resource system has been initialized.

  tResourceElementList links; // List of associations created by Attach.

  wxDialog *dialog;           // The actual dialog we create.

  wxString dialogName;        // The name of the dialog resource.  Used in
                              // error reporting.
};

#endif
