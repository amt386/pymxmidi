# pymxmidi
Python MIDI tools

> Author: Artyom Panov  

This package contains wrappers for accessing the [ALSA](http://www.alsa-project.org/) API from Python.

There are some tools included:

* [midisend.py](https://github.com/amt386/pymxmidi/blob/master/midisend.py) command-line tool to send MIDI events to the specified ALSA sequencer port


# Installation

Install ALSA library and headers (`libasound.so` and `asoundlib.h`). On Ubuntu you can do this with the following command:

```
  $ sudo apt-get install libasound2-dev
```

Get the sources and change to the source directory:
```
  $ git clone https://github.com/amt386/pymxmidi.git
  $ cd pymxmidi
```
  
Then, build and install:
```
  $ sudo python3 setup.py install
```
