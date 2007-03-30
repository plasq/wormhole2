/*
 *  WHprefs.cpp
 *  apulVSTSDK
 *
 *  Created by Adrian Pflugshaupt on 21.02.05.
 *  Copyright 2005 plasq LLC. All rights reserved.
 *
 */

#include "WHprefs.h"

#if MAC

static char WHPREFSNAME[]="com.plasq.Wormhole2";

float WHprefsGetFloat(char* name)
{
  CFStringRef cfKey = CFStringCreateWithCString(NULL, WHPREFSNAME, kCFStringEncodingMacRoman);
	CFStringRef cfName = CFStringCreateWithCString(NULL, name, kCFStringEncodingMacRoman);
	CFPreferencesAppSynchronize(cfKey); // make sure things are updated
	CFNumberRef val=(CFNumberRef)CFPreferencesCopyAppValue(cfName, cfKey);
	CFRelease(cfName);
  CFRelease(cfKey);
	if (val==NULL)
		return 0.0f;
	else
	{
		float value;
		CFNumberGetValue(val,kCFNumberFloatType,&value);
		CFRelease(val);
		return value;
	}
}

void WHprefsSetFloat(char* name, float value)
{
  CFStringRef cfKey = CFStringCreateWithCString(NULL, WHPREFSNAME, kCFStringEncodingMacRoman);
	CFNumberRef val=CFNumberCreate(NULL,kCFNumberFloatType,&value);
	CFStringRef cfName=CFStringCreateWithCString(NULL,name,kCFStringEncodingMacRoman);
	CFPreferencesSetAppValue(cfName, val, cfKey);
	CFPreferencesAppSynchronize(cfKey);
	CFRelease(val);
	CFRelease(cfName);
  CFRelease(cfKey);
}

int WHprefsGetInt(char* name)
{
  CFStringRef cfKey = CFStringCreateWithCString(NULL, WHPREFSNAME, kCFStringEncodingMacRoman);
	CFStringRef cfName=CFStringCreateWithCString(NULL, name, kCFStringEncodingMacRoman);
	CFPreferencesAppSynchronize(cfKey); // make sure things are updated
	CFNumberRef val=(CFNumberRef)CFPreferencesCopyAppValue(cfName, cfKey);
	CFRelease(cfName);
  CFRelease(cfKey);
	if (!val)
		return 0;
	else
	{
		int value;
		CFNumberGetValue(val, kCFNumberIntType, &value);
		CFRelease(val);
		return value;
	}
}

void WHprefsSetInt(char* name, int value)
{
  CFStringRef cfKey = CFStringCreateWithCString(NULL, WHPREFSNAME, kCFStringEncodingMacRoman);
	CFNumberRef val=CFNumberCreate(NULL,kCFNumberIntType,&value);
	CFStringRef cfName=CFStringCreateWithCString(NULL, name, kCFStringEncodingMacRoman);
	CFPreferencesSetAppValue(cfName, val, cfKey);
	CFPreferencesAppSynchronize(cfKey);
	CFRelease(val);
	CFRelease(cfName);
  CFRelease(cfKey);
}

void WHprefsGetString(char* name, char* value, int len)
{
  CFStringRef cfKey = CFStringCreateWithCString(NULL, WHPREFSNAME, kCFStringEncodingMacRoman);
	CFStringRef cfName=CFStringCreateWithCString(NULL, name, kCFStringEncodingMacRoman);
	CFPreferencesAppSynchronize(cfKey); // make sure things are updated
	CFStringRef val=(CFStringRef)CFPreferencesCopyAppValue(cfName, cfKey);
	CFRelease(cfName);
  CFRelease(cfKey);
	if (!val)
	{
		strcpy(value,"");
	}
	else
	{
		CFStringGetCString(val, value, len, kCFStringEncodingMacRoman);
		CFRelease(val);
	}
}

void WHprefsSetString(char* name, char* value)
{
  CFStringRef cfKey = CFStringCreateWithCString(NULL, WHPREFSNAME, kCFStringEncodingMacRoman);
	CFStringRef val=CFStringCreateWithCString(NULL,value,kCFStringEncodingMacRoman);
	CFStringRef cfName=CFStringCreateWithCString(NULL,name,kCFStringEncodingMacRoman);
	CFPreferencesSetAppValue(cfName, val, cfKey);
	CFPreferencesAppSynchronize(cfKey);
	CFRelease(val);
	CFRelease(cfName);
  CFRelease(cfKey);
}

#elif WINDOWS

int WHprefsGetInt(char* name)
{
	HKEY regHandle=0;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, WHPREFSNAME, 0, KEY_READ, &regHandle)!=ERROR_SUCCESS)
		return 0;
	int value=0;
	DWORD type,size;
	size=sizeof(int);
	if (RegQueryValueEx(regHandle,name,0,&type,(LPBYTE)&value,&size)!=ERROR_SUCCESS)
		value=0;
	RegCloseKey(regHandle);
	return value;
}

void WHprefsSetInt(char* name, int value)
{
	HKEY regHandle=0;
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,WHPREFSNAME,0,NULL, REG_OPTION_NON_VOLATILE,KEY_WRITE, NULL,&regHandle, NULL)!=ERROR_SUCCESS)
		return;

	RegSetValueEx(regHandle,name,0,REG_DWORD,(LPBYTE)&value,sizeof(DWORD));
	RegCloseKey(regHandle);
}

float WHprefsGetFloat(char* name)
{
	HKEY regHandle=0;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, WHPREFSNAME, 0, KEY_READ, &regHandle)!=ERROR_SUCCESS)
		return 0.0f;
	DWORD value=0;
	DWORD type,size;
	size=sizeof(int);
	if (RegQueryValueEx(regHandle,name,0,&type,(LPBYTE)&value,&size)!=ERROR_SUCCESS)
	{
		RegCloseKey(regHandle);
		return 0.0f;
	}	
	RegCloseKey(regHandle);
	
	float* pFloatValue=(float*)&value;
	if (value)
		return *pFloatValue;
	else
		return 0.0f;
}

void WHprefsSetFloat(char* name, float value)
{
	HKEY regHandle=0;
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,WHPREFSNAME,0,NULL, REG_OPTION_NON_VOLATILE,KEY_WRITE, NULL,&regHandle, NULL)!=ERROR_SUCCESS)
		return;

	RegSetValueEx(regHandle,name,0,REG_DWORD,(LPBYTE)&value,sizeof(DWORD));
	RegCloseKey(regHandle);
}

void WHprefsGetString(char* name, char* value, int len)
{
	value[0]=0;

	HKEY regHandle=0;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, WHPREFSNAME, 0, KEY_READ, &regHandle)!=ERROR_SUCCESS) return;
	DWORD size;
	size=len;
	if (RegQueryValueEx(regHandle,name,0,NULL,(LPBYTE)value,&size)!=ERROR_SUCCESS)
		value[0]=0;
	RegCloseKey(regHandle);
}

void WHprefsSetString(char* name, char* value)
{
	HKEY regHandle=0;
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,WHPREFSNAME,0,NULL, REG_OPTION_NON_VOLATILE,KEY_WRITE, NULL,&regHandle, NULL)!=ERROR_SUCCESS)
		return;

	RegSetValueEx(regHandle,name,0,REG_SZ,(LPBYTE)value,strlen(value));
	RegCloseKey(regHandle);
}

#endif