/*
 *  WHsocket.h
 *  apulVSTSDK
 *
 *  Created by Adrian Pflugshaupt on 08.01.05.
 *  Copyright 2005 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __WHsocket
#define __WHsocket

#if WINDOWS
 #include "windows.h"
#else
 #include <sys/types.h>
 #include <sys/socket.h>
 #include <arpa/inet.h>
 #include <unistd.h>
 #include <stdio.h>
 #include <string.h>
 #include <netinet/in_systm.h> // For ip.h .. go figure...
 #include <netinet/in.h>
 #include <netinet/ip.h>
 #include <fcntl.h>
#endif

#ifndef __WHHeaders
#include "WHHeaders.h"
#endif

#define WHSOCKETBUFFERSIZE (0x30000)

class WHsocket
{
	public:
		WHsocket();
		~WHsocket();
		void send(char* buffer, long len);
		int receivefrom(char* buffer, long bufsize, long* addr, unsigned short* port);
//		int receive(char* buffer, long bufsize);
		void setDestination(long addr, unsigned short port);
		bool bind(unsigned short port);
		void flush();
		unsigned short getPort();
	
	private:
		int handle;
		struct sockaddr_in destaddr,srcaddr;
};

#endif // WHsocket
