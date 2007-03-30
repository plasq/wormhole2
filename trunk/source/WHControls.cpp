/*
 *  WHControls.cpp
 *  apulVSTSDK
 *
 *  Created by Adrian Pflugshaupt on 21.02.05.
 *  Copyright 2005 apulSoft. All rights reserved.
 *
 */


#include "WHControls.h"

WHexpSlider::WHexpSlider(const CRect &rect, CControlListener *listener, long tag, CPoint &offsetHandle, long rangeHandle, 
						CBitmap *handle, CBitmap  *background, CPoint &offset, const long style)
						:CHorizontalSlider(rect, listener, tag, offsetHandle, rangeHandle, handle, background, offset, style)
{}

void WHexpSlider::draw(CDrawContext* pContext)
{
	float fValue;
	if (style & kLeft || style & kTop)
		fValue = value;
	else 
		fValue = 1.f - value;
	
	fValue=sqrt((double)fValue); // exp fader!!

	// (re)draw background
	CRect rect (0, 0, widthControl, heightControl);
	if (pBackground)
	{
		if (bTransparencyEnabled)
			pBackground->drawTransparent (pOScreen, rect, offset);
		else
			pBackground->draw (pOScreen, rect, offset);
	}
	
	// calc new coords of slider
	CRect   rectNew;

	rectNew.top    = offsetHandle.v;
	rectNew.bottom = rectNew.top + heightOfSlider;	

	rectNew.left   = offsetHandle.h + (int)(fValue * rangeHandle);
	rectNew.left   = (rectNew.left < minTmp) ? minTmp : rectNew.left;

	rectNew.right  = rectNew.left + widthOfSlider;
	rectNew.right  = (rectNew.right > maxTmp) ? maxTmp : rectNew.right;

	// draw slider at new position
	if (pHandle)
	{
		if (bDrawTransparentEnabled)
			pHandle->drawTransparent (pOScreen, rectNew);
		else 
			pHandle->draw (pOScreen, rectNew);
	}

	pOScreen->copyFrom (pContext, size);
	
	setDirty (false);
}

//------------------------------------------------------------------------
void WHexpSlider::mouse (CDrawContext *pContext, CPoint &where)
{
	if (!bMouseEnabled)
		return;

	long button = pContext->getMouseButtons ();

	// set the default value
	if (button == (kControl|kLButton))
	{
		value = getDefaultValue ();
		if (isDirty () && listener)
			listener->valueChanged (pContext, this);
		return;
	}
	
	// allow left mousebutton only
	if (!(button & kLButton))
		return;

	long delta;
	if (style & kHorizontal)
		delta = size.left + offsetHandle.h;
	else
		delta = size.top + offsetHandle.v;
	if (!bFreeClick)
	{
		float fValue;
		if (style & kLeft || style & kTop)
			fValue = value;
		else 
			fValue = 1.f - value;
		long actualPos;
		CRect rect;

		actualPos = offsetHandle.h + (int)(fValue * rangeHandle) + size.left;

		rect.left   = actualPos;
		rect.top    = size.top  + offsetHandle.v;
		rect.right  = rect.left + widthOfSlider;
		rect.bottom = rect.top  + heightOfSlider;

		if (!where.isInside (rect))
			return;
		else
			delta += where.h - actualPos;
	} 
	else
	{
			delta += widthOfSlider / 2 - 1;
	}
	
	float oldVal    = value;
	long  oldButton = button;

	// begin of edit parameter
	getParent ()->beginEdit (tag);
	getParent ()->setEditView (this);

	while (1)
	{
		button = pContext->getMouseButtons ();
		if (!(button & kLButton))
			break;

		if ((oldButton != button) && (button & kShift))
		{
			oldVal    = value;
			oldButton = button;
		}
		else if (!(button & kShift))
			oldVal = value;

		if (style & kHorizontal)
			value = (float)(where.h - delta) / (float)rangeHandle;
		else
			value = (float)(where.v - delta) / (float)rangeHandle;
			
		if (style & kRight || style & kBottom)
			value = 1.f - value;

		if (value>=0)
			value*=value;

		if (button & kShift)
			value = oldVal + ((value - oldVal) / zoomFactor);
		bounceValue ();
    	
		if (isDirty () && listener)
			listener->valueChanged (pContext, this);

		pContext->getMouseLocation (where);

		doIdleStuff ();
	}

	//end of edit parameter
	getParent ()->endEdit (tag);
}

WHchannelDisplay::WHchannelDisplay(const CRect &size, CControlListener* listener) : CTextEdit(size, listener, 0, NULL)
{
	setFont(kNormalFont);
	CColor backColor;
	backColor.red=0xf5; backColor.green=0xf5; backColor.blue=0xf5;
	setBackColor(backColor);
	setFrameColor(backColor);
}

void WHchannelDisplay::draw(CDrawContext *pContext)
{
	CColor mColor;
	char temp[256];
	getText(temp);
	if (temp[0]==0)
	{
		setText("enter channel name or use chooser");
		mColor.red=0x99; mColor.blue=0x99; mColor.green=0x99;
		setFontColor(mColor);
		CTextEdit::draw(pContext);
		setText(temp);
		mColor.red=0x33; mColor.blue=0x33; mColor.green=0x33;
		setFontColor(mColor);
	}
	else
	{
		mColor.red=0x33; mColor.blue=0x33; mColor.green=0x33;
		setFontColor(mColor);
		CTextEdit::draw(pContext);
	}
}

WHselectableString::WHselectableString(const CRect &size, CControlListener* listener)
	:CControl (size, listener, 0, NULL)
{
	setMouseEnabled(true);
	selected=false;
}

WHselectableString::~WHselectableString()
{
}

void WHselectableString::mouse(CDrawContext *pContext, CPoint &where)
{
	if (!bMouseEnabled)
		return;
		
	if (selected)
		return;
	
	long button = pContext->getMouseButtons ();
	if (!(button & kLButton))
		return;

	if (listener)
		listener->valueChanged (pContext, this);	
}

void WHselectableString::draw (CDrawContext *pContext)
{
	CRect bg;
	bg(size.left-1, size.top-1, size.right, size.bottom);
	pContext->setFillColor (kGreyCColor);
	pContext->setFrameColor (kGreyCColor);
	pContext->drawRect(bg);
	pContext->fillRect (bg);

	if (selected)
	{
		pContext->setFont(kNormalFontVerySmall,0,kBoldFace);
		pContext->setFontColor (kWhiteCColor);
	}
	else
	{
		pContext->setFont(kNormalFontVerySmall,0,kNormalFace);
		pContext->setFontColor (kBlackCColor);
	}	
	pContext->drawString (string, size, true, kLeftText);

	setDirty (false);	
}

//------------------------------------------------------------------------
// WHmodeButton
//------------------------------------------------------------------------
WHmodeButton::WHmodeButton (const CRect &size, CControlListener *listener,
                            CBitmap *background)
:	CControl (size, listener, 0, background)
{
}

//------------------------------------------------------------------------
WHmodeButton::~WHmodeButton ()
{}

//------------------------------------------------------------------------
void WHmodeButton::draw (CDrawContext *pContext)
{
	long off;

	if (value && pBackground)
		off = int(value)*(pBackground->getHeight () / 4);
	else
		off = 0;

	if (pBackground)
	{
		if (bTransparencyEnabled)
			pBackground->drawTransparent (pContext, size, CPoint (0, off));
		else
			pBackground->draw (pContext, size, CPoint (0, off));
	}
	setDirty (false);
}

//------------------------------------------------------------------------
void WHmodeButton::mouse (CDrawContext *pContext, CPoint &where)
{
	if (!bMouseEnabled)
		return;

 	long button = pContext->getMouseButtons ();
	if (!(button & kLButton))
		return;

	if (value==0.0f)
	{
		//value = ((long)value) ? 0.f : 1.f;
		value=1.0f;
		
		doIdleStuff ();
		if (listener)
			listener->valueChanged (pContext, this);
	}
	else
	{
		doIdleStuff();
		if (value==2.0f || value==3.0f)
			if (listener) listener->valueChanged (pContext, this);
	}
}

//------------------------------------------------------------------------
// WHlevelLed
//------------------------------------------------------------------------
WHlevelLed::WHlevelLed (const CRect &size, CBitmap *background, int _nrPixmaps)
:	CControl (size, listener, 0, background)
{
	nrPixmaps=_nrPixmaps;
}

//------------------------------------------------------------------------
WHlevelLed::~WHlevelLed ()
{}

//------------------------------------------------------------------------
void WHlevelLed::draw (CDrawContext *pContext)
{
	if (!pBackground) return;

	bounceValue();

	int off=int(sqrt((double)value)*(float)nrPixmaps-0.01f)*size.height();

	if (bTransparencyEnabled)
		pBackground->drawTransparent (pContext, size, CPoint (0, off));
	else
		pBackground->draw (pContext, size, CPoint (0, off));
	setDirty (false);
}

// STRING display

WHStringDisplay::WHStringDisplay (const CRect &size, CBitmap* background)
:	CControl (size, 0, 0, background), 
	horiTxtAlign (kRightText)
{
	backOffset (0, 0);
	
	backColor.red=backColor.green=backColor.blue=0xcc;
	fontColor.red=fontColor.green=fontColor.blue=0x33;

	string=new char[512];
	string[0]=0;
}

WHStringDisplay::~WHStringDisplay ()
{
	delete(string);
}

//------------------------------------------------------------------------
void WHStringDisplay::draw (CDrawContext *pContext)
{

	if (pBackground)
	{
		if (bTransparencyEnabled)
			pBackground->drawTransparent (pContext, size, CPoint (0, 0));
		else
			pBackground->draw (pContext, size, CPoint (0, 0));
	}
	else
	{
		CRect bg;
		bg(size.left-1, size.top-1, size.right, size.bottom);
		pContext->setFillColor (backColor);
		pContext->setFrameColor (backColor);
		pContext->drawRect(bg);
		pContext->fillRect (bg);
	}
	pContext->setFont(kNormalFontVerySmall,0,kNormalFace);
	pContext->setFontColor (fontColor);
	pContext->drawString (string, size, false, horiTxtAlign);
	setDirty (false);
}

//------------------------------------------------------------------------
void WHStringDisplay::setFontColor (CColor color)
{
	// to force the redraw
	if (fontColor != color)
		setDirty ();
	fontColor = color;
}

//------------------------------------------------------------------------
void WHStringDisplay::setBackColor (CColor color)
{
	// to force the redraw
	if (backColor != color)
		setDirty ();
	backColor = color;
}

//------------------------------------------------------------------------
void WHStringDisplay::setHoriAlign (CHoriTxtAlign hAlign)
{
	// to force the redraw
	if (horiTxtAlign != hAlign)
		setDirty ();
	horiTxtAlign = hAlign;
}


