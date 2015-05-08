yoshida-lab-cap
===============

## Description

A small cross-platform program that will capture multiple USB and IP cameras simultaneously and sync frames according to some common frame rate. Requires the Booost, OpenCV, and RTAudio libraries. You'll also need to add the `opencv_ffmpeg.dll` to your path or in the same folder as the release in order to write videos with non-standard codecs to a file.

The program in used primarily in our lab for syncing:

- Eye-tracking cameras (4 analog to usb capture cards)
- Wall/ceiling cameras (2 Axis IP cameras)

## External Dependencies

- OpenCV (3.0)
- Boost (1.58)
- RtAudio (4.1.1)
- cmake (3.2)

### OpenCV

Download at least version 3.0 BETA. You will need to install `ffmpeg` first if on OSX.
If on Windows, the `opencv_ffmpeg300_64.dll` should already be included. You should make sure all paths to the OpenCV libraries can be found, including any third-party libraries and binaries. You can place the ffmpeg.dll in the lib folder and it should be located by cmake.

### Boost

Downloaded directly from the website on Windows and followed install instructions, used homebrew for OSX.

I built boost on Windows using the following:

`b2.exe --toolset=msvc variant=release address-model=64 link=static threading=multi runtime-link=static stage`

### RtAudio

Can be found here: [https://www.music.mcgill.ca/~gary/rtaudio/](https://www.music.mcgill.ca/~gary/rtaudio/)

I built using cmake with the DirectSound option on Windows and the Core option on OSX.

You should either create an environment variable called `RTAUDIO` which points to the built static library, or place it in a location that can be easily found, such as `/usr/local/lib` on Mac. You can also but the static library `librtaudio.a` in the lib folder from the source directory in this repository.

On Windows, if using DirectSound, you will need the files `dsound.h` and the `dsound.lib`, which can be obtained from the Windows SDK.

Place `RtAudio.h` in the include folder, and if on Windows, also place `dsound.h` in this folder as well.

Also on Windows, place `rtaudio_static.lib` and `dsound.lib` in the lib folder.



