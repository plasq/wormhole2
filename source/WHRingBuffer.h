/*
 *  WHRingBuffer.h
 *
 *  Created by Adrian Pflugshaupt on 20.10.04.
 *  Copyright 2004 plasq LLC. All rights reserved.
 *
 */
 
#ifndef __WHRingBuffer
#define __WHRingBuffer
 
#include <string.h> // For memcpy
#include <stdlib.h> // Calloc
#include <sys/types.h>
#ifndef WINDOWS
#include <sys/mman.h>
#endif
		 
class WHRingBuffer
{
public:
	WHRingBuffer(size_t byteSize);
	~WHRingBuffer();

	void readFromFront(void* target, size_t count);
	void readFromFrontAndClear(void* target, size_t count);
	void writeAtOffset(void* source, size_t offset, size_t count);
	void flush();

	void addFloatFromFrontAndClear(void* target, size_t count); // For WH
		
private:
	void memAddFloat(float* t, float* s, long c); // for WH
	void* buffer;
	long size;
	long phase;
};

#endif