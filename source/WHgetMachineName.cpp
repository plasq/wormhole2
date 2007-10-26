/*
 *  WHgetMachineName.cpp
 *  apulVSTSDK
 *
 *  Created by Adrian Pflugshaupt on 14.02.05.
 *  Copyright 2005 plasq LLC. All rights reserved.
 *
 */

#include "WHgetMachineName.h"

char* WHgetMachineName::getName()
{
	static char machineName[256]; // Space for the name
	if (gethostname(machineName,255))
	{
		printf("[WHgetMachineName] gethostname failed!\n");
	}
	else
	{
		char* dot=strrchr(machineName,'.'); // truncate string at "."
		if (dot) *dot=0;
		printf("[WHgetMachineName] Machine name: %s\n",machineName);
	}
	return machineName;
}