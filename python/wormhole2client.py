#!/usr/bin/python
# -*- coding: utf-8 -*-
"""
This is a simple client implementation of a Wormhole2 audio receiver.
It announces itself, listens to multicastand calls a callback function when audio data has arrived.

2018-11-27 Bernhard "HotKey" Slawik
"""

import socket
import binascii
import struct
import time
#import thread
import threading

def put(txt):
	print(str(txt))

def put_debug(t):
	pass

### Local info
WHLOCALPORT = 48102
WHREMOTEPORT = 48101
MULTICAST_TTL = 2

WHMULTICASTADDR_SEND_BIND = ''	#WHMULTICASTADDR
WHMULTICASTADDR_LISTEN_BIND = ''	#WHMULTICASTADDR

### Wormhole2 info
WHMULTICASTADDR = '239.111.231.77'
WHMULTICASTPORT = 48100
WHMULTICASTINTERVAL = 0.4

### WHHeaders
WHAUDIOPACKETID = 0x4144494F	#b'ADIO'
WHAUDIOPACKETIDINV = 0x4F494441	#b'OIDA'
WHMIDIPACKETID = 0x4d494449	#b'MIDI'

WHMAXPORTNAMESIZE = 92

WHMINPACKETSIZE = 512
WHRECBUFFERSIZE = 0x20000
WHRECMSGBUFSIZE = 0x10000
WH_INVALIDTICKOFFSET = 0x10000000
WH_MAXIFIDSIZE = 256

# The 5 modes
WH_START = 0
WH_END = 1
WH_ORIGIN = 2
WH_DESTINATION = 3
WH_LOOPBACK = 4
WH_NUMMODES = 5
WH_UNUSED = 6

WH_INVALIDMODE = 200
WH_DESTROY = 100 # Remove this Instance from the list
WH_RESET = 150 # Reset the channel this instance is on

WHFLAG_SENDERACTIVE = 0x01
WHFLAG_RECEIVERCONNECTED	= 0x02
WHFLAG_SYNCED = 0x04
WHFLAG_DESTROY = 0x08
WHFLAG_RESET = 0x10
WHFLAG_NOTPROCESSED = 0x20



class Wormhole2Client:
	def __init__(self, name=__file__, on_data=None):
		self.name = name
		self.on_data = on_data	# Callback
		self.mc_sock = None
		self.running = False
	
	def start(self):
		self.running = True
		
		self.start_mc_listen()
		self.start_local_listen()
		self.start_announce()
	
	def stop(self):
		self.running = False
	
	def start_mc_listen(self):
		"Start listening for Wormhole2 port packets on multicast address"
		
		### Listen to Wormhole2 Multicast
		# Info from https://stackoverflow.com/questions/603852/how-do-you-udp-multicast-in-python
		self.mc_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
		try:
			self.mc_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
		except AttributeError:
			pass
		self.mc_sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, MULTICAST_TTL) 
		self.mc_sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_LOOP, 1)
		
		self.mc_sock.bind((WHMULTICASTADDR_LISTEN_BIND, WHMULTICASTPORT))
		host = socket.gethostbyname(socket.gethostname())
		self.mc_sock.setsockopt(socket.SOL_IP, socket.IP_MULTICAST_IF, socket.inet_aton(host))
		self.mc_sock.setsockopt(socket.SOL_IP, socket.IP_ADD_MEMBERSHIP, socket.inet_aton(WHMULTICASTADDR) + socket.inet_aton(host))
		
		#thread.start_new_thread(self._mc_listen_thread, ())
		threading.Thread(target=self._mc_listen_thread).start()
	
	def _mc_listen_thread(self):
		put('Listening for multicast')
		while self.running:
			m = self.mc_sock.recvfrom(1024)
			#put('RECV: %s' % (str(m)))
			data, remote_address = m
			
			if (len(data) == 112):
				self.handle_mc_port_packet(data, remote_address)
			else:
				put('RECV (%d)' % (len(data)))
				#put('RECV (%d): %s' % (len(data), data))
				#put('RECV (%d): %s' % (len(data), binascii.hexlify(data)))

	def handle_mc_port_packet(self, data, remote_address):
		# Parse header
		port_packet = struct.unpack('%dsHHHLfi' % WHMAXPORTNAMESIZE, data)
		channel_name, endian, mode, port, id, buffer_size, flags = port_packet
		
		# channel_name = Name of instance
		# endian = 0x00ff / 0xff00 to determine endianness
		# mode = 0=isStartPoint 1=isEndPoint
		# port = UDP port of this channel
		# id = uid
		
		channel_name = str(channel_name)
		if chr(0) in channel_name:
			channel_name = channel_name[0:channel_name.index(chr(0))]
		put_debug('Got Port Packet: channel_name="%s", endian=%04x, mode=%d, port=%d, id=%04x, buffer_size=%d, flags=%d' % (channel_name, endian, mode, port, id, buffer_size, flags))
		
	
	def start_announce(self):
		#thread.start_new_thread(self._announce_thread, ())
		threading.Thread(target=self._announce_thread).start()
	
	def _announce_thread(self):
		while self.running:
			time.sleep(WHMULTICASTINTERVAL)
			self.send_announce_packet()
	
	def send_announce_packet(self):
		
		channel_name = self.name
		channel_name_padded = ''
		for i in range(WHMAXPORTNAMESIZE):
			if (i < len(channel_name)):
				channel_name_padded += channel_name[i]
			else:
				channel_name_padded += chr(0)
		
		endian = 0x00ff
		mode = 1
		port = WHLOCALPORT
		id = 0x0002
		buffer_size = 0
		flags = WHFLAG_NOTPROCESSED
		data = struct.pack('%dsHHHLfi' % WHMAXPORTNAMESIZE, bytes(channel_name_padded, 'utf8'), endian, mode, port, id, buffer_size, flags)
		
		#put(binascii.hexlify(data))
		#put(str(data))
		
		put_debug('Announcing')
		sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
		try:
			sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
		except AttributeError:
			pass
		sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, MULTICAST_TTL) 
		#sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_LOOP, 1)
		
		sock.bind((WHMULTICASTADDR_SEND_BIND, WHMULTICASTPORT))
		host = socket.gethostbyname(socket.gethostname())
		sock.setsockopt(socket.SOL_IP, socket.IP_MULTICAST_IF, socket.inet_aton(host))
		sock.setsockopt(socket.SOL_IP, socket.IP_ADD_MEMBERSHIP, socket.inet_aton(WHMULTICASTADDR) + socket.inet_aton(host))
		sock.sendto(data, (WHMULTICASTADDR, WHMULTICASTPORT))
	
	
	def start_local_listen(self):
		"Start listening for incoming packets"
		
		self.local_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
		#try:
		#	self.local_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
		#except AttributeError:
		#	pass
		#self.local_sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, MULTICAST_TTL) 
		#self.local_sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_LOOP, 1)
		
		self.local_sock.bind((WHMULTICASTADDR_LISTEN_BIND, WHLOCALPORT))
		#host = socket.gethostbyname(socket.gethostname())
		#self.local_sock.setsockopt(socket.SOL_IP, socket.IP_MULTICAST_IF, socket.inet_aton(host))
		#self.local_sock.setsockopt(socket.SOL_IP, socket.IP_ADD_MEMBERSHIP, socket.inet_aton(WHMULTICASTADDR) + socket.inet_aton(host))
		
		#thread.start_new_thread(self._local_listen_thread, ())
		threading.Thread(target=self._local_listen_thread).start()
	
	def _local_listen_thread(self):
		put('Listening for data')
		while self.running:
			m = self.local_sock.recvfrom(1024)
			#put('RECV: %s' % (str(m)))
			data, remote_address = m
			
			if (len(data) > WHMINPACKETSIZE):
				self.handle_local_packet(data, remote_address)
			else:
				#put('RECV (%d)' % (len(data)))
				#put('RECV (%d): %s' % (len(data), data))
				put('RECV (%d): %s' % (len(data), binascii.hexlify(data)))
			
	
	def handle_local_packet(self, data, remote_address):
		#put('Got %d bytes from %s' % (len(data), str(remote_address)))
		
		# Parse header
		id, endian = struct.unpack('LH', data[:6])
		#put('Got Packet: id=%04X from %s' % (id, str(remote_address)))
		
		# Parse according to id
		if id == WHAUDIOPACKETID:
			# Audio packet!
			
			#put(binascii.hexlify(data[0:32]))
			
			#size, sample_rate, channels, frames, u2, tick = struct.unpack('HfHLHL', data[6:6+18])
			size = struct.unpack('H', data[6:6+2])[0]
			sample_rate = struct.unpack('f', data[8:8+4])[0]
			channels = struct.unpack('H', data[12:12+2])[0]
			frames = struct.unpack('L', data[16:16+4])[0]
			tick = struct.unpack('L', data[20:20+4])[0]
			
			payload = data[24:]
			put('Got audio: size=%d, sample_rate=%f, channels=%d, frames=%d, tick=%d, payload_size=%d' % (size, sample_rate, channels, frames, tick, len(payload)))
			#put(binascii.hexlify(payload))
			if self.on_data is not None:
				self.on_data(channels, sample_rate, payload)
			
			
		else:
			put('Got unhandled packet id %04X' % (id))
		

if __name__ == '__main__':
	
	whc = Wormhole2Client()
	
	whc.start()
	
	while True:
		time.sleep(1)
	
	whc.stop()
	
	put('EOF.')
