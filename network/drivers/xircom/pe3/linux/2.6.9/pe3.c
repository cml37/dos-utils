/* pe3.c -- Linux driver for the Xircom PE3 parallel port Ethenet adaptor 
 *
 * Copyright (c) 1996-97  Jonathan A. Buzzard (jab@hex.prestel.co.uk)
 *
 * Valuable assistance from:
 *                  Alan Cox <alan@lxorguk.ukuu.org.uk>
 *                  Gaylan Ohlhausen <gaylano@worldnet.att.net>
 *                  C. Scott Ananian <cananian@princeton.edu>
 *
 * The information used to write this program was obtained by tracing the
 * interactions of the DOS packet driver and the PE3 using the Linux DOS
 * emulator. I have no idea if the programming model I have derived bears
 * any resemblance to Xircom's. I have not at any stage disassembled the
 * Xircom supplied packet driver.
 *
 *
 * This is ALPHA software -- use at your own risk.
 *
 * This is a preliminary driver, that just checks that what I have worked
 * out about the Xircom PE3 is correct. You should have an Xircom PE3 
 * attached to your Linux machine and powered.
 *
 * Compile it with the corresponding makefile
 *
 * Load with:
 *
 *      insmod pe3.ko [ pe3_base=0x378/0x278/0x3bc pe3_irq=7/5 ]
 *
 * And check your system log for messages! You should find the type of adaptor,
 * the ethernet address and the date/time of manufacture etc. Please e-mail any
 * success or failures to me, jab@hex.prestel.co.uk .
 *
 */

static char *version = "pe3.c:v0.3 1/2/23 Chris Lenderman\n";


#include <linux/module.h>
#include <linux/version.h>
#include <linux/init.h>

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <linux/delay.h>

#include <linux/in.h>
#include <linux/inet.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>


typedef struct {
	unsigned short base;		/* Base IO address */
	int irq;			/*  */
	int mode;			/* Transfer mode */
	unsigned char eth_addr[6];	/* Ethernet address (also serial no.) */
	int year;			/* Year of manufacture */
	int month;			/*     ... month */
	int day;			/*     ... day */
	int hour;			/*     ... hour */
	int minute;			/*     ... minute */
	int second;			/*     ... second */
} pe3_struct;

#define NO_HOSTS 4
static pe3_struct pe3_hosts[NO_HOSTS] =
{
  {0x378, 7, 0, {0, 0, 0, 0, 0, 0}, 0, 0, 0, 0, 0, 0},
  {0x278, 5, 0, {0, 0, 0, 0, 0, 0}, 0, 0, 0, 0, 0, 0},
};


/* port i/o support (stollen from parbus cos one day I plan to support it) */

/*#define PE3_BASE(x)     pe3_hosts[(x)].base*/
#define PE3_BASE(x)     pe3_base

#define r_dtr(x)        inb(PE3_BASE(x))
#define r_str(x)        inb(PE3_BASE(x)+1)
#define r_ctr(x)        inb(PE3_BASE(x)+2)
#define r_epp(x)        inb(PE3_BASE(x)+4)
#define r_fifo(x)       inb(PE3_BASE(x)+0x400)
#define r_ecr(x)        inb(PE3_BASE(x)+0x402)

#define w_dtr(x,y)      outb((y), PE3_BASE(x))
#define w_str(x,y)      outb((y), PE3_BASE(x)+1)
#define w_ctr(x,y)      outb((y), PE3_BASE(x)+2)
#define w_epp(x,y)      outb((y), PE3_BASE(x)+4)
#define w_fifo(x,y)     outb((y), PE3_BASE(x)+0x400)
#define w_ecr(x,y)      outb((y), PE3_BASE(x)+0x402)


int base[NO_HOSTS] = 
{0x3bc, 0x378, 0x278, 0x0000};
#define parbus_no	NO_HOSTS
#define parbus_base	base

/*
 * Macro to join the two 'Xircom' nibbles together.
 */

#define j44(a,b)	(((char)a>>3)&0x47)|((char)b&0xb8)


/*
 * Change these variables here or with insmod or with a LILO or LOADLIN
 * command line argument
 */

static int pe3_base  = 0x378;
static int pe3_irq   = 7;
static int pe3_mode  = 0;  /* 0 for nibble; 1 for bidirectional */

/* 
 * module_param(foo, int, 0000)
 * The first param is the parameters name
 * The second param is it's data type
 * The final argument is the permissions bits, 
 * for exposing parameters in sysfs (if non-zero) at a later stage.
 */
module_param(pe3_base, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(pe3_base, "PE3 Base Address");
module_param(pe3_irq, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(pe3_irq, "PE3 IRQ");

/* Declare functions */

extern int pe3_probe(struct net_device *dev);

static struct net_device *pe3_dev = NULL;

int pe3_irqfound = 0;


/*
 * Special I/O functions for debuggin purposes
 */
void inline W_dtr(int x, unsigned char y)
{
	outb(y, x);
	printk("write port 0x%3x value %02x\n", x, y);
}
void inline W_ctr(int x, unsigned char y)
{
	outb(y, x+2);
	printk("write port 0x%3x value %02x\n", x+2, y);
}
unsigned char inline R_dtr(int x)
{
	unsigned char temp;

	temp = inb(x);
	printk("read port 0x%3x gave %02x\n", x, temp);
	return temp;
}
unsigned char inline R_str(int x)
{
	unsigned char temp;

	temp = inb(x+1);
	printk("read port 0x%3x gave %02x\n", x+1, temp);
	return temp;
}


/*
 * Don't have a clue what the next four routines do!!! Please enlighten
 * me Xircom!!!
 */

void pe3_bigloop(int base_addr, unsigned char value)
{
	int loop;

	for (loop=0;loop<126;loop++) {
		w_ctr(base_addr, value);
		w_ctr(base_addr, 0x04);
	}
	return;
}


/*
 *  This particular routine took a week to work out from the traces
 */

void pe3_noidea(int base_addr, unsigned char com1, unsigned char com2,
                unsigned char com3, int mask)
{
	w_dtr(base_addr, com1);
	w_ctr(base_addr, 0x0c);
	udelay(1);
	w_ctr(base_addr, (com2 | 0x08) & 0xf7);
	w_dtr(base_addr, com3);
	w_ctr(base_addr, com2 | mask);
	udelay(1);
	w_ctr(base_addr, com2);

	return;
}


/*
 * Don't have the slightest idea what this does either!!!
 */

void pe3_notaclue(int base_addr, unsigned char command)
{
	w_dtr(base_addr, 0x58);
	w_ctr(base_addr, 0x0c);
	udelay(1);
	w_ctr(base_addr, 0x04);
	w_dtr(base_addr, 0x00);
	w_ctr(base_addr, 0x0c);
	udelay(1);
	w_ctr(base_addr, 0x04);
	w_dtr(base_addr, command);
	w_ctr(base_addr, 0x0c);
	udelay(1);
	w_ctr(base_addr, 0x04);
	w_dtr(base_addr, 0x00);
	w_ctr(base_addr, 0x0c);
	udelay(1);
	w_ctr(base_addr, 0x04);
	w_ctr(base_addr, (command<0x04) ? 0x06 : 0x05);
	udelay(1);
	w_ctr(base_addr, 0x04);

	return;
}


/*
 * No idea what this does either!!!
 */

int pe3_stumped(int base_addr, unsigned char value)
{
	unsigned char read1,read2;

	w_dtr(base_addr, value);
	w_ctr(base_addr, 0x0c);
	w_ctr(base_addr, 0x04);
	if (pe3_mode==0) {
		w_ctr(base_addr, 0x05);
		read1 = r_str(base_addr);
	} else {
		w_ctr(base_addr, 0x25);
		read1 = r_dtr(base_addr);
	}
	w_ctr(base_addr, 0x04);

	if (pe3_mode==0) {
		read2 = r_str(base_addr);
		read1 = j44(read1,read2);
	}

	printk("stumped gave %02x\n", read1); 
	return read1;
}


/*
 * This sequence appears to be uploading a byte to the PE3
 *
 *     Note: it might not be doing this though 
 */

void pe3_upload(int base_addr, unsigned char address, unsigned char value)
{
	w_dtr(base_addr, address);
	w_ctr(base_addr, 0x0c);
	w_ctr(base_addr, 0x04);
	w_dtr(base_addr, value);
	w_ctr(base_addr, 0x05);
	w_ctr(base_addr, 0x04);

	return;
}

/*
 * This section of the trace looks to be sending a sequence of 256 bytes to
 * the adaptor. The sequence differs depending on the type of interface
 * detected (nibble/byte). What this is for/does I have not the faintest
 * idea.
 *
 */

void pe3_loader(int base_addr)
{
	int loop;
	unsigned char bytes[256] = { 0xf7,0x06,0xfc,0x07,0x00,0x80,0x74,0x06,
	0xb9,0x03,0x00,0xe9,0xef,0x00,0xf7,0x06,
	0xb5,0x11,0x10,0x00,0x75,0xf2,0xa3,0xa7,
	0x11,0x8b,0xf3,0xe8,0x05,0x05,0x74,0x06,
	0xb9,0x02,0x00,0xe9,0xd7,0x00,0x83,0x0e,
	0xb5,0x11,0x10,0xf7,0x06,0xb5,0x11,0x04,
	0x00,0x75,0x09,0xe8,0x7e,0x05,0xc7,0x06,
	0xa5,0x11,0x01,0x00,0xfb,0xfc,0xb8,0x07,
	0x28,0xff,0x16,0x8c,0x0c,0xa1,0xa7,0x11,
	0x8a,0xe0,0xb0,0x08,0xff,0x16,0x8c,0x0c,
	0xa1,0xa7,0x11,0xb0,0x08,0xff,0x16,0x8c,
	0x0c,0x56,0x03,0x36,0x02,0x1a,0x83,0xfe,
	0x00,0x75,0x03,0x5e,0xeb,0x75,0xe8,0x6f,
	0xfe,0x51,0x8b,0x0e,0x02,0x1a,0xe3,0x17,
	0x1e,0x06,0x8c,0xda,0x8e,0xc2,0xc5,0x36,
	0xfe,0x19,0x26,0xff,0x16,0x9c,0x0c,0x07,
	0x1f,0xe3,0x04,0x58,0x5b,0xeb,0x60,0x59,
	0x5b,0xe3,0x35,0x51,0x1e,0x26,0x8b,0x4e,
	0x04,0xe3,0x1f,0x3b,0xd9,0x77,0x02,0x8b,
	0xcb,0x2b,0xd9,0x53,0x8c,0xda,0x26,0xc5,
	0x76,0x00,0x06,0x8e,0xc2,0x26,0xff,0x16,
	0x9c,0x0c,0x07,0x5b,0xe3,0x04,0x1f,0x58,
	0xeb,0x35,0x1f,0x59,0x83,0xfb,0x00,0x74,
	0x22,0x83,0xc5,0x06,0xe2,0xcd,0xeb,0x1b,
	0x8b,0xcb,0xe3,0x17,0x1e,0x06,0x8c,0xda,
	0x8c,0xc0,0x8e,0xd8,0x8e,0xc2,0x8b,0xf5,
	0x26,0xff,0x16,0x9c,0x0c,0x07,0x1f,0xe3,
	0x02,0xeb,0x0c,0xe8,0x79,0xfe,0xb8,0x0a,
	0x81,0xff,0x16,0x8c,0x0c,0x33,0xc9,0xfa,
	0x83,0x26,0xb5,0x11,0xef,0x83,0x3e,0xa5,
	0x11,0x01,0x75,0x09,0xc7,0x06,0xa5,0x11,
	0x00,0x00,0xe9,0x9b,0x04,0xc3,0xfa,0xf7 };

	
	for(loop=0;loop<256;loop+=2) {
		w_dtr(base_addr, bytes[loop]);
		w_ctr(base_addr, 0x05);
		w_dtr(base_addr, bytes[loop+1]);
		w_ctr(base_addr, 0x04);
	}

	return;
}


/*
 * Seems to be sending some sort of command sequence to the PE3
 */

void pe3_command(int base_addr, unsigned char command, unsigned char value)
{
	w_dtr(base_addr, (value==0x04) ? 0x51 : 0x50);
	w_ctr(base_addr, value | 0x08);
	w_ctr(base_addr, value & 0xf7);
	w_dtr(base_addr, command);
	w_ctr(base_addr, value | 0x01);
	w_ctr(base_addr, value);

	return;
}


/*
 * Download the EEPROM contents from the PE3
 *
 *     returns zero if the checksum is OK, otherwise non-zero
 *     Note: this will only work on little endian machines
 */
 
int pe3_readeeprom(struct net_device *dev)
{
	unsigned char com[38] = { 0,0,2,0,4,4,6,4,5,7,5,5,7,5,4,6,4,4,6,
	                          4,4,6,4,5,7,5,5,7,5,5,7,5,5,7,5,4,6,4 };
	unsigned long time;
	int bit,check,junk,i,j;
	int eeprom[16] = { 0x00 };

	for (i=0;i<16;i++) {
		if (i/8==1) {
			com[23] = 4; com [24] = 6; com [25] = 4;
		} else {
			com[23] = 5; com [24] = 7; com [25] = 5;
		}
		if ((i%8)/4==1) {
			com[26] = 4; com [27] = 6; com [28] = 4;
		} else {
			com[26] = 5; com [27] = 7; com [28] = 5;
		}
		if ((i%4)/2==1) {
			com[29] = 4; com [30] = 6; com [31] = 4;
		} else {
			com[29] = 5; com [30] = 7; com [31] = 5;
		}
		if (i%2==1) {
			com[32] = 4; com [33] = 6; com [34] = 4;
		} else {
			com[32] = 5; com [33] = 7; com [34] = 5;
		}
		for (j=0;j<38;j++) {
			pe3_command(dev->base_addr, com[j], 0x04);
		}

		for (j=15;j>=0;j--) {
			w_dtr(dev->base_addr, 0x71);
			w_ctr(dev->base_addr, 0x0c);
			w_ctr(dev->base_addr, 0x04);

			if (pe3_mode==1) {
				w_ctr(dev->base_addr, 0x25);
				bit = r_dtr(dev->base_addr) & 0x01;
				w_ctr(dev->base_addr, 0x04);
			}
			else {
				w_ctr(dev->base_addr, 0x05);
				bit = (r_str(dev->base_addr) & 0x08)/8;
				w_ctr(dev->base_addr, 0x04);
				junk = r_str(dev->base_addr); /*What this for?*/
			}
			eeprom[i] = eeprom[i]+(bit<<j);

			pe3_command(dev->base_addr, 0x04, 0x04);
			pe3_command(dev->base_addr, 0x06, 0x04);
			pe3_command(dev->base_addr, 0x04, 0x04);
		}
	}

	dev->dev_addr[0] = eeprom[13]/256;
	dev->dev_addr[1] = eeprom[14]%256;
	dev->dev_addr[2] = eeprom[14]/256;
	dev->dev_addr[3] = eeprom[9]%256;
	dev->dev_addr[4] = eeprom[8]/256;
	dev->dev_addr[5] = eeprom[8]%256;
	
	time = (eeprom[11]/256)+(eeprom[11]%256)*256+
	       (eeprom[10]/256)*65536+(eeprom[10]%256)*16777216;

	printk("pe3: Manufactured on %02ld/%02ld/%4ld at %02ld:%02ld:%02ld\n",
	       (time>>23) & 0x1f, (time>>28) & 0x0f, 1990+((time>>17) & 0x3f),
	       (time>>12) & 0x1f, (time>> 6) & 0x3f, (time>> 0) & 0x3f);

	for (i=0,check=0;i<16;i++)
		check += eeprom[i];

	return (check & 0xffff);
}


/*
 * Seems to be sending some sort of command to the PE3
 */

void pe3_pulse(int base_addr, unsigned char value)
{
	w_dtr(base_addr, value);
	w_ctr(base_addr, 0x0c);
	w_ctr(base_addr, 0x04);

	return;
}


/*
 * Get some sort of status report back from the PE3.
 *
 *     Note: The two reads are a single byte using Xircom's unique
 *           nibble mode.
 */

int pe3_status(int base_addr, unsigned char value1, unsigned char value2)
{
	int low,high;

	w_dtr(base_addr, value1);
	w_ctr(base_addr, 0x0c);
	w_ctr(base_addr, 0x04);
	w_ctr(base_addr, value2);
	udelay(1);
	low = r_str(base_addr);
	w_ctr(base_addr, 0x04);
	udelay(1);
	high = r_str(base_addr);

	return j44(low,high);
}


/*
 * Appears to be determining whether the port is bidirectional or nibble.
 *
 *     Note: we return 0 for nibble mode and 1 for bidirectional. This
 *           represents the first difference between the bidirectional
 *           and nibble mode trace. I think more than simple nibble/
 *           bidirectional detection goes on here but these are the only
 *           modes I can set my parallel port to. Traces from other
 *           parallel ports are invited (but contact me first).
 */

int pe3_getmode(int base_addr)
{
	unsigned char mode;

	pe3_notaclue(base_addr, 0x02);
	pe3_noidea(base_addr, 0x93, 0x04, 0xa5, 0x02);

	w_dtr(base_addr, 0x73);
	w_ctr(base_addr, 0x0c);
	udelay(1);
	w_ctr(base_addr, 0x04);
	w_ctr(base_addr, 0x26);
	udelay(16);
	mode = r_dtr(base_addr);
	w_ctr(base_addr, 0x04);

	if (mode!=0xa5) {
		w_ctr(base_addr, 0xe6);
		udelay(16);
		mode = r_dtr(base_addr);
		w_ctr(base_addr, 0x04);

		/* I think this is a test to see if an SPP can be used in
		   bidirectional mode. We output 0xff and the adaptor pulls
		   some lines low, which can then be read. Could be dodgy
		   though as it stresses the chip(s). */
		
		if (mode!=0xa5) {
			w_dtr(base_addr, 0xff);
			w_ctr(base_addr, 0x26);
			udelay(16);
			mode = r_dtr(base_addr);
			w_ctr(base_addr, 0x04);		
		}
	}

	pe3_notaclue(base_addr, 0x06);

	w_dtr(base_addr, 0x53);
	w_ctr(base_addr, 0x0c);
	udelay(1);
	w_ctr(base_addr, 0x04);
	w_ctr(base_addr, 0x05);
	udelay(1);
	w_dtr(base_addr, 0x5a);
	udelay(1);
	w_ctr(base_addr, 0x04);
	w_dtr(base_addr, 0x73);
	w_ctr(base_addr, 0x0c);
	udelay(1);
	w_ctr(base_addr, 0x04);
	w_ctr(base_addr, 0x25);
	udelay(1);
	mode = r_dtr(base_addr);
	w_ctr(base_addr, 0x04);

	/* Hum, probably not accurate but it works */

	return (mode==0x73) ? 0 : 1;
}


/*
 * Get the PE3 to raise the IRQ on the parallel port.
 *
 *     Stricktly speeking this is not entirely the truth, as it is called
 *     after probing for the IRQ in some sort of handshaking
 */

void pe3_raiseirq(int base_addr, unsigned char command, unsigned char flag)
{
	w_dtr(base_addr, 0x50);
	udelay(1);
	w_ctr(base_addr, command);
	udelay(1);
	command = command ^ 0x02;
	w_ctr(base_addr, command);
	udelay(1);
	command = command ^ 0x08;
	w_ctr(base_addr, command);
	udelay(1);
	command = command ^ 0x08;
	w_ctr(base_addr, command);
	udelay(1);
	w_dtr(base_addr, flag);
	udelay(1);
	command = command ^ 0x02;
	w_ctr(base_addr, command);
	udelay(1);
	command = command ^ 0x02;
	w_ctr(base_addr, command);

	return;
}


/*
 * Test for a specified IRQ to make sure it works!
 *
 */

static irqreturn_t pe3_irqtest(int irq, void *dev_id, struct pt_regs *regs)
{
	struct net_device *dev = dev_id;

	if (dev==NULL) {
		printk ("pe3: IRQ %d for unknown device.\n", irq);
		return IRQ_NONE;
	}

	if (irq==pe3_irq) {
		pe3_irqfound = irq;
		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}


/*
 * Probe for an IRQ
 *
 *    This function reconstructs (more or less) the portion of the trace
 *    that follows immediately after the inital probe for the presence
 *    of the adaptor. I believe it's probing for an IRQ for the parallel
 *    port. If compiling for a module then rather than probe for an IRQ
 *    we check that the supplied IRQ will work.
 */
 
int pe3_getirq(struct net_device *dev)
{
	unsigned char command,junk;
	int loop,irqfound;


	udelay(2);
	w_ctr(dev->base_addr, 0x14);
	/* wait a bit, no idea why!!! */
	udelay(1000);
	w_ctr(dev->base_addr, 0x14);

	if (request_irq(dev->irq, pe3_irqtest, 0, "pe3", dev)!=0) {
		printk("pe3: unable to get IRQ %d\n", pe3_irq);
		return 1;
	}

	irqfound = 0;
	command = 0x03;
	loop = 2;

	while ((loop!=0) && (irqfound==0)) {

		pe3_raiseirq(dev->base_addr, 0x16, command);

		/* the 255us delay is based on pe3pd.com reading port 0x61 255 times */

		udelay(255);
		irqfound = pe3_irqfound;

		loop--;
		command = 0x00;
		}

	free_irq(pe3_irq, NULL);

	if (irqfound>0) {
		w_ctr(dev->base_addr, 0x04);
		udelay(1);
	}

	/* Why do we read the status port here? Nothing seems to depend
	   on the outcome? */

	junk = r_str(dev->base_addr);

	if (irqfound>0)
		pe3_raiseirq(dev->base_addr, 0x06, 0x00);
	else
		pe3_raiseirq(dev->base_addr, 0x16, 0x00);
		
	w_ctr(dev->base_addr, 0x04);
	udelay(1);

	if (irqfound>0)
		printk("pe3: using supplied IRQ %d.\n", irqfound);
	else
		printk("pe3: failed to detect IRQ, switching to polling mode\n");

	return 0;
}


/*
 * Probe for the presence of a Xircom PE3
 *
 *     returns zero if present, and non-zero if unable to detect
 *     a functioning adaptor.
 */

int pe3_probe(struct net_device *dev)
{
	unsigned char address[28] = { 0x46,0x40,0x41,0x42,0x43,0x44,0x45,
	                              0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,
	                              0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,
	                              0x4e,0x4f,0x47,0x4b,0x4d,0x46,0x46 };
	unsigned char bytes[28] = { 0xfa,0xab,0x8f,0xab,0x8f,0x06,0x02,
	                            0x20,0x00,0x80,0xc7,0x2f,0x76,0xad,
	                            0x24,0x00,0x00,0x00,0x00,0x00,0x00,
	                            0x00,0x00,0x28,0x07,0x00,0x7a,0xfa };
	int loop,detect[2];
	int iosize = (dev->base_addr==0x3bc) ? 3 : 8;


	/* this is based on trace of what Xircom's PE3 packet driver does. */

	w_dtr(dev->base_addr, 0x00);
	w_ctr(dev->base_addr, 0x0b);
	
	/* wait pe3_base*2 usecs !!! */
	udelay(dev->base_addr*2);
		
	w_ctr(dev->base_addr, 0x04);
	w_dtr(dev->base_addr, 0x00);
	
	/* pe3pd.com reads port 0x61 1200000 times here, should be the same */
	//mdelay(120000);

	w_dtr(dev->base_addr, 0xff);
	udelay(2);

	/* test for presence of the adaptor */

	for (loop=0;loop<2;loop++) {
		w_dtr(dev->base_addr, 0x00);
		w_ctr(dev->base_addr, 0x04);
		w_dtr(dev->base_addr, 0x1c);
		w_dtr(dev->base_addr, 0x14);
		w_dtr(dev->base_addr, 0x18);
		w_dtr(dev->base_addr, 0x10);
		w_dtr(dev->base_addr, 0x18);
		w_dtr(dev->base_addr, 0x10);
		w_dtr(dev->base_addr, 0x18);
		w_dtr(dev->base_addr, 0x1c);
		w_dtr(dev->base_addr, 0x14);

		pe3_noidea(dev->base_addr, 0x80, 0x04, 0x00, 0x02);
		pe3_notaclue(dev->base_addr, 0x00);
		pe3_noidea(dev->base_addr, 0x93, 0x04, 0xa5, 0x02);

		detect[loop] = pe3_status(dev->base_addr, 0x73, 0x06);
	}

	/* this might not be the whole truth but it works! */

	if ((detect[0]!=0xa5) && (detect[1]!=0xa5)) {
		printk("pe3: A Pocket Ethernet Adaptor could not be found\n");
		return 1;
	}

	/* If we have got this far then we have a PE3 attached (probably)! */

	request_region(dev->base_addr, iosize, dev->name);
	printk("pe3: whee found a Xircom PE3!!!\n");

	/* Find/test the IRQ */

	pe3_getirq(dev);

	pe3_notaclue(dev->base_addr, 0x04);
	pe3_noidea(dev->base_addr, 0x94, 0x04, 0x90, 0x01);
	pe3_noidea(dev->base_addr, 0x96, 0x04, 0x90, 0x01);
	pe3_noidea(dev->base_addr, 0x98, 0x04, 0x90, 0x01);

	pe3_bigloop(dev->base_addr, 0x05);
	loop = pe3_status(dev->base_addr, 0x76, 0x05);

	pe3_notaclue(dev->base_addr, 0x00);
	pe3_noidea(dev->base_addr, 0x94, 0x04, 0x90, 0x02);
	pe3_noidea(dev->base_addr, 0x96, 0x04, 0x90, 0x02);
	pe3_noidea(dev->base_addr, 0x98, 0x04, 0x90, 0x02);

	pe3_bigloop(dev->base_addr, 0x06);
	loop = pe3_status(dev->base_addr, 0x76, 0x06);
	
	pe3_noidea(dev->base_addr, 0x80, 0x04, 0x00, 0x02);

	/* Determine what sort of parallel port we have */

	pe3_mode = pe3_getmode(dev->base_addr);

	if (pe3_mode==0) {
		printk("pe3: using nibble transfer mode\n");
	} else {
		printk("pe3: using bi-directional transfer mode\n");	
	}

	w_ctr(dev->base_addr, 0x05);
	w_ctr(dev->base_addr, 0x04);

	pe3_pulse(dev->base_addr, 0x98);
	pe3_pulse(dev->base_addr, 0x00);
	pe3_pulse(dev->base_addr, (pe3_mode==0) ? 0x04 : 0x06);
	pe3_pulse(dev->base_addr, 0x00);

	w_dtr(dev->base_addr, 0x00);
	w_ctr(dev->base_addr, 0x05);
	w_ctr(dev->base_addr, 0x04);

	/* Download the contents of the PE3 EEPROM */

	if (pe3_readeeprom(dev)!=0) {
		printk("pe3: Adapter Address EEPROM unreadable\n");
		release_region(dev->base_addr, iosize);
		return 1;
	}

	printk("pe3: Ethernet Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
	        dev->dev_addr[0], dev->dev_addr[1], dev->dev_addr[2],
	        dev->dev_addr[3], dev->dev_addr[4], dev->dev_addr[5]);


	/* Possibly upload some info into the PE3 RAM */

	bytes[8] = dev->dev_addr[0];
	bytes[9] = dev->dev_addr[1];
	bytes[10] = dev->dev_addr[2];
	bytes[11] = dev->dev_addr[3];
	bytes[12] = dev->dev_addr[4];
	bytes[13] = dev->dev_addr[5];
	for (loop=0;loop<28;loop++) {
		pe3_upload(dev->base_addr, address[loop], bytes[loop]);
	}

	/* these return 0xff or if the adaptor has been previously
	   initialized 0xf7, why??? */

	printk("1: ");
	pe3_stumped(dev->base_addr, 0x68);
	printk("2: ");
	pe3_stumped(dev->base_addr, 0x68);
	printk("3: ");
	pe3_stumped(dev->base_addr, 0x68);
	printk("4: ");
	pe3_stumped(dev->base_addr, 0x68);

	pe3_upload(dev->base_addr, 0x44, 0x04);
	pe3_upload(dev->base_addr, 0x45, 0x03);
	pe3_upload(dev->base_addr, 0x46, 0x7a);
	pe3_upload(dev->base_addr, 0x48, 0x00);
	pe3_upload(dev->base_addr, 0x48, 0x01);
	pe3_pulse(dev->base_addr, 0xc8);

	/* send 256 bytes to adaptor */
	pe3_loader(dev->base_addr);

	udelay(3);
	pe3_upload(dev->base_addr, 0x4a, 0x81);
	pe3_stumped(dev->base_addr, 0x60);
	pe3_upload(dev->base_addr, 0x40, 0xff);
	pe3_upload(dev->base_addr, 0x41, 0xff);
	pe3_upload(dev->base_addr, 0x45, 0x02);

	if (pe3_mode==0) {
		pe3_pulse(dev->base_addr, 0x68);
	} else {
		pe3_pulse(dev->base_addr, 0xe8);
		pe3_pulse(dev->base_addr, 0x00);
		pe3_pulse(dev->base_addr, 0x05);
	}

	/* get the 256 bytes back */

	for (loop=0;loop<130;loop++) {
		w_ctr(dev->base_addr, 0x25);
		r_dtr(dev->base_addr);
		w_ctr(dev->base_addr, 0x24);
		r_dtr(dev->base_addr);
	}

	/* ^^ This could be a check of the adaptors RAM, ideas, proof? */

	if (pe3_mode==1) {
		w_ctr(dev->base_addr, 0x0c);
		udelay(1);
		w_ctr(dev->base_addr, 0x0c);
		udelay(1);
		w_dtr(dev->base_addr, 0x58);
		udelay(1);
		w_ctr(dev->base_addr, 0x04);
		udelay(1);
		pe3_pulse(dev->base_addr, 0x00);
		pe3_pulse(dev->base_addr, 0x06);
		w_ctr(dev->base_addr, 0x05);
		udelay(1);
		w_ctr(dev->base_addr, 0x04);
	}

	pe3_upload(dev->base_addr, 0x44, 0x02);
	R_str(dev->base_addr);
	w_ctr(dev->base_addr, 0x14);
	pe3_command(dev->base_addr, 0x01, 0x14);
	R_str(dev->base_addr);
	w_ctr(dev->base_addr, 0x14);
	pe3_command(dev->base_addr, 0x01, 0x14);
	


	/* for now return failed to stop the module being installed */

	release_region(dev->base_addr, iosize);
	return 1;
}

void pe3_setup(struct net_device *dev) {
	ether_setup(dev);
	dev->base_addr = pe3_base;
	dev->irq = pe3_irq;
	dev->open = pe3_probe;
}

static int __init load(void)
{
	printk("%s\n", version);
	pe3_dev = alloc_netdev(0, "pe%d", pe3_setup);
	if (pe3_dev != NULL && register_netdev(pe3_dev) != 0)
		return -EIO;
	return 0;
}

static void __exit cleanup(void)
{
	if (pe3_dev != NULL) {
		unregister_netdev(pe3_dev);
	}
}

module_init(load);
module_exit(cleanup);
MODULE_LICENSE("GPL");
