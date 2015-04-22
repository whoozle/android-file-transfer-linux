# Android File Transfer For Linux
Android File Transfer for Linux — reliable MTP client with minimalistic UI similar to Android File Transfer for Mac. It just works™.

## Do I need it?
If you're happy with gmtp/gvfs/mtpfs or any other mtp software, you might not need this software(but give it a try!). If you suffered from crashes, missing tags and album covers, usb freezes and corrupted files, this software is right for you. It just works. Files with size more than 4 Gb *ARE* supported. Thank you for your interest! :)

## Features
* Simple Qt UI with progress dialogs.
* FUSE wrapper (If you'd prefer mounting your device), supporting partial read/writes, allowing instant access to your files.
* No file size limits.
* Automatically renames album cover to make it visible from media player.
* No extra dependencies (e.g. libptp/libmtp).
* Available as static/shared library.
* Simple CLI tool.

## Building instructions
### Prerequisites
You will need qt libraries for building ui program. If you're want to use only library *Qt is not needed*, you could turn the option ```BUILD_QT_UI``` off.
For debian based distros use the following command: `sudo apt-get install build-essential cmake libqt4-dev ninja-build libfuse-dev`. Basically, you need libqtX-dev for UI, libfuse-dev for FUSE interface, cmake, ninja or make for building the project.

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
###FUSE interface
```
mkdir ~/my-device
./aft-mtp-mount ~/my-device
```
Remember, if you want album art to be displayed, it must be named 'albumart.xxx' and placed *first* in the destination folder. Then copy other files.
Also, note that fuse could be 7-8 times slower than ui/cli file transfer.
###UI
Start application, choose destination folder and click any button on toolbar. There's several options there: «Upload Album», «Upload Directory» and «Upload Files». The latter two are self-explanatory. Uploading album tries searching source directory for album cover and sets best available cover.
