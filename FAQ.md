# FAQ

## Is it possible to use cli tool for automated file management?

Yes. There's two options of achieving this. 
1. Pass every command as **single** command line argument, e.g. ```aft-mtp-cli "get Pictures" "cd Pictures" "ls"```. If you need quoted argument for commands, you have to escape them, e.g. ```aft-mtp-cli "get \"Pictures\""```. 
2. Pass -b command line option and feed your script to stdin. 

## How to unmount my device?

Run
```fusermount -u <path>```


## I'm getting «ioctl(_fd, USBDEVFS_CLAIMINTERFACE, &interfaceNumber): Device or resource busy» or «Device is already used by another process» exception/message box right after you started the application. This clearly indicates that some other process is accessing MTP device right now.
You could do the following steps to find the offending process:
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

## No MTP device found, but it's listed in lsusb

Maybe you don't have sufficient privileges to access usb device under /dev/bus/usb

First, try checking what bus and device numbers your device has by running lsusb command and finding your device in its output.

For instance, my old Nexus 5 phone:
```
Bus 010 Device 003: ID 18d1:4ee2 Google Inc. Nexus Device (debug)
```

Note the bus/device number in row with your device Then, go /dev/bus/usb/<bus> and check file <device> there. (010 and 003 respectively in my case).
```
ls -l /dev/bus/usb/010/
crw-rw-r--  1 root usb     189, 1152 Jan  5 21:30 001
crw-rw----+ 1 root plugdev 189, 1154 Jan  5 22:46 003
```

plugdev group may have different name for different distros, consult your distro's manual for details. You can check what groups you're in by running 'id' command.

If device's group is 'root', then you have to add udev rule:
http://reactivated.net/writing_udev_rules.html
Please find some examples there.
