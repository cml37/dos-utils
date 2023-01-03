# Makefile -- makefile for Xircom PE3 Ethernet adaptor 
#
KVERS ?= $(shell uname -r)

# distros use different paths for kernel include files
KBUILD  ?= /lib/modules/$(KVERS)/build
KSRC ?= $(shell if \
        [ -f /lib/modules/$(KVERS)/source/include/linux/kernel.h ]; then \
                echo /lib/modules/$(KVERS)/source ; \
        else \
                echo $(KBUILD); \
        fi)

CFLAGS += $(shell [ -f $(KSRC)/include/linux/modversions.h ] && \
                  echo -DEXPORT_SYMTAB -DMODVERSIONS \
                  -include $(KSRC)/include/linux/modversions.h)

# specify object modules
obj-m := pe3.o

all: clean default

clean:
	rm -f *.ko *.o *.bak pe3.o *.mod.c core

default:
	$(MAKE) -C $(KBUILD) SUBDIRS=$(PWD) modules
