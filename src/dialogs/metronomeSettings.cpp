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

#include "metronomeSettings.h"

#ifndef __PORTING


tMetronomeSettingsDlg::tMetronomeSettingsDlg(tEventWin *w)
: wxForm( USED_WXFORM_BUTTONS )
{
  EventWin = w;
  IsAccented = ((tTrackWin*)EventWin)->MetronomeInfo.IsAccented;
  KeyAcc = ((tTrackWin*)EventWin)->MetronomeInfo.KeyAcc;
  KeyNorm = ((tTrackWin*)EventWin)->MetronomeInfo.KeyNorm;
  Veloc = ((tTrackWin*)EventWin)->MetronomeInfo.Veloc;

  numNames = 0;
  for (int i = 0; Config.DrumName(i).Name; i++)
  {
    if (Config.DrumName(i).Name[0])
    {
      index2Pitch[numNames] = Config.DrumName(i).Value - 1;
      pitch2Index[Config.DrumName(i).Value - 1] = numNames;
      index2Name[numNames++] = Config.DrumName(i).Name;
    }
  }
  KeyAccName = copystring( index2Name[ pitch2Index[ KeyAcc ] ] );
  KeyNormName = copystring( index2Name[ pitch2Index[ KeyNorm ] ] );
}


void tMetronomeSettingsDlg::OnHelp()
{
	HelpInstance->ShowTopic("Metronome Settings");
}


void tMetronomeSettingsDlg::OnCancel()
{
  EventWin->DialogBox = 0;
  wxForm::OnCancel();
}


void tMetronomeSettingsDlg::OnOk()
{
  ((tTrackWin*)EventWin)->MetronomeInfo.IsAccented = IsAccented;
  int i;
  for (i = 0; i < numNames; i++)
  {
    if ( !strcmp(index2Name[i], KeyNormName) )
      break;
  }
  if (i < numNames)
    ((tTrackWin*)EventWin)->MetronomeInfo.KeyNorm = index2Pitch[i];
  for (i = 0; i < numNames; i++)
  {
    if (!strcmp( index2Name[i], KeyAccName ) )
      break;
  }
  if (i < numNames)
    ((tTrackWin*)EventWin)->MetronomeInfo.KeyAcc = index2Pitch[i];
  ((tTrackWin*)EventWin)->MetronomeInfo.Veloc = Veloc;
  EventWin->Redraw();
  EventWin->DialogBox = 0;
  wxForm::OnOk();
}



void tMetronomeSettingsDlg::EditForm(wxPanel *panel)
{
  Add(wxMakeFormBool( "Accented", &IsAccented ));
  Add(wxMakeFormNewLine());
  Add(wxMakeFormShort(	" Velocity:", &Veloc, wxFORM_DEFAULT,
                       	new wxList(wxMakeConstraintRange(0.0, 127.0), 0)));
  Add(wxMakeFormNewLine());
  Add(wxMakeFormString(	"Normal click:",
			&KeyNormName,
			wxFORM_DEFAULT,
			new wxList(wxMakeConstraintStrings(
				index2Name[ pitch2Index[ 37 ] ],
				index2Name[ pitch2Index[ 42 ] ],
				index2Name[ pitch2Index[ 56 ] ],
				0 ), 0),
			NULL,
			wxVERTICAL
			)
  );
  Add(wxMakeFormNewLine());
  Add(wxMakeFormString(	"Accented click:",
			&KeyAccName,
			wxFORM_DEFAULT,
			new wxList(wxMakeConstraintStrings(
				index2Name[ pitch2Index[ 36 ] ],
				index2Name[ pitch2Index[ 38 ] ],
				index2Name[ pitch2Index[ 54 ] ],
				0 ), 0),
			NULL,
			wxVERTICAL
			)
  );
  AssociatePanel(panel);
}



#endif // Porting

