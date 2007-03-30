/*
 *  WHgetMachineName.h
 *  apulVSTSDK
 *
 *  Cross-plattform machine name fetcher
 *
 *  Created by Adrian Pflugshaupt on 14.02.05.
 *  Copyright 2005 apulSoft. All rights reserved.
 *
 */

#include <unistd.h>
#include <string.h>
#include <stdio.h>

class WHgetMachineName
{
public:
	WHgetMachineName() {}
	~WHgetMachineName() {}
	char* getName();
};