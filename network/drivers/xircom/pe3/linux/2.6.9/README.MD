# Xircom Pocket Ethernet PE3 Linux drivers for kernel 2.6.9 

### Some notes:
* This is an incomplete driver based on the driver referenced here: http://www.buzzard.me.uk/jonathan/pe3.html
* This driver converts the driver found at the above resource to work on kernel 2.6.x
* "Tested" under Fedora Core 3


### To build:
* `make`

### To test:
 * Create the file `/etc/sysconfig/network-scripts/ifcfg-pe0` with these contents:
 ```
DEVICE=pe0
BOOTPROTO=none
TYPE=Ethernet
```
 * `modprobe pe3.ko`
 * `ifup pe0`
 * check `/var/log/messages` for messages that contain `pe3`
