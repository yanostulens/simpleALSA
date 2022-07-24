# SimpleALSA
The purpose of this library is to provide an easy to use wrapper around the rather complicated ALSA API's for audio playback. In its essence, simpleALSA provides the user with an easy to use callback system that sits right on top of ALSA, which allows for audio playback in a timely manner. This library is heavily focused towards efficiency which makes it a suitible option for embedded devices.

Want to give it a try? Have a look at the example.c file! 
Make sure to compile with: `-lasound -lsndfile -lpthread` or just use the included Makefile!

At the moment simpleALSA.h is working, it is however still under construction, so the API might still change.

Credits must be given to the miniaudio library (https://github.com/mackron/miniaudio), as simpleALSA work in a similar way.



