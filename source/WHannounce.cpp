/*
 *  WHannounce.cpp
 *  PKS
 *
 *  Created by Adrian Pflugshaupt on 04.02.05.
 *  Copyright 2005 apulSoft. All rights reserved.
 *
 */
#if WINDOWS
#define _WIN32_WINNT 0x0500
#endif

#include "WHannounce.h"

#if MAC
void announceCallBack(CFRunLoopTimerRef timer, void* info)
#elif WINDOWS
VOID CALLBACK announceCallBack(PVOID info, BOOL timerOrWait)
#endif
{
	WHannounce* base = (WHannounce*) info;
	base->sendPacket();
	
#if WINDOWS
	SetEvent(base->myEvent);
#endif
}

WHannounce::WHannounce()
{
	// Set up UDP multicast socket

	sock=socket(AF_INET, SOCK_DGRAM ,0);

	int yes=1;
#if WINDOWS
	ioctlsocket(sock,FIONBIO,(u_long*)&yes); // Set to nonblocking
#elif MAC
	fcntl(sock, F_SETFL, O_NONBLOCK);	  // Make it a non-blocking socket!!!
#endif

	// Very important if multiple instances exist
#if MAC
	setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, (char*)&yes, sizeof(yes)); 
#elif WINDOWS
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof(yes)); // Let's hope this is enough
#endif

	memset(&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(WHMULTICASTPORT);
	addr.sin_addr.s_addr = inet_addr(WHMULTICASTADDR);

	// Set up message

	memset(&message,0,sizeof(message));	
	
	// Set up timer

	#if MAC
	CFRunLoopTimerContext c={0, this, NULL, NULL, NULL};
	myTimerRef= CFRunLoopTimerCreate(NULL, CFAbsoluteTimeGetCurrent()+WHMULTICASTINTERVAL, WHMULTICASTINTERVAL, 0, 0, announceCallBack, &c);
	CFRunLoopAddTimer(CFRunLoopGetCurrent(), myTimerRef, kCFRunLoopDefaultMode);	
	#elif WINDOWS
	myEvent=CreateEvent(NULL, TRUE, FALSE, NULL);
	myTimerQueue=CreateTimerQueue();
	CreateTimerQueueTimer(&myTimerQueueTimer, myTimerQueue,(WAITORTIMERCALLBACK)announceCallBack, this, 0, (WHMULTICASTINTERVAL*1000.0),0);
	#endif
}

WHannounce::~WHannounce()
{	
	// Stop timer, close socket
#if MAC
	CFRunLoopRemoveTimer(CFRunLoopGetCurrent(),  myTimerRef, kCFRunLoopDefaultMode);
	CFRelease(myTimerRef);

	// Send destroy message!
	WHdebug("[WHannounce] sending destroy packet");
	message.mode=htons(WH_DESTROY);
	sendto(sock,(char*)&message,sizeof(message), 0, (struct sockaddr*)&addr,sizeof(addr));

	::close(sock);
#elif WINDOWS
	if (myTimerQueueTimer) DeleteTimerQueueTimer(myQueue,myTimerQueueTimer,INVALID_HANDLE_VALUE);
	if (myQueue) DeleteTimerQueue(myQueue);
	if (myEvent) CloseHandle(myEvent);

	closesocket(sock);
#endif
}

void WHannounce::sendPacket()
{
	if (message.channelName[0]!=0)
	{
		sendto(sock,(char*)&message,sizeof(message), 0, (struct sockaddr*)&addr,sizeof(addr));
	}
}

void WHannounce::setChannelName(char* name)
{
	strncpy(message.channelName,name,WHMAXPORTNAMESIZE-1);
//	if (strcmp(name,"")==0) // This instance is being deactivated
//	{
//		sendDestroyPacket();
//	}
}

void WHannounce::setID(long ID)
{
	message.ID=htonl(ID);
//	sendPacket();
}

void WHannounce::setMode(unsigned short mode)
{
	if (message.mode!=htons(mode))
	{
		message.mode=htons(mode);
//		sendPacket();
	}
}

void WHannounce::setPort(unsigned short port)
{
	message.port=htons(port);
//	sendPacket();
}
