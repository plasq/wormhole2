/*
 *  WHsender.cpp
 *  apulVSTSDK
 *
 *  Created by Adrian Pflugshaupt on 07.01.05.
 *  Copyright 2005 plasq LLC. All rights reserved.
 *
 */

#include "WHsender.h"

WHsender::WHsender(WHsocket* theSocket)
{	
	packet=(unsigned char*)malloc(WHRECMSGBUFSIZE);
	packetPos=packet;
	destaddr=0;
	socket=theSocket;
	minPacketSize=0;
}

WHsender::~WHsender()
{
	free(packet);
}

void WHsender::setTarget(long addr, unsigned short port)
{
	WHdebug("[WHsender] Setting target to addr: %8x port: %d\n",addr,port);
	socket->setDestination(addr,port);
	packetPos=packet; // Reset the packet pointer
	destaddr=addr;
}

void WHsender::addAudio(float** buffers, short channels, long frames, long tick)
{
	if (isActive()==false) return;
	// Checking the buffer size
	
	//long size=frames*channels*sizeof(float)+sizeof(WHAudioPacketHeader);

	// Writing the header

	WHAudioPacketHeader* head=(WHAudioPacketHeader*)packetPos;
	packetPos+=sizeof(WHAudioPacketHeader);
	
	head->ID = WHAUDIOPACKETID;
//	head->size = size;
	head->sampleRate = sampleRate;
	head->endian = 0x00ff; // allows auto-detection of endianness!
	head->channels = channels; // number of channels
	head->frames = frames; // number of samples
	head->tick = tick;
			
	// Writing the sample data
	
	int i;
	for (i=0; i<channels; i++)
	{
		memcpy(packetPos,buffers[i],frames*sizeof(float));
		packetPos+=frames*sizeof(float);
	}
}

// Sending the compiled packet

void WHsender::send()
{
	int packetSize = packetPos - packet;
	if (isActive()) 
	{
		if (packetSize>=minPacketSize)
			socket->send((char*)packet,packetPos-packet);
		else return;
	}
	packetPos=packet; // Reset the packet pointer
}