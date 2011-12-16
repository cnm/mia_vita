RT28xx_MODE = STA

TARGET = LINUX

CHIPSET = 3070

#OS ABL - YES or NO
OSABL = NO

ifneq ($(TARGET),THREADX)
#RT28xx_DIR = home directory of RT28xx source code
RT28xx_DIR = $(shell pwd)
endif

#ifndef $(BOARD)
#echo "Error: BOARD not specified"
#endif

RTMP_SRC_DIR = $(RT28xx_DIR)/RT$(CHIPSET)

#PLATFORM: Target platform
PLATFORM = PC

#RELEASE Package
RELEASE = DPO


ifeq ($(TARGET),LINUX)
MAKE = make
endif

ifeq ($(TARGET), UCOS)
MAKE = make
endif
ifeq ($(TARGET),THREADX)
MAKE = gmake
endif

ifeq ($(PLATFORM),PC)
ifeq ($(BOARD),7500)
# Linux 2.6
LINUX_SRC = /lib/modules/2.6.24.4/build/
# Linux 2.4 Change to your local setting
#LINUX_SRC = /usr/src/linux-2.4
LINUX_SRC_MODULE = /lib/modules/2.6.24.4/kernel/drivers/net/wireless/ralinkdir
CROSS_COMPILE = arm-unknown-linux-gnu-
else

ifeq ($(BOARD),73x0)
# Linux 2.6
LINUX_SRC = /home/karl/ts/kernel/73x0-linux26/
# Linux 2.4 Change to your local setting
#LINUX_SRC = /usr/src/linux-2.4
LINUX_SRC_MODULE = /lib/modules/2.6.21-ts/kernel/drivers/net/wireless/ralinkdir
CROSS_COMPILE = /home/karl/ts/arm-none-linux-gnueabi/bin/arm-none-linux-gnueabi-
else

ifeq ($(BOARD),7800)
# Linux 2.6
LINUX_SRC = /home/karl/ts/kernel/7800-linux26/
# Linux 2.4 Change to your local setting
#LINUX_SRC = /usr/src/linux-2.4
LINUX_SRC_MODULE = /lib/modules/2.6.21-ts/kernel/drivers/net/wireless/ralinkdir
CROSS_COMPILE = /home/karl/ts/arm-none-linux-gnueabi/bin/arm-none-linux-gnueabi-
else

ifeq ($(BOARD),4200)
# Linux 2.6
LINUX_SRC = /home/karl/ts/kernel/4200-linux26/
# Linux 2.4 Change to your local setting
#LINUX_SRC = /usr/src/linux-2.4
LINUX_SRC_MODULE = /lib/modules/2.6.36.2/kernel/drivers/net/wireless/ralinkdir
CROSS_COMPILE = /home/karl/ts/usr/local/opt/crosstool/arm-2007q1/bin/arm-none-linux-gnueabi-
GTLINUX2628 = TRUE
else  
echo "Error: Unknown BOARD specified"
endif
endif
endif
endif
endif

export OSABL RT28xx_DIR RT28xx_MODE LINUX_SRC CROSS_COMPILE CROSS_COMPILE_INCLUDE PLATFORM RELEASE CHIPSET RTMP_SRC_DIR LINUX_SRC_MODULE TARGET GTLINUX2628

# The targets that may be used.
PHONY += all build_tools test UCOS THREADX LINUX release prerelease clean uninstall install libwapi osabl

ifeq ($(TARGET),THREADX)
all: $(TARGET)
else
all: build_tools $(TARGET)
endif 



build_tools:
	$(MAKE) -C tools
	$(RT28xx_DIR)/tools/bin2h

test:
	$(MAKE) -C tools test

UCOS:
	$(MAKE) -C os/ucos/ MODE=$(RT28xx_MODE)
	echo $(RT28xx_MODE)


THREADX:
	$(MAKE) -C $(RT28xx_DIR)/os/Threadx -f $(RT28xx_DIR)/os/ThreadX/Makefile

LINUX:
ifneq (,$(findstring 2.4,$(LINUX_SRC)))

ifeq ($(OSABL),YES)
	cp -f os/linux/Makefile.4.util $(RT28xx_DIR)/os/linux/Makefile
	$(MAKE) -C $(RT28xx_DIR)/os/linux/
endif

	cp -f os/linux/Makefile.4 $(RT28xx_DIR)/os/linux/Makefile
	$(MAKE) -C $(RT28xx_DIR)/os/linux/

ifeq ($(OSABL),YES)
	cp -f os/linux/Makefile.4.netif $(RT28xx_DIR)/os/linux/Makefile
	$(MAKE) -C $(RT28xx_DIR)/os/linux/
endif

ifeq ($(RT28xx_MODE),AP)
	cp -f $(RT28xx_DIR)/os/linux/rt$(CHIPSET)ap.o .
ifeq ($(OSABL),YES)
	cp -f $(RT28xx_DIR)/os/linux/rtutil$(CHIPSET)ap.o .
	cp -f $(RT28xx_DIR)/os/linux/rtnet$(CHIPSET)ap.o .
endif
ifeq ($(PLATFORM),INF_AMAZON_SE)
	cp -f /tftpboot/rt2870ap.o /backup/ifx/build/root_filesystem/lib/modules/2.4.31-Amazon_SE-3.6.2.2-R0416_Ralink/kernel/drivers/net
endif
else	
ifeq ($(RT28xx_MODE),APSTA)
	cp -f $(RT28xx_DIR)/os/linux/rt$(CHIPSET)apsta.o .
ifeq ($(OSABL),YES)
	cp -f $(RT28xx_DIR)/os/linux/rtutil$(CHIPSET)apsta.o .
	cp -f $(RT28xx_DIR)/os/linux/rtnet$(CHIPSET)apsta.o .
endif
else
	cp -f $(RT28xx_DIR)/os/linux/rt$(CHIPSET)sta.o .
ifeq ($(OSABL),YES)
	cp -f $(RT28xx_DIR)/os/linux/rtutil$(CHIPSET)sta.o .
	cp -f $(RT28xx_DIR)/os/linux/rtnet$(CHIPSET)sta.o .
endif
endif	
endif	
else

ifeq ($(OSABL),YES)
	cp -f os/linux/Makefile.6.util $(RT28xx_DIR)/os/linux/Makefile
	$(MAKE) -C $(LINUX_SRC) SUBDIRS=$(RT28xx_DIR)/os/linux modules
endif

	cp -f os/linux/Makefile.6 $(RT28xx_DIR)/os/linux/Makefile
ifeq ($(PLATFORM),DM6446)
	$(MAKE)  ARCH=arm CROSS_COMPILE=arm_v5t_le- -C  $(LINUX_SRC) SUBDIRS=$(RT28xx_DIR)/os/linux modules
else
ifeq ($(PLATFORM),FREESCALE8377)
	$(MAKE) ARCH=powerpc CROSS_COMPILE=$(CROSS_COMPILE) -C  $(LINUX_SRC) SUBDIRS=$(RT28xx_DIR)/os/linux modules
else
	$(MAKE) -C $(LINUX_SRC) SUBDIRS=$(RT28xx_DIR)/os/linux modules
endif
endif

ifeq ($(OSABL),YES)
	cp -f os/linux/Makefile.6.netif $(RT28xx_DIR)/os/linux/Makefile
	$(MAKE) -C $(LINUX_SRC) SUBDIRS=$(RT28xx_DIR)/os/linux modules
endif

ifeq ($(RT28xx_MODE),AP)
	cp -f $(RT28xx_DIR)/os/linux/rt$(CHIPSET)ap.ko .
ifeq ($(OSABL),YES)
	cp -f $(RT28xx_DIR)/os/linux/rtutil$(CHIPSET)ap.ko .
	cp -f $(RT28xx_DIR)/os/linux/rtnet$(CHIPSET)ap.ko .
endif
	rm -f os/linux/rt$(CHIPSET)ap.ko.lzma
	/root/bin/lzma e os/linux/rt$(CHIPSET)ap.ko os/linux/rt$(CHIPSET)ap.ko.lzma
else	
ifeq ($(RT28xx_MODE),APSTA)
	cp -f $(RT28xx_DIR)/os/linux/rt$(CHIPSET)apsta.ko .
ifeq ($(OSABL),YES)
	cp -f $(RT28xx_DIR)/os/linux/rtutil$(CHIPSET)apsta.ko .
	cp -f $(RT28xx_DIR)/os/linux/rtnet$(CHIPSET)apsta.ko .
endif
else
	cp -f $(RT28xx_DIR)/os/linux/rt$(CHIPSET)sta.ko .
ifeq ($(OSABL),YES)
	cp -f $(RT28xx_DIR)/os/linux/rtutil$(CHIPSET)sta.ko .
	cp -f $(RT28xx_DIR)/os/linux/rtnet$(CHIPSET)sta.ko .
endif
endif
endif
endif


release:
ifeq ($(TARGET), LINUX)
	$(MAKE) -C $(RT28xx_DIR)/striptool -f Makefile.release clean
	$(MAKE) -C $(RT28xx_DIR)/striptool -f Makefile.release
	striptool/striptool.out
endif

prerelease:
ifeq ($(CHIPSET), 2880)
	$(MAKE) -C $(RT28xx_DIR)/os/linux -f Makefile.release.2880 prerelease
else
	$(MAKE) -C $(RT28xx_DIR)/os/linux -f Makefile.release prerelease
endif
	cp $(RT28xx_DIR)/os/linux/Makefile.DPB $(RTMP_SRC_DIR)/os/linux/.
	cp $(RT28xx_DIR)/os/linux/Makefile.DPA $(RTMP_SRC_DIR)/os/linux/.
	cp $(RT28xx_DIR)/os/linux/Makefile.DPC $(RTMP_SRC_DIR)/os/linux/.
ifeq ($(RT28xx_MODE),STA)
	cp $(RT28xx_DIR)/os/linux/Makefile.DPD $(RTMP_SRC_DIR)/os/linux/.
	cp $(RT28xx_DIR)/os/linux/Makefile.DPO $(RTMP_SRC_DIR)/os/linux/.
endif	

clean:
ifeq ($(TARGET), LINUX)
ifneq (,$(findstring 2.4,$(LINUX_SRC)))
	cp -f os/linux/Makefile.4 os/linux/Makefile
else
	cp -f os/linux/Makefile.6 os/linux/Makefile
endif
	$(MAKE) -C os/linux clean
	rm -rf os/linux/Makefile
endif	
ifeq ($(TARGET), UCOS)
	$(MAKE) -C os/ucos clean MODE=$(RT28xx_MODE)
endif

uninstall:
ifeq ($(TARGET), LINUX)
ifneq (,$(findstring 2.4,$(LINUX_SRC)))
	$(MAKE) -C $(RT28xx_DIR)/os/linux -f Makefile.4 uninstall
else
	$(MAKE) -C $(RT28xx_DIR)/os/linux -f Makefile.6 uninstall
endif
endif

install:
ifeq ($(TARGET), LINUX)
ifneq (,$(findstring 2.4,$(LINUX_SRC)))
	$(MAKE) -C $(RT28xx_DIR)/os/linux -f Makefile.4 install
else
	$(MAKE) -C $(RT28xx_DIR)/os/linux -f Makefile.6 install
endif
endif

libwapi:
ifneq (,$(findstring 2.4,$(LINUX_SRC)))
	cp -f os/linux/Makefile.libwapi.4 $(RT28xx_DIR)/os/linux/Makefile
	$(MAKE) -C $(RT28xx_DIR)/os/linux/
else
	cp -f os/linux/Makefile.libwapi.6 $(RT28xx_DIR)/os/linux/Makefile	
	$(MAKE) -C  $(LINUX_SRC) SUBDIRS=$(RT28xx_DIR)/os/linux modules	
endif	

osabl:
ifeq ($(OSABL),YES)
ifneq (,$(findstring 2.4,$(LINUX_SRC)))
	cp -f os/linux/Makefile.4.util $(RT28xx_DIR)/os/linux/Makefile
	$(MAKE) -C $(RT28xx_DIR)/os/linux/
	cp -f os/linux/Makefile.4.netif $(RT28xx_DIR)/os/linux/Makefile
	$(MAKE) -C $(RT28xx_DIR)/os/linux/
else
	cp -f os/linux/Makefile.6.util $(RT28xx_DIR)/os/linux/Makefile
	$(MAKE) -C $(LINUX_SRC) SUBDIRS=$(RT28xx_DIR)/os/linux modules
	cp -f os/linux/Makefile.6.netif $(RT28xx_DIR)/os/linux/Makefile
	$(MAKE) -C $(LINUX_SRC) SUBDIRS=$(RT28xx_DIR)/os/linux modules
endif
endif

# Declare the contents of the .PHONY variable as phony.  We keep that information in a variable
.PHONY: $(PHONY)

