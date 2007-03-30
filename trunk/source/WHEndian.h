/*
 *  WHEndian.h
 *  apulVSTSDK
 *
 *  Created by Adrian Pflugshaupt on 10.02.05.
 *  Copyright 2005 plasq LLC. All rights reserved.
 *
 */

#ifndef __WHendian
#define __WHendian

// Swap endian for any length
#define EndianSwap(x)  _ByteSwap((unsigned char *) &x,sizeof(x))
// Swap endian 4 bytes
#define EndianSwap4(x) _ByteSwap4((long *) &x)
// Swap endian 2 bytes
#define EndianSwap2(x) _ByteSwap2((unsigned short*) &x)

// Swap endian for any length
inline void _ByteSwap(unsigned char * b, int n)
{
   register int i = 0;
   register int j = n-1;
	 register unsigned char d;
   while (i<j)
   {
			d=b[j];
			b[j]=b[i];
			b[i]=d;
      i++, j--;
   }
}

// Swap endian 4 bytes
inline void _ByteSwap4(long* b)
{
	register long v=*b;
	*b = ((v & 0x000000FF)<<24) +
			 ((v & 0x0000FF00)<< 8) +
			 ((v & 0x00FF0000)>> 8) +
			 ((v & 0xFF000000)>>24);
}

// Swap endian 2 bytes
inline void _ByteSwap2(unsigned short * b)
{
	register unsigned short v=*b;
	*b=((v>>8)+(v<<8));
}

inline void swapEndianAudio(float* stream, long numSamples)
{
	register long v;
	long* stream_l=(long*)stream;
	while (numSamples--)
	{
		v=*stream_l;
		*stream_l++ = ((v & 0x000000FF)<<24) +
									((v & 0x0000FF00)<< 8) +
									((v & 0x00FF0000)>> 8) +
									((v & 0xFF000000)>>24);
	}
}

#endif