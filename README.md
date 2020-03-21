# Android File Transfer For Linux (FreeBSD and Mac OS X!)

[![License](http://img.shields.io/:license-LGPLv2.1-blue.svg)](https://github.com/whoozle/android-file-transfer-linux/blob/master/LICENSE)
[![Version](http://img.shields.io/:version-3.10-green.svg)](https://github.com/whoozle/android-file-transfer-linux)
[![Build Status](https://travis-ci.org/whoozle/android-file-transfer-linux.svg?branch=master)](https://travis-ci.org/whoozle/android-file-transfer-linux)

Android File Transfer for Linux — reliable MTP client with minimalistic UI similar to [Android File Transfer for Mac](https://www.android.com/intl/en_us/filetransfer/).

It just works™.

## Do I need it?

If you're happy with `gmtp`/`gvfs`/`mtpfs` or any other mtp software, you might not need this software (but give it a try!).

If you're suffering from crashes, missing tags, album covers, usb freezes and corrupted files, this software is right for you.

## Features

* Simple Qt UI with progress dialogs.
* FUSE wrapper (If you'd prefer mounting your device), supporting partial read/writes, allowing instant access to your files.
* No file size limits.
* Automatically renames album cover to make it visible from media player.
* USB 'Zerocopy' support found in recent Linux kernel (no user/kernel data copying)
* No extra dependencies (e.g. `libptp`/`libmtp`).
* Available as static/shared library.
* Command line tool (aft-mtp-cli)

## FAQ
[Please take a look at FAQ if you have issues with your operating system](FAQ.md). It's not that big, but those are the questions asked very often. 

## Support me
If you want to help me with development, click on the link below and follow the instructions. I'm developing AFTL in my spare time and try to fix everything as fast as possible, sometimes adding features in realtime (more than 100 tickes closed by now).
Any amount would help relieving pain of using MTP. :D

https://www.paypal.me/whoozle

## Building instructions

### Gentoo

  AFT for Linux is now included in Gentoo, you don't have to build anything, just run
  ```
  sudo emerge -av sys-fs/android-file-transfer-linux
  ```

  If you need fuse mount helper to mount MTP filesystem, you have to enable fuse use flag, e.g. adding the following in /etc/portage/package.use (which can either be a directory or a file)
  ```
  sys-fs/android-file-transfer-linux fuse
  ```

  You can use ```sys-fs/android-file-transfer-linux-9999``` ebuild if you want the latest git-version by adding the following entry into /etc/portage/package.accept_keywords (which can either be a directory or a file)
  ```
  =sys-fs/android-file-transfer-linux-9999 **
  ```

### Prerequisites

* You will need Qt libraries for building ui program. If you're planning to use only library (*Qt is not needed*), you could turn the option ```BUILD_QT_UI``` off.
* For ubuntu and other debian-based distros use the following command:

  ```shell
  sudo apt-get install build-essential cmake libqt4-dev ninja-build libfuse-dev libreadline-dev
  ```

  For Fedora:
  ```
  dnf install make automake gcc gcc-c++ kernel-devel cmake fuse fuse-devel qt-devel readline-devel
  ```

* Basically, you need `libqtX-dev` for UI, `libfuse-dev` for FUSE interface, `cmake`, `ninja` or `make` for building the project. You could use libqt5-dev as well.

### Building with ninja

```shell
mkdir build
cd build
cmake -G Ninja ..
ninja

./qt/android-file-transfer
```

### Building with make

```shell
mkdir build
cd build
cmake ..
make

./qt/android-file-transfer
```

### Installing binary package on OS X / macOS
There is a binary package that can be installed via Homebrew:
 * First [install brew](https://brew.sh) if you don't have it already installed.
 * Then the stable package may be installed via:

 ```shell
brew install homebrew/cask/whoozle-android-file-transfer
 ```
 * Nighlty build may be installed via;

 ```shell
brew install homebrew/cask-versions/whoozle-android-file-transfer-nightly
 ```

 * Please note: they are in conflict, so please make sure to uninstall it when you want switch between stable and nightly.

### Building app package on OS X / macOS

You'll need Qt installed to build the GUI app. Here are the build instructions with qt5 from homebrew (`brew install qt5`):

```shell
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=~/Applications -DCMAKE_PREFIX_PATH=/usr/local/opt/qt5
make
make install

open ~/Applications/android-file-transfer.app
```

### Installation

`sudo ninja install` or `sudo make install` will install program into cmake prefix/bin directory (usually /usr/local/bin)


## How to use

### FUSE interface

```shell
mkdir ~/my-device
./aft-mtp-mount ~/my-device
```
Remember, if you want album art to be displayed, it must be named 'albumart.xxx' and placed *first* in the destination folder. Then copy other files.
Also, note that fuse could be 7-8 times slower than ui/cli file transfer.

### Qt user interface

1. Start application, choose destination folder and click any button on toolbar.

2. The options available there are: `Upload Album`, `Upload Directory` and `Upload Files`.
   The latter two are self-explanatory. `Upload album` tries searching source directory for album cover and sets best available cover.

3. You could drop any files or folders right into application window, the transfer will start automatically.

### Known problems

* Samsung removed android extensions from MTP, so fuse will be available readonly, sorry. Feel free to post your complaints to http://developer.samsung.com/forum/en
* Sometimes downloading fails with usb timeout, then phone becomes unresponsive. [Android bug #75259](https://code.google.com/p/android/issues/detail?id=75259)
* Objects created in UI will not show up in FUSE filesystem. [Android bug #169547](https://code.google.com/p/android/issues/detail?id=169547)

The actual list of all known problems and bugs available [here](https://github.com/whoozle/android-file-transfer-linux/issues)

## Contacts
Please do not hesitate to contact me if you have any further questions, my email address is <vladimir.menshakov@gmail.com>.

## Special thanks
* All who filed bugs on github and wrote emails, many features appeared only because of your feedback. Thanks!
* Alexey [gazay](https://github.com/gazay) Gaziev for useful suggestions, support and invaluable help with MacBook and MacOSX port.
* @ssnjrthegr8 for the new logo!

## License

Android File Transfer for Linux is released under [GNU LGPLv2.1 License](https://github.com/whoozle/android-file-transfer-linux/blob/master/LICENSE).

Copyright © 2015-2020 Vladimir Menshakov
