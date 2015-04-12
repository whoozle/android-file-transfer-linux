# Android File Transfer For Linux
Android File Transfer for Linux - interactive MTP client implemented in C++ with no dependencies on libmtp/libptp. Simple reference Qt4/Qt5 application included. Now with Nokia Lumia support!

## Do I need it?
If you're happy with gmtp/gvfs/mtpfs or any other mtp software, you might not need this software(but give it a try!). If you suffered from crashes, missing tags and album covers, usb transfer freezes and corrupted files, this software is right for you. It just works. Thank you for your interest! :)
Files with size more than 4 Gb *ARE* supported.

## Building instructions
### Prerequisites
You will need qt libraries for building ui program. If you're want to use only library *Qt is not needed*, you could turn the option ```BUILD_QT_UI``` off.
For debian based distros use the following command: `sudo apt-get install libqt4-dev ninja-build`

### Building with ninja
```
mkdir build
cd build
cmake -G Ninja ..
ninja

./qt/android-file-transfer
```

### Building with make (not-recommended)
```
mkdir build
cd build
cmake ..
make

./qt/android-file-transfer
```

###Installation

`sudo ninja install` or `sudo make install` will install program into cmake prefix/bin directory (usually /usr/local/bin)


## How to use

Start application, choose destination folder and click any button on toolbar. There's several options there: «Upload Album», «Upload Directory» and «Upload Files». The latter two are self-explanatory. Uploading album tries searching source directory for album cover and send best available cover as "albumart.xxx", or android media player will not show it as album art.

