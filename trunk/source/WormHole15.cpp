//-------------------------------------------------------------------------------------------------------
// VST Plug-Ins SDK
//-
//WormHole2 (VST 2.0)
//-
// © 2003, Steinberg Media Technologies, All Rights Reserved
//-------------------------------------------------------------------------------------------------------


#if MAC
#include <Carbon/Carbon.h>
#elif WINDOWS
#include "windows.h"
#endif

#ifndef __wormholeeditor__
#include "WormHole15Editor.h"
#endif

#ifndef __WormHole__
#include "WormHole15.hpp"
#endif

const char* whModeNames[] = {"start","end","insert","before","after"}; 

long WormHole2::instanceCounter=0;
WHStatic* WormHole2::globalInstance=NULL;

void __stdcall globalCallback(int what, void* param)
{
	WormHole2* base=(WormHole2*)param;

	if (base==NULL) return;

	switch(what)
	{
	case WH_CALLBACK_UPDATELIST:
		base->updateList();
		break;
	case WH_CALLBACK_RESET:
		base->displayMessage("the flush button was pressed on a remote instance");
		base->setChannelName("");
		break;
	case WH_CALLBACK_SETBUFFERSIZE:
		base->displayMessage("the buffer value was changed by a synced instance");
		base->setParameter(kBufferSize, base->localInstance->param2);
		break;
	case WH_CALLBACK_SLAVEMESSAGE:
		base->displayMessage("connected start instance not synced -> sync impossible");
		break;
	case WH_CALLBACK_RESETLATENCY:
		base->latencyMeasureMode=0;
		break;
	case WH_CALLBACK_DROPOUT:
		base->displayError("the connection has timed out");
		break;
	case WH_CALLBACK_TEST:
		WHdebug("[WormHole] Test Callback called");
		break;
	case WH_CALLBACK_SETDELAY:
		base->displayMessage("the latency value was changed by a synced instance");
		base->setParameter(kDelay, base->localInstance->param2);
		break;
	case WH_CALLBACK_NOTRUNNING:
		base->displayError("the host is not feeding audio to Wormhole2");
		base->receiverResume();
		break;
	case WH_CALLBACK_FETCHINTERFACE:
		base->fetchInterface();
		break;
	}
}

//-----------------------------------------------------------------------------
#ifdef MACAU
COMPONENT_ENTRY(WormHole2)
#endif
WormHole2::WormHole2 (audioMasterCallback audioMaster)	: AudioEffectX (audioMaster, 1, kNumParams) // 0 Presets
{
	instanceCounter++;
	WHdebug("[WormHole2] %d Instances created\n",(int)instanceCounter);
	if (!globalInstance)
	{
	#if WINDOWS
		WORD sockVersion;
		WSADATA wsaData;
		sockVersion=MAKEWORD(1,1);
		WSAStartup(sockVersion,&wsaData);
		WHdebug("[WormHole2] WinSock initialized");
	#endif
		globalInstance=new WHStatic(globalCallback);		
		WHdebug("[WormHole2] Global instance created");
	}

	// Setup playthrough&delay buffers

	playThroughBufferSize=2048;
	playThroughBuffers[0]=(float*)malloc(playThroughBufferSize*sizeof(float));
	playThroughBuffers[1]=(float*)malloc(playThroughBufferSize*sizeof(float));
	delayPosition=0;
	delayBuffers[0]=(float*)malloc((WHMAXDELAY+1)*sizeof(float));
	delayBuffers[1]=(float*)malloc((WHMAXDELAY+1)*sizeof(float));
	latencyDelayPosition=0;
	latencyBuffers[0]=(float*)malloc((WHMAXDELAY+1)*sizeof(float));
	latencyBuffers[1]=(float*)malloc((WHMAXDELAY+1)*sizeof(float));
	
	// Set up vst stuff
	
	editor=0;
	setNumInputs (2);
	setNumOutputs (2);
	canMono(true);
	hasVu (false);
	noTail(false);
	canProcessReplacing();
	programsAreChunks(true); // Programme als Chunks

	// Set up internal parameters / fetch from global Instance

	memcpy(iParameters, globalInstance->lastParameters, sizeof(iParameters));
	if (iParameters[kDelay]>0.0f)
		setInitialDelay(WHMAXDELAY);
	else
		setInitialDelay(0);

	// Allocating Sender& Receiver
	
	mySocket = new WHsocket(); // After WSAstartup!!!
	sender = new WHsender(mySocket);
	receiver = new WHreceiver(mySocket);

	// fetching last values from global instance


	char* lastChannelName=globalInstance->lastChannelName;
	channelName[0]=0;
	int i;
	int numberOfDigits=0;
	for (i=strlen(lastChannelName)-1; i>=0; i--)
	{
		if (!isdigit(lastChannelName[i])) break;
		numberOfDigits++;
	}
	if (numberOfDigits>0)
	{
		int j;
		for (j=i; j>=0; j--)
			if (lastChannelName[j]=='-' || lastChannelName[j]!=' ') break;
		if (lastChannelName[j]=='-')
		{
			int k;
			for (k=0; k<=i; k++)
				channelName[k]=lastChannelName[k];
			char format[10];
			sprintf(format,"%%0%dd",numberOfDigits);
			WHdebug("[WormHole2] lastChannelName Numberformat %s",format);
			sprintf(&channelName[k],format,atoi(&lastChannelName[i+1])+1);
			displayMessage("tha channel name was automatically generated");
		}
	}
	strncpy(globalInstance->lastChannelName,channelName,WHMAXPORTNAMESIZE-1);

	mode=globalInstance->lastMode;
	
	// more parameters here.... maybe a bit redundant, but I'm lazy now
	
	masterTick=0;
//	senderExists=false;
	measuredLatency=0;
	latencyMeasureMode=0;
	silenceCounter=0;
	originCounter=0;
	theLatency=0;
	lastPosition=-1.0;
	lastSamplePos=-1.0;
	diffsum=0.0;
//	synced=false;
	inputVu=outputVu=0.0f;
	peerLinked=false;
	remoteSender=NULL;
	remoteReceiver=NULL;
	sync=true;
	
	ID = (long)WHprefsGetInt("InstanceNumber");
	ID++;
	WHprefsSetInt("InstanceNumber",(int)ID);

	setUniqueID ('Wh2d');
				
	// Editor starten

#ifndef MACAU
	editor = new WormHoleEditor (this);
#endif

	sender->setSampleRate(getSampleRate());
	WHdebug("[WormHole2] Audio port: %d\n",receiver->getPort());

	// Passing information to sub-objects

	localInstance = globalInstance->initAnnouncement(ID, channelName, 0, receiver->getPort(), iParameters[kBufferSize], (void*)this);
	
	setChannelName(channelName);
	
	resume ();		// flush buffers

}

//------------------------------------------------------------------------
WormHole2::~WormHole2 ()
{
	WHdebug("[WormHole2] Destruktor!");

	globalInstance->exitAnnouncement(localInstance);

	delete sender;
	delete receiver;
	delete mySocket;
	
	free(playThroughBuffers[0]);
	free(playThroughBuffers[1]);
	free(delayBuffers[0]);
	free(delayBuffers[1]);
	free(latencyBuffers[0]);
	free(latencyBuffers[1]);

	instanceCounter--;
	if (instanceCounter==0)
	{
		WHdebug("[WormHole2] last instance destroyed. Deallocating global instance\n");
		delete globalInstance;
		globalInstance=NULL;
	 #if WINDOWS
		WSACleanup();
	 #endif
	}
}

void WormHole2::resume()
{
	WHdebug("[WormHole2] resume");
	
	memset(delayBuffers[0],0,WHMAXDELAY*sizeof(float)); // Clean delay buffers
	memset(delayBuffers[1],0,WHMAXDELAY*sizeof(float)); // Clean delay buffers
	memset(latencyBuffers[0],0,WHMAXDELAY*sizeof(float)); // Clean delay buffers
	memset(latencyBuffers[1],0,WHMAXDELAY*sizeof(float)); // Clean delay buffers

//	senderExists=false;
	measuredLatency=0;
	latencyMeasureMode=0;
	silenceCounter=0;
	originCounter=0;
	theLatency=0;
	lastPosition=-1.0;
	lastSamplePos=-1.0;
	inputVu=outputVu=0.0f;
	listChangeCounter=0;
	peerLinked=false; // Is the connected start instance linked right now?
	remoteSender=NULL;
	remoteReceiver=NULL;
	nextTick=0;

	sender->setTarget(0,0);

	localInstance->flags &= (~(WHFLAG_SENDERACTIVE));
	localInstance->remoteAddr=0;
	localInstance->remoteGlobalPort=0;
	
	receiverResume();

	if (editor) 
	{	
		((WormHoleEditor*)editor)->resume ();
		((WormHoleEditor*)editor)->setStatusSendAddr(sender->getDestAddr());
	}
	updateList();
}

void WormHole2::receiverResume()
{
	localInstance->flags &= (~(WHFLAG_RECEIVERCONNECTED));
	//localInstance->mode = WH_DESTROY; // Destroy packets!
	receiver->resume(localInstance); 
	updateList();
}

long WormHole2::getChunk(void** data, bool isPreset)
{
	WHdebug("[WormHole2] getChunk\n");
	strncpy(chunkData.channelName, channelName, WHMAXPORTNAMESIZE-1);
	chunkData.mode=mode;
	strncpy(chunkData.interfaceID, globalInstance->getInterfaceID(), WH_MAXIFIDSIZE-1);

	memcpy(chunkData.iParameters,iParameters,sizeof(float)*kNumParams);
	#if MAC && __i386__
	chunkData.endianSwap();
	#endif
	*data=&chunkData;
	return sizeof(chunkData);
}

long WormHole2::setChunk(void* data, long byteSize, bool isPreset)
{
	WHdebug("[WormHole2] setChunk\n");
	if (byteSize!=sizeof(chunkData))
		return 0;

	WHchunk newParams=*((WHchunk*)data);
	#if MAC && __i386__
	newParams.endianSwap();
	#endif	
	
	memcpy(iParameters,newParams.iParameters,sizeof(float)*kNumParams);
	setChannelName(newParams.channelName);
	setMode(newParams.mode);
	globalInstance->setInterfaceID(newParams.interfaceID); // Choosing the interface
	return 1;
} 


//------------------------------------------------------------------------
void WormHole2::setParameter(long index, float value)
{
	if (index<kNumParams) 
	{
		switch(index)
		{
		case kBufferSize: 
			localInstance->bufferSize = value;
			// Adjust Latency in auto mode
			if (mode==WH_ORIGIN && iParameters[kLatencyCorrection]>=0.5f)
			{
				int oldSize=int(iParameters[kBufferSize]*WHMAXBUFFER);
				int newSize=int(value*WHMAXBUFFER);
				int change=newSize-oldSize;
				int delay=int(iParameters[kDelay]*WHMAXDELAY);
				delay += change;
				if (delay<0) delay=0;
				if (delay>WHMAXDELAY) delay=WHMAXDELAY;
				setParameter(kDelay, float(delay)/float(WHMAXDELAY));
			}
			else if (mode==WH_END && (localInstance->flags & WHFLAG_SYNCED))
			{
				if (globalInstance->updateingParams==false)
					globalInstance->updateLinkedBufferSize(localInstance, value);
			}
			break;
		case kDelay:
			if (mode==WH_START && (localInstance->flags & WHFLAG_SYNCED))
			{
				if (globalInstance->updateingParams==false)
					globalInstance->updateLinkedDelay(localInstance, value);
			}
			break;
		}

		iParameters[index]=value;
		globalInstance->lastParameters[index]=value;


		#ifdef MACAU
		AUBase::SetParameter((UInt32)index,kAudioUnitScope_Global,0,(Float32)value,(UInt32)0);
		#endif

	  if (editor) 
	  {
			((AEffGUIEditor*)editor)->setParameter(index, value);
			editor->postUpdate();
	  }
	}
}

//------------------------------------------------------------------------
float WormHole2::getParameter (long index)
{
	if (index<kNumParams)
	  return iParameters[index];
	else
	  return 0;
}

//------------------------------------------------------------------------
void WormHole2::getParameterName (long index, char *label)
{
	switch (index)
	{
	case kSync: strcpy(label,"Sync To Host"); break;
	case kDelay: strcpy(label,"Delay"); break;
	case kBufferSize: strcpy(label,"Buffer Size"); break;
	case kLatencyCorrection: strcpy(label,"Audio Ping"); break;
	case kPlayThrough: strcpy(label,"Play Through"); break;
	default: strcpy(label,"reserved"); break;
	}
}

//------------------------------------------------------------------------
void WormHole2::getParameterDisplay (long index, char *text)
{
	switch (index)
	{
		case kPlayThrough:
		case kLatencyCorrection:
		case kSync:
		  if (iParameters[index]>=0.5)
				strcpy(text,"ON"); 
		  else
				strcpy(text,"OFF");
		  break;
		case kDelay:
			sprintf(text, "%d",int(iParameters[index]*WHMAXDELAY));
			break;
		case kBufferSize:
			sprintf(text, "%d",int(iParameters[index]*WHMAXBUFFER));
			break;
		break;
	}
}

//------------------------------------------------------------------------
void WormHole2::getParameterLabel (long index, char *label)
{
	strcpy(label,"");
}

//------------------------------------------------------------------------
bool WormHole2::getEffectName (char* name)
{
	strcpy (name, "Wormhole2");
	return true;
}

//------------------------------------------------------------------------
bool WormHole2::getProductString (char* text)
{
  return getEffectName(text);
}

//------------------------------------------------------------------------
bool WormHole2::getVendorString (char* text)
{
	strcpy (text, "plasq.com");
	return true;
}

void WormHole2::displayError(char* string)
{
	if (editor && ((AEffGUIEditor*)editor)->getFrame())
	{
		((WormHoleEditor*)editor)->setStatusError(string, 2.0);
	}
	else
	{
		WHdebug("[WormHole2] %s",string);
	}
}

void WormHole2::displayMessage(char* string)
{
	if (editor && ((AEffGUIEditor*)editor)->getFrame())
	{
		((WormHoleEditor*)editor)->setStatusMessage(string, 1.5);
	}
	else
	{
		WHdebug("[WormHole2] %s",string);
	}
}

void WormHole2::setChannelName(char* name)
{
	WHdebug("[WormHole2] Setting channel name to %s",name);
	strncpy(channelName,name,WHMAXPORTNAMESIZE-1);
	
	unsigned short oldmode=mode;
	
	// Check for other instances on that channel
	WHremoteInstance* modeList[WH_NUMMODES];
	if (name[0]!=0 && globalInstance->getModes(channelName,modeList))
//	unsigned short amode=globalInstance->getAMode(channelName);
//	if (amode!=WH_INVALIDMODE)
	{
		if ((modeList[WH_START] || modeList[WH_END]) && !WHisDirectMode(mode))
			mode=WH_START;
		else if ((!modeList[WH_START] && !modeList[WH_END]) && WHisDirectMode(mode))
			mode=WH_ORIGIN;
		
		// Get rid of THIS instance
		for (int i=0; i<WH_NUMMODES; i++)
			if (modeList[i])
				if (modeList[i]->ID==ID && modeList[i]->addr==INADDR_LOOPBACK)
					modeList[i]=NULL;
		
		if (modeList[mode])
		{		
			WHdebug("[WormHole2] Channel & Mode already taken by another instance");
		
			bool completeError=false;
		
			switch(mode) {
			case WH_START:
				if (modeList[WH_END])
					completeError=true;
				else
					mode=WH_END;
				break;
			case WH_END:
				if (modeList[WH_START])
					completeError=true;
				else
					mode=WH_START;
				break;
			case WH_ORIGIN:
				if (modeList[WH_DESTINATION])
					if (modeList[WH_LOOPBACK])
						completeError=true;
					else
						mode=WH_LOOPBACK;
				else
					mode=WH_DESTINATION;
				break;
			case WH_DESTINATION:
				if (modeList[WH_LOOPBACK])
					if (modeList[WH_ORIGIN])
						completeError=true;
					else
						mode=WH_ORIGIN;
				else
					mode=WH_LOOPBACK;
				break;
			case WH_LOOPBACK:
				if (modeList[WH_ORIGIN])
					if (modeList[WH_DESTINATION])
						completeError=true;
					else
						mode=WH_DESTINATION;
				else
					mode=WH_ORIGIN;
				break;
			}
			if (completeError)
			{
				displayError("channel name cannot be used");
				setChannelName("");
			}
		}
	}
		
	// If the name changed from something to nothing, send destroy message
	if (channelName[0]==0 && localInstance->channelName[0]!=0)
	{
		localInstance->flags |= WHFLAG_DESTROY;
		if (editor && ((AEffGUIEditor*)editor)->getFrame())
		{
			// Make Editor immediately display "not connected" -> It's a workaround
			((WormHoleEditor*)editor)->setStatusSendAddr(0);		
			((WormHoleEditor*)editor)->setStatusRecvAddr(0);		
		}
	}
	// Save to local instance
	strncpy(localInstance->channelName, channelName, WHMAXPORTNAMESIZE-1);
	// save to global for next opened plugin
	strncpy(globalInstance->lastChannelName, channelName, WHMAXPORTNAMESIZE-1);

	resume(); // Flush all buffers
	if (mode!=oldmode)
		setMode(mode);
}

void WormHole2::setMode(unsigned short newmode)
{	
	mode=newmode;
	
	remoteSender=remoteReceiver=NULL;
		
	switch(mode) {
	case WH_START:
		WHdebug("[WormHole2] Mode set to start");
		remoteSender=NULL;
		remoteReceiver=globalInstance->getRemoteInstance(channelName, WH_END, ID);
		if (remoteReceiver) sender->setTarget(remoteReceiver->addr, remoteReceiver->port);
		else sender->setTarget(0,0);
		if (remoteReceiver)
		{
			localInstance->remoteAddr=remoteReceiver->addr;
			localInstance->remoteGlobalPort=remoteReceiver->globalPort;
		}
		else
		{
			localInstance->remoteAddr=0;
			localInstance->remoteGlobalPort=0;
		}
		break;
		
	case WH_END:
		WHdebug("[WormHole2] Mode set to end");
		sender->setTarget(0,0);
		//if (synced=false) break;
		// Store info about remote global instance
		remoteSender=globalInstance->getRemoteInstance(channelName, WH_START, ID);
		remoteReceiver=NULL;
		// Update connection if remote instance changes link state.
			
		if (remoteSender)
		{
			if (((remoteSender->flags & WHFLAG_SYNCED)!=0) != peerLinked)
				receiver->resume(localInstance);
			peerLinked = ((remoteSender->flags & WHFLAG_SYNCED)!=0);
			
			localInstance->remoteAddr=remoteSender->addr;
			localInstance->remoteGlobalPort=remoteSender->globalPort;
		}
		else
		{
			peerLinked=false;
			
			localInstance->remoteAddr=0;
			localInstance->remoteGlobalPort=0;
		}
		break;
		
	case WH_ORIGIN:
		WHdebug("[WormHole2] Mode set to origin.");
		remoteSender = globalInstance->getRemoteInstance(channelName, WH_LOOPBACK, ID);
		remoteReceiver=globalInstance->getRemoteInstance(channelName, WH_DESTINATION, ID);
//		globalInstance->getAddr(channelName, WH_DESTINATION, ID, &addr, &port);
		if (remoteReceiver) sender->setTarget(remoteReceiver->addr, remoteReceiver->port);
		else sender->setTarget(0, 0);
		
		// Check for loopback sender
		break;
		
	case WH_DESTINATION:
		sender->setTarget(0,0);
		remoteSender = globalInstance->getRemoteInstance(channelName, WH_ORIGIN, ID);
		remoteReceiver=NULL;
		WHdebug("[WormHole2] Mode set to destination.");
		break;
		
	case WH_LOOPBACK:
		WHdebug("[WormHole2] Mode set to loopback.");
		remoteSender=NULL;
		remoteReceiver=globalInstance->getRemoteInstance(channelName, WH_ORIGIN, ID);
		//globalInstance->getAddr(channelName,WH_ORIGIN, ID, &addr,&port);
		if (remoteReceiver) sender->setTarget(remoteReceiver->addr,remoteReceiver->port);
		else sender->setTarget(0,0);
		if (remoteReceiver) WHdebug("[WormHole2] Loopback sending to %08x",remoteReceiver->addr);
		break;
	}
	
	localInstance->mode=newmode;
	globalInstance->lastMode=newmode;

	WHdebug("Setmode sender %u  recevier %u mode %s channel %s",(long)remoteSender, (long)remoteReceiver, whModeNames[mode],channelName);

	if (editor)
	{
		((WormHoleEditor*)editor)->setStatusSendAddr(sender->getDestAddr());
		if (((AEffGUIEditor*)editor)->getFrame()) 
			((WormHoleEditor*)editor)->setMode(newmode);
	}
}

void WormHole2::resetChannel()
{
	WHdebug("[WormHole2] Reseting channel");
	// Send reset message
	localInstance->flags |= WHFLAG_RESET;
	// Make other instances on this channel time out
	WHremoteInstance* modeList[WH_NUMMODES];
	if (globalInstance->getModes(channelName,modeList))
	{
		for (int j=0; j<WH_NUMMODES; j++)
		{
			if (modeList[j] && j!=mode)
			{
				WHdebug("Reseting a remote instance");
				modeList[j]->lastseen=-WHOPENPORTTIMEOUT;
			}
		}
	}	
}

inline float WormHole2::getMaxVu(float** &buffers, int &channels, long &frames)
{
	float max=0.0f;
	for (int j=0; j<channels; j++)
		for (int i=0; i<frames; i++)
		{
			float val=fabs(buffers[j][i]);
			if (val>max)
				max=val;
		}
	return max;
}

void WormHole2::procSync(long iFrames)
{
	VstTimeInfo* vstTimeInfo = getTimeInfo(0); //kVstNanosValid | kVstPpqPosValid); // | kVstPpqPosValid);
	
	double mSamplePos=-1.0;
	bool jump=false;
	if (vstTimeInfo && vstTimeInfo->samplePos!=lastSamplePos)
	{
		mSamplePos = vstTimeInfo->samplePos;
		if (fabs(((lastSamplePos + (double)iFrames) - mSamplePos)) > 2.0)
			jump=true;
		lastSamplePos = mSamplePos;
	}
	
	if (WHisDirectMode(mode)) 
	{	
		sync = (iParameters[kSync]>=0.5f);

		if (mSamplePos>=0.0)
	  {
		//	lastSamplePos=vstTimeInfo->samplePos;
			if (sync)
			{
				tickDelta=0;
				masterTick = long(mSamplePos);
										
			  // Start sync mode
			  if ((localInstance->flags & WHFLAG_SYNCED)==0)
			  {
				  if (globalInstance->synced==false)
				  {
					  globalInstance->invalidateTickOffsets();
					  WHdebug("[WormHole2] Tickoffset local list reset");
				  }
				  receiver->resume(localInstance);
			  }
			  globalInstance->synced=true;
			  localInstance->flags |= WHFLAG_SYNCED;
			  // Update editor
			  if (editor && ((AEffGUIEditor*)editor)->getFrame())
			  {
				  if (mode==WH_END)
					  ((WormHoleEditor*)editor)->setStatusSynced(receiver->syncedCount);
				  else
				  	((WormHoleEditor*)editor)->setStatusSynced(0);
			  }
		  }
		}
	  else
			sync=false;
			
	  if (sync==false)
		{
		  tickDelta=iFrames;
		  localInstance->flags &= (~WHFLAG_SYNCED);
		  globalInstance->synced=false;
		  if (editor && ((AEffGUIEditor*)editor)->getFrame())
		  	((WormHoleEditor*)editor)->setStatusSynced(-1);
	  }
	}
	else
	{
		// insert loop mode
		tickDelta=iFrames;
	}
	
	// Testing masterTick & resume if necessary
	
//	int diffTick = masterTick - nextTick;
//	if (diffTick<0) diffTick=-diffTick;
	// Live 4 makes errors of 1 sample!
	//if (diffTick>1 && nextTick!=0 && mSamplePos>=0.0 && ((localInstance->flags & (WHFLAG_SYNCED))!=0 || WHisDirectMode(mode)==false))
	if (jump && vstTimeInfo && ((localInstance->flags & (WHFLAG_SYNCED))!=0 || WHisDirectMode(mode)==false))
	{
	//	printf("tick jumped!\n");
	
		// Quick resume
	
		receiverResume(); // Will send destroy message -> works for sender as well ... somehow ?????
		nextTick=0;
	}
	else
		nextTick = masterTick + iFrames;
}

void WormHole2::updateList()
{
	// check for connection changes
	if (listChangeCounter!=globalInstance->getChangeCounter())
	{
		WHdebug("[WormHole2] Updateing Instance List");
					
		setMode(mode);
		listChangeCounter=globalInstance->getChangeCounter();
	}	
}

void WormHole2::fetchInterface()
{
	// The interface ID has changed, tell the editor
	if (editor)
	{
		((WormHoleEditor*)editor)->updateInterface();
	}
}

void WormHole2::connectionCheck()
{
	long newLatency=int(iParameters[kDelay]*WHMAXDELAY);
	if (mode==WH_END || mode==WH_LOOPBACK)
		newLatency=0;
	if (theLatency!=newLatency)
	{
		if (newLatency==0)
		{
			setInitialDelay(0);
			ioChanged();
		}
		else if (theLatency==0)
		{
			setInitialDelay(WHMAXDELAY);
			ioChanged();
		}
		theLatency=int(iParameters[kDelay]*WHMAXDELAY);
	}	

	// Update Flags

	if (sender->isActive()) localInstance->flags |= WHFLAG_SENDERACTIVE;
	else	localInstance->flags &= (~WHFLAG_SENDERACTIVE);
	
	if (receiver->isConnected()) localInstance->flags |= WHFLAG_RECEIVERCONNECTED;
	else localInstance->flags &= (~WHFLAG_RECEIVERCONNECTED);
}

//---------------------------------------------------------------------------
void WormHole2::process (float** inputs, float** outputs, long sampleFrames)
{
	processReplacing(inputs, outputs, sampleFrames);
}

void WormHole2::processReplacing (float** inputs, float** outputs, long iFrames)
{
	// Sync instances if possible

	procSync(iFrames);
	
	// Check for latency changes
	connectionCheck();
		
	// Detect mono/stereo
	bool stereo=isInputConnected(1) && (inputs[1]!=NULL) && (inputs[0]!=inputs[1] || outputs[0]!=outputs[1]);
	int channels=(stereo)?2:1;
	
	// Prepare Playthrough by buffering
	
	float* buffers[2];
	if (iParameters[kPlayThrough]>=0.5f)
	{
		// A Copy is necessary
		if (iFrames>playThroughBufferSize)
		{
			playThroughBufferSize=iFrames;
			playThroughBuffers[0]=(float*)realloc(playThroughBuffers[0],iFrames*sizeof(float));
			playThroughBuffers[1]=(float*)realloc(playThroughBuffers[1],iFrames*sizeof(float));
		}
		memcpy(playThroughBuffers[0],inputs[0],iFrames*sizeof(float));
		memcpy(playThroughBuffers[1],inputs[1],iFrames*sizeof(float));
		buffers[0]=playThroughBuffers[0];
		buffers[1]=playThroughBuffers[1];
	}

	// Delay input for flexible latency
	
	if (theLatency!=0 && (mode==WH_ORIGIN || mode==WH_START))
	{
		long delayCount=WHMAXDELAY-theLatency;
		long i;
		if (stereo)
			for (i=0; i<iFrames; i++)
			{
				latencyBuffers[0][latencyDelayPosition]=inputs[0][i];
				latencyBuffers[1][latencyDelayPosition]=inputs[1][i];
				int rPos=(latencyDelayPosition+delayCount) % (WHMAXDELAY+1);
				inputs[0][i]=latencyBuffers[0][rPos];
				inputs[1][i]=latencyBuffers[1][rPos];
				if (--latencyDelayPosition==-1) latencyDelayPosition=WHMAXDELAY;
			}
		else
			for (i=0; i<iFrames; i++)
			{
 				latencyBuffers[0][latencyDelayPosition]=inputs[0][i];
				inputs[0][i]=latencyBuffers[0][(latencyDelayPosition+delayCount) % (WHMAXDELAY+1)];
				if (--latencyDelayPosition==-1) latencyDelayPosition=WHMAXDELAY;
			}

	}

	// input VU, processed?
	
	localInstance->flags &= (~WHFLAG_NOTPROCESSED);
	if (editor && ((AEffGUIEditor*)editor)->getFrame()) // only if editor is open!!!
	{
		inputVu=getMaxVu(inputs, channels, iFrames);
	}
	
	// Latency measurement (1)
	
	if (iParameters[kLatencyCorrection]>=0.5f && mode==WH_ORIGIN)
	{
		// Tell the status bar
		if (editor && ((AEffGUIEditor*)editor)->getFrame())
			((WormHoleEditor*)editor)->setStatusLatency(latencyMeasureMode);
		// Only send silence until measurement is complete
		if (latencyMeasureMode<4 && latencyMeasureMode!=2)
		{
			memset(inputs[0],0,iFrames*sizeof(float));
			if (stereo) memset(inputs[1],0,iFrames*sizeof(float));
		}
//		WHdebug("%d %d",globalInstance->isReceiverConnected(channelName,WH_DESTINATION), globalInstance->isSenderActive(channelName,WH_LOOPBACK));
		//if (globalInstance->isLoopComplete(channelName))
		if (sender->isActive() && receiver->isConnected() && remoteSender && remoteReceiver && ((remoteSender->flags & WHFLAG_SENDERACTIVE) != 0) && ((remoteReceiver->flags & WHFLAG_RECEIVERCONNECTED) != 0))
	//	if (globalInstance->isReceiverConnected(channelName,WH_DESTINATION) && globalInstance->isSenderActive(channelName,WH_LOOPBACK))
		{
			originCounter += iFrames;
		}
		else
		{
			originCounter = 0;
			latencyMeasureMode = 0;
			silenceCounter = 0;
		}
			
		if (originCounter>WH_STARTLATENCYMEASURETIMEOUT)
		{
			switch (latencyMeasureMode) {
			case 0: // Off
				displayMessage("latency measurement: initializing");
				measuredLatency=0;
				latencyMeasureMode=1; 
				silenceCounter=0;
				//lastDestinationBufferSize=globalInstance->getBufferSize(channelName,WH_DESTINATION);
				lastDestinationBufferSize=remoteReceiver->bufferSize;

			case 1: // Wait for silent packet
				displayMessage("latency measurement: waiting for silence");
				break;
			case 3: // Wait for silent packet 2
				displayMessage("latency measurement: completing");
			// send silent packet
				break;
			case 2: // send ping
				displayMessage("latency measurement: sending ping");
			// send ping packet
				{
					for (int i=0; i<iFrames; i++)
						inputs[0][i]=1.0f;
					if (stereo) memcpy(inputs[1],inputs[0],iFrames*sizeof(float));
				}
				break;
			}
		}
	}
	else
	{
		latencyMeasureMode=0;
		silenceCounter=0;
		originCounter=0;
		if (editor && ((AEffGUIEditor*)editor)->getFrame())
			((WormHoleEditor*)editor)->setStatusLatency(-1);
	}
		
	// Sender
	
	// Check minPacketSize
	
	long minSize;
	if (instanceCounter>3)
		if (instanceCounter>7)
			if (instanceCounter>15) 
				if (instanceCounter>31) minSize=16384;
				else minSize=8192;
			else minSize=4096;
		else minSize=2048;
	else minSize=0;
	
	sender->setMinPacketSize(minSize);
	sender->addAudio(inputs,channels,iFrames,masterTick);
	sender->send();

	// Receiver
	
	if (mode==WH_DESTINATION || mode==WH_END || (mode==WH_ORIGIN && remoteSender!=NULL))
	{
		receiver->receive(iFrames, int(iParameters[kBufferSize]*WHMAXBUFFER), masterTick, globalInstance, localInstance);
		if (editor && ((AEffGUIEditor*)editor)->getFrame())
			((WormHoleEditor*)editor)->setStatusRecvAddr(receiver->getRecvAddr());
	}
		
	receiver->copyAudio(outputs,channels,iFrames);

	// Latency measurement (1)

	if (latencyMeasureMode!=0)
	{
		// Check if destination buffer changed
	
		if (remoteReceiver)
		{	
			//float currentBufferSize=globalInstance->getBufferSize(channelName, WH_DESTINATION);
			float currentBufferSize=remoteReceiver->bufferSize;
			if (currentBufferSize!=lastDestinationBufferSize)
			{
				int oldSize=int(lastDestinationBufferSize*WHMAXBUFFER);
				int newSize=int(currentBufferSize*WHMAXBUFFER);
				int change=newSize-oldSize;
				int delay=int(iParameters[kDelay]*WHMAXDELAY);
				delay+=change;
				if (delay<0) delay=0;
				if (delay>WHMAXDELAY) delay=WHMAXDELAY;
				displayMessage("latency value changed by remote instance");
				setParameter(kDelay,float(delay)/float(WHMAXDELAY));
				lastDestinationBufferSize=currentBufferSize;
			}
		}

		// Measure mode stuff
		
		int i=0;
		int numSams;
		switch (latencyMeasureMode) {
		case 1:
			if (getMaxVu(outputs,channels,iFrames)<0.01f)
				silenceCounter+=iFrames;
			if (silenceCounter>=2*int(iParameters[kBufferSize]*WHMAXBUFFER)) // Wait at least 2 times buffer size
			{
				WHdebug("[WormHole2] Latency Ping %d silence received",silenceCounter);
				latencyMeasureMode=2; // Send Ping next time
				measuredLatency=0; // now the real measurement starts
				silenceCounter=0;
			}
			break;
		case 3:
			if (getMaxVu(outputs,channels,iFrames)<0.01f)
				silenceCounter+=iFrames;
			if (silenceCounter>=WHSILENCETIMEOUT) // Wait 
			{
				WHdebug("[WormHole2] Latency Ping %d silence received",silenceCounter);
				latencyMeasureMode=4; // Measurement complete
				silenceCounter=0;
			}
			break;
		case 2: // Wait for ping to return
			if (!stereo)
			{
				for (i=0; i<iFrames; i++)
					if (outputs[0][i]>0.05f || outputs[0][i]<-0.05f) break;
				numSams=i;
			}
			else
			{
				for (i=0; i<iFrames; i++)
					if (outputs[0][i]>0.05f || outputs[0][i]<-0.05f || outputs[1][i]>0.05f || outputs[1][i]<-0.05f)	break;
				numSams=i;
			}
			
			measuredLatency+=numSams;
			if (i!=iFrames)
			{
				if (latencyMeasureMode==2)
				{
					displayMessage("latency was successfully measured");
					WHdebug("[WormHole2] Latency measurement complete %d",measuredLatency);
					latencyMeasureMode=3; // Go to second silence mode
					silenceCounter=0;
					setParameter(kDelay,float(measuredLatency)/float(WHMAXDELAY));
				}
			}
			if (measuredLatency>WHMAXDELAY)
			{
				displayError("latency measurement failed, ping not received");
				//WHdebug("[WormHole2] Latency measurement failed, latency > maxdelay %d",WHMAXDELAY);
				latencyMeasureMode=5;
			}
			break;
		}		
	}
	// Mute pings
	if (mode==WH_ORIGIN && iParameters[kLatencyCorrection]>=0.5f && latencyMeasureMode<4)
	{
		memset(outputs[0],0,iFrames*sizeof(float));
		if (stereo) memset (outputs[1],0,iFrames*sizeof(float));
	}
		
	// Playthrough & delay
		
	if (iParameters[kPlayThrough]>=0.5f)
	{	
		// Add playthrough + delay it according to latency correction
		long delayCount;
		if (theLatency!=0)
			delayCount=WHMAXDELAY;
		else 
			delayCount=0;
//		long delayCount=long(iParameters[kDelay]*WHMAXDELAY);
		int i;
		if (stereo && outputs[0]!=outputs[1])
			for (i=0; i<iFrames; i++)
			{
				delayBuffers[0][delayPosition]=buffers[0][i];
				delayBuffers[1][delayPosition]=buffers[1][i];
				int rPos=(delayPosition+delayCount) % (WHMAXDELAY+1);
				outputs[0][i]+=delayBuffers[0][rPos];
				outputs[1][i]+=delayBuffers[1][rPos];
				if (--delayPosition==-1) delayPosition=WHMAXDELAY;
			}
		else
			for (i=0; i<iFrames; i++)
			{
 				delayBuffers[0][delayPosition]=buffers[0][i];
				outputs[0][i]+=delayBuffers[0][(delayPosition+delayCount) % (WHMAXDELAY+1)];
				if (--delayPosition==-1) delayPosition=WHMAXDELAY;
			}
	}
	
	// output VU
	
	if (editor && ((AEffGUIEditor*)editor)->getFrame()) // only if editor is open!!!
		outputVu=getMaxVu(outputs, channels, iFrames);

	// Increase masterTick

	masterTick+=tickDelta;
}


