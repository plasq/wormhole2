//-------------------------------------------------------------------------------------------------------
// VST Plug-Ins SDK
//-
// WormHole2 (VST 2.0)
//-
//-------------------------------------------------------------------------------------------------------

#ifndef __WormHole__
#include "WormHole15.hpp"
#endif

bool oome = false;

#if defined (__GNUC__)
	#define VST_EXPORT	__attribute__ ((visibility ("default")))
#else
	#define VST_EXPORT
#endif

#if MAC

#pragma export on
#endif

//------------------------------------------------------------------------
// Prototype of the export function main
//------------------------------------------------------------------------
#if BEOS
#define main main_plugin
extern "C" __declspec(dllexport) AEffect *main_plugin (audioMasterCallback audioMaster);

#elif MACX
#define main main_macho
extern "C" VST_EXPORT AEffect *main_macho (audioMasterCallback audioMaster);

 #ifndef __ppc__
 extern "C" VST_EXPORT AEffect* VSTPluginMain (audioMasterCallback audioMaster) { return main_macho(audioMaster); }
 #endif

#else
AEffect *main (audioMasterCallback audioMaster);
#endif

//------------------------------------------------------------------------
AEffect* main (audioMasterCallback audioMaster)
{
	// Get VST Version
	if (!audioMaster (0, audioMasterVersion, 0, 0, 0, 0))
		return 0;  // old version

	// Create the AudioEffect
	WormHole2* effect = new WormHole2(audioMaster);
	if (!effect)
		return 0;
	
	// Check if no problem in constructor of AGain
	if (oome)
	{
		delete effect;
		return 0;
	}
	return effect->getAeffect ();
}

#if MAC
#pragma export off
#endif

//------------------------------------------------------------------------
#if WIN32
#define _WINSOCKAPI_
#include <windows.h>
#undef _WINSOCKAPI_
void* hInstance;
BOOL WINAPI DllMain (HINSTANCE hInst, DWORD dwReason, LPVOID lpvReserved)
{
	hInstance = hInst;
	return 1;
}
#endif
