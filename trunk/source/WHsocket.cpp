/*
 *  WHsocket.cpp
 *  apulVSTSDK
 *
 *  Created by Adrian Pflugshaupt on 08.01.05.
 *  Copyright 2005 apulSoft. All rights reserved.
 *
 */

#include "WHsocket.h"

WHsocket::WHsocket()
{
	handle=socket(AF_INET, SOCK_DGRAM, 0);
	unsigned long yes=1;
#if MAC
	int tos=(IPTOS_THROUGHPUT | IPTOS_RELIABILITY | IPTOS_LOWDELAY);
	setsockopt(handle, IPPROTO_IP, IP_TOS, (void*)&tos, sizeof(tos));
	fcntl(handle, F_SETFL, O_NONBLOCK);	  // Make it a non-blocking socket!!!
#elif WINDOWS
	ioctlsocket(handle,FIONBIO,&yes); // Set to nonblocking
#endif

	long socksize=WHSOCKETBUFFERSIZE;
	setsockopt(handle, SOL_SOCKET, SO_RCVBUF, (char*)&socksize, sizeof(socksize));
	setsockopt(handle, SOL_SOCKET, SO_SNDBUF, (char*)&socksize, sizeof(socksize));
#if MAC
	setsockopt(handle, SOL_SOCKET, SO_REUSEADDR,(char*)&yes,sizeof(yes)); // Adresse frei halten 
#endif
//	setsockopt(handle, SOL_SOCKET, SO_DONTROUTE,(char*)&yes,sizeof(yes)); // Don't route (faster??)
}

WHsocket::~WHsocket()
{
#if WINDOWS
	if (handle) closesocket(handle);
#else
	if (handle) close(handle);
#endif
}

void WHsocket::setDestination(long addr, unsigned short port)
{
	memset(&destaddr,0,sizeof(destaddr));
	destaddr.sin_family=AF_INET;
	destaddr.sin_port=htons(port);
	destaddr.sin_addr.s_addr=htonl(addr);
}

bool WHsocket::bind(unsigned short port)
{
	memset(&srcaddr,0,sizeof(srcaddr));
	srcaddr.sin_family=AF_INET;
	srcaddr.sin_port=htons(port); 
	srcaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	int err= ::bind(handle, (struct sockaddr*)&srcaddr, sizeof(srcaddr));
	if (err) WHdebug("[WHsocket] bind error %d",err);
	return (err==0);
}

void WHsocket::flush()
{
	char buffer[1024];
	long dummy1;
	unsigned short dummy2;
	while (receivefrom(buffer, 1024, &dummy1, &dummy2)>0) {}
}

unsigned short WHsocket::getPort()
{
#if MAC
	unsigned int len=sizeof(srcaddr);
#else
	int len=sizeof(srcaddr);
#endif
	getsockname(handle, (struct sockaddr*)&srcaddr, &len);
	return ntohs(srcaddr.sin_port);
}

void WHsocket::send(char* buffer, long len)
{
	sendto(handle, buffer, len, 0, (struct sockaddr*)&destaddr, sizeof(destaddr));
}

int WHsocket::receivefrom(char* buffer, long bufsize, long* addr, unsigned short* port)
{
	struct sockaddr_in dummy;
#if MAC
	unsigned int size=sizeof(dummy);
#else
	int size=(int)sizeof(dummy);
#endif
	int count=recvfrom(handle, buffer, bufsize, 0, (sockaddr*)&dummy, &size);
	*addr=ntohl(dummy.sin_addr.s_addr);
	*port=ntohs(dummy.sin_port);
	return count;
}

//int WHsocket::receive(char* buffer, long bufsize)
//{
//	return recv(handle, buffer, bufsize, 0);
//}