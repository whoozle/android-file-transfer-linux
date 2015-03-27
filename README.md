# android-file-transfer-linux
Android File Transfer for Linux - interactive MTP client implemented in C++ using Qt toolkit

## Do I need it?
If you're happy with gmtp/gvfs/mtpfs or any other mtp software, you might not need this software(but give it a try!). If you suffered from crashes, missing tags and album covers, usb transfer freezes and corrupted files, this software is right for you. It just works. Thank you for your interest! :)

## Building instructions
### Prerequisites
You will need libusb-1.0 and qt libraries for building this software.
For debian based distros use the following command: `sudo apt-get install libusb-1.0-0-dev libqt4-dev ninja`

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

## How to use

Start application, choose destination folder and click any button on toolbar. There's several options there: «Upload Album», «Upload Directory» and «Upload Files». The latter two are self-explanatory. Uploading album tries searching source directory for album cover and send best available cover as "albumart.xxx", or android media player will not show it as album art.

## Known bugs

Linux kernel 3.x and newer return flag which lures libusb into thinking that kernel has unlimited usb buffers for bulk transfer and later fails with ENOMEM. I reported issue in libusb tracker, but still got no approval for it. The program will warn you about it with instructions how to fix it.
