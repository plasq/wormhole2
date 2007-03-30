/*
 *  WHHeaders.h
 *  apulVSTSDK
 *
 *  Created by Adrian Pflugshaupt on 07.01.05.
 *  Copyright 2005 apulSoft. All rights reserved.
 *
 */

#ifndef __WHHeaders
#define __WHHeaders

#if MAC
#include "Carbon/Carbon.h"
#endif

#define WHAUDIOPACKETID 'ADIO'
#define WHAUDIOPACKETIDINV 'OIDA'
#define WHMIDIPACKETID 'MIDI'

#define WHMAXPORTNAMESIZE (92)

#define WHMULTICASTPORT 48100 // Just some port, but unreserved yet!
#define WHMULTICASTADDR "239.111.231.77" // Noone wants this address...
#define WHOPENPORTTIMEOUT 2.0 // 2.0 Seconds
#define WHMULTICASTINTERVAL 0.4 // 0.4 sec
#define WHMAXLINKEDTICKOFFSET (4096) // Max offset the sample clocks are allowed to have to still be linked

#define WHMINPACKETSIZE (512) // Don't make UDP packets with less than 512 samples

#define WHMAXBUFFER (32768)
#define WHMAXDELAY (32768)
#define WHSILENCETIMEOUT (2000) // 2000 samples of silence are the min silence accepted
#define WH_STARTLATENCYMEASURETIMEOUT (50000) // Wait 50000 Samples before measuring latency
#define WHRECBUFFERSIZE (0x20000) // In bytes, 128kByte
#define WHRECMSGBUFSIZE (0x10000)

#define WH_INVALIDTICKOFFSET (0x10000000)
#define WH_MAXIFIDSIZE (256)

// The 5 modes

#define WH_START (0)
#define WH_END (1)
#define WH_ORIGIN (2)
#define WH_DESTINATION (3)
#define WH_LOOPBACK (4)
#define WH_NUMMODES (5)
#define WH_UNUSED (6)

extern const char* whModeNames[];

#define WH_INVALIDMODE (200)
#define WH_DESTROY (100) // Remove this Instance from the list
#define WH_RESET (150) // Reset the channel this instance is on

#define WHFLAG_SENDERACTIVE				(0x01)
#define WHFLAG_RECEIVERCONNECTED	(0x02)
#define WHFLAG_SYNCED							(0x04)
#define WHFLAG_DESTROY						(0x08)
#define WHFLAG_RESET							(0x10)
#define WHFLAG_NOTPROCESSED				(0x20)

static bool WHisDirectMode(unsigned short mode)
{
	return (mode == WH_START || mode==WH_END);
}

// Parameters Tags

enum
{
	kPlayThrough, // Will be the only one soon	
	kDelay,
	kBufferSize,
	kLatencyCorrection,
	kSync,
	kReserved2,
	kReserved3,
	kReserved4,

  kNumParams
};

#if MAC
inline void _endianSwapFloat(float* a)
{
	UInt32* b = (UInt32*)a;
	*b = CFSwapInt32(*b);
}
#endif

// This is stored by getChunk
struct WHchunk
{
	char channelName[WHMAXPORTNAMESIZE];
	float iParameters[kNumParams];
	unsigned short mode;
	char interfaceID[WH_MAXIFIDSIZE];
	#if MAC
	void endianSwap()
	{
		for (int i=0; i<kNumParams; i++)
			_endianSwapFloat(&iParameters[i]);
		mode = (UInt16)CFSwapInt16((UInt16)mode);
	}
	#endif
};
 
// Callback global->local

#if MAC
#define __stdcall
#endif

//#if win
typedef void (__stdcall*WH_CALLBACK) (int, void*);
//#elif mac
//typedef void (*WH_CALLBACK) (int, void*);
//#endif

#define WH_CALLBACK_UPDATELIST (1)
#define WH_CALLBACK_RESET (2)
#define WH_CALLBACK_SETBUFFERSIZE (3)
#define WH_CALLBACK_SLAVEMESSAGE (4)
#define WH_CALLBACK_RESETLATENCY (5)
#define WH_CALLBACK_DROPOUT (6)
#define WH_CALLBACK_SETDELAY (7)
#define WH_CALLBACK_NOTRUNNING (8)
#define WH_CALLBACK_FETCHINTERFACE (9)
#define WH_CALLBACK_TEST (13)

// This is transferred for the announcing

struct WHportPacket
{
	char channelName[WHMAXPORTNAMESIZE]; // channel name
	unsigned short endian; 
	unsigned short mode; // instance mode 
	unsigned short port; // Sender port
	long ID; // Unique plugin-ID for its machine
	float bufferSize;
	int flags;
};

struct WHlocalInstance
{
	char channelName[WHMAXPORTNAMESIZE]; // channel name
	unsigned short endian; 
	unsigned short mode; // instance mode 
	unsigned short port; // Sender port
	long ID; // Unique plugin-ID for its machine
	float bufferSize;
	int flags;
	long remoteAddr; // Addr this instance is connected to it end_mode
	unsigned short remoteGlobalPort; // Port of the global Instance socket this instance is connected to in end_mode
	int tickOffset;
	float param2; // Parameter to be used with callback
	WH_CALLBACK callback;
	void* callbackparam;
};

// To make the internal list of instances

struct WHremoteInstance
{
	char channelName[WHMAXPORTNAMESIZE]; // channel name
	unsigned short mode; // instance mode
	long addr; // The Ip address, will be detected!
	long ID; // Unique plugin-ID for its machine
	unsigned short port; // Sender Port...?
	unsigned short globalPort; // Port of the global Instance socket -> used to identify linked channels
	bool linked;
	double lastseen; // When was this instance last seen -> timeout
	float bufferSize;
	int flags; // Connection status,...
};

struct WHPacketHeader
{
	long ID;
	unsigned short endian;
//	long size;
};

struct WHAudioPacketHeader
{
	long ID; // ADIO
	unsigned short endian; // 0x00FF or 0xFF00
//	long size;
	float sampleRate;
	short channels;
	long frames; // number of samples
	long tick; // the WormHole sample tick, sample offset of the packet start
};

struct WHinterface
{
	char id[WH_MAXIFIDSIZE];
	char name[16];
	long addr;
	int sock;
	bool active;
};

/*
struct WHMidiPacketHeader
{
	long ID; // MIDI
//	long size; // Size of the packet + header
	double sampleRate; // Samplerate midi offsets are related to
	long frames; // Audio packet size midi is related to
	long tick; // Packet start tick
};
*/
#include "stdio.h"
#if MAC
#include "stdarg.h"
#include "string.h"
#elif WINDOWS
#include "windows.h"
#endif

static void WHdebug(char* fmt,...)
{
#ifdef WHDEBUG
	char s[512];
	va_list argptr;
	va_start(argptr, fmt);
	vsprintf(s, fmt, argptr);
	va_end(argptr);
#if WINDOWS
	OutputDebugString(s);
#elif MAC
	if (s[strlen(s)-1]!='\n')
		printf("%s\n",s);
	else
		printf(s);
#endif
#endif
}

#endif