/*
 *  WHannounce.h
 *  apulVSTSDK
 *
 *  Created by Adrian Pflugshaupt on 12.02.05.
 *  Copyright 2005 apulSoft. All rights reserved.
 *
 */

#ifndef __WHannounce
#define __WHannounce

#if MAC
 #include <stdio.h>
 #include <sys/socket.h>
 #include <sys/ioctl.h>
 #include <arpa/inet.h>
 #include <netinet/in.h>
 #include <net/if.h> // For ifreq
 #include <unistd.h>
 #include <time.h>
 #include <string.h>
 #include <fcntl.h>
 #include <Carbon/Carbon.h>
#elif WINDOWS
 #include <windows.h>
#endif

#ifndef __WHHeaders
#include "WHHeaders.h"
#endif

//class WHannounce;

//#ifndef __wormHole__
//#include "WormHole15.hpp"
//#endif

class WHannounce
{
public:
	WHannounce();
	~WHannounce();
//	void setParent(WormHole2* val) { parent=val; }

	void sendPacket();
	void setChannelName(char* name);
	void setID(long ID);
	void setMode(unsigned short mode);
	void setPort(unsigned short port);

//	WormHole2* parent;
#if WINDOWS
	HANDLE myEvent;
#endif

private:
#if MAC
	CFRunLoopTimerRef myTimerRef;
#elif WINDOWS
	HANDLE myTimerQueueTimer;
	HANDLE myQueue;
#endif

	WHportPacket message;	
	int sock;
	struct sockaddr_in addr;
};

#endif

