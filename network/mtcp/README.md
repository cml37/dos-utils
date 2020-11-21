# mTCP Client Configuration / Raspberry Pi Server Configuration

## Project
Link to mTCP Project: http://www.brutman.com/mTCP/

## Sample Files
In this directory, you will find several sample files for getting started with mTCP.  The files have been divided into two categories
* **Target System**: Configuration steps and sample configuration files for mTCP for your target system
* **Raspberry Pi**: Configuration steps for setting up the following services on your Raspberry Pi
  * telnet server
  * FTP server
  * mutt email client
 
Reviewing each of the above files should help get you started!

## Suggested Order
I'd suggest running through this procedure in the following order:
* Configure the Raspberry Pi
  * Install and configure the [ftp server](raspberry_pi/ftp_server.txt)
  * Install and configure the [telnet server](raspberry_pi/telnet_server.txt)
  * Install and configure [mutt](raspberry_pi/mutt.txt)
* On your local system, configure files for the target system using [these instructions](target_system/mtcp.txt)
  * I suggest creating a local directory on your target system and copying my [sample files](target_system/mtcp_files) there
  * From there, I also suggest copying your Ethernet packet driver and the unzipped mtcp into that folder
* Copy the directory with the configured files to your target system using a floppy disk (or two... or three!)

## Video Walkthrough
If you'd like a complete walkthrough on how to apply the above, a tutorial video is available on my [channel](https://www.youtube.com/channel/UCq2-mTyQ0EVh5FmUG9D6xyA) 