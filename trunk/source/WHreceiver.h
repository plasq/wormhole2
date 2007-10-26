/*
 *  WHreceiver.h
 *  apulVSTSDK
 *
 *  Created by Adrian Pflugshaupt on 07.01.05.
 *  Copyright 2005 plasq LLC. All rights reserved.
 *
 */

#ifndef __WHreceiver
#define __WHreceiver

#ifndef __WHHeaders
#include "WHHeaders.h"
#endif

#ifndef __WHsocket
#include "WHsocket.h"
#endif

#ifndef __WHRingBuffer
#include "WHRingBuffer.h"
#endif

#ifndef __WHStatic
#include "WHStatic.h"
#endif

#ifndef __WHendian
#include "WHEndian.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

class WHreceiver
{
public:
	WHreceiver(WHsocket* theSocket);
	~WHreceiver();
	void resume(WHlocalInstance*);
	void receive(long numSamples, long recBuffer, long tick, WHStatic*, WHlocalInstance*);
	void copyAudio(float** outputs, short channels, long numSamples);
	bool isConnected() { return connected; }
	unsigned short getPort();
	int syncedCount;
	long getRecvAddr() { return recvaddr; }

private:
	long recvaddr;
	unsigned short recvport;

	bool connected;
	int receivedChannels; // Number of data channels received
	WHRingBuffer* ring[2]; // Ring buffers for audio
	unsigned char* buffer; // Buffer for UDP Messages
	WHsocket* socket;
	int dropoutcounter;
	int tickOffset;
};

#endif
