yoshida-lab-cap
===============

## Description

A small cross-platform program that will capture multiple USB and IP cameras simultaneously and sync frames according to some common frame rate. It also generates timestamps for samples for syncing offline, after data has been collected.

Video sources:

- multiple USB camera devices
- multiple IP cameras

Audio:

- Record single input source
- Generate audio pulses at specific frequency

See the help documentation in the program for a list of options:

```

```

## External Dependencies

- OpenCV (3.3.1)
    - You'll also need to add the `opencv_ffmpeg.dll` to your path or in the same folder as the release in order to write videos with non-standard codecs to a file (e.g., locate file from `bin/opencv_ffmpeg331_64.dll` and place in program folder.).
- Boost (1.65.1)
- RtAudio (5.0.0)
    - The audio API's are system dependent. On Windows, if using DirectSound (which is the default in the cmake options), you will need the files `dsound.h` and the `dsound.lib`, which can be obtained from the Windows SDK. [https://www.music.mcgill.ca/~gary/rtaudio/apinotes.html]
- cmake (>3.6)


### Building dependencies from source

#### Windows

See the file `./tools/build_deps/windows/build_dependencies.bat`

Open the `.bat` file and change the location for one of `__msvc_toolchain`,
`__mingw_toolchain`,
`__cygwin_toolchain` depending on which toolchain you are using.

To automatically grab and unzip the source files you'll need 7-zip [http://www.7-zip.org/] and to change `__7z_exe`

#### Linux

Run the `.sh` files in `./tools/build_deps/linux`

#### OSX

I used Homebrew to grab the dependencies

```
brew install opencv
brew install rt-audio
brew install boost
```

# After dependencies are installed

## Cmake generation

You can build the program with cmake [https://cmake.org/] and by setting some cmake options to locate the libraries.

Make sure cmake can be found on your path environment. Below is an example using the MinGW toolset (see `cmake -h` for other Generator names, must be installed on your system).

```
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TARGET=main -DBIN_NAME=cogdevcam -DOPENCV_INSTALL_DIR=libs/opencv -DBOOST_INSTALL_DIR=libs/boost  -DRTAUDIO_INSTALL_DIR=libs/rtaudio  -G "MinGW Makefiles" ..
```

If all dependencies are in a specific folder you can use the `CUSTOM_LIB_ROOT` and `COMPILER_SUBDIR` options, where the structure is as follows: `"${CUSTOM_LIB_ROOT}/${COMPILER_SUBDIR}/boost"` or `"${CUSTOM_LIB_ROOT}/${COMPILER_SUBDIR}/opencv"` or `"${CUSTOM_LIB_ROOT}/${COMPILER_SUBDIR}/rtaudio"`

```
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TARGET=main -DBIN_NAME=cogdevcam -DCUSTOM_LIB_ROOT=C:\Users\josep\libs -DCOMPILER_SUBDIR=gcc -G "MinGW Makefiles" ..
```

List of options:

```
 -DCUSTOM_LIB_ROOT=""
 -DCOMPILER_SUBDIR=""
 -DWITH_OPENCV=TRUE
 -DOPENCV_INSTALL_DIR=libs/opencv
 -DWITH_BOOST=TRUE
 -DBOOST_INSTALL_DIR=libs/boost
 -DWITH_RTAUDIO=TRUE
 -DRTAUDIO_INSTALL_DIR=libs/rtaudio
 -DBUILD_TARGET=main
 -DBIN_NAME=cogdevcam
 ```

## Build/install

You can build normally either via solution folders if using Visual Studio, using the generated make file, or with cmake

From the build directory ...

```
cmake --build . --target install -- -j 2
```

Test that it is running by doing ...

```
cd install/bin
cogdevcam -h
```