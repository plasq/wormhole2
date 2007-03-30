//------------------------------------------------------------------------
//-
//- Filename    : WormHoleEditor.h
//- Created by  : Adrian Pflugshaupt
//- Description : Header file for the VST editview of WormHole2
//- 
//------------------------------------------------------------------------

#ifndef __wormholeeditor__
#define __wormholeeditor__

#ifndef __vstgui__
#include "vstgui.h"
#endif

#include "WHControls.h"

#include <math.h>
#include <stdlib.h>	
#include <stdio.h>
#if MAC
#include "time.h"
#endif

#ifndef __wormhole__
#include "WormHole15.hpp"
#endif

#ifndef __WHprefs
#include "WHprefs.h"
#endif

class WormHoleEditor : public AEffGUIEditor, public CControlListener
{
public:
#ifdef MACAU
	WormHoleEditor(AudioUnitCarbonView auv);
#else
	WormHoleEditor (AudioEffect *effect);
#endif

	virtual ~WormHoleEditor ();

	void suspend ();
	void resume ();

	void setMode(unsigned short mode);

	void updateInterface();

	void setStatusSynced(int count);
	void setStatusSendAddr(long);
	void setStatusRecvAddr(long);
	void setStatusError(char* message, double duration);
	void setStatusMessage(char* message, double duration);
	void setStatusLatency(int);
protected:
	virtual long open (void *ptr);
	virtual void idle ();
	void setParameter (long index, float value);
	virtual void close ();
	void openNormal();

private:
	void updateStatusString();
	void updateModeButtons();
	long listChangeCounter;
	int kickCounter;
	unsigned short currentMode;
	
	void valueChanged (CDrawContext* context, CControl* control);

	char statusString[512];
	char statusMessage[512];
	char statusError[512];
	double statusErrorTimeout;
	double statusMessageTimeout;
	int statusSynced;
	int statusLatency;
	long statusSendAddr;
	long statusRecvAddr;

	COnOffButton *cPlayThrough;
	COnOffButton *cLatencyCorrection;
	COnOffButton *cSync;

	WHmodeButton* cMode[WH_NUMMODES];

	WHexpSlider *cBufferSlider;
	CKickButton *cBufferKickLeft;
	CKickButton *cBufferKickRight;
	CTextEdit *cBufferDisplay;
	
	WHexpSlider *cDelaySlider;
	CKickButton *cDelayKickLeft;
	CKickButton *cDelayKickRight;
	CTextEdit *cDelayDisplay;

	WHchannelDisplay *cChannelName;
	COptionMenu *cChannelMenu;
	COptionMenu *cIFMenu;
	
	WHlevelLed* cInputLed;
	WHlevelLed* cOutputLed;
	
	CKickButton *cReset;
	
	CSplashScreen *cSplash;
	WHStringDisplay *cStatusDisplay;
};

#endif
