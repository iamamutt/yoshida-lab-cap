#!/usr/bin/env bash
# ------------------------------------------------------------------------------------------------
# Joseph M. Burling <josephburling@gmail.com> 2017
# ------------------------------------------------------------------------------------------------
script_file=$(readlink -f "$0")
script_dir=$(dirname "$script_file")
rtaudio_ver="5.0.0"

cd "$HOME/Desktop"

if [ ! -d "RtAudio_temp" ]; then
    mkdir -p "RtAudio_temp"
fi

cd RtAudio_temp

logfile="rtaudio_log.txt"
echo `date '+%Y-%m-%d %H:%M:%S'` > $logfile
echo "Installing RtAudio version: $rtaudio_ver" >> $logfile

sudo apt-add-repository -y universe
sudo apt-get -y update 2>> $logfile
sudo apt-get -y upgrade 2>> $logfile
sudo apt-get install -y unzip wget doxygen build-essential 2>> $logfile
sudo apt-get -y install git cmake cmake-qt-gui pkg-config libomp-dev checkinstall 2>> $logfile
sudo apt-get -y install libasound2-dev 2>> $logfile

zip_ext=".tar.gz"
rtaudio_filename="rtaudio-$rtaudio_ver"

if [ ! -f "$rtaudio_filename$zip_ext" ]; then
    wget "http://www.music.mcgill.ca/~gary/rtaudio/release/$rtaudio_filename$zip_ext" 2>> $logfile
    tar -xvzf $rtaudio_filename$zip_ext
fi

if [ -d "$rtaudio_filename" ]; then
    cd $rtaudio_filename
fi

./configure --with-alsa 2>> $logfile
sudo make -j8 2>> $logfile
sudo install 2>> $logfile
sudo /bin/bash -c 'echo "/usr/local/lib" > /etc/ld.so.conf.d/local.conf'
sudo ldconfig
