/*
 *  WHreceiver.cpp
 *  apulVSTSDK
 *
 *  Created by Adrian Pflugshaupt on 07.01.05.
 *  Copyright 2005 apulSoft. All rights reserved.
 *
 */

#include "WHreceiver.h"

WHreceiver::WHreceiver(WHsocket* theSocket)
{		
	socket=theSocket;
	// initalize Ring buffers
	ring[0]=new WHRingBuffer(WHRECBUFFERSIZE);
	ring[1]=new WHRingBuffer(WHRECBUFFERSIZE);
	buffer=(unsigned char*)malloc(WHRECMSGBUFSIZE);

	dropoutcounter=0;
	connected=false; // Ist the audio stream established

	recvaddr=0;
	recvport=0;

	int port=48101;
	while (socket->bind(port)==false)
	{
		WHdebug("[WHreceiver] Socket binding %d failed",port);
		port++;
	}
	tickOffset=0;
	syncedCount=0;
}

WHreceiver::~WHreceiver()
{
	delete ring[0];
	delete ring[1];
	free(buffer);
}

void WHreceiver::resume(WHlocalInstance* local)
{
	//ring[0]->flush();
	//ring[1]->flush();
	socket->flush();
	connected=false;
	dropoutcounter=0;
	if (recvaddr==0) return; 
	local->tickOffset=WH_INVALIDTICKOFFSET;
	local->callback(WH_CALLBACK_RESETLATENCY, local->callbackparam);
	recvaddr=0; // Maybe this was the bug after all!
}

unsigned short WHreceiver::getPort()
{
	return socket->getPort();
}

void WHreceiver::receive(long numSamples, long recBuffer, long tick, WHStatic* global, WHlocalInstance* local)
{
	// Read as many packets as needed
	int size;
//	unsigned char* packetStart;
	int position=-1;
	bool received=false;

	long fromaddr;
	unsigned short fromport;

	size=socket->receivefrom((char*)buffer,WHRECMSGBUFSIZE,&fromaddr,&fromport);
//	WHdebug("received %d from port %d",size,socket->getPort());

	while (size>0)
	{
		if (!connected) // only use last available packet in that case
		{
			int asize;
			while ((asize=socket->receivefrom((char*)buffer,WHRECMSGBUFSIZE,&fromaddr,&fromport))>0) 
			{
				size=asize;
			}
		}
	//	WHdebug("Packet received from %08x %d",fromaddr,fromport);
		// Are we getting the packet from the right peer?
		if (connected && (fromaddr!=recvaddr || fromport!=recvport))
			resume(local); // Reset Receiver

		unsigned char* pos = buffer;
		unsigned char* packetEnd = pos+size; // The first byte which doesn't belong to the packet
		
	//	packetStart=pos;
		do
		{
			WHPacketHeader* header=(WHPacketHeader*)pos;
			bool endian=(header->endian==0xff00);
			//if (endian) EndianSwap4(header->ID);
			if (header->ID==WHAUDIOPACKETID || header->ID==WHAUDIOPACKETIDINV)
			{
				WHAudioPacketHeader* audioheader=(WHAudioPacketHeader*)pos;
				pos+=sizeof(WHAudioPacketHeader);
				// Endian stuff
				if (endian)
				{
					EndianSwap4(audioheader->sampleRate);
					EndianSwap2(audioheader->channels);
					EndianSwap4(audioheader->frames);
					EndianSwap4(audioheader->tick);
				}
						
				receivedChannels = audioheader->channels; // Save this for later use!
				// Calculating the position (relative to the play position)
				if (connected)
				{ // Connected and peer stayed the same
					position=audioheader->tick - (tick-tickOffset);
				}
				else
				{
					tickOffset=tick - audioheader->tick; // Calculate offset to internal tick
					if (local->mode==WH_END && local->flags & WHFLAG_SYNCED)
					{
						if ((syncedCount=global->getSetLinkedTickOffset(&tickOffset, local)))
							WHdebug("[WHreceiver] synced to linked instance");
					}
					else 
						WHdebug("[WHreceiver] instance not synced");
				
					WHdebug("[WHreceiver] First packet processed. TickOffset: %d\n",tickOffset);
					connected=true;
					recvaddr=fromaddr;
					recvport=fromport;
				}
				position+=recBuffer;
					
				long frames = audioheader->frames;
				if (position>=0 && (position+frames)<int(WHRECBUFFERSIZE/sizeof(float)))
				{
					// Copying the audio to the ring buffers
					// && Endian conversion
					received=true;
				
					//float* audiodata=(float*)pos;
					if (endian) swapEndianAudio((float*)pos,frames);
	//			WHdebug("Writing at %d , %d samples",position, frames);
					ring[0]->writeAtOffset((float*)pos, position * sizeof(float), frames * sizeof(float));
					//audiodata+=frames; // Go to the next channel
					pos += frames*sizeof(float);

					if (receivedChannels == 2) //stereo packet!
					{
						if (endian) swapEndianAudio((float*)pos,frames);
						ring[1]->writeAtOffset((float*)pos, position * sizeof(float), frames * sizeof(float));
						pos += frames*sizeof(float);
					}
				}
				else // If the packet was skipped, still go to the next one
					pos += frames*sizeof(float)*receivedChannels;
			}
			else 
			{
				WHdebug("Invalid packet! pos %08x end %08x id %08x\n",(int)pos,(int)packetEnd,(int)header->ID);
				break;
			}
		} while (pos<packetEnd);
		
		// Break if enough data is collected
		// For larger buffer sizes, collect all packets.
		//if (numSamples<=1024 && position>numSamples) break;
		
		size=socket->receivefrom((char*)buffer,WHRECMSGBUFSIZE,&fromaddr,&fromport);
	}

	if (received==false && recvaddr!=0) // Dropout?
	{
		dropoutcounter+=numSamples;
		if (dropoutcounter>=WHMAXBUFFER)
		{
			local->callback(WH_CALLBACK_DROPOUT, local->callbackparam);
			resume(local);
		}
	}
	else
		dropoutcounter=0;
}

void WHreceiver::copyAudio(float** outputs, short channels, long numSamples)
{	
	if (!connected) 
	{
		memset(outputs[0],0,numSamples*sizeof(float));
		if (channels==2)
			memset(outputs[1],0,numSamples*sizeof(float));
		return;
	}
	if (channels==1)
	{ // mono output
		ring[0]->readFromFrontAndClear(outputs[0],numSamples*sizeof(float));
		if (receivedChannels==2)
		{ // Mix two channels
			ring[1]->addFloatFromFrontAndClear(outputs[0],numSamples*sizeof(float));
		}
	}
	else 
	{ // stereo output
		ring[0]->readFromFrontAndClear(outputs[0],numSamples*sizeof(float));
		if (receivedChannels==2)
			ring[1]->readFromFrontAndClear(outputs[1],numSamples*sizeof(float));
		else
			memcpy(outputs[1], outputs[0], numSamples*sizeof(float));
	}
}

