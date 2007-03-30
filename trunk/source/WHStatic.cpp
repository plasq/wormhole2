/*
 *  WHstatic.cpp
 *  apulVSTSDK
 *
 *  Created by Adrian Pflugshaupt on 10.02.05.
 *  Copyright 2005 apulSoft. All rights reserved.
 *
 */

#include "WHStatic.h"

inline double get_time()
{
#if MAC
	struct timeval tv;
	gettimeofday(&tv,NULL); // The hour
	return double(tv.tv_sec)+double(tv.tv_usec)*1e-6;
#elif WINDOWS
	return double(GetTickCount())/1000.0; // Time since system started -> good enuff
#endif
}

#if MAC
void socketCallBack(EventLoopTimerRef inTimer, void* info)
{
#elif WINDOWS
VOID CALLBACK socketCallBack(UINT wTimerID, UINT msg, DWORD info, DWORD dw1, DWORD dw2)
{
#endif
		if (info==NULL) return;
		WHStatic* base=(WHStatic*)info;
		// Sending Packets
		base->sendAnnouncePackets();

		// Receiving Packets
		base->receiveAndProcessPackets();
	
		// Timeout Instances
		base->timeoutCounter+=float(WHMULTICASTINTERVAL);
		if (base->timeoutCounter>=(WHOPENPORTTIMEOUT/2))
		{
			base->timeoutCounter=0;
			base->timeoutPorts();
		}	

		if (base->updateListNextTime)
		{
			base->changeCounter++;
			base->localUpdate();
			base->updateListNextTime=false;
		}
}

WHStatic::WHStatic(WH_CALLBACK _callback)
{
	callback=_callback;
	#if WIN
	classPointer=this; // for SetTimer
	#endif
	changeCounter=1; // every time the list changes, this will be incremented
	timeoutCounter=0.0f;
	position=-1.0;
	sampleClock=0;
	syncCount=0;
	synced=false;
	updateListNextTime=false;
	updateingParams=false;
	
	lastMode=(unsigned short)WHprefsGetInt("Mode");
	lastChannelName[0]=0;
	lastID=-1;
	
	char temp[32];
	for (int i=0; i<kNumParams; i++)
	{
		sprintf(temp,"Param%d",i);
		lastParameters[i] = WHprefsGetFloat(temp);
	}
	if (lastParameters[kBufferSize]==0.0f)
		lastParameters[kBufferSize]=0.25f;
					
	setupInterfaceList();	
	
	// Configure multicast-sockets
		
	int on=1;
	memset(&sockaddr,0,sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(WHMULTICASTPORT);
	sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	for (unsigned int f=0; f<interfaceCount; f++)
	{
		WHdebug("Configuring socket for interface at %08x",interfaceList[f].addr);
	#if WINDOWS
		ioctlsocket(interfaceList[f].sock,FIONBIO,(u_long*)&on); // Set to nonblocking
		setsockopt(interfaceList[f].sock, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on)); // Let's hope this is enough
	#elif MAC
		fcntl(interfaceList[f].sock, F_SETFL, O_NONBLOCK);	  // Make it a non-blocking socket!!!
		setsockopt(interfaceList[f].sock, SOL_SOCKET, SO_REUSEPORT, (char*)&on, sizeof(on)); 
	#endif

		bind(interfaceList[f].sock, (struct sockaddr*)&sockaddr, sizeof(sockaddr)); // Bind to multicast port

		// Add socket to multicast group
		struct ip_mreq mreq;
		memset(&mreq,0,sizeof(mreq));
		mreq.imr_multiaddr.s_addr = inet_addr(WHMULTICASTADDR);
		mreq.imr_interface.s_addr = htonl(interfaceList[f].addr);
		setsockopt(interfaceList[f].sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq));		

		// Interface setup
		struct in_addr addr;         
		memset(&addr,0,sizeof(addr));
		addr.s_addr = htonl(interfaceList[f].addr); //htonl(INADDR_LOOPBACK);  
		setsockopt(interfaceList[f].sock, IPPROTO_IP, IP_MULTICAST_IF, (char*)&addr, sizeof(addr));

		// Enable loopback
		setsockopt(interfaceList[f].sock, IPPROTO_IP, IP_MULTICAST_LOOP, (char*)&on, sizeof(on));

		// Time To Live 255!
	//	long ttl = 255;
	//	setsockopt(interfaceList[f].sock, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&ttl, sizeof(ttl));
	}
	
	memset(&sendaddr,0,sizeof(sendaddr));
	sendaddr.sin_family = AF_INET;
	sendaddr.sin_port = htons(WHMULTICASTPORT);
	sendaddr.sin_addr.s_addr = inet_addr(WHMULTICASTADDR);
	
	unsigned int c;
	for (c=0; c<sizeof(localInstanceList)/sizeof(WHlocalInstance); c++)
	{
		memset(&localInstanceList[c],0,sizeof(WHlocalInstance));
		localInstanceList[c].mode=WH_UNUSED;
	}
	
	// Clear list of open ports.
	for (c=0; c<512; c++)
	{
		memset(&remoteInstanceList[c],0,sizeof(WHremoteInstance));
		remoteInstanceList[c].mode=WH_UNUSED;
	}
	localInstanceCount=0;
	remoteInstanceCount=0;
	
	// Install the timer
#if MAC	
	socketTimerRef = NULL;
	InstallEventLoopTimer(GetMainEventLoop(), WHMULTICASTINTERVAL, WHMULTICASTINTERVAL, NewEventLoopTimerUPP(socketCallBack), this, &socketTimerRef);
#elif WINDOWS
	TIMECAPS tc;
	timeGetDevCaps(&tc, sizeof(tc));
	resolution = min(max(tc.wPeriodMin, 0), tc.wPeriodMax);
	timeBeginPeriod(resolution);
	socketTimerRef = timeSetEvent(int(WHMULTICASTINTERVAL*1000.0), resolution, socketCallBack, (int)this, TIME_PERIODIC);
	if (socketTimerRef==0)
		OutputDebugString("Installing the timer miserably failed!");
#endif	
}

WHStatic::~WHStatic()
{
	// Writing Preferences
	WHprefsSetInt("Mode",(int)lastMode);
	char temp[32];
	for (int i=0; i<kNumParams; i++)
	{
		sprintf(temp,"Param%d",i);
		WHprefsSetFloat(temp, lastParameters[i]);
	}
#if MAC
	WHdebug("[WHStatic] Shutting down Timer");

	if (socketTimerRef) RemoveEventLoopTimer(socketTimerRef);
	for (unsigned int f=0; f<interfaceCount; f++)
		close(interfaceList[f].sock);
#elif WINDOWS
	WHdebug("[WHStatic] Stop timer");
	timeKillEvent(socketTimerRef);
	timeEndPeriod(resolution);
	for (unsigned int f=0; f<interfaceCount; f++)
		closesocket(interfaceList[f].sock);
#endif
}

char* WHStatic::getInterfaceID()
{
	static char undefinedID[]="undefined";
	int activeCount=0;
	char* activeID=NULL;

	for (unsigned int i=0; i<interfaceCount; i++)
		if (interfaceList[i].active) 
		{
			activeCount++;
			activeID=interfaceList[i].id;
		}
	if (activeCount==1)
		return activeID;
	else
		return (char*)undefinedID;
}

void WHStatic::setInterfaceID(char* ifID)
{
	WHdebug("setInterfaceID %s",ifID);
	// Search the interface list for the ID
	unsigned int i;
	for (i=0; i<interfaceCount; i++)
	{
		if (strncmp(interfaceList[i].id, ifID, WH_MAXIFIDSIZE-1)==0)
			break;
	}
	if (i<interfaceCount)
	{
		// Set the right interface to active, all the others to inactive
		for (unsigned int j=0; j<interfaceCount; j++)
			interfaceList[j].active = (i==j);
	}
	else
	{
		// Set all the interfaces to active
		for (unsigned int j=0; j<interfaceCount; j++)
			interfaceList[j].active = true;
	}
	// Debug output the list info
	for (i=0; i<interfaceCount; i++)
		WHdebug("IFList ID: %s Addr: %s Active: %d",interfaceList[i].id, interfaceList[i].name, interfaceList[i].active);

	// Tell all the instances
	for (int k=0; k<localInstanceCount; k++)
	{
		WHlocalInstance* i = &localInstanceList[k];
		if (i->mode != WH_UNUSED && i->callback!=NULL)
			i->callback(WH_CALLBACK_FETCHINTERFACE, i->callbackparam);
	}
}

void WHStatic::setupInterfaceList()
{
	interfaceCount=0;
//	interfaceList.clear();
//	WHinterface newInterface;

#if MAC
	struct ifaddrs* myIflist;
	getifaddrs(&myIflist);
	struct ifaddrs* pos=myIflist;
	while (pos)
	{
		if (pos->ifa_addr && pos->ifa_addr->sa_family==AF_INET)
		{
			sockaddr_in* s_addr=(sockaddr_in*)pos->ifa_addr;
			if (s_addr->sin_addr.s_addr && s_addr->sin_addr.s_addr!=ntohl(INADDR_LOOPBACK))
			{
				strncpy(interfaceList[interfaceCount].id, pos->ifa_name, WH_MAXIFIDSIZE);
				
				strncpy(interfaceList[interfaceCount].name, inet_ntoa(s_addr->sin_addr), 15);
				
				interfaceList[interfaceCount].addr=ntohl(s_addr->sin_addr.s_addr);
				interfaceList[interfaceCount].sock=socket(AF_INET, SOCK_DGRAM ,0);
				interfaceList[interfaceCount].active=true;
//				interfaceList.push_back(newInterface);
				interfaceCount++;
				WHdebug("Found interface %s @ %08x",interfaceList[interfaceCount].id, interfaceList[interfaceCount].addr);
			}
		}
		pos=pos->ifa_next;
	}
	freeifaddrs(myIflist);
#else
	PIP_ADAPTER_INFO pAdapterInfo;
	PIP_ADAPTER_INFO pAdapter = NULL;
	DWORD dwRetVal = 0;

	pAdapterInfo = (IP_ADAPTER_INFO *) malloc( sizeof(IP_ADAPTER_INFO) );
  ULONG	ulOutBufLen = sizeof(IP_ADAPTER_INFO);

	// Make an initial call to GetAdaptersInfo to get
	// the necessary size into the ulOutBufLen variable
	if (GetAdaptersInfo( pAdapterInfo, &ulOutBufLen) != ERROR_SUCCESS) 
	{
		GlobalFree (pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO *) malloc (ulOutBufLen);
	}

	if ((dwRetVal = GetAdaptersInfo( pAdapterInfo, &ulOutBufLen)) == NO_ERROR) 
	{
		pAdapter = pAdapterInfo;
		long addr;
		for ( ; pAdapter; pAdapter = pAdapter->Next) 
		{
			if (addr=inet_addr(pAdapter->IpAddressList.IpAddress.String))
			{
				strncpy(interfaceList[interfaceCount].id, pAdapter->AdapterName, WH_MAXIFIDSIZE-1);
				WHdebug("Found interface %s %08x",pAdapter->IpAddressList.IpAddress.String, ntohl(addr));
				strncpy(interfaceList[interfaceCount].name, pAdapter->IpAddressList.IpAddress.String, 15);
				interfaceList[interfaceCount].addr=ntohl(addr);
				interfaceList[interfaceCount].sock=socket(AF_INET, SOCK_DGRAM ,0);
				interfaceList[interfaceCount].active=true;
				//interfaceList.push_back(newInterface);
				interfaceCount++;
			}
		}
	}
	else 
		WHdebug("GetAdaptersInfo failed!");
#endif

	if (interfaceCount==0)
	{
		strcpy(interfaceList[interfaceCount].id,"localhost");
		interfaceList[interfaceCount].addr=INADDR_LOOPBACK;
		interfaceList[interfaceCount].sock=socket(AF_INET, SOCK_DGRAM ,0);
		interfaceList[interfaceCount].active=true;
		WHdebug("[WHStatic] No active Interfaces detected");
		//interfaceList.push_back(newInterface);
		interfaceCount++;
	}
}

bool WHStatic::isOnThisMachine(long addr)
{
	for (int i=0; i<(int)interfaceCount; i++)
	{
		if (addr==interfaceList[i].addr)
			return true;
	}
	return false;
}

void WHStatic::timeoutPorts()
{
	double time=get_time();
	for (int j=0; j<remoteInstanceCount; j++)
	{
		WHremoteInstance* i=&remoteInstanceList[j];
		if (i->mode != WH_UNUSED)
		{	
			double diff=time - i->lastseen;
			if (diff > WHOPENPORTTIMEOUT)
			{
				WHdebug("[WHStatic] remote Instance timed out addr %08x channel %s mode %d channelhex %08x", i->addr, i->channelName, i->mode, *((long*)&i->channelName));
				if (j==remoteInstanceCount-1) remoteInstanceCount--;
				memset(i,0,sizeof(WHremoteInstance));
				i->mode=WH_UNUSED;
				updateList();
			}
		}
	}

}

void WHStatic::endianConvertPacket(WHportPacket* packet)
{	
	EndianSwap2(packet->mode);
	EndianSwap2(packet->port);
	EndianSwap4(packet->ID);
	EndianSwap4(packet->bufferSize);
	EndianSwap4(packet->flags);
}

void WHStatic::receiveAndProcessPackets()
{
	for (unsigned int f=0; f<interfaceCount; f++)
	{
		if (interfaceList[f].active==false) continue;

#if MAC
		unsigned int addrlen=sizeof(sockaddr);
#else
		int addrlen=(int)sizeof(sockaddr);
#endif
		int cnt;
		while ((cnt = recvfrom(interfaceList[f].sock, sockBuffer, sizeof(sockBuffer), 0, (struct sockaddr*)&sockaddr, &addrlen)) > 0)
		{
		//	WHdebug("Incoming global packet port:%d\n",sockaddr.sin_port);
			double time=get_time();
			long addr=ntohl(sockaddr.sin_addr.s_addr); // Get ip address from recvfrom data
			unsigned short globalPort=ntohs(sockaddr.sin_port); // get global port
			if (isOnThisMachine(addr)) addr=INADDR_LOOPBACK; // Use loopback device on local machine!
			char* buffer=sockBuffer;

			while (cnt>0)
			{
				WHportPacket* thePacket=(WHportPacket*)buffer;
				buffer+=sizeof(WHportPacket);
				cnt-=sizeof(WHportPacket);
				
				if (thePacket->endian==0xff00) 
					endianConvertPacket(thePacket);
				
				// Search for instance by ID & ip-address
				bool found=false;
			//	vector<WHremoteInstance>::iterator i;
				for (int j=0; j<remoteInstanceCount; j++)
				{
					WHremoteInstance* i=&remoteInstanceList[j];
					if (i->ID == thePacket->ID && i->addr == addr)
					{
						found=true;
						if (thePacket->mode == WH_RESET)
						{
							localReset(thePacket->channelName, thePacket->ID);
						}
						// Check & Update data for this instance
						if (strncmp(i->channelName, thePacket->channelName, WHMAXPORTNAMESIZE)!=0)
						{
							strncpy(i->channelName,thePacket->channelName,WHMAXPORTNAMESIZE-1);
							updateList(); // notify
						}
						if (i->mode != thePacket->mode)
						{
							i->mode=thePacket->mode;
							updateList(); // notify
						}
				/*		if (i->port != thePacket->port) // currently not possible
						{
							i->port=thePacket->port;
							updateList(); // notify
						}*/
						// Update timestamp & data
						if (thePacket->mode == WH_DESTROY)
						{
							// Force remove this instance
							WHdebug("[WHStatic] Received destroy message: Channel %s addr %08x mode %d",thePacket->channelName,i->addr, i->mode);
		//					i->lastseen = -WHOPENPORTTIMEOUT;
							if (j==remoteInstanceCount-1) remoteInstanceCount--;
							memset(i,0,sizeof(WHremoteInstance));
							i->mode=WH_UNUSED;
						}
						else
						{
								i->lastseen=time; // Update timestamp 
						}
						
						if (i->bufferSize!=thePacket->bufferSize)
							i->bufferSize=thePacket->bufferSize;
						if ((i->flags & WHFLAG_SYNCED) != (thePacket->flags & WHFLAG_SYNCED))
							updateList();
						i->flags=thePacket->flags;
						i->globalPort=globalPort; // Set peer's global instance port number
						break;
					}
				}
		
				if (found==false)
				{
					int k;
					for (k=0; k<remoteInstanceCount; k++)
						if (remoteInstanceList[k].mode==WH_UNUSED) break;

					WHremoteInstance* newInstance=&remoteInstanceList[k];
					strncpy(newInstance->channelName, thePacket->channelName, WHMAXPORTNAMESIZE-1);
					newInstance->mode = thePacket->mode;
					newInstance->addr = addr;
					newInstance->ID = thePacket->ID;
					newInstance->port = thePacket->port;
					newInstance->bufferSize = thePacket->bufferSize;
					newInstance->flags = thePacket->flags;
					newInstance->lastseen = time;
					newInstance->globalPort = globalPort;
					
					if (k==remoteInstanceCount) remoteInstanceCount++;
					
					WHdebug("[WHStatic] New remote instance detected");
			//		WHdebug("[WHStatic] name %s mode %d addr %08x id %d port %d", newInstance.channelName, newInstance.mode, newInstance.addr, newInstance.ID, newInstance.port);
					updateList(); // notify
				}
			}
		}
	}
}

WHremoteInstance* WHStatic::getRemoteInstance(char* name, unsigned short mode, long callerID)
{
	// Find an instance with the right name & mode, but not the instance which is calling
	for (int j=0; j<remoteInstanceCount; j++)
	{
		WHremoteInstance* i=&remoteInstanceList[j];
		if (i->mode==mode && strncmp(i->channelName,name,WHMAXPORTNAMESIZE-1)==0 && (i->addr!=INADDR_LOOPBACK || i->ID!=callerID))
		{
//			WHdebug("[WHstatic] Found a target instance for channel %s mode %d id %d",name,mode,i->ID);
			return i;
		}
	}
	return NULL;
}

float WHStatic::getBufferSize(char* name, unsigned short mode)
{
	WHremoteInstance* i=getRemoteInstance(name, mode, 0);
	if (i) return i->bufferSize;
	else return 0.0f;
}

bool WHStatic::isSenderActive(char* name, unsigned short mode)
{
	WHremoteInstance* i=getRemoteInstance(name, mode, 0);
	if (i) return (((i->flags) & WHFLAG_SENDERACTIVE) !=0 );
	else return false;
}

bool WHStatic::isLinked(char* name, unsigned short mode)
{
	WHremoteInstance* i=getRemoteInstance(name, mode, 0);
	if (i) return (((i->flags) & WHFLAG_SYNCED)!=0);
	else return false;
}

bool WHStatic::isReceiverConnected(char* name, unsigned short mode)
{
	WHremoteInstance* i=getRemoteInstance(name, mode, 0);
	if (i) return (((i->flags) & WHFLAG_RECEIVERCONNECTED)!=0);
	else return false;
}

// Called by Origin mode only to see is the loop is complete
bool WHStatic::isLoopComplete(char* name)
{
	for (int j=0; j<remoteInstanceCount; j++)
	{
		if (remoteInstanceList[j].mode==WH_DESTINATION && strncmp(remoteInstanceList[j].channelName, name, WHMAXPORTNAMESIZE-1)==0)
		{
			for (int k=j; k<remoteInstanceCount; k++)
				if (remoteInstanceList[k].mode==WH_LOOPBACK && strncmp(remoteInstanceList[j].channelName, name, WHMAXPORTNAMESIZE-1)==0)
					return true;
			return false;
		}
		if (remoteInstanceList[j].mode==WH_LOOPBACK && strncmp(remoteInstanceList[j].channelName, name, WHMAXPORTNAMESIZE-1)==0)
		{
			for (int k=j; k<remoteInstanceCount; k++)
				if (remoteInstanceList[k].mode==WH_DESTINATION && strncmp(remoteInstanceList[j].channelName, name, WHMAXPORTNAMESIZE-1)==0)
					return true;
			return false;
		}
	}
	return false;	
}

unsigned short WHStatic::getAMode(char* name)
{
	// Finds a mode for a channel name
	for (int j=0; j<remoteInstanceCount; j++)
	{
		WHremoteInstance* i=&remoteInstanceList[j];
		if (i->mode<WH_NUMMODES && strncmp(i->channelName,name,WHMAXPORTNAMESIZE-1)==0)
		{
			return i->mode;
		}
	}
	return WH_INVALIDMODE;
}

bool WHStatic::getModes(char* name, WHremoteInstance** modeList)
{
	memset(modeList, 0, sizeof(WHremoteInstance*)*WH_NUMMODES);
	// Finds a mode for a channel name
	bool found=false;
	for (int j=0; j<remoteInstanceCount; j++)
	{
		WHremoteInstance* i=&remoteInstanceList[j];
		if (i->mode<WH_NUMMODES && strncmp(i->channelName,name,WHMAXPORTNAMESIZE-1)==0)
		{
			modeList[i->mode]=&(*i);
			found=true;
		}
	}
	return found;
}

void WHStatic::invalidateTickOffsets()
{
	for (int j=0; j<localInstanceCount; j++)
		localInstanceList[j].tickOffset=WH_INVALIDTICKOFFSET;
}

int WHStatic::getSetLinkedTickOffset(int* offset, WHlocalInstance* local)
{
	// Find the instance we receive from
	WHremoteInstance* i=getRemoteInstance(local->channelName, WH_START, 0);
	
	if (i==NULL) 
	{
		WHdebug("[WHStatic] getLinkedTickOffset: Connected start instance not found");
		local->tickOffset=*offset;
		return 0;
	}
	
	if ((i->flags & WHFLAG_SYNCED) == 0)
	{
		local->callback(WH_CALLBACK_SLAVEMESSAGE, local->callbackparam);
		//WHdebug("[WHStatic] getLinkedTickOffset: Remote start instance is not synced/linked");
		local->tickOffset=*offset;
		return 0;
	}

	int count=0;
	int result=WH_INVALIDTICKOFFSET;
	for (int k=0; k<localInstanceCount; k++)
	{
		WHlocalInstance* j=&localInstanceList[k];
		WHdebug("remoteInstance mode %d remoteAddr %08x remoteGlobalport %d tickoffset %d",j->mode,j->remoteAddr, j->remoteGlobalPort, j->tickOffset);
		if (j->mode==WH_END && j->remoteAddr == i->addr && j->remoteGlobalPort==i->globalPort) // Coming from the right host
		{
			if (j->tickOffset!=WH_INVALIDTICKOFFSET)
			{
				result=j->tickOffset;
			}
			count++;
		}
	}

	WHdebug("[WHStatic] getLinkedTickOffset: %d linked end instances found",count);
	
	if (result!=WH_INVALIDTICKOFFSET)
	{
		local->tickOffset=result;
		*offset=result;
		count=1; // Let status know syncing is possible
	}
	else
	{
		WHdebug("GetSetLinkedTickOffset: storing offset from this instance offset %d",*offset);
		local->tickOffset=*offset; // Save offset to the list
		count=1; // Let status know syncing is possible
	}
	return count;
}

int compareInstances(const void* a, const void* b)
{
	return strncmp(*((char**)a), *((char**)b), WHMAXPORTNAMESIZE-1);
}

int WHStatic::getChannelList(char ***list, int* numDirect)
{
	// Will need sorting and deletion of identical entries...
	static char* theList[512]; // Let's hope that's enuff
	static char* theList2[512];

	*numDirect=0;

	int c=0;
	// Make a list of all the channel names
	int j;
	for (j=0; j<remoteInstanceCount; j++)
	{
		if (remoteInstanceList[j].mode< WH_NUMMODES)
			theList[c++]=remoteInstanceList[j].channelName;
	}
	
	if (c==0) return 0;
	
	// Sort strings alphabetically
	
	qsort(theList,c,sizeof(char*),compareInstances);
		
	// get rid of duplicates
	
	int count=0;
	theList2[count++]=theList[0];
	for (j=1; j<c; j++)
	{
		if (strncmp(theList[j-1],theList[j],WHMAXPORTNAMESIZE-1)!=0)
			theList2[count++]=theList[j];
	}
					
/*	// Remove complete channels
	
	c=0;
	for (j=0; j<count; j++)
	{
		if (!(((getAddr(theList2[j],WH_START,0,NULL,NULL) && getAddr(theList2[j],WH_END,0,NULL,NULL)) ||
			 ((getAddr(theList2[j],WH_ORIGIN,0,NULL,NULL) && getAddr(theList2[j],WH_DESTINATION,0,NULL,NULL) && getAddr(theList2[j],WH_LOOPBACK,0,NULL,NULL))))))
			 theList[c++]=theList2[j];
	}*/
	
	// Add modes
	
	openChannelCount=0;
//	menuListEntry temp;
	for (j=0; j<count; j++)
	{
		char* current=theList2[j];
		
		WHremoteInstance* modeList[WH_NUMMODES];
		getModes(current,modeList);
		
		if (modeList[WH_END] && !modeList[WH_START])
		{
			sprintf(openChannelList[openChannelCount].name,"  %s - %s",current,whModeNames[WH_START]);
			//openChannelList.push_back(temp);
			openChannelCount++;
		}
		else if (modeList[WH_START] && !modeList[WH_END])
		{
			sprintf(openChannelList[openChannelCount].name,"  %s - %s",current,whModeNames[WH_END]);
			//openChannelList.push_back(temp);
			openChannelCount++;
		}
	}
	*numDirect=openChannelCount;
	for (j=0; j<count; j++)
	{
		char* current=theList2[j];
		
		WHremoteInstance* modeList[WH_NUMMODES];
		getModes(current,modeList);
			
		if (!modeList[WH_ORIGIN] && (modeList[WH_DESTINATION] || modeList[WH_LOOPBACK]))
		{
			sprintf(openChannelList[openChannelCount].name,"  %s - %s",current,whModeNames[WH_ORIGIN]);
			//openChannelList.push_back(temp);
			openChannelCount++;
		}
		if (!modeList[WH_DESTINATION] && (modeList[WH_ORIGIN] || modeList[WH_LOOPBACK]))
		{
			sprintf(openChannelList[openChannelCount].name,"  %s - %s",current,whModeNames[WH_DESTINATION]);
			//openChannelList.push_back(temp);
			openChannelCount++;
		}
		if (!modeList[WH_LOOPBACK] && (modeList[WH_ORIGIN] || modeList[WH_DESTINATION]))
		{
			sprintf(openChannelList[openChannelCount].name,"  %s - %s",current,whModeNames[WH_LOOPBACK]);
			//openChannelList.push_back(temp);
			openChannelCount++;
		}			
	}
	
	// Make pointer array

	c=0;	
	for (j=0; j<(int)openChannelCount; j++)
		theList[c++]=openChannelList[j].name;
	
	*list=theList;
	return c;
}

void WHStatic::updateList()
{
	updateListNextTime=true;
}

void WHStatic::sendAnnouncePackets()
{
	int count=0;
	//vector<WHlocalInstance>::iterator i;
	for (int k=0; k<localInstanceCount; k++)
	{
		WHlocalInstance* i=&localInstanceList[k];
			// check for not running-ness
		if ((i->ID != 0) && ((i->flags & WHFLAG_NOTPROCESSED)!=0))
		{
			i->callback(WH_CALLBACK_NOTRUNNING, i->callbackparam);
		}
		i->flags |= WHFLAG_NOTPROCESSED;

		if (i->channelName[0]!=0) // Don't transmit channels without names
		{

			// Add to send packet
			memcpy(&sockBuffer[count], i, sizeof(WHportPacket));
			if (i->flags & WHFLAG_RESET) // make this a reset message?
			{
				((WHportPacket*)(&sockBuffer[count]))->mode=WH_RESET; // make this a reset packet
				i->flags &= (~WHFLAG_RESET); // Zero flag
			}
			count+=sizeof(WHportPacket);
		}
		else if (i->flags & WHFLAG_DESTROY) //... unless destroy flag is set
		{
			memcpy(&sockBuffer[count],&(*i),sizeof(WHportPacket));
			((WHportPacket*)(&sockBuffer[count]))->mode=WH_DESTROY; // make this a destroy packet
			count+=sizeof(WHportPacket);
			i->flags &= (~WHFLAG_DESTROY); // Zero flag
		}
	}
	if (count>0)
	{
		// Send to all interfaces
		for (unsigned int f=0; f<interfaceCount; f++)
		{
			if (interfaceList[f].active)
				sendto(interfaceList[f].sock, sockBuffer, count, 0, (struct sockaddr*)&sendaddr, sizeof(sendaddr));
		}
	}
}

WHlocalInstance* WHStatic::initAnnouncement(long ID, char* name, unsigned short mode, unsigned short port, float bufferSize, void* callbackparam)
{
	WHdebug("[WHStatic] New announcement Packet added ID %d name %s mode %d port %d",ID,name,mode,port);
	int i;
	for (i=0; i<localInstanceCount; i++)
		if (localInstanceList[i].mode==WH_UNUSED) break;
	
	WHlocalInstance* newInstance=&localInstanceList[i];
	newInstance->ID=ID;
	strncpy(newInstance->channelName, name, WHMAXPORTNAMESIZE-1);
	newInstance->endian=0x00FF;
	newInstance->mode=mode;
	newInstance->port=port;
	newInstance->bufferSize=bufferSize;
	newInstance->flags=0;
	newInstance->remoteAddr=0;
	newInstance->remoteGlobalPort=0;
	newInstance->param2=-1.0f; // To remotely update buffersize
	newInstance->tickOffset=WH_INVALIDTICKOFFSET;
	newInstance->callbackparam=callbackparam;
	newInstance->callback=callback;

	if (i==localInstanceCount) localInstanceCount++;

	return newInstance;
}

void WHStatic::exitAnnouncement(WHlocalInstance* i)
{
	WHdebug("[WHStatic] Remove announcement ID %d name %s mode %d port %d", i->ID, i->channelName, i->mode, i->port);	
	if (remoteInstanceCount>0 && (&localInstanceList[remoteInstanceCount-1]==i))
		remoteInstanceCount--;
	memset(i, 0, sizeof(WHlocalInstance));
	i->mode=WH_UNUSED;
}

void WHStatic::updateLinkedBufferSize(WHlocalInstance* local, float value)
{
	updateingParams=true;
//	bool changed=false;
	for (int k=0; k<localInstanceCount; k++)
	{
		WHlocalInstance* i = &localInstanceList[k];
		if (i->mode==WH_END && i->remoteGlobalPort==local->remoteGlobalPort && i->remoteAddr==local->remoteAddr && i!=local)
		{
			i->param2=value;
			i->callback(WH_CALLBACK_SETBUFFERSIZE,i->callbackparam);
//			changed=true;
		}
	}
//	if (changed)
//		updateList();
	updateingParams=false;
}

void WHStatic::updateLinkedDelay(WHlocalInstance* local, float value)
{
	updateingParams=true;
//	bool changed=false;
	for (int k=0; k<localInstanceCount; k++)
	{
		WHlocalInstance* i = &localInstanceList[k];
		if (i->mode==WH_START && i->remoteGlobalPort==local->remoteGlobalPort && i->remoteAddr==local->remoteAddr && i!=local)
		{
			i->param2=value;
			i->callback(WH_CALLBACK_SETDELAY,i->callbackparam);
		//	changed=true;
		}
	}
//	if (changed)
//		updateList();
	updateingParams=false;
}

// Update local Instances

void WHStatic::localUpdate()
{
	for (int k=0; k<localInstanceCount; k++)
	{
		WHlocalInstance* i=&localInstanceList[k];
		if (i->callbackparam)
			i->callback(WH_CALLBACK_UPDATELIST, i->callbackparam);
	}
}

// Reset channelnames of local instances on channel channelName, but not the ID

void  WHStatic::localReset(char* channelName, long ID)
{
	for (int k=0; k<localInstanceCount; k++)
	{
		WHlocalInstance* i=&localInstanceList[k];
		if (i->callbackparam && i->ID!=ID && strncmp(i->channelName, channelName, WHMAXPORTNAMESIZE-1)==0)
			i->callback(WH_CALLBACK_RESET, i->callbackparam);
	}
}

