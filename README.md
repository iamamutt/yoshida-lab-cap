yoshida-lab-cap
===============

## Description

A small cross-platform program that will capture multiple usb and ip cameras simultaneously and sync them at some specified frame rate. Requires the OpenCV and Boost libraries. You'll also need to add the opencv_ffmpeg.dll to your path or in the same folder as the release in order to write videos to a file.

- Eye-tracking camera sync
- Wall/ceiling camera sync

## External Dependencies

- OpenCV
- Boost
- RtAudio
- cmake

### OpenCV

Download at least version 3.0 BETA. You will need to install `ffmpeg` first if on OSX.
If on Windows, the `opencv_ffmpeg300_64.dll` should already be included.

### Boost

Downloaded directly from the website on Windows and followed install instructions, used homebrew for OSX.

I built boost on Windows using the following:

`b2.exe --toolset=msvc variant=release address-model=64 link=static threading=multi runtime-link=static stage`

### RtAudio

Can be found here: [https://www.music.mcgill.ca/~gary/rtaudio/](https://www.music.mcgill.ca/~gary/rtaudio/)

I built using cmake with the DirectSound option on Windows and the Core option on OSX.

You should either create an environment variable called `RTAUDIO` which points to the built static library, or place it in a location that can be easily found, such as `/usr/local/lib` on Mac.

On Windows, if using DirectSound, you will need the files `dsound.h` and the dsound.lib`, which can be obtained from the Windows SDK.

Place `RtAudio.h` in the include folder, and if on Windows, also place `dsound.h` in this folder as well.

Also on Windows, place `rtaudio_static.lib` and `dsound.lib` in the lib folder.



