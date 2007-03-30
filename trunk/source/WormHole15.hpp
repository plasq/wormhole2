//-------------------------------------------------------------------------------------------------------
////-
// plasq LLC WormHole2 1.0 VST
//-
// © 2004 plasq LLC
//-------------------------------------------------------------------------------------------------------

#ifndef __wormhole__
#define __wormhole__

#ifndef __audioeffectx__
#include "audioeffectx.h"
#endif

#ifndef __apConfiguration
#include "apConfiguration.h"
#endif

#ifndef __WHHeaders
#include "WHHeaders.h"
#endif

#ifndef __WHstatic
#include "WHStatic.h"
#endif

#ifndef __WHreceiver
#include "WHreceiver.h"
#endif

#ifndef __WHsender
#include "WHsender.h"
#endif

#ifndef __WHprefs
#include "WHprefs.h"
#endif

#ifndef __WHRingBuffer
#include "WHRingBuffer.h"
#endif

//------------------------------------------------------------------------

class WormHole2: public AudioEffectX
{
public:
	WormHole2(audioMasterCallback audioMaster);
	~WormHole2();
	
	virtual void process (float **inputs, float **outputs, long sampleframes);
	virtual void processReplacing (float **inputs, float **outputs, long sampleFrames);
	
	virtual long getChunk(void** data, bool isPreset);
	virtual long setChunk(void* data, long byteSize, bool isPreset);
	virtual long getGetTailSize() { return long(getSampleRate())*5; }

	virtual void setParameter (long index, float value);
	virtual float getParameter (long index);
	virtual void getParameterLabel (long index, char *label);
	virtual void getParameterDisplay (long index, char *text);
	virtual void getParameterName (long index, char *text);
	
	virtual void resume ();

	virtual bool getEffectName (char* name);
	virtual bool getVendorString (char* text);
	virtual bool getProductString (char* text);
	virtual long getVendorVersion () { return kapVersion; }

	// It might be more safe to have 1 program instead of none

	virtual void getProgramName(char* name) { strcpy(name,"Default"); }
	virtual void setProgramName(char* name) {}
	virtual void setProgram(long program) {}
	virtual bool getProgramNameIndexed(long category, long index, char* text) { getProgramName(text); return (index<1); }

//-------------
	
	void updateList();
	void setChannelName(char* name);
	void setMode(unsigned short mode);
	void resetChannel();
	
	char* getChannelName() { return channelName; }
	unsigned short getMode() { return mode; }
	long getID() { return ID; }
	bool isLinked() { return ((localInstance->flags & WHFLAG_SYNCED) && (mode==WH_START || mode==WH_END)); }

	float inputVu, outputVu;
	void displayError(char* string);
	void displayMessage(char* string);
		
	static WHStatic* globalInstance; // One global instance for receiving multicast,...
	WHlocalInstance* localInstance;
	WHremoteInstance* remoteSender;
	WHremoteInstance* remoteReceiver;

	int latencyMeasureMode;
	void receiverResume(); 
	void fetchInterface();

private:
	void connectionCheck();
	void procSync(long iFrames);
	inline float getMaxVu(float** &buffers, int &channels, long &frames);
	
	WHsocket* mySocket;
	WHsender* sender;
	WHreceiver* receiver; 

	long masterTick; // Sample clock
	long nextTick;
	long tickDelta;

	long listChangeCounter;
	long theLatency;
	
	// Variables for audio-ping
	int measuredLatency;
	int silenceCounter;
	int originCounter;
	float lastDestinationBufferSize;
	double lastPosition; // The last ppq position reported to the plug
	double lastSamplePos;
	double nextSamplePos;
	double diffsum;
	bool sync;
	
	bool peerLinked;
	char channelName[WHMAXPORTNAMESIZE];
	unsigned short mode; 
	long ID; // per machine unique instance ID for correct remote updates

	WHchunk chunkData; // For save&restore
	
	static long instanceCounter; // How many instances habe been created
	float* playThroughBuffers[2];
	long playThroughBufferSize;
	float* delayBuffers[2];
	float* latencyBuffers[2];
	int delayPosition;
	int latencyDelayPosition;
	
	float iParameters[kNumParams];
};

#endif
