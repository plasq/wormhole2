/*
 *  WHstatic.h
 *  apulVSTSDK
 *
 *  Created by Adrian Pflugshaupt on 10.02.05.
 *
 */

#ifndef __WHstatic
#define __WHstatic

#if MAC
 #include <sys/types.h>
 #include <sys/socket.h>
 #include <arpa/inet.h>
 #include <netinet/in.h>
 #include <netdb.h>
 #include <unistd.h>
 #include <net/if.h>
 #include <sys/ioctl.h>
 #include <ifaddrs.h>
 #include <fcntl.h>
 #include <Carbon/Carbon.h>
#elif WINDOWS
 #include "windows.h"
 #include "iphlpapi.h"
#endif

#include <stdio.h>
#include <string.h>

#ifndef __WHHeaders
#include "WHHeaders.h"
#endif

#ifndef __WHendian
#include "WHEndian.h"
#endif

#ifndef __WHprefs
#include "WHprefs.h"
#endif

struct menuListEntry
{
	char name[WHMAXPORTNAMESIZE];
};

class WHStatic
{
public:
	WHStatic(WH_CALLBACK _callback);
	~WHStatic();

	long getChangeCounter() { return changeCounter; }

//	bool getAddr(char* name, unsigned short mode, long callerID, long* addr, unsigned short* port);
	WHremoteInstance* getRemoteInstance(char* name, unsigned short mode, long callerID);
	int getChannelList(char ***list, int* numDirect);
	unsigned short getAMode(char* name);
	bool getModes(char* name,WHremoteInstance** modeList);
	
	float getBufferSize(char* name, unsigned short mode);
	bool isSenderActive(char* name, unsigned short mode);
	bool isReceiverConnected(char* name, unsigned short mode);
	bool isLinked(char* name, unsigned short mode);
	bool isLoopComplete(char* name);

	int getSetLinkedTickOffset(int* offset, WHlocalInstance* local);
	void invalidateTickOffsets();

	void updateLinkedBufferSize(WHlocalInstance*, float value);
	void updateLinkedDelay(WHlocalInstance*, float value);

	// Announcement stuff
	
	WHlocalInstance* initAnnouncement(long ID, char* name, unsigned short mode, unsigned short port, float bufferSize, void* callbackparam);
	void exitAnnouncement(WHlocalInstance*);
	void sendAnnouncePackets();
		
	void timeoutPorts();
	void receiveAndProcessPackets();
	void endianConvertPacket(WHportPacket* packet);
	
	void localUpdate();
	void localReset(char*, long ID);	
	
	unsigned short lastMode;
	char lastChannelName[WHMAXPORTNAMESIZE];
	float timeoutCounter;
	float lastParameters[kNumParams];
	
	double position; // The beat position of this host
	long sampleClock;
	int syncCount;
	bool synced;
	bool updateListNextTime;
	long changeCounter;
	bool updateingParams;
	
	int lastID; // ID of the last instance which processed
	double lastSamplePos;
	int lastDiff;
	
#if WINDOWS
	HANDLE timeoutEvent;
#endif

	WHinterface* getInterface(unsigned int nr) { return (nr<interfaceCount)?&interfaceList[nr]:NULL; }
	char* getInterfaceID();
	void setInterfaceID(char*);

private:
	char sockBuffer[65536]; // max. Space for socket calls
	void setupInterfaceList();
	bool isOnThisMachine(long addr);	
	void updateList();
	
	struct sockaddr_in sockaddr;
	struct sockaddr_in sendaddr;

	WHinterface interfaceList[32];
	unsigned int interfaceCount;
	menuListEntry openChannelList[512];
	unsigned int openChannelCount;
	WHremoteInstance remoteInstanceList[512]; // Space for remote Instances
	int remoteInstanceCount;
	WHlocalInstance localInstanceList[512]; // Space for local instances
	int localInstanceCount;

	WH_CALLBACK callback; // To call attached instances

	#ifdef MAC
    EventLoopTimerRef socketTimerRef;

	#elif WINDOWS
    DWORD resolution;
    UINT socketTimerRef;
	#endif
};

#endif