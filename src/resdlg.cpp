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

#include <wx/fs_zip.h>
#include <wx/xrc/xmlres.h>

#include "resdlg.h"

#include <wx/listimpl.cpp>
WX_DEFINE_LIST(tResourceElementList);


/******************************************************************************
 * tResourceElement
 *****************************************************************************/

tResourceElement::tResourceElement() {
  // Set all pointer fields to zero.
  string = 0;
  longptr = 0;
  boolptr = 0;
  wxArrayLong longarr;
}


/******************************************************************************
 * tResourceDialog
 *****************************************************************************/

// Static Members

bool tResourceDialog::initialized;

void tResourceDialog::LoadResource(const wxString& xrcfile) {
  if(!initialized) {
    wxFileSystem::AddHandler(new wxZipFSHandler);
    wxXmlResource::Get()->InitAllHandlers();
    initialized = true;
  }
  
  wxXmlResource::Get()->Load(xrcfile);
}

// Instance Methods

tResourceDialog::tResourceDialog(wxWindow* parent, const wxString& name) {
  dialogName = name;

  // The system will report any errors in loading, assuming we don't crash
  // ourselves first.
  dialog = wxXmlResource::Get()->LoadDialog(parent, name);

  // Make the list delete its data items when the tResourceDialog is deleted.
  links.DeleteContents(true);
}

tResourceDialog::~tResourceDialog() {
  dialog->Destroy();
}

void tResourceDialog::Attach(const wxString& name, wxString *data) {
  tResourceElement *elem = new tResourceElement;
  elem->resource = name;
  elem->string = data;
  links.Append(elem);
}

void tResourceDialog::Attach(const wxString& name, bool *data) {
  tResourceElement *elem = new tResourceElement;
  elem->resource = name;
  elem->boolptr = data;
  links.Append(elem);
}

void tResourceDialog::Attach(const wxString& name, long *data, wxArrayLong a) {
  tResourceElement *elem = new tResourceElement;
  elem->resource = name;
  elem->longptr = data;
  elem->longarr = a;
  links.Append(elem);
}

void tResourceDialog::Attach(const wxString& name, long *data, long *a) {
  wxArrayLong arr;
  while(*a != -1) {
    arr.Add(*a);
    a++;
  }
  Attach(name, data, arr);
}

int tResourceDialog::ShowModal() {

  // Don't bother with dialogs that don't exist.  An error message will be
  // produced by the system at some point.
  if(!dialog) return wxID_CANCEL;


  // Iterate through the list of attachments.  If we can't find the resource,
  // let the user know.  Upon finding the resource, attempt to load the
  // current value(s) into it.

  wxtResourceElementListNode *node = links.GetFirst();
  tResourceElement *elem;
  
  while(node) {
    elem = node->GetData();

    wxWindow *win = wxWindow::FindWindowByName(elem->resource, dialog);

    if(!win) {
      wxMessageBox("Unable to locate widget named:\n"
		   "    " + elem->resource + "\n"
		   "Tried to find it in the dialog named:\n"
		   "    " + dialogName,
		   "Error Finding Resource",
		   wxOK | wxICON_ERROR);
      return wxID_CANCEL;
    }

    if(!LoadData(elem, win)) return wxID_CANCEL;

    node = node->GetNext();
  }

  int res = dialog->ShowModal();

  if(res == wxID_OK) {
    wxtResourceElementListNode *node = links.GetFirst();
    tResourceElement *elem;

    // Iterate through list of attached links.  For each link, try and move
    // the data from the wxWidget to the location of the relevant pointer.

    while(node) {
      elem = node->GetData();

      wxWindow *win = wxWindow::FindWindowByName(elem->resource, dialog);

      if(!StoreData(elem, win)) return wxID_CANCEL;

      node = node->GetNext();
    }
  }

  return res;
}

bool tResourceDialog::LoadData(tResourceElement *elem, wxWindow *win) {
  bool used = 0;

  if(elem->string) {
    if(win->IsKindOf(CLASSINFO(wxTextCtrl))) {
      used = 1;
      ((wxTextCtrl*)win)->SetValue(*elem->string);
    }
  } else if(elem->boolptr) {
    if(win->IsKindOf(CLASSINFO(wxCheckBox))) {
      used = 1;
      ((wxCheckBox*)win)->SetValue(*elem->boolptr);
    }
  } else if(elem->longptr) {
    if(win->IsKindOf(CLASSINFO(wxChoice))) {
      wxChoice *choice = (wxChoice*)win;
      if(choice->GetCount() != elem->longarr.GetCount()) {
	wxMessageBox("Error handling data for this widget:\n"
                     "    dialog = " + dialogName + "\n"
                     "    control = " + elem->resource + "\n"
		     "Length mismatch with translation array.",
		     "Error Loading Data",
		     wxOK | wxICON_ERROR);
      } else {
	int i;
	for(i=0; i<elem->longarr.GetCount(); i++) {
	  if(*elem->longptr == elem->longarr[i]) break;
	}
	
	if(i == elem->longarr.GetCount()) {
	  // We couldn't find the value in the list.
	  wxMessageBox("Error handling data for this widget:\n"
		       "    dialog = " + dialogName + "\n"
		       "    control = " + elem->resource + "\n"
		       "Initial value not found in translation array.",
		       "Error Loading Data",
		       wxOK | wxICON_ERROR);
	  return false;
	}

	choice->SetSelection(i);
	used = 1;
      }
    }
  }
  
  if(!used) {
    wxMessageBox("Unable to locate a mapping for this widget:\n"
		 "    dialog = " + dialogName + "\n"
		 "    widget = " + elem->resource,
		 "Error Loading Data",
		 wxOK | wxICON_ERROR);
  }

  return used;
}

bool tResourceDialog::StoreData(tResourceElement *elem, wxWindow *win) {
  // We don't need to do as much error checking here.  Most of that will have
  // been done in LoadData before things have had opportunity to modify
  // themselves.

  if(elem->string) {
    if(win->IsKindOf(CLASSINFO(wxTextCtrl))) {
      *(elem->string) = ((wxTextCtrl*)win)->GetValue();
    }
  } else if(elem->boolptr) {
    if(win->IsKindOf(CLASSINFO(wxCheckBox))) {
      *(elem->boolptr) = ((wxCheckBox*)win)->GetValue();
    }
  } else if(elem->longptr) {
    if(win->IsKindOf(CLASSINFO(wxChoice))) {
      int sel = ((wxChoice*)win)->GetSelection();

      if(sel < 0 || sel >= elem->longarr.GetCount()) {
	wxMessageBox("Error handling data for this widget:\n"
		     "    dialog = " + dialogName + "\n"
		     "    control = " + elem->resource + "\n"
		     "Selection value out of range.",
		     "Error Loading Data",
		     wxOK | wxICON_ERROR);
	return false;
      }

      *(elem->longptr) = elem->longarr[sel];
    }
  }

  return true;
}
