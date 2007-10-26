//------------------------------------------------------------------------
//-
//- Filename    : WormHoleEditor.cpp
//- Created by  : Adrian Pflugshaupt
//------------------------------------------------------------------------

#ifndef __wormholeeditor__
#include "WormHole15Editor.h"
#endif

enum
{
	// bitmaps
	kBackgroundBitmap = 10001,
	
	kPopupBitmap = 10002,

	kSliderHandleBitmap = 10003,	
	kSliderBgBitmap = 10004,
	kSliderKickLeftBitmap = 10005,
	kSliderKickRightBitmap = 10007,
	kModeButtonBitmap = 10006,
	kOnOffBitmap = 10008,
	kLevelLedBitmap = 10009,
	kResetBitmap = 10010,
	kLinkedBitmap = 10011,
//	kChannelNameBitmapOff = 10012,
	kAutoLatencyBitmap = 10013,
	kEditionBitmap = 10014,
//	kChannelNameBitmapOn = 10015,
	kAuthBoxBitmap = 10016,
	kSplashBitmap = 10017,
	kIFBitmap = 10018,
	kDemoBitmap = 10019,
	kAuthButtonBitmap = 10020,
	
	// others
	kBackgroundW = 355,
	kBackgroundH = 211,
};

inline double get_time2()
{
#if MAC
	struct timeval tv;
	gettimeofday(&tv,NULL); // The hour
	return double(tv.tv_sec)+double(tv.tv_usec)*1e-6;
#elif WINDOWS
	return double(GetTickCount())/1000.0; // Time since system started -> good enuff
#endif
}

void stringConvert (float value, char* string);

void bufferConvert (float value, char* string)
{
	sprintf(string,"%d",int(value*WHMAXBUFFER));
}

float bufferConvertValue (char* string)
{
	int val;
	sscanf(string,"%d",&val);
	return float(val)/float(WHMAXBUFFER);
}

void delayConvert (float value, char* string)
{
	sprintf(string,"%d",int(value*WHMAXDELAY));
}

float delayConvertValue (char* string)
{
	int val;
	sscanf(string,"%d",&val);
	return float(val)/float(WHMAXDELAY);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#ifdef MACAU
COMPONENT_ENTRY(WormHoleEditor)
WormHoleEditor::WormHoleEditor (AudioUnitCarbonView auv) : AEffGUIEditor (auv)
#else
WormHoleEditor::WormHoleEditor (AudioEffect *effect) : AEffGUIEditor (effect)
#endif
{
	frame = 0;
	
	close(); // Clears all pointers.

	rect.left   = 0;
	rect.top    = 0;
	rect.right  = kBackgroundW;
	rect.bottom = kBackgroundH;
	
	kickCounter=0;
	currentMode=1000;

//	ChannelNameBitmapOff = new CBitmap(kChannelNameBitmapOff);
//	ChannelNameBitmapOn = new CBitmap(kChannelNameBitmapOn);
	
	statusSynced=-1;
	statusSendAddr=0;
	statusRecvAddr=0;
	statusString[0]=0;
	statusErrorTimeout=0;
	statusMessageTimeout=0;
	statusLatency=-1;
}

//-----------------------------------------------------------------------------
WormHoleEditor::~WormHoleEditor ()
{
	getEffect()->setEditor(NULL);
	//ChannelNameBitmapOff->forget();
	//ChannelNameBitmapOn->forget();
}

//-----------------------------------------------------------------------------
long WormHoleEditor::open (void *ptr)
{
	// always call this !!!
	AEffGUIEditor::open (ptr);

	// get version
	//int version = getVstGuiVersion ();
	//int verMaj = (version & 0xFF00) >> 16;
	//int verMin = (version & 0x00FF);

	// init the background bitmap
	CBitmap *background = new CBitmap (kBackgroundBitmap);

	//--CFrame-----------------------------------------------
	CRect size (0, 0, background->getWidth () + 100, background->getHeight ());
	frame = new CFrame (size, ptr, this);
	frame->setBackground (background);
	background->forget();

    openNormal();

	resume();
		
	return true;
}

void WormHoleEditor::openNormal()
{
	CRect size;
	CPoint point (0, 0);

	//--COnOffButton-----------------------------------------------
	CBitmap *onOffButton = new CBitmap (kOnOffBitmap);

	size (0, 0, onOffButton->getWidth (), onOffButton->getHeight () / 2);
	size.offset (263, 103);
	cPlayThrough = new COnOffButton (size, this, kPlayThrough, onOffButton);
	frame->addView (cPlayThrough);

	onOffButton->forget ();

	// Auto Latency

	CBitmap* autoLatency = new CBitmap (kAutoLatencyBitmap);

	size (0, 0, autoLatency->getWidth (), autoLatency->getHeight () / 2);
	size.offset (62, 134);
	cLatencyCorrection = new COnOffButton (size, this, kLatencyCorrection, autoLatency);
	frame->addView (cLatencyCorrection);

	autoLatency->forget();
	
	// Sync Button
	
	CBitmap* syncBitmap=new CBitmap(kLinkedBitmap);
	size(0,0, syncBitmap->getWidth(), syncBitmap->getHeight()/2);
	size.offset(31,112);
	//size.offset(40, 113);
	cSync=new COnOffButton(size, this, kSync, syncBitmap);
	//cSync=new WHlevelLed(size, syncBitmap, 2);
	frame->addView(cSync);
	syncBitmap->forget();

	// The mode buttons

	CColor transColor;
	transColor.red=0; transColor.green=255; transColor.blue=0;

	onOffButton = new CBitmap (kModeButtonBitmap);
	onOffButton->setTransparentColor(transColor);

	size (0, 0, onOffButton->getWidth (), onOffButton->getHeight () / 4);
	size.offset (22, 81);
	cMode[WH_START] = new WHmodeButton (size, this, onOffButton);
	cMode[WH_START]->setTransparency(true);
	frame->addView (cMode[WH_START]);

	size (0, 0, onOffButton->getWidth (), onOffButton->getHeight () / 4);
	size.offset (72, 81);
	cMode[WH_END] = new WHmodeButton (size, this, onOffButton);
	cMode[WH_END]->setTransparency(true);
	frame->addView (cMode[WH_END]);

	size (0, 0, onOffButton->getWidth (), onOffButton->getHeight () / 4);
	size.offset (134, 94);
	cMode[WH_ORIGIN] = new WHmodeButton (size, this, onOffButton);
	cMode[WH_ORIGIN]->setTransparency(true);
	frame->addView (cMode[WH_ORIGIN]);

	size (0, 0, onOffButton->getWidth (), onOffButton->getHeight () / 4);
	size.offset (175, 87);
	cMode[WH_DESTINATION] = new WHmodeButton (size, this, onOffButton);
	cMode[WH_DESTINATION]->setTransparency(true);
	frame->addView (cMode[WH_DESTINATION]);

	size (0, 0, onOffButton->getWidth (), onOffButton->getHeight () / 4);
	size.offset (224, 93);
	cMode[WH_LOOPBACK] = new WHmodeButton (size, this, onOffButton);
	cMode[WH_LOOPBACK]->setTransparency(true);
	frame->addView (cMode[WH_LOOPBACK]);
	
	// Channel Name text edit

	size(0,0,269,17);
	//size(1,0,ChannelNameBitmapOff->getWidth()-2,ChannelNameBitmapOff->getHeight());
	size.offset(20,16); 
	cChannelName = new WHchannelDisplay(size, this); //,0,NULL,ChannelNameBitmapOff,0);

	frame->addView (cChannelName);
	
	// Interface Pop-Up Menu
	WormHole2* fx=(WormHole2*)effect;

	CBitmap* IFBitmap=new CBitmap(kIFBitmap);

	size(0,0,IFBitmap->getWidth(),IFBitmap->getHeight()-1);
	size.offset(252,119);
	cIFMenu = new COptionMenu (size, this, 0, IFBitmap, 0, kPopupStyle);
	IFBitmap->forget();
	CColor mColor;
	mColor.red=0x74; mColor.green=0x74; mColor.blue=0x74;
	cIFMenu->setFont(kNormalFontVerySmall);
	cIFMenu->setFontColor(mColor);
	cIFMenu->setHoriAlign(kRightText);
	int iIFcount=0;
	WHinterface* theInterface;
	cIFMenu->addEntry("all interfaces    ");
	while ((theInterface = fx->globalInstance->getInterface(iIFcount)))
	{
		char temp[100];
		strcpy(temp,theInterface->name);
		strcat(temp,"    ");
		cIFMenu->addEntry(temp);
		iIFcount++;
	}
	// Only display the menu if there is more than one interface!
	if (iIFcount>1)
		frame->addView(cIFMenu);

	// Channel Pop-Up Menu
		
	CBitmap *popupBitmap = new CBitmap (kPopupBitmap);

	size(0, 0 , popupBitmap->getWidth(), popupBitmap->getHeight());
	size.offset(304,10);
	
	cChannelMenu = new COptionMenu (size, this, 0, popupBitmap, 0, kPopupStyle | kNoTextStyle);	
	cChannelMenu->addEntry("none");

	frame->addView(cChannelMenu);
	
	popupBitmap->forget();
	
	
	int xpos,ypos,textboxsize;
	CBitmap *sliderBg = new CBitmap (kSliderBgBitmap);
	CBitmap *handle = new CBitmap (kSliderHandleBitmap);
	CBitmap *kickLeft = new CBitmap (kSliderKickLeftBitmap);
	CBitmap *kickRight = new CBitmap (kSliderKickRightBitmap);

	CColor fColor;
	fColor.red=0x52; fColor.green=0x52; fColor.blue=0x52;
	CColor bColor;
	bColor.red=231; bColor.green=231; bColor.blue=231;

	// Buffer Slider
	
	xpos=121; ypos=152; textboxsize=29;
	
	size(0,0, kickLeft->getWidth(), kickLeft->getHeight()/2);
	size.offset(xpos+34,ypos+14);
	cBufferKickLeft = new CKickButton(size, this, 0, kickLeft->getHeight()/2, kickLeft, point);
	frame->addView(cBufferKickLeft);

	size(0,0,sliderBg->getWidth(),sliderBg->getHeight());
	size.offset(xpos,ypos);	
	cBufferSlider = new WHexpSlider(size, this, kBufferSize, point, sliderBg->getWidth(), handle, sliderBg, point, kLeft);
	frame->addView(cBufferSlider);

	size(0,0, kickRight->getWidth(), kickRight->getHeight()/2);
	size.offset(xpos+71,ypos+14);
	cBufferKickRight = new CKickButton(size, this, 0, kickRight->getHeight()/2, kickRight, point);
	frame->addView(cBufferKickRight);
	
	size(0,0, textboxsize, 9);
	size.offset(xpos+41,ypos+12);
	cBufferDisplay = new CTextEdit(size,this, kBufferSize, NULL,NULL,0);
	cBufferDisplay->setBackColor(bColor);
	cBufferDisplay->setFrameColor(bColor);
	cBufferDisplay->setFontColor(fColor);
	cBufferDisplay->setFont(kNormalFontVerySmall);
	frame->addView(cBufferDisplay);

	// Delay Slider
	
	xpos=21; ypos=152; textboxsize=29;
	
	size(0,0, kickLeft->getWidth(), kickLeft->getHeight()/2);
	size.offset(xpos+34,ypos+14);
	cDelayKickLeft = new CKickButton(size, this, 0, kickLeft->getHeight()/2, kickLeft, point);
	frame->addView(cDelayKickLeft);

	size(0,0,sliderBg->getWidth(),sliderBg->getHeight());
	size.offset(xpos,ypos);	
	cDelaySlider = new WHexpSlider(size, this, kDelay, point, sliderBg->getWidth(), handle, sliderBg, point, kLeft);
	frame->addView(cDelaySlider);

	size(0,0, kickRight->getWidth(), kickRight->getHeight()/2);
	size.offset(xpos+71,ypos+14);
	cDelayKickRight = new CKickButton(size, this, 0, kickRight->getHeight()/2, kickRight, point);
	frame->addView(cDelayKickRight);
	
	size(0,0, textboxsize, 9);
	size.offset(xpos+41,ypos+12);
	cDelayDisplay = new CTextEdit(size,this, kDelay, NULL,NULL,0);
	cDelayDisplay->setBackColor(bColor);
	cDelayDisplay->setFrameColor(bColor);
	cDelayDisplay->setFontColor(fColor);
	cDelayDisplay->setFont(kNormalFontVerySmall);
	frame->addView(cDelayDisplay);
			
	sliderBg->forget();
	handle->forget();
	kickLeft->forget();
	kickRight->forget();
	
	// Level Leds
	
	CBitmap* levelLedBitmap=new CBitmap(kLevelLedBitmap);
	levelLedBitmap->setTransparentColor(transColor);

	size(0,0,levelLedBitmap->getWidth(),levelLedBitmap->getHeight()/8);
	size.offset(263, 71);
	cInputLed=new WHlevelLed(size,levelLedBitmap,8);
	cInputLed->setTransparency(true);
	frame->addView(cInputLed);
	
	size(0,0,levelLedBitmap->getWidth(),levelLedBitmap->getHeight()/8);
	size.offset(263, 88);
	cOutputLed=new WHlevelLed(size,levelLedBitmap,8);
	cOutputLed->setTransparency(true);
	frame->addView(cOutputLed);
	
	levelLedBitmap->forget();
		
	// Reset BUtton

	CBitmap* resetBitmap=new CBitmap(kResetBitmap);
	size(0,0, resetBitmap->getWidth(), resetBitmap->getHeight()/2);
	size.offset(194,45);
	cReset = new CKickButton(size, this, 0, resetBitmap->getHeight()/2, resetBitmap, point);
	frame->addView(cReset);
	resetBitmap->forget();

	// Status display
	
	size(0,0,314,11);
	size.offset(27, 188);
	cStatusDisplay = new WHStringDisplay(size, NULL);
	strcpy(cStatusDisplay->string, "...");
	CColor statusBack;
	statusBack.red=230; statusBack.green=230; statusBack.blue=230;
	cStatusDisplay->setFontColor(kGreyCColor);
	cStatusDisplay->setBackColor(statusBack);
	cStatusDisplay->setHoriAlign(kLeftText);
	frame->addView(cStatusDisplay);
	
	// Splashscreen
	
	CBitmap* splashBitmap = new CBitmap (kSplashBitmap);
	size (0, 0, splashBitmap->getWidth(), splashBitmap->getHeight());
	size.offset((kBackgroundW - splashBitmap->getWidth())/2, (kBackgroundH - splashBitmap->getHeight())/2);
	CRect zone(0,0,122,40);
	zone.offset(219, 138);
	
	cSplash = new CSplashScreen(zone, this, 9999, splashBitmap, size, point); 
	frame->addView(cSplash);

	splashBitmap->forget();
}
//-----------------------------------------------------------------------------
void WormHoleEditor::resume ()
{

	WormHole2* fx=(WormHole2*)effect;
	if (!fx) return;
	// parameter holen

	if (cPlayThrough) cPlayThrough->setValue (effect->getParameter(kPlayThrough));
	if (cLatencyCorrection) cLatencyCorrection->setValue (effect->getParameter(kLatencyCorrection));
	if (cSync) cSync->setValue (effect->getParameter(kSync));
	if (cChannelName) 
	{
		cChannelName->setText(fx->getChannelName());
	/*	if (fx->getChannelName()[0]==0)
			cChannelName->setBackground(ChannelNameBitmapOff);
		else
			cChannelName->setBackground(ChannelNameBitmapOn);*/
		cChannelName->setDirty();
	}
	listChangeCounter=0;
	if (cBufferSlider) cBufferSlider->setValue (effect->getParameter(kBufferSize));
	if (cBufferDisplay)
	{
		char temp[20];
		bufferConvert(effect->getParameter(kBufferSize),temp);
		cBufferDisplay->setText(temp);
	}
	if (cDelaySlider) cDelaySlider->setValue (effect->getParameter(kDelay));
	if (cDelayDisplay)
	{
		char temp[20];
		delayConvert(effect->getParameter(kDelay),temp);
		cDelayDisplay->setText(temp);
	}

	// Re-Initalize mode buttons 
	currentMode=1000;
	setMode(fx->getMode());
	// Initialize Status String
	updateStatusString();
	// Update interface display
	if (cIFMenu)
	{
		int active=0;
		int count=0;
		int activeCount=0;
		WHinterface* theInterface;
		while ((theInterface = fx->globalInstance->getInterface(count)))
		{
			WHdebug("Editor IF %s active %d",theInterface->id,theInterface->active);
			if (theInterface->active)
			{
				activeCount++;
				active=count;
			}
			count++;
		}
		WHdebug("Activecount %d",activeCount);
		if (activeCount==1)
			cIFMenu->setCurrent(active+1);
		else
			cIFMenu->setCurrent(0);
	}
}

void WormHoleEditor::setMode(unsigned short mode)
{
	if (mode==currentMode) return;
	currentMode=mode;

	updateModeButtons();
	if (!frame || !cBufferSlider) return;
	
	// remove controls
	
	frame->removeView(cBufferSlider);
	frame->removeView(cBufferKickLeft);
	frame->removeView(cBufferKickRight);
	frame->removeView(cBufferDisplay);

	frame->removeView(cDelaySlider);
	frame->removeView(cDelayKickLeft);
	frame->removeView(cDelayKickRight);
	frame->removeView(cDelayDisplay);
	
	frame->removeView(cLatencyCorrection);
	frame->removeView(cSync);
	
	// add controls

	if (mode==WH_END || mode==WH_ORIGIN || mode==WH_DESTINATION)
	{
		frame->addView(cBufferSlider);
		frame->addView(cBufferKickLeft);
		frame->addView(cBufferKickRight);
		frame->addView(cBufferDisplay);
	}
	if (mode==WH_START || mode==WH_ORIGIN)
	{
		frame->addView(cDelaySlider);
		frame->addView(cDelayKickLeft);
		frame->addView(cDelayKickRight);
		frame->addView(cDelayDisplay);
	}
	if (mode==WH_ORIGIN)
		frame->addView(cLatencyCorrection);
	if (WHisDirectMode(mode))
		frame->addView(cSync);
		
	updateStatusString();
	frame->setDirty();
}

//-----------------------------------------------------------------------------
void WormHoleEditor::suspend ()
{
	// called when the plugin will be Off
}

//-----------------------------------------------------------------------------
void WormHoleEditor::close ()
{
	// don't forget to remove the frame !!
	if (frame)
		delete frame;
	frame = 0;
	
	cPlayThrough=NULL;
	cLatencyCorrection=NULL;
	cMode[WH_START]=NULL;
	cMode[WH_END]=NULL;
	cMode[WH_ORIGIN]=NULL;
	cMode[WH_DESTINATION]=NULL;
	cMode[WH_LOOPBACK]=NULL;
	
	cBufferSlider=NULL;
	cBufferKickLeft=NULL;
	cBufferKickRight=NULL;
	cBufferDisplay=NULL;

	cDelaySlider=NULL;
	cDelayKickLeft=NULL;
	cDelayKickRight=NULL;
	cDelayDisplay=NULL;

	cChannelName=NULL;
	cChannelMenu=NULL;
	cIFMenu=NULL;
	
	cInputLed=NULL;
	cOutputLed=NULL;
	cSync=NULL;
	cReset=NULL;
	
	cStatusDisplay=NULL;
	cSplash=NULL;
}

//-----------------------------------------------------------------------------
void WormHoleEditor::idle ()
{
	AEffGUIEditor::idle ();		// always call this to ensure update

	if (!frame) return;
	if (!effect) return;
	WormHole2* fx=(WormHole2*)effect;

//	long newTicks = getTicks ();

	// KickButtons
	
	if (cBufferKickLeft && cBufferKickRight && cDelayKickLeft && cDelayKickRight)
	{
		if (cBufferKickLeft->getValue() != 0.0f)
		{
			if (kickCounter==0 || kickCounter>10)
			{
				int bufferValue=int(effect->getParameter(kBufferSize)*WHMAXBUFFER);
				if (kickCounter<=25) bufferValue--;
				else /*if (kickCounter<=60)*/ bufferValue-=10;
				//else bufferValue-=100;
				if (bufferValue<0) bufferValue=0;
				effect->setParameterAutomated(kBufferSize,float(bufferValue)/float(WHMAXBUFFER));
			}
			kickCounter++;
		}
		else if (cBufferKickRight->getValue() != 0.0f)
		{
			if (kickCounter==0 || kickCounter>10)
			{
				int bufferValue=int(effect->getParameter(kBufferSize)*WHMAXBUFFER);
				if (kickCounter<=25) bufferValue++;
				else /* if (kickCounter<=60)*/ bufferValue+=10;
				//else bufferValue+=100;
				if (bufferValue>WHMAXBUFFER) bufferValue=WHMAXBUFFER;
				effect->setParameterAutomated(kBufferSize,float(bufferValue)/float(WHMAXBUFFER));
			}
			kickCounter++;
		}
		else if (cDelayKickLeft->getValue() != 0.0f)
		{
			if (kickCounter==0 || kickCounter>10)
			{
				int bufferValue=int(effect->getParameter(kDelay)*WHMAXDELAY);
				if (kickCounter<=25) bufferValue--;
				else /*if (kickCounter<=60)*/ bufferValue-=10;
				//else bufferValue-=100;
				if (bufferValue<0) bufferValue=0;
				effect->setParameterAutomated(kDelay,float(bufferValue)/float(WHMAXDELAY));
			}
			kickCounter++;
		}
		else if (cDelayKickRight->getValue() != 0.0f)
		{
			if (kickCounter==0 || kickCounter>10)
			{
				int bufferValue=int(effect->getParameter(kDelay)*WHMAXDELAY);
				if (kickCounter<=25) bufferValue++;
				else /* if (kickCounter<=60)*/ bufferValue+=10;
				//else bufferValue+=100;
				if (bufferValue>WHMAXDELAY) bufferValue=WHMAXDELAY;
				effect->setParameterAutomated(kDelay,float(bufferValue)/float(WHMAXDELAY));
			}
			kickCounter++;
		}
		else
		{
			kickCounter=0;
		}
	}

	// Check List changes
	WHStatic* global=fx->globalInstance;
	if (listChangeCounter!=global->getChangeCounter() && cChannelMenu)
	{
		listChangeCounter=global->getChangeCounter();
		WHdebug("[WHEditor] updateing port list from global instance.\n");
		char** portList;
		int count;
		int k=0;
			
		// Popup Menu
		char temp[WHMAXPORTNAMESIZE];

		int direct;
		count=global->getChannelList(&portList,&direct);

		cChannelMenu->removeAllEntry();

		if (fx->getChannelName()[0]!=0)
		{
			sprintf(temp,"%s",fx->getChannelName());
			cChannelMenu->addEntry(temp);
			cChannelMenu->setStyle(kCheckStyle | kNoTextStyle);
			cChannelMenu->checkEntryAlone(0); // Flag it
		}
		else
		{
			cChannelMenu->setStyle(kPopupStyle | kNoTextStyle);
		}

		bool channelAvail=false;
		sprintf(temp,"  %s - %s",fx->getChannelName(), whModeNames[fx->getMode()]);
		if (direct>0)
		{
			if (fx->getChannelName()[0]!=0)
				cChannelMenu->addEntry("-");
			cChannelMenu->addEntry("-Gdirect");
			for (k=0; k<direct; k++)
			{	
				WHdebug("[Editor] ChannelList %d %s\n",k,portList[k]);
				if (strncmp(portList[k],temp, WHMAXPORTNAMESIZE-1)!=0)
				{
					cChannelMenu->addEntry(portList[k]);
					channelAvail=true;
				}
			}
		}
		if (k<count)
		{
			if (direct>0 || fx->getChannelName()[0]!=0)
				cChannelMenu->addEntry("-");
			cChannelMenu->addEntry("-Ginsert chain");
			for (; k<count; k++)
			{
				WHdebug("[Editor] ChannelList %d %s\n",k,portList[k]);
				if (strncmp(portList[k],temp, WHMAXPORTNAMESIZE-1)!=0)
				{
					cChannelMenu->addEntry(portList[k]);
					channelAvail=true;
				}	
			}
		}
	
		if (channelAvail==false)
		{
			if (fx->getChannelName()[0]!=0) cChannelMenu->addEntry("-");
			cChannelMenu->addEntry("-Gno incomplete channels");
		}
		
		if (fx->getChannelName()[0]!=0)
		{
			cChannelMenu->addEntry("-");
			cChannelMenu->addEntry("disable");
		}	
		
		updateModeButtons();
	}
	
	// Level leds
	
	if (cInputLed && cOutputLed)
	{
		float val=cInputLed->getValue();
		if (fx->inputVu > val)
			val=fx->inputVu;
		else
		{
			val-=0.05f;
			if (val<0) val=0.0f;
		}
		cInputLed->setValue(val);
		
		val=cOutputLed->getValue();
		if (fx->outputVu > val)
			val=fx->outputVu;
		else
		{
			val-=0.05f;
			if (val<0) val=0.0f;
		}
		cOutputLed->setValue(val);
	}
		
	// Status Display
	if (cStatusDisplay)
	{
		double theTime=get_time2();
		if (statusErrorTimeout>theTime)
		{
			if (strcmp(cStatusDisplay->string, statusError))
			{
				strcpy(cStatusDisplay->string, statusError);
				CColor mColor;
				mColor.red=0xD7; mColor.green=0x48; mColor.blue=0x48;
				cStatusDisplay->setFontColor(mColor);
				cStatusDisplay->setDirty();
			}
		}
		else if (statusMessageTimeout>theTime)
		{
			if (strcmp(cStatusDisplay->string, statusMessage))
			{
				strcpy(cStatusDisplay->string, statusMessage);
				CColor mColor;
				mColor.red=0x41; mColor.green=0x8A; mColor.blue=0x34;
				cStatusDisplay->setFontColor(mColor);
				cStatusDisplay->setDirty();
			}
		}
		else
		{
			if (strcmp(statusString,cStatusDisplay->string))
			{
				strcpy(cStatusDisplay->string, statusString);
				CColor mColor;
				mColor.red=0x74; mColor.green=0x74; mColor.blue=0x74;
				cStatusDisplay->setFontColor(mColor);
				cStatusDisplay->setDirty();
			}
		}
	}
}

void WormHoleEditor::updateModeButtons()
{
	if (!effect) return;
	WormHole2* fx=(WormHole2*)effect;
	if (!fx->globalInstance) return;
	WHStatic* global=(WHStatic*)fx->globalInstance;
	for (int i=0; i<WH_NUMMODES; i++) if (cMode[i]==NULL) return;
	// Mode Buttons

	WHremoteInstance* modeList[WH_NUMMODES];
	if (global->getModes(fx->getChannelName(),modeList))
	{
		// Set occupied modes
		int i;
		for (i=0; i<WH_NUMMODES; i++)
		{
			if (modeList[i]) cMode[i]->setValue(2.0f);
			else cMode[i]->setValue(0.0f);
		}
			
		// Disable unavailable modes
		if ((fx->getMode()==WH_START && modeList[WH_END]) || (fx->getMode()==WH_END && modeList[WH_START]))
		{
			cMode[WH_ORIGIN]		 ->setValue(3.0f);
			cMode[WH_DESTINATION]->setValue(3.0f);
			cMode[WH_LOOPBACK]	 ->setValue(3.0f);
		}
			
		if (fx->getMode()==WH_ORIGIN || fx->getMode()==WH_DESTINATION || fx->getMode()==WH_LOOPBACK)
		{
			int count=0;
			for (i=WH_ORIGIN; i<WH_NUMMODES; i++)
				if (modeList[i]) count++;
				
			if (count>1)
			{
				cMode[WH_START]			 ->setValue(3.0f);
				cMode[WH_END]				 ->setValue(3.0f);
			}
		}
	}
	else
	{
		for (int i=0; i<WH_NUMMODES; i++)
			cMode[i]->setValue(0.0f);
	}
	// Set this mode
	cMode[fx->getMode()]->setValue(1.0f);
}

//-----------------------------------------------------------------------------
void WormHoleEditor::setParameter (long index, float value)
{
	if (!frame) return;
	if (!effect) return; // Naja, wer weiss? Vielleicht ist ja der effect gar noch nicht da...?

	switch (index)
	{
	case kPlayThrough:	if (cPlayThrough)		cPlayThrough	->setValue(value); break;
	case kLatencyCorrection:	if (cLatencyCorrection)		cLatencyCorrection	->setValue(value); break;
	case kSync: if (cSync) cSync ->setValue(value); break;
	case kBufferSize:   
		if (cBufferSlider)	cBufferSlider	->setValue(value);
		if (cBufferDisplay)
		{
			char temp[20];
			bufferConvert(value,temp);
			cBufferDisplay->setText(temp);
		}
		break;
	case kDelay:   
		if (cDelaySlider)	cDelaySlider	->setValue(value);
		if (cDelayDisplay)
		{
			char temp[20];
			delayConvert(value,temp);
			cDelayDisplay->setText(temp);
		}
		break;
	}
	// call this to be sure that the graphic will be updated
	postUpdate ();
}

void WormHoleEditor::updateInterface()
{
	if (!effect) return;
	if (!cIFMenu) return;
	WormHole2* fx=(WormHole2*)effect;
	int count=0, activecount=0, activeID=0;
	WHinterface* theInterface;
	while ((theInterface = fx->globalInstance->getInterface(count)))
	{
		if (theInterface->active)
		{
			activeID=count;
			activecount++;
		}
		count++;
	}
	if (activecount==1)
		cIFMenu->setCurrent(activeID+1);
	else
		cIFMenu->setCurrent(0); // The to "All Interfaces"
}

//-----------------------------------------------------------------------------
void WormHoleEditor::valueChanged (CDrawContext* context, CControl* control)
{
	WormHole2* fx=(WormHole2*)effect;
	char temp[WHMAXPORTNAMESIZE];
	if (!fx) return;
	if (control)
	{
		if (control==cChannelName)
		{
			//control->update(context);
			cChannelName->getText(temp);
	/*		if (temp[0]!=0)
				cChannelName->setBackground(ChannelNameBitmapOn);
			else
				cChannelName->setBackground(ChannelNameBitmapOff);*/
			cChannelName->setDirty(); 
			fx->setChannelName(temp);
			return;
		}
		else if (control==cChannelMenu)	
		{
		//	cChannelMenu->checkEntryAlone(0); // Make sure Flag does not move
			control->update(context);
			if (cChannelMenu->getCurrent(temp, false)<0) return;
			
			if (strcmp(temp,"disable")==0)
			{
				fx->setChannelName("");
				return;
			}
			
			char* mode=strrchr(temp,'-'); // Check for '-'
			if (mode==NULL) return; // No valid entry
			
			*mode=0; // Trunc at '-'
			temp[strlen(temp)-1]=0; // Trunc one more ' '
			mode+=2; // Advance to mode name
			
			//cChannelName->setText(&temp[2]);
			fx->setChannelName(&temp[2]);
			// Set mode
			for (int i=0; i<WH_NUMMODES; i++)
				if (strcmp(mode,whModeNames[i])==0)
				{
					fx->setMode(i);
					break;
				}
			
			return;
		}
		else if (control==cIFMenu)
		{
			int entry=cIFMenu->getCurrent()-1;
			if (entry>=0)
				fx->globalInstance->setInterfaceID(fx->globalInstance->getInterface(entry)->id);
			else
				fx->globalInstance->setInterfaceID("undefined");
		}
		else if (control==cBufferKickLeft)
		{}
		else if (control==cBufferKickRight)
		{}
		else if (control==cDelayKickLeft)
		{}
		else if (control==cDelayKickRight)
		{}
		else if (control==cReset)
		{
			if (cReset->getValue()>0.f)
				fx->resetChannel();
		}
		else if (control==cBufferDisplay)
		{
			char temp[128];
			cBufferDisplay->getText(temp);
			int dummy;
			float val=-1.0f;
			if (sscanf(temp,"%d",&dummy)==1)
				val=bufferConvertValue(temp);
			if (val>1.0f) val=1.0f;
			if (val>=0.0f)
			{
				effect->setParameterAutomated(kBufferSize, val);
			}
			else
			{
				bufferConvert(effect->getParameter(kBufferSize),temp);
				cBufferDisplay->setText(temp);
				setStatusError("invalid buffer value",2.0);
			}
		}
		else if (control==cDelayDisplay)
		{
			char temp[128];
			cDelayDisplay->getText(temp);
			int dummy;
			float val=-1.0f;
			if (sscanf(temp,"%d",&dummy)==1)
				val=delayConvertValue(temp);

			if (val>1.0f) val=1.0f;
			if (val>=0.0f)
				effect->setParameterAutomated(kDelay, val);
			else
			{
				delayConvert(effect->getParameter(kDelay),temp);
				cDelayDisplay->setText(temp);
				setStatusError("invalid latency value",2.0);
			}
		}
		else
		{
			// mode buttons
			for (int i=0; i<WH_NUMMODES; i++)
				if (control==cMode[i])
				{
					char temp[100];
					strcpy(temp,whModeNames[i]);
					if (control->getValue() == 1.0f)
						fx->setMode(i);
					else if (control->getValue() == 2.0f)
					{
						strcat(temp,"-mode occupied by another instance");
						setStatusError(temp, 2.0);
					}
					else if (control->getValue() == 3.0f)
					{
						strcat(temp,"-mode not available on this channel");
						setStatusError(temp, 2.0);
					}
					control->update(context);
					return;
				}
		
			switch (control->getTag ())
			{
			case kPlayThrough: 
			case kLatencyCorrection:
			case kSync:
			case kBufferSize:
			case kDelay:
				effect->setParameterAutomated (control->getTag(), control->getValue ());
				break;
			}
		}
		control->update(context);
	}
}

void WormHoleEditor::setStatusSynced(int count)
{
	if (count==statusSynced) return;
	statusSynced=count;
	updateStatusString();
}

void WormHoleEditor::setStatusLatency(int count)
{
	if (count==statusLatency) return;
	statusLatency=count;
	updateStatusString();
}

void WormHoleEditor::setStatusSendAddr(long addr)
{
	if (addr==statusSendAddr) return;
	statusSendAddr=addr;
	updateStatusString();
}

void WormHoleEditor::setStatusRecvAddr(long addr)
{
	if (addr==statusRecvAddr) return;
	statusRecvAddr=addr;
	updateStatusString();
}

void WormHoleEditor::setStatusError(char* message, double duration)
{
	strncpy(statusError, message, 511);
  statusErrorTimeout=get_time2()+duration;
}

void WormHoleEditor::setStatusMessage(char* message, double duration)
{
	strncpy(statusMessage, message, 511);
  statusMessageTimeout=get_time2()+duration;
}

inline char* printIP(long addr)
{
	static char temp[32];
	unsigned int addr2=(unsigned int)addr;
	sprintf(temp,"%u.%u.%u.%u", addr2>>24, addr2>>16 & 0xff, addr2>>8 & 0xff, addr2 & 0xff);
	return temp;
}

void WormHoleEditor::updateStatusString()
{
	statusString[0]=0;
	char* string=statusString;
	
	switch (currentMode) {
	case WH_START:
	case WH_LOOPBACK:
		if (statusSendAddr)
			sprintf(string,"sending to %s", printIP(statusSendAddr));
		break;
	case WH_END:
	case WH_DESTINATION:
		if (statusRecvAddr)
			sprintf(string,"receiving from %s", printIP(statusRecvAddr));
		break;
	case WH_ORIGIN:
		if (statusSendAddr==statusRecvAddr && statusSendAddr)
			sprintf(string,"connected to %s", printIP(statusSendAddr));
		else
		{
			if (statusSendAddr)
			{
				sprintf(string,"send: %s", printIP(statusSendAddr));
				string=&string[strlen(string)];
			}
			if (statusRecvAddr)
				sprintf(string,"recv: %s", printIP(statusRecvAddr));
		}
		break;
	}
	if (strlen(statusString)==0)
		sprintf(string,"not connected");
	
	string=&string[strlen(string)];
	
	char* separator;
	if (string==statusString) separator="";
	else separator=" - ";
	
	switch(currentMode)
	{
	case WH_START:
		if (statusSynced==0)
			sprintf(string, "%sstart-instances synced",separator);
		break;
	case WH_END:
		if (statusSynced>0)
			sprintf(string, "%send-instances synced",separator);
		break;
	case WH_ORIGIN:
		if (statusLatency==0)
			sprintf(string, "%slatency measurement: waiting for loop",separator);
		else if (statusLatency==4)
			sprintf(string, "%slatency measured",separator);
		else if (statusLatency==5)
			sprintf(string, "%slatency ping failed",separator);
		break;
	}
}