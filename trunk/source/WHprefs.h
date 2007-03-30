/*
 *  WHprefs.h
 *  apulVSTSDK
 *
 *  Created by Adrian Pflugshaupt on 21.02.05.
 *  Copyright 2005 plasq LLC. All rights reserved.
 *
 *  Class to manage preferences for Wormhole2 
 *	Uses Registry on win and CFPreferences on Mac OSx
 *
 */

#ifndef __WHprefs
#define __WHprefs

#if MAC
 #include <Carbon/Carbon.h>
#elif WINDOWS
 #define _WINSOCKAPI_
 #include "windows.h"
 #undef _WINSOCKAPI_
 #define WHPREFSNAME "SOFTWARE\\plasq\\Wormhole2"
#endif

int WHprefsGetInt(char* name);
void WHprefsSetInt(char* name, int value);

float WHprefsGetFloat(char* name);
void WHprefsSetFloat(char* name, float value);

void WHprefsGetString(char* name, char* value, int len);
void WHprefsSetString(char* name, char* value);

#endif
