# MS-DOS Push TSR

* Features code excerpts from Michael Brutman's [mTCP](http://www.brutman.com/mTCP/)
* Feature code excerpts from [this Open Watclock example](https://www.vcfed.org/forum/forum/technical-support/vintage-computer-programming/23586-tsr-programs-with-open-watcom?p=304280#post304280)

## Building
1. Download and install Open Watcom
2. Download and install NASM
3. From there, it's just `wmake` which will produce a binary called `tsrpush.exe`

## Running
Simple! 
1. Load a Packet Driver
2. Start up `tsrpush.exe` on your MS-DOS system by passing in the Packet Driver Interrupt Vector Number and a UDP port for listening, i.e.
`tsrpush.exe 0x65 20000` (use Packet Driver at Interrupt 0x65 and listen on UDP Port 20000)

## Unloading
Simple!
1. Just type `tsrpush.exe`

## Testing
See the `send.py` script in the test directory.  Change the broadcast address to that of your network and the message accordingly.  Run it, and you will see a push message in MS-DOS.

## Known Issues
1. Will not work with all programs resident (uses pretty primitive screen control)
2. Does not appear to receive new packets after entering and leaving protected mode (i.e. Windows 3.11)
3. Does not support ARP, DNS, or TCP (only supports UDP and access via IP address)
4. Only supports receiving broadcast packets and does not verify the broadcast sender (should update configuration to properly use mTCP's parseEnv, do away with passing in the packet driver interupt vector, and assign an IP address and verify broadcast address)
5. Does not send a reply to acknowledge a UDP packet