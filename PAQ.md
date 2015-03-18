Wormhole2 PAQ (Pre-emptively Answered Questions)

**Q. Does WH2 run on intel-macs?**

A. Yes. Please download the latest version to get support for intel macs.


---


**Q. WH2 doesn't show up in (add windows vst host here)...?**

A. It seems this is caused by some missing DLLs on windows installations. Wormhole2.0 used quite a few. Starting from v2.0.2 Wormhole2 only relies on two dlls: wsock32.dll and mmsystem.dll which are very important for many applications. Please download the latest version of Wormhole2 from here.


---


**Q. WH2 does not show up on OSX 10.3...?**

A. Please download version 2.0.3 or later for mac from here.


---


**Q. Why can't I make a connection?**

A1. Before any connection can be made, the Wormhole2 channel needs to be given a name. This is necessary for the connection negotiation inside Wormhole2.

A2. The network might not be set up right. If any other network service like sharing files work, you are all set for WH2. What is important is that your network is a LAN and all the machines need to be on the same subnet (their IP addresses need to start with the same 2/3 numbers, depending on your subnet mask).

A3. Some network device (like a firewall) might be blocking the WH2 packets. In that case, please find out what type of firewall you are running and search the web for how to open ports 48100-48200 in that firewall.


---


**Q. What are good buffer settings for Wormhole2?**

A. There are two kinds of relevant buffer settings with Wormhole2. The buffer settings you use in your host(s) are important as well as the buffer setting within the WH2 instances. For the host buffers it is recommended to use 1024 samples or less, preferably 512 samples. The Wormhole2 buffer setting should always be higher than the respective hosts buffer setting, otherwise dropouts might happen.


---


**Q. Can I use Wormhole2 to send audio from application to application on the same computer?**

A. Yes, that is possible as every modern operating system has a network loopback device, making it possible to send network packets from one application to another. However, you still have to deal with network latency when using Wormhole2 on just one machine. As a matter of fact the network latency is worse with just one machine as its network stack has to deal with both sending and receiving.


---


**Q. Can I send midi over the network with Wormhole2?**

A. No, if you need to do this cross-platform, please check out MidiOverLanCP. For mac, only, get Tiger and use the built-in network midi feature.


---


**Q. How do I get the lowest possible latency for a "live" connection (direct mode)?**

A. Move the latency slider to 0 in the sending instance. That causes WH2 to tell the host it has 0 latency which you need it such a case. You might have to hit stop/start in your host to make it scan the latency again. Use low audio buffer settings (256 samples) and move the buffer slider to a low value (about 400 samples) in the receiving WH2.


---


**Q. Does WH2 require gigabit ethernet? Does it work better on gigabit ethernet?**

A. The only advantage of using GigaBit (1000mBit) ethernet with WH2 is the max number of channels you can transfer at the same time. A 100 mBit connection already gives you about 30 mono channels, so it's normally sufficient.


---


**Q. Does Wormhole2 work with multiple sample rates running on multiple machines?**

A. No. In order to stay connected, Wormhole requires all machines to run with the same hardware audio sample rate. It is recommended to even connect the hardware sample clocks of multiple machines if possible.


---


**Q. How can I get rid of "the host is not feeding audio to Wormhole"?**

A. If you have a resource saving host like Logic or DP4.5+, plugins which are not feed audio are not calculated. When using Wormhole to receive audio, this can be a problem as then you basically have a plugin which doesn't need input and still creates output. In such a case, use WH2 on another type of track, like a aux track / input track or insert some tiny instrument and use WH2 after that, to have a valid stream of audio. There is something called DummyInst just for that purpose. Get it here: http://www.apulsoft.ch/dummyinst.zip


---


**Q. Why is there a gap in the beginning of the audio when logic cycles a section?**

A. Wormhole2 always tells the hosts it has 32768 samples latency (unless the latency fader is on zero). This is because most hosts cannot adapt to dynamical changing plugin latencies and that's exactly what WH2 needs. So WH2 has to always go for the max and make up for the rest by itself. There is a problem in Logic with the loop mode & latency compensation. Obviously hosts need to send audio in advance to make up for latency, however logic fails to do that at the end of the cycle. Instead of sending the beginning of the loop (the real future audio) it sends silence and that's why you get a gap of 32000 samples with logics cycle


---


**Q. Why does WH2 in auto latency mode remeasure the latency if I have a non-continuos audio track (a track with just some blocks of audio and big gaps in between)?**

A. This behavior happens with all hosts which turn off plugins when they think they are not needed (to save cpu). Most famous hosts doing that are: Logic (mac&windows), Digital Performer. WH2 needs to remeasure the latency in such a case because the connection is obviously lost when those hosts just turn off the plugin. While this is a bit of a drag (and can't be circumvented) it saves cpu on the other hand. To avoid the gaps produced by the remeasuring, place some empty audio before the sections you really need or use WH2 on another type of channel which is not muted (like aux channels).


---


**Q. WH2 (insert chain mode) obviously did not measure/make up for the latency right. What can I do?**

A1. Make sure you have a host with delay compensation and make sure you have turned it on. Some hosts (logic5&6) don't have delay compensation on all types of channels. If your host does not have delay compensation, you could still use WH2 and delay all the non-wormholed channels by 32768 samples per instance (a unused WH2 with play through enabled and the latency slider on non-zero position does just that.)

A2. If you changed the plugins in between the "before" and the "after" instance of WH2, you have to make WH2 remeasure the latency as it might have changed and there is no way for the "insert" instance to know that.  To force WH2 to rescan, toggle the "auto" button twice.

A3. There are some types of plugins which lead to wrong results of the measurement. The worst are delays as they lead to weird pings coming back through the loop. To figure out which plugins can work, just imagine how the measurement works. In auto mode, WH2 first waits for the network loop to be complete, then it waits until silence if coming through the loop. After that, a short impulse is sent through the loop and WH2 counts the samples until it comes back and then waits for silence again in order to mute reverb tails caused by the impulse. For instants a delay can throw it off because multiple weird impulses might come back through the loop.


---


**Q. I see audio is coming into WH2 (the leds blink), but I still can't hear it in DP4.5+. What is wrong?**

A. DP4.5+ not only doesn't calculate plugins it thinks to be not needed, it also mutes tracks it thinks to be silent. In DP4.5+, use an aux track or an instrument track with DummyInst as explained above.