/*
**  Alacrity Midi Sequencer
**
** Some Code Copyright (C) 1994-2000 Andreas Voss and Per Sigmond, all rights reserved.
**    I don't know why it says "All Rights Reserved" and then is licensed GPL
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

#include "synthSettings.h"

#ifndef __PORTING

tSynthSettingsDlg::tSynthSettingsDlg(tEventWin *w)
: wxForm( USED_WXFORM_BUTTONS )
{
  EventWin = w;
  SynthTypeName = copystring( Config.StrValue(C_SynthType) );
  OldSynthTypeName = copystring( Config.StrValue(C_SynthType) );

  ResetString[0] = "Never";
  ResetString[1] = "Song start";
  ResetString[2] = "Start play";

  ResetVal = copystring( ResetString[Config( C_SendSynthReset )] );
}

void tSynthSettingsDlg::OnHelp()
{
        HelpInstance->ShowTopic("Synthesizer Type Settings");
}

void tSynthSettingsDlg::OnCancel()
{
  EventWin->DialogBox = 0;
  wxForm::OnCancel();
}

void tSynthSettingsDlg::OnOk()
{
   int i;
   char* str = copystring( SynthTypeName );

   for (i = 0; i < NumSynthTypes; i++)
   {
      if (!strcmp( str, SynthTypes[i].Name))
         break;
   }

   Config.Put( C_SynthConfig, SynthTypeFiles[i].Name );
   Config.StrValue(C_SynthType) = copystring(SynthTypes[i].Name);

   delete str;
   str = copystring( ResetVal );

   for (i = 0; i < NumResetStrings; i++)
   {
      if (!strcmp( str, ResetString[i]))
         break;
   }

   Config( C_SendSynthReset ) = i;
   Config.Put( C_SendSynthReset );

   if ( Config( C_SynthDialog ) ) {
      Config( C_SynthDialog ) = 0;
      Config.Put( C_SynthDialog );
   }

   if (strcmp( SynthTypeName, OldSynthTypeName ))
      wxMessageBox("Restart jazz for the changes to take effect", "Info", wxOK);

   delete str;
   delete SynthTypeName;
   delete OldSynthTypeName;
   delete ResetVal;
   EventWin->Redraw();
   EventWin->DialogBox = 0;
   wxForm::OnOk();
}

void tSynthSettingsDlg::EditForm(wxPanel *panel)
{
  int i;
   // following adapted from wxwin/src/base/wb_form.cc
   wxList *list = new wxList;
   for (i = 0; i < NumSynthTypes; i++)
      if (*SynthTypes[i].Name)	// omit empty entries
	 list->Append((wxObject *)copystring(SynthTypes[i].Name));

   wxFormItemConstraint *constraint = wxMakeConstraintStrings(list);

   Add(wxMakeFormNewLine());
   Add(wxMakeFormString( "Synthesizer type:",
			 &SynthTypeName,
			 //wxFORM_RADIOBOX,
			 wxFORM_DEFAULT,
			 new wxList(constraint, 0),
			 NULL,
			 wxVERTICAL
      )
      );

   // following adapted from wxwin/src/base/wb_form.cc
   wxList *list2 = new wxList;
   for (i = 0; i < NumResetStrings; i++)
      list2->Append((wxObject *)copystring( ResetString[i] ));

   wxFormItemConstraint *constraint2 = wxMakeConstraintStrings(list2);

   Add(wxMakeFormNewLine());
   Add(wxMakeFormString( "Auto send MIDI reset:",
                         &ResetVal,
                         //wxFORM_RADIOBOX,
                         wxFORM_DEFAULT,
                         new wxList(constraint2, 0),
                         NULL,
                         wxVERTICAL
   )
   );

   Add(wxMakeFormNewLine());

   AssociatePanel(panel);
}

#endif // Porting
