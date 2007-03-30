/*
 *  WHRingBuffer.cpp
 *  experimental
 *
 *  Created by Adrian Pflugshaupt on 20.10.04.
 *  Copyright 2004 apulSoft. All rights reserved.
 *
 */

#include "WHRingBuffer.h"

WHRingBuffer::WHRingBuffer(size_t byteSize):size(byteSize),phase(0)
{
	buffer=malloc(size); // Get empty memory
#ifndef WINDOWS
	mlock(buffer,size);  // Lock the page
#endif
	flush();
}

WHRingBuffer::~WHRingBuffer()
{
#ifndef WINDOWS
	munlock(buffer,size);
#endif
	free(buffer);
}

void WHRingBuffer::flush()
{
	memset(buffer,0,size);
	phase=0;
}

// Read count bytes from the "front" of the buffer and shift it.

void WHRingBuffer::readFromFront(void* target, size_t count)
{
	char* cTarget=(char*)target;
	if (long(phase+count) > size)
	{ // overlapping block
		// possible leak! Fix: if(phase>=size) return;
		long tail=size-phase;
		memcpy(cTarget,(char*)buffer+phase,tail); // copy everything up to the end of the buffer
		cTarget +=tail; 
		phase=count-tail; // Save 1 cycle..
		memcpy(cTarget,buffer,phase); // copy the remaining bytes from the beginning
	}
	else
	{ // normal block
		memcpy(cTarget,(char*)buffer+phase,count); // Copy the block
		
		phase+=count; // shift the buffer
		while (phase>=size) phase-=size;
	}
}

void WHRingBuffer::readFromFrontAndClear(void* target, size_t count)
{
	char* cTarget=(char*)target;
	if (long(phase+count) > size)
	{ // overlaping block
		// possible leak! Fix: if(phase>=size) return;
		long tail=size-phase;
		memcpy(cTarget,(char*)buffer+phase,tail); // copy everything up to the end of the buffer
		memset((char*)buffer+phase,0,tail);
		cTarget+=tail; 
		phase=count-tail; // Save 1 cycle..
		memcpy(cTarget,buffer,phase); // copy the remaining bytes from the beginning
		memset(buffer,0,phase);
	}
	else
	{ // normal block
		memcpy(cTarget,(char*)buffer+phase,count); // Copy the block
		memset((char*)buffer+phase,0,count);
		
		phase+=count; // shift the buffer
		while (phase>=size) phase-=size;
	}
}

inline void WHRingBuffer::memAddFloat(float* t, float* s, long c)
{
	const float fact=0.7071067f;
	while (c)
	{
		*t = (*t + *s)*fact;
		t++; s++;
		c-=sizeof(float);
	}
}

void WHRingBuffer::addFloatFromFrontAndClear(void* target, size_t count)
{
	char* cTarget=(char*)target;
	if (long(phase+count) > size)
	{ // overlaping block
		// possible leak! Fix: if(phase>=size) return;
		long tail=size-phase;
		memAddFloat((float*)cTarget,(float*)((char*)buffer+phase),tail); // copy everything up to the end of the buffer
		memset((char*)buffer+phase,0,tail);
		cTarget+=tail; 
		phase=count-tail; // Save 1 cycle..
		memAddFloat((float*)cTarget,(float*)buffer,phase); // copy the remaining bytes from the beginning
		memset(buffer,0,phase);
	}
	else
	{ // normal block
		memAddFloat((float*)cTarget,(float*)((char*)buffer+phase),count); // Copy the block
		memset((char*)buffer+phase,0,count);
		
		phase+=count; // shift the buffer
		while (phase>=size) phase-=size; // Maybe waste of CPU?
	}
}

// Write bytes to the buffer with an offset (positive) to the reading position/ write into the furure...

void WHRingBuffer::writeAtOffset(void* source, size_t offset, size_t count)
{
//	printf("WHRingBuffer: offset %d count %d / size %d phase %d\n");
	char* cSource=(char*)source;
	
	long writepos=phase+offset;
	if (writepos>=size) writepos-=size;
		
	if (long(writepos+count) > size)
	{
		// possible memory leak! Fix = if(writepos>=size) return;
		long tail=size-writepos;
		memcpy((char*)buffer+writepos,cSource,tail);
		cSource+=tail;
		memcpy(buffer,cSource,count-tail); // copy the remaining bytes
	}
	else
	{
		memcpy((char*)buffer+writepos,cSource,count);
	}
}

