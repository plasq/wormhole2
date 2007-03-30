/*
 *  WHsender.h
 *  apulVSTSDK
 *
 *  Created by Adrian Pflugshaupt on 07.01.05.
 *  Copyright 2005 plasq LLC. All rights reserved.
 *
 */
 
#ifndef __WHsender
#define __WHsender

#ifndef __WHHeaders
#include "WHHeaders.h"
#endif

#ifndef __WHsocket
#include "WHsocket.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

class WHsender
{
public:
	WHsender(WHsocket* theSocket);
	~WHsender();
	void setSampleRate(float rate) { sampleRate=rate; }
	void setTarget(long addr, unsigned short port);
	void addAudio(float** buffers, short channels, long frames, long tick);
	void send();
	bool isActive() { return (destaddr!=0); }
	long getDestAddr() { return destaddr; }
	void setMinPacketSize(long size) { minPacketSize=size; }
	
private:
	long minPacketSize;
	long destaddr;

	unsigned char* packet; 
	unsigned char* packetPos;

	float sampleRate;
	
	WHsocket* socket;
};

#endif
