#include "WxWidgets.h"

#include "ProjectManager.h"

#include "PianoFrame.h"
#include "TrackFrame.h"
#include "GuitarFrame.h"
#include "Globals.h"

//*****************************************************************************
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZProjectManager* JZProjectManager::mpProjectManager = 0;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZProjectManager::JZProjectManager()
  : mpTrackFrame(0),
    mpPianoFrame(0),
    mpGuitarFrame(0)
{
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZProjectManager::~JZProjectManager()
{
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZProjectManager* JZProjectManager::Instance()
{
  if (!mpProjectManager)
  {
    mpProjectManager = new JZProjectManager;
  }
  return mpProjectManager;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZProjectManager::Destroy()
{
  delete mpProjectManager;
  mpProjectManager = 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZTrackFrame* JZProjectManager::CreateTrackView()
{
  if (!mpTrackFrame)
  {
    // Create the main application window.
    mpTrackFrame = new JZTrackFrame(
      0,
      "Jazz++",
      gpSong,
      wxPoint(10, 10),
      wxSize(600, 400));
  }

  mpTrackFrame->Show(true);

  return mpTrackFrame;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZProjectManager::CreatePianoView()
{
  if (!mpPianoFrame)
  {
    mpPianoFrame = new JZPianoFrame(
      mpTrackFrame,
      "Piano",
      gpSong,
      wxDefaultPosition,
      wxSize(640, 480));
  }

  mpPianoFrame->Show(true);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZProjectManager::Detach(JZPianoFrame* pPianoFrame)
{
  if (mpPianoFrame == pPianoFrame)
  {
    mpPianoFrame = 0;
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZProjectManager::CreateGuitarView()
{
  if (!mpGuitarFrame)
  {
    if (mpPianoFrame)
    {
      mpGuitarFrame = new JZGuitarFrame(mpPianoFrame);
    }
  }
  mpGuitarFrame->Show(true);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZProjectManager::Detach(JZGuitarFrame* pGuitarFrame)
{
  if (pGuitarFrame == mpGuitarFrame)
  {
    mpGuitarFrame = 0;
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZProjectManager::NewPlayPosition(int Clock)
{
  if (mpTrackFrame)
  {
    mpTrackFrame->NewPlayPosition(Clock);
  }

  if (mpPianoFrame)
  {
    mpPianoFrame->NewPlayPosition(Clock);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZProjectManager::ShowPitch(int Pitch)
{
  if (mpPianoFrame)
  {
    mpPianoFrame->ShowPitch(Pitch);
    mpPianoFrame->Update();
  }

  if (mpGuitarFrame)
  {
    mpGuitarFrame->ShowPitch(Pitch);
    mpGuitarFrame->Update();
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZProjectManager::UpdateAllViews()
{
  if (mpTrackFrame)
  {
    mpTrackFrame->Refresh();
  }

  if (mpPianoFrame)
  {
    mpPianoFrame->Refresh();
  }

  if (mpGuitarFrame)
  {
    mpGuitarFrame->Refresh();
  }
}

