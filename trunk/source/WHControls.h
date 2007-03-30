/*
 *  WHControls.h
 *  apulVSTSDK
 *
 *  Created by Adrian Pflugshaupt on 21.02.05.
 *  Copyright 2005 plasq LLC. All rights reserved.
 *
 */

#include "math.h" // FOR sqrt

#ifndef __vstcontrols__
#include "vstcontrols.h"
#endif

class WHexpSlider: public CHorizontalSlider
{
public:
	WHexpSlider(const CRect &rect, CControlListener *listener, long tag,
                          CPoint   &offsetHandle,    // handle offset
                          long     rangeHandle, // size of handle range
                          CBitmap  *handle,     // bitmap of slider
                          CBitmap  *background, // bitmap of background
                          CPoint   &offset,     // offset in the background
                          const long style);
	virtual ~WHexpSlider() {}
	virtual void mouse (CDrawContext *pContext, CPoint &where);
	virtual void draw (CDrawContext *pContext);
};

class WHchannelDisplay : public CTextEdit
{
public:
	WHchannelDisplay (const CRect &size, CControlListener *listener);
	virtual ~WHchannelDisplay () {}

	virtual void draw (CDrawContext*);
};

class WHselectableString: public CControl
{
public:
	WHselectableString(const CRect &size, CControlListener* listener);
	virtual ~WHselectableString();
	virtual void mouse (CDrawContext *pContext, CPoint &where);
	virtual void draw (CDrawContext *pContext);
	bool selected;
	char string[512];
};

class WHmodeButton : public CControl
{
public:
	WHmodeButton (const CRect &size, CControlListener *listener, CBitmap *background);
	virtual ~WHmodeButton ();

	virtual void draw (CDrawContext*);
	virtual void mouse (CDrawContext *pContext, CPoint &where);
};

class WHlevelLed : public CControl
{
public:
	WHlevelLed(const CRect &size, CBitmap *background, int _nrPixmaps);
	virtual ~WHlevelLed();
	virtual void draw (CDrawContext*);
private:
	int nrPixmaps;
};

// String Display

class WHStringDisplay : public CControl
{
public:
	WHStringDisplay (const CRect &size, CBitmap*);
	virtual ~WHStringDisplay ();
	
	virtual void setFontColor (CColor color);
	virtual void setBackColor (CColor color);
	virtual void setHoriAlign (CHoriTxtAlign hAlign);
	virtual void draw (CDrawContext *pContext);

	char*	string;
protected:
	CHoriTxtAlign horiTxtAlign;
	CColor  fontColor;
	CColor  backColor;
};

