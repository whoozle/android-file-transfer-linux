## FAQ
##### You're getting «ioctl(_fd, USBDEVFS_CLAIMINTERFACE, &interfaceNumber): Device or resource busy» or «Device is already used by another process» exception/message box right after you started the application. This clearly indicates that some other process is accessing MTP device right now.
You could do the following steps to find it:
* Open you console emulator (gnome-terminal, konsole, whatever) and type: ```lsusb``` (sudo apt-get install usbutils if it did not start) and find your device in its output, for example
```
Bus 006 Device 070: ID 18d1:4ee2 Google Inc. Nexus 4 (debug)
```
* Start fuser ```sudo fuser /dev/bus/usb/<BUS>/<DEVICE>``` (sudo apt-get install psmisc if it did not start)
* It should output something like this: ```/dev/bus/usb/006/070: 23253 24377``` (actually, there could be more of them after semicolon, like : 24377, 24378, …) so, 23253 and 24377 are the pids of the processes which opened your device.
* So, finally run:
```
ps -x -q 23253
23253 ?        Sl     0:00 /usr/local/bin/android-file-transfer
ps -x -q 24377
24377 ?        Sl    21:14 adb -P 5037 fork-server server
```
Usually, adb is not offending process, because it uses another interface, so the /usr/local/bin/android-file-transfer is the one

