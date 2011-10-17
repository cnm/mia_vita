/*  Copyright 2004-2011, Unpublished Work of Technologic Systems
 *  All Rights Reserved.
 *
 *  THIS WORK IS AN UNPUBLISHED WORK AND CONTAINS CONFIDENTIAL,
 *  PROPRIETARY AND TRADE SECRET INFORMATION OF TECHNOLOGIC SYSTEMS.
 *  ACCESS TO THIS WORK IS RESTRICTED TO (I) TECHNOLOGIC SYSTEMS EMPLOYEES
 *  WHO HAVE A NEED TO KNOW TO PERFORM TASKS WITHIN THE SCOPE OF THEIR
 *  ASSIGNMENTS  AND (II) ENTITIES OTHER THAN TECHNOLOGIC SYSTEMS WHO
 *  HAVE ENTERED INTO  APPROPRIATE LICENSE AGREEMENTS.  NO PART OF THIS
 *  WORK MAY BE USED, PRACTICED, PERFORMED, COPIED, DISTRIBUTED, REVISED,
 *  MODIFIED, TRANSLATED, ABRIDGED, CONDENSED, EXPANDED, COLLECTED,
 *  COMPILED,LINKED,RECAST, TRANSFORMED, ADAPTED IN ANY FORM OR BY ANY
 *  MEANS,MANUAL, MECHANICAL, CHEMICAL, ELECTRICAL, ELECTRONIC, OPTICAL,
 *  BIOLOGICAL, OR OTHERWISE WITHOUT THE PRIOR WRITTEN PERMISSION AND
 *  CONSENT OF TECHNOLOGIC SYSTEMS . ANY USE OR EXPLOITATION OF THIS WORK
 *  WITHOUT THE PRIOR WRITTEN CONSENT OF TECHNOLOGIC SYSTEMS  COULD
 *  SUBJECT THE PERPETRATOR TO CRIMINAL AND CIVIL LIABILITY.
 */
/* To compile ts7500ctl, use the appropriate cross compiler and run the
 * command:
 *
 *  gcc -Wall -O -mcpu=arm9 -o ts7500ctl ts7500ctl.c ispvm.c 
 *
 * You need ispvm.c and vmopcode.h in the same directory as ts7500ctl to
 * compile.
 *
 * On uclibc based initrd's, the following additional gcc options are
 * necessary: -Wl,--rpath,/slib -Wl,-dynamic-linker,/slib/ld-uClibc.so.0
 *
 * If you want --loadfpga to work with .jed files, the executable "jed2vme"
 * should be in the $PATH during runtime.
 */

const char copyright[] = "Copyright (c) Technologic Systems - " __DATE__ ;

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <libgen.h>
#include <limits.h>
#include <linux/unistd.h>
#include <netdb.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/timex.h>
#include <sys/types.h>
#include <time.h>
#include <time.h>
#include <unistd.h>

static void poke16(unsigned int, unsigned short);
static unsigned short peek16(unsigned int);
void sbuslock(void);
void sbusunlock(void);

volatile unsigned int *cvspiregs, *cvgpioregs, *cvtwiregs;
volatile unsigned int *cvmiscregs, *cvtimerregs;
static int last_gpio_adr = 0;

extern signed char ispVM(char *);

static void poke16(unsigned int adr, unsigned short dat) {
	unsigned int dummy = 0;
	unsigned int d = dat;
	if (last_gpio_adr != adr >> 5) {
		last_gpio_adr = adr >> 5;
		cvgpioregs[0] = (cvgpioregs[0] & ~(0x3<<15))|((adr>>5)<<15);
	}
	adr &= 0x1f;

	if (!cvspiregs) {
		*(unsigned short *)adr = dat;
	} else asm volatile (
		"mov %0, %1, lsl #18\n"
		"orr %0, %0, #0x800000\n"
		"orr %0, %0, %2, lsl #3\n"
		"3: ldr r1, [%3, #0x64]\n"
		"cmp r1, #0x0\n"
		"bne 3b\n"
		"2: str %0, [%3, #0x50]\n"
		"1: ldr r1, [%3, #0x64]\n"
		"cmp r1, #0x0\n"
		"beq 1b\n"
		"ldr %0, [%3, #0x58]\n"
		"ands r1, %0, #0x1\n"
		"moveq %0, #0x0\n"
		"beq 3b\n"
		: "+r"(dummy) : "r"(adr), "r"(d), "r"(cvspiregs) : "r1","cc"
	);
}


static inline void 
poke16_stream(unsigned int adr, unsigned short *dat, unsigned int len) {
	volatile unsigned int *spi = cvspiregs;
	unsigned int cmd, ret, i, j;

	if (last_gpio_adr != adr >> 5) {
		last_gpio_adr = adr >> 5;
		cvgpioregs[0] = ((adr>>5)<<15|1<<3|1<<17);
	}
	adr &= 0x1f;

	spi[0x4c/4] = 0x0;	/* continuous CS# */
	cmd = (adr<<18) | 0x800000 | (dat[0]<<3) | (dat[1]>>13);
	do {
		spi[0x50/4] = cmd;
		cmd = dat[1]>>13;
		while (spi[0x64/4] == 0);
		ret = spi[0x58/4];
		assert (spi[0x64/4] == 0); /* */
	} while (!(ret & 0x1));
	
	spi[0x40/4] = 0x80000c01; /* 16 bit mode */
	i = len - 1;
	len -= 6;
	dat++;

	for (j = 0; j < 4; j++) {
		spi[0x50/4] = (dat[0]<<3) | (dat[1]>>13);
		dat++;
	}

	while (len--) {
		spi[0x50/4] = (dat[0]<<3) | (dat[1]>>13);
		dat++;
		while (spi[0x64/4] == 0);
		spi[0x58/4]; 
		i--;
	} 

	spi[0x4c/4] = 0x4;	/* deassert CS# */
	spi[0x50/4] = dat[0]<<3;

	while (i) {
		while ((spi[0x64/4]) == 0);
		spi[0x58/4];
		i--;
	}

	spi[0x40/4] = 0x80000c02; /* 24 bit mode */
}


static unsigned short peek16(unsigned int adr) {
	unsigned short ret = 0;

	if (last_gpio_adr != adr >> 5) {
		last_gpio_adr = adr >> 5;
		cvgpioregs[0] = ((adr>>5)<<15|1<<3|1<<17);
	}

	adr &= 0x1f;

	if (!cvspiregs) ret = *(unsigned short *)adr;
	else asm volatile (
		"mov %0, %1, lsl #18\n"
		"2: str %0, [%2, #0x50]\n"
		"1: ldr r1, [%2, #0x64]\n"
		"cmp r1, #0x0\n"
		"beq 1b\n"
		"ldr %0, [%2, #0x58]\n"
		"ands r1, %0, #0x10000\n"
		"bicne %0, %0, #0xff0000\n"
		"moveq %0, #0x0\n"
		"beq 2b\n" 
		: "+r"(ret) : "r"(adr), "r"(cvspiregs) : "r1", "cc"
	);

	return ret;
}


static inline
void peek16_stream(unsigned int adr, unsigned short *dat, unsigned int len) {
	unsigned int dummy = 0;
	if (last_gpio_adr != adr >> 5) {
		last_gpio_adr = adr >> 5;
		cvgpioregs[0] = ((adr>>5)<<15|1<<3|1<<17);
	}
	adr &= 0x1f;

	asm volatile(
		"mov %0, #0x0\n"
		"str %0, [%4, #0x4c]\n"
		"mov %1, %1, lsl #18\n"
		"orr %1, %1, #(1<<15)\n"
		"2: str %1, [%4, #0x50]\n"
		"1: ldr %0, [%4, #0x64]\n"
		"cmp %0, #0x0\n"
		"beq 1b\n"
		"ldr %0, [%4, #0x58]\n"
		"tst %0, #0x10000\n"
		"beq 2b\n"
		"\n"
		"3:\n"
		"strh %0, [%3], #0x2\n"
		"mov %0, #0x80000001\n"
		"orr %0, %0, #0xc00\n"
		"str %0, [%4, #0x40]\n"
		"ldr %0, [%4, #0x40]\n"
		"str %1, [%4, #0x50]\n"
		"str %1, [%4, #0x50]\n"
		"sub %2, %2, #0x4\n"
		"4: str %1, [%4, #0x50]\n"
		"5: ldr %0, [%4, #0x64]\n"
		"cmp %0, #0x0\n"
		"beq 5b\n"
		"ldr %0, [%4, #0x58]\n"
		"subs %2, %2, #0x1\n"
		"strh %0, [%3], #0x2\n"
		"bne 4b\n"
		"\n"
		"mov %0, #0x4\n"
		"str %0, [%4, #0x4c]\n"
		"mov %1, #0x0\n"
		"str %1, [%4, #0x50]\n"
		"6: ldr %0, [%4, #0x64]\n"
		"cmp %0, #0x0\n"
		"beq 6b\n"
		"ldr %0, [%4, #0x58]\n"
		"strh %0, [%3], #0x2\n"
		"\n"
		"7: ldr %0, [%4, #0x64]\n"
		"cmp %0, #0x0\n"
		"beq 7b\n"
		"ldr %0, [%4, #0x58]\n"
		"strh %0, [%3], #0x2\n"
		"\n"
		"8: ldr %0, [%4, #0x64]\n"
		"cmp %0, #0x0\n"
		"beq 8b\n"
		"ldr %0, [%4, #0x58]\n"
		"strh %0, [%3], #0x2\n"
		"\n"
		"mov %0, #0x80000002\n"
		"orr %0, %0, #0xc00\n"
		"str %0, [%4, #0x40]\n"
		: "+r"(dummy), "+r"(adr), "+r"(len), "+r"(dat)
		: "r"(cvspiregs)
		: "cc", "memory"
	);
}


void sspi_cmd(unsigned int cmd) {
	unsigned int i;
	unsigned int s = peek16(0x66) & 0xfff0;
	
	// pulse CS#
	poke16(0x66, s | 0x2); 
	poke16(0x66, s | 0x0);

	for (i = 0; i < 32; i++, cmd <<= 1) {
		if (cmd & 0x80) {
			poke16(0x66, s | 0x4);
			poke16(0x66, s | 0xc);
		} else {
			poke16(0x66, s | 0x0);
			poke16(0x66, s | 0x8);
		}
	}
}


int read_tagmem(unsigned int *tagmem) {
	int i, j;
	unsigned int ret;
	unsigned int s = peek16(0x66) & 0xfff0;

	sspi_cmd(0xac); // X_PROGRAM_EN
	sspi_cmd(0x4e); // READ_TAG
	
	for (j = 0; j < 20; j++) {
		for (ret = 0x0, i = 0; i < 32; i++) {
			poke16(0x66, s | 0x0);
			poke16(0x66, s | 0x8);
			ret = ret << 1 | (peek16(0x66) & 0x1);
		}
		tagmem[j] = ret;
	}
	
	sspi_cmd(0x78); // PROGRAM_DIS
	
	poke16(0x66, s | 0x2);
	return 1;
}


int write_tagmem(unsigned int *tagmem) {
	int i, j;
	unsigned int s = peek16(0x66) & 0xfff0;
	
	sspi_cmd(0xac); // X_PROGRAM_EN
	sspi_cmd(0x8e); // WRITE_TAG
	
	for (j = 0; j < 20; j++) {
		unsigned int x = tagmem[j];
		for (i = 0; i < 32; i++, x <<= 1) {
			if (x & 0x80000000UL) {
				poke16(0x66, s | 0x4);
				poke16(0x66, s | 0xc);
			} else {
				poke16(0x66, s | 0x0);
				poke16(0x66, s | 0x8);
			}
			if (i == 23 && j == 19) break;
		}
	}
	
	for (i = 0; i < 8; i++) {
		poke16(0x66, s | 0x2);
		poke16(0x66, s | 0xa);
	}
	poke16(0x66, s | 0x2);
	sbusunlock();
	usleep(25000);
	sbuslock();
	return 1;
}


int erase_tagmem() {
	int i;
	unsigned int s = peek16(0x66) & 0xfff0;

	sspi_cmd(0xac); // X_PROGRAM_EN
	sspi_cmd(0xe); // ERASE_TAG

	for (i = 0; i < 8; i++) {
		poke16(0x66, s | 0x2);
		poke16(0x66, s | 0xa);
	}
	poke16(0x66, s | 0x2);
	sbusunlock();
	usleep(1000000);
	sbuslock();
	return 1;
}


static inline void pokebit16(int adrs,int bit,int value) {
       value = value ? (1<<bit) : 0;
       poke16(adrs, (peek16(adrs) & ~(1 << bit)) | value);
}

static inline int peekbit16(int adrs,int bit) {
       return (peek16(adrs) & (1 << bit)) ? 1 : 0;
}

int getbaseboardid() {
       int i,gled,rled,dio25,dio25dir;
       int value = 0;

       gled = peekbit16(0x62, 15);
       rled = peekbit16(0x62, 14);
       dio25 = peekbit16(0x6a, 4);
       dio25dir = peekbit16(0x6c, 4);
       pokebit16(0x72, 0, 0); // DIO 5 dir
       pokebit16(0x6c, 4, 1); // DIO 25 dir
       for (i=7;i>=0;i--) {
	       pokebit16(0x62, 14, i&1?0:1); // red LED
	       pokebit16(0x62, 15, i&2?0:1); // green LED
	       pokebit16(0x6a, 4, i&4?1:0); // DIO 25
	       value <<= 1;
	       if (peekbit16(0x6e, 0)) value |= 1; // DIO 5
       }
       pokebit16(0x62, 15, gled); // green LED
       pokebit16(0x62, 14, rled); // red LED
       pokebit16(0x6a, 4, dio25); // DIO 25
       pokebit16(0x6c, 4, dio25dir); // DIO 25 dir
       return value;
}


static void reservemem(void) {
	char dummy[32768];
	int i, pgsize;
	FILE *maps;

	pgsize = getpagesize();
	mlockall(MCL_CURRENT|MCL_FUTURE);
	for (i = 0; i < sizeof(dummy); i += 4096) {
		dummy[i] = 0;
	}

	maps = fopen("/proc/self/maps", "r"); 
	if (maps == NULL) {
		perror("/proc/self/maps");
		exit(1);
	}
	while (!feof(maps)) {
		size_t s, e, x;
		char perms[16];
		int r = fscanf(maps, "%zx-%zx %s %*x %zx:%*x %*d",
		  &s, &e, perms, &x);
		if (r == EOF) break;
		assert (r == 4);

		while ((r = fgetc(maps)) != '\n') if (r == EOF) break;
		assert(s <= e && (s & 0xfff) == 0);
		if (perms[0] == 'r' && perms[3] == 'p' && x != 1)
		  while (s < e) {
			volatile unsigned char *ptr = (unsigned char *)s;
			unsigned char d;
			d = *ptr;
			if (perms[1] == 'w') *ptr = d;
			s += pgsize;
		}
	}

}


static int semid = -1;
static int sbuslocked = 0;
void sbuslock(void) {
	int r;
	struct sembuf sop;
	if (semid == -1) {
		key_t semkey;
		reservemem();
		semkey = 0x75000000;
		semid = semget(semkey, 1, IPC_CREAT|IPC_EXCL|0777);
		if (semid != -1) {
			sop.sem_num = 0;
			sop.sem_op = 1;
			sop.sem_flg = 0;
			r = semop(semid, &sop, 1);
			assert (r != -1);
		} else semid = semget(semkey, 1, 0777);
		assert (semid != -1);
	}
	sop.sem_num = 0;
	sop.sem_op = -1;
	sop.sem_flg = SEM_UNDO;
	r = semop(semid, &sop, 1);
	assert (r == 0);
	cvgpioregs[0] = (cvgpioregs[0] & ~(0x3<<15))|(last_gpio_adr<<15);
	sbuslocked = 1;
}


void sbusunlock(void) {
	struct sembuf sop = { 0, 1, SEM_UNDO};
	int r;
	if (!sbuslocked) return;
	r = semop(semid, &sop, 1);
	assert (r == 0);
	sbuslocked = 0;
}


int i2cwrite(unsigned adrs, int len, unsigned data) {
	unsigned int s;

	cvtwiregs[3] = data;
	cvtwiregs[2] = adrs;
	cvtwiregs[0] = 0x80000050 | (((len-1)&3)<<2);
	while (((s = cvtwiregs[5]) & 0x3) == 0);
	cvtwiregs[4] = 3; /* clear status */
	if (s & 0x1) return -1; else return 0;
}


int i2cread(unsigned adrs, int len, unsigned *ret) {
	unsigned int s;

	cvtwiregs[0] = 0x80000040 | ((len-1)&3);
	while (((s = cvtwiregs[5]) & 0x3) == 0);
	cvtwiregs[4] = 3; /* clear status */
	*ret = cvtwiregs[4];
	if (s & 0x1) return -1; else return 0;
}


/* Returns I2C temp in *val in milliCelsius */
int read_LM73_temp(int *val) {
	int s;
	unsigned int id, dat;

	*val = 0;

	cvtwiregs[4] = 3; /* clear status */
	cvtwiregs[6] = 0; /* no interrupts enabled */
	cvtwiregs[1] = 0x4d90; /* timeout enabled, 400Khz on 62.5Mhz PCLK */
	
	/* Detect LM73 magic */
	s = i2cwrite(0x92, 1, 0x7);
	if (s == -1) return -1;
	i2cread(0x92, 2, &id);
	if ((id & 0xffff) != 0x9001) return -1;
	
	/* Power on/off required in initialization sequence */
	i2cwrite(0x92, 1, 0x1); 
	i2cwrite(0x92, 2, 0x4001 | (1 << 15)); /* Power off */
	usleep(150000);
	i2cwrite(0x92, 2, 0x4001); /* Power on */
	usleep(150000);
	
	/* Read temperature reading - 16bit 2-complement 1/128th degC */
	i2cwrite(0x92, 1, 0x0);
	i2cread(0x92, 2, &dat);
	*val = (short)((dat & 0xff) << 8 | (dat & 0xff00) >> 8);
	*val = (*val * 1000) / 128;
	return 0;
}


static void usage(char **argv) {
	fprintf(stderr, "Usage: %s [OPTION] ...\n"
	  "Technologic Systems SBUS manipulation.\n"
	  "\n"
	  "General options:\n"
	  "  -a, --address=ADR       SBUS address\n"
	  "  -r, --peek16            16-bit SBUS read\n"
	  "  -w, --poke16=VAL        16-bit SBUS write\n"
	  "  -g, --getmac            Display ethernet MAC address\n"
	  "  -s, --setmac=MAC        Set ethernet MAC address\n"
	  "  -R, --reboot            Reboot the board\n"
	  "  -t, --getrtc            Display RTC time/date\n"
	  "  -S, --setrtc            Set RTC time/date from system time\n"
	  "  -F, --diffrtc           Print RTC drift from system time\n"
	  "  -i, --info              Display board FPGA info\n"
	  "  -e, --greenledon        Turn green LED on\n"
	  "  -b, --greenledoff       Turn green LED off\n"
	  "  -c, --redledon          Turn red LED on\n"
	  "  -d, --redledoff         Turn red LED off\n"
	  "  -D, --setdio=LVAL       Set DIO output to LVAL\n"
	  "  -O, --setdiodir=LVAL    Set DIO direction to LVAL (1 - output)\n"
	  "  -G, --getdio            Get DIO input\n"
	  "  -Z, --getdioreg         Get DIO direction and output register values\n"
	  "  -x, --random            Get 16-bit hardware random number\n"
	  "  -W, --watchdog          Daemonize and set up /dev/watchdog\n"
	  "  -A, --autofeed=SETTING  Daemonize and auto feed watchdog\n"
	  "  -n, --setrng            Seed the kernel random number generator\n"
	  "  -X, --resetswitchon     Enable reset switch\n"
	  "  -Y, --resetswitchoff    Disable reset switch\n"
	  "  -I, --extendedtempon    Enable extended temp (200Mhz CPU)\n"
	  "  -C, --extendedtempoff   Disable extended temp (250Mhz CPU)\n"
	  "  -l, --loadfpga=FILE     Load FPGA bitstream from FILE\n"
	  "  -T, --gettemp           Print board temperature\n"
	  "  -f, --rs422={1|0}       On TS-4500, enable RS422/full-duplex RS485\n"
	  "  -q, --rtcdrift=PPM      Set board RTC oscillator calibration\n"
	  "  -Q, --cpudrift=PPM      Set board CPU oscillator calibration\n"
	  "  -h, --help              This help\n",
	  argv[0]
	);
}

static inline
unsigned char tobcd(unsigned int n) {
	unsigned char ret;
	ret = (n / 10) << 4;
	ret |= n % 10;
	return ret;
}

static inline unsigned int frombcd(unsigned char x) {
	unsigned int ret;
	ret = (x >> 4) * 10;
	ret += x & 0xf;
	return ret;
}

int main(int argc, char **argv) {
	int devmem, i, j, c, ret, wdog = 0;
	unsigned int tagmem[20];
	int opt_peek16 = 0, opt_address = 0, opt_poke16 = 0, opt_pokeval = 0;
	int opt_getmac = 0, opt_setmac = 0, opt_reboot = 0, opt_getrtc = 0;
	int opt_setrtc = 0, opt_info = 0, opt_greenledon = 0;
	int opt_greenledoff = 0, opt_redledon = 0, opt_redledoff = 0;
	int opt_setdio = 0, opt_setdiodir = 0, opt_getdio = 0, opt_random = 0;
	int opt_watchdog = 0, opt_autofeed = 0, opt_resetswitchon = 0;
	int opt_resetswitchoff = 0, opt_extendedtempon = 0, opt_gettemp = 0;
	int opt_extendedtempoff = 0, opt_getdioreg = 0, opt_setrng = 0;
	int opt_rs422 = 0, opt_rs422_set = 0, opt_diffrtc = 0;
	int opt_rtcdrift = 0, opt_cpudrift = 0, opt_rtcdrift_set = 0;
	int opt_cpudrift_set = 0;
	unsigned long long dio = 0, oe = 0;
	char *opt_mac = NULL, *opt_loadfpga = NULL;
	static struct option long_options[] = {
	  { "help", 0, 0, 'h' },
	  { "peek16", 0, 0, 'r' },
	  { "poke16", 1, 0, 'w' },
	  { "address", 1, 0, 'a' },
	  { "getmac", 0, 0, 'g' },
	  { "setmac", 1, 0, 's' },
	  { "reboot", 0, 0, 'R' },
	  { "getrtc", 0, 0, 't' },
	  { "setrtc", 0, 0, 'S' },
	  { "diffrtc", 0, 0, 'F' },
	  { "greenledon", 0, 0, 'e'},
	  { "greenledoff", 0, 0, 'b'},
	  { "redledon", 0, 0, 'c'},
	  { "redledoff", 0, 0, 'd'},
	  { "setdio", 1, 0, 'D'},
	  { "setdiodir", 1, 0, 'O'},
	  { "getdio", 0, 0, 'G'},
	  { "getdioreg", 0, 0, 'Z'},
	  { "info", 0, 0, 'i' },
	  { "random", 0, 0, 'x' },
	  { "watchdog", 0, 0, 'W' },
	  { "autofeed", 1, 0, 'A' },
	  { "setrng", 0, 0, 'n' },
	  { "resetswitchon", 0, 0, 'X' },
	  { "resetswitchoff", 0, 0, 'Y' },
	  { "extendedtempon", 0, 0, 'I' },
	  { "extendedtempoff", 0, 0, 'C' },
	  { "loadfpga", 1, 0, 'l' },
	  { "gettemp", 0, 0, 'T' },
	  { "rs422", 1, 0, 'f' },
	  { "rtcdrift", 1, 0, 'q' },
	  { "cpudrift", 1, 0, 'Q' },
	  { 0, 0, 0, 0 }
	};

	while((c = getopt_long(argc, argv,
	  "hra:w:gs:RtSiebcdD:O:GxWA:XYICZl:nTfFq:Q:", long_options, NULL)) != -1) {
		switch (c) {
		case 'q':
			opt_rtcdrift = strtoul(optarg, NULL, 0);
			opt_rtcdrift_set = 1;
			break;
		case 'Q':
			opt_cpudrift = strtoul(optarg, NULL, 0);
			opt_cpudrift_set = 1;
			break;
		case 'F':
			opt_diffrtc = 1;
			break;
		case 'f':
			if (strcmp(optarg, "on") == 0 || 
			  strcmp(optarg, "ON") == 0 || 
			  strtoul(optarg, NULL, 0) == 1) opt_rs422 = 1;
			opt_rs422_set = 1;
			break;
		case 'T':
			opt_gettemp = 1;
			break;
		case 'n':
			opt_setrng = 1;
			break;
		case 'l':
			opt_loadfpga = strdup(optarg);
			break;
		case 'Z':
			opt_getdioreg = 1;
			break;
		case 'I':
			opt_extendedtempon = 1;
			break;
		case 'C':
			opt_extendedtempoff = 1;
			break;
		case 'X':
			opt_resetswitchon = 1;
			break;
		case 'Y':
			opt_resetswitchoff = 1;
			break;
		case 'W':
			opt_watchdog = 1;
			break;
		case 'A':
			opt_watchdog = 1;
			opt_autofeed = 1;
			wdog = strtoul(optarg, NULL, 0);
			if (wdog > 3) {
				fprintf(stderr, 
				  "autofeed must be between 0 and 3\n");
				exit(1);
			}
			break;
		case 'D':
			opt_setdio = 1;	
			dio = strtoull(optarg, NULL, 0);
			break;
		case 'O':
			opt_setdiodir = 1;
			oe = strtoull(optarg, NULL, 0);
			break;
		case 'G':
			opt_getdio = 1;
			break;
		case 'e':
			opt_greenledon = 1;
			break;
		case 'b':
			opt_greenledoff = 1;
			break;
		case 'c':
			opt_redledon = 1;
			break;
		case 'd':
			opt_redledoff = 1;
			break;
		case 'i':
			opt_info = 1;
			break;
		case 'S':
			opt_setrtc = 1;
			break;
		case 'r':
			opt_peek16 = 1;
			break;
		case 'a':
			opt_address = strtoul(optarg, NULL, 0);
			break;
		case 'w':
			opt_poke16 = 1;
			opt_pokeval = strtoul(optarg, NULL, 0);
			break;
		case 'g':
			opt_getmac = 1;
			break;
		case 's':
			opt_setmac = 1;
			opt_mac = strdup(optarg);
			break;
		case 'R':
			opt_reboot = 1;
			break;
		case 't':
			opt_getrtc = 1;
			break;
		case 'x':
			opt_random = 1;
			break;
		case 'h':
		default:
			usage(argv);
			return(1);
		}
	} 
	
	devmem = open("/dev/mem", O_RDWR|O_SYNC);
	assert(devmem != -1);
	cvspiregs = (unsigned int *) mmap(0, 4096,
	  PROT_READ | PROT_WRITE, MAP_SHARED, devmem, 0x71000000);
	cvtwiregs = cvspiregs + (0x20/sizeof(unsigned int));
	cvgpioregs = (unsigned int *) mmap(0, 4096,
	  PROT_READ | PROT_WRITE, MAP_SHARED, devmem, 0x7c000000);

	if (opt_loadfpga) {
		key_t shmkey;
		int shmid;
		unsigned int *sbus_shm;
		unsigned int prev_gpioa_en, prev_gpioa_ddr;
		signed char x;
		const char * ispvmerr[] = { "pass", "verification fail", 
		  "can't find the file", "wrong file type", "file error", 
		  "option error", "crc verification error" };
	
		cvmiscregs = (unsigned int *) mmap(0, 4096, 
		  PROT_READ | PROT_WRITE, MAP_SHARED, devmem, 0x76000000);
		cvtimerregs = (unsigned int *) mmap(0, 4096, 
		  PROT_READ | PROT_WRITE, MAP_SHARED, devmem, 0x79000000);
		prev_gpioa_en = cvmiscregs[0x20/4];
		prev_gpioa_ddr = cvgpioregs[0x8/4];
		/* switch A2, A22-A24 into GPIO mode */
		cvmiscregs[0x20/4] &= ~0x1c00004; 
		/* A2, A22, A23 as outputs */
		cvgpioregs[0x8/4] |= (1<<22|1<<23|1<<2); 
		cvgpioregs[0x8/4] &= ~(1<<24); /* A24 as inputs */

		x = ispVM(opt_loadfpga);
		if (x == 0) {
			printf("loadfpga_ok=1\n");
		} else {
			assert(x < 0);
			printf("loadfpga_ok=0\n");
			printf("loadfpga_error=\"%s\"\n", ispvmerr[-x]);
		}

		cvmiscregs[0x20/4] = prev_gpioa_en;
		cvgpioregs[0x8/4] = prev_gpioa_ddr;

		shmkey = 0x75000000;
		shmid = shmget(shmkey, 0x1000, IPC_CREAT);
		assert(shmid != -1);
		sbus_shm = shmat(shmid, NULL, 0);
		for (i = 0; i < 9; i++) if (sbus_shm[i*32] != 0) {
			ret = kill(sbus_shm[i*32], SIGHUP);
			if (ret == -1 && errno == ESRCH) sbus_shm[i*32] = 0;
		}
		sbusunlock();
	} 
	
	sbuslock();
	cvspiregs[0x64 / 4] = 0x0; /* RX IRQ threahold 0 */
	cvspiregs[0x40 / 4] = 0x80000c02; /* 24-bit mode, no byte swap */
	cvspiregs[0x60 / 4] = 0x0; /* 0 clock inter-transfer delay */
	cvspiregs[0x6c / 4] = 0x0; /* disable interrupts */
	cvspiregs[0x4c / 4] = 0x4; /* deassert CS# */
	for (i = 0; i < 8; i++) cvspiregs[0x58 / 4];
	last_gpio_adr = 3;
	cvgpioregs[0] = (0x3<<15|1<<3|1<<17);

/*
	n = basename(argv[0]);
	if (strcmp(n, "sbus_peek16") == 0) {
		int adr = strtoul(argv[1], NULL, 0);
		printf("0x%x\n", peek16(adr));
		return 0;
	} else if (strcmp(n, "sbus_poke16") == 0) {
		int adr = strtoul(argv[1], NULL, 0);
		int dat = strtoul(argv[2], NULL, 0);
		poke16(adr, dat);
		return 0;
	}
*/

	if (opt_gettemp) {
		sbusunlock();
		ret = read_LM73_temp(&i);
		printf("tempsensor_ok=%d\n", ret == -1 ? 0 : 1);
		printf("temperature=%d.%03d\n", i / 1000, abs(i % 1000));
		sbuslock();
	}

	if (opt_random) {
		i = peek16(0x64);
		sbusunlock();
		printf("random=0x%x\n", i);
		sbuslock();
	}

	if (opt_setrng) {
		FILE *urandom = NULL;
		unsigned short rng = peek16(0x64);

		sbusunlock();
		urandom = fopen("/dev/urandom", "w");
		assert(urandom != NULL);
		fwrite(&rng, 2, 1, urandom);
		fclose(urandom);
		sbuslock();
	}

	if (opt_resetswitchon) {
		unsigned short x;
		x = peek16(0x76);
		x |= 0x10;
		poke16(0x76, x);
	} else if (opt_resetswitchoff) {
		unsigned short x;
		x = peek16(0x76);
		x &= ~0x10;
		poke16(0x76, x);
	}

	if (opt_extendedtempon) {
		read_tagmem(tagmem);
		erase_tagmem();
		tagmem[0] &= ~0x80;
		write_tagmem(tagmem);
	} else if (opt_extendedtempoff) {
		read_tagmem(tagmem);
		erase_tagmem();
		tagmem[0] |= 0x80;
		write_tagmem(tagmem);
	}

	if (opt_rtcdrift_set) {
		if (opt_rtcdrift > 128) opt_rtcdrift = 128;
		if (opt_rtcdrift < -64) opt_rtcdrift = -64;
		read_tagmem(tagmem);
		erase_tagmem();
		tagmem[1] &= ~0xff;
		tagmem[1] |= (~(unsigned char)opt_rtcdrift & 0xff);
		write_tagmem(tagmem);
	}

	if (opt_cpudrift_set) {
		read_tagmem(tagmem);
		erase_tagmem();
		opt_cpudrift = opt_cpudrift / 2;
		tagmem[1] &= ~0xff00;
		tagmem[1] |= (~(unsigned char)opt_cpudrift & 0xff) << 8;
		write_tagmem(tagmem);
	}

	if (opt_info) {
		int a, b, c;
		unsigned short x = 0;
		a = peek16(0x60);
		b = peek16(0x62);
		c = peek16(0x76);
		if (a == 0x4500) x = getbaseboardid();
		sbusunlock();
		printf("model=0x%x\n", a);
		if (a == 0x4500) {
			printf("baseboard_model=0x%x\n", x);
			printf("rs422=0x%x\n", (c & (3<<12)) ? 1 : 0);
		}
		printf("submodel=0x%x\n", (b >> 4) & 0xf);
		printf("revision=0x%x\n", b & 0xf);
		printf("bootmode=0x%x\n", c & 0x3);
		printf("bootdev=0x%x\n", (c >> 2) & 0x3);
		printf("resetsw_en=0x%x\n", (c >> 4) & 0x1);
		printf("latched_dio9=0x%x\n", (c >> 5) & 0x1);
		opt_getdio = 1;
		sbuslock();
	}

	if (opt_rs422_set && peek16(0x60) == 0x4500) {
		int a, b;

		a = peek16(0x76);
		b = getbaseboardid();
		a &= ~(0x3<<12); 
		if (b == 3) a |= 1<<12; /* base board is TS-8510 */
		else a |= 1<<13; /* all other base boards */

		sbusunlock();
		printf("rs422_set=%d\n", opt_rs422);
		sbuslock();
	}

	if (opt_getdio) {
		unsigned long long v;
		v = peek16(0x6e) << 5;
		v |= (unsigned long long)peek16(0x68) << 21;
		v |= (unsigned long long)((peek16(0x66) >> 12)) << 37;
		if (peek16(0x60) == 0x4500) { /* TS-4500 has more DIO */
			v |= (unsigned long long)(peek16(0x78) & 0x7c00) >> 10;
			v |= (unsigned long long)(peek16(0x7c) & 0xfff) << 41;
		}
		sbusunlock();
		printf("dio=0x%llx\n", v);
		sbuslock();
	}

	if (opt_setdio) {
		unsigned int x, y;
		x = (dio >> 5) & 0xffff;
		poke16(0x70, x);
		x = (dio >> 21) & 0xffff;
		poke16(0x6a, x);
		y = peek16(0x66) & ~(0xf00);
		x = (dio >> 37) & 0xf;
		poke16(0x66, y | (x<<8));
		if (peek16(0x60) == 0x4500) {  /* TS-4500 has more DIO */
			y = peek16(0x78) & ~(0x3e0);
			x = dio & 0x1f;
			poke16(0x78, y | (x<<5));
			y = peek16(0x7a) & ~(0xff00);
			x = (dio >> 41) & 0xff;
			poke16(0x7a, y | (x<<8));
			y = peek16(0x7e) & ~(0x39);
			x = (dio >> 49) & 0xf;
			poke16(0x7e, y | (x<<4));
		}
	}

	if (opt_setdiodir) {
		unsigned int x, y;
		x = (oe >> 5) & 0xffff;
		poke16(0x72, x);
		x = (oe >> 21) & 0xffff;
		poke16(0x6c, x);
		y = peek16(0x66) & ~(0xf0);
		x = (oe >> 37) & 0xf;
		poke16(0x66, y | (x<<4));
		if (peek16(0x60) == 0x4500) {  /* TS-4500 has more DIO */
			y = peek16(0x78) & ~(0x1f);
			x = oe & 0x1f;
			poke16(0x78, y | x);
			y = peek16(0x7a) & ~(0xff);
			x = (oe >> 41) & 0xff;
			poke16(0x7a, y | x);
			y = peek16(0x7e) & ~(0x7);
			x = (oe >> 49) & 0xf;
			poke16(0x7e, y | x);
		}
	}

	if (opt_getdioreg) {
		unsigned int x, y, z, m;
		unsigned int a = 0, b = 0, c = 0;
		unsigned long long v, v2;
		m = peek16(0x60); /* Model reg */
		x = peek16(0x72);
		y = peek16(0x6c);
		z = peek16(0x66);
		v = x << 5;
		v |= (unsigned long long)y << 21;
		v |= (unsigned long long)((z & 0xf0) >> 4) << 37;
		if (m == 0x4500) {
			a = peek16(0x78);
			b = peek16(0x7a);
			c = peek16(0x7e);
			v |= (unsigned long long)(a & 0x1f);
			v |= (unsigned long long)(b & 0xff) << 41;
			v |= (unsigned long long)(c & 0xf) << 49;
		}
		v2 = peek16(0x70) << 5;
		v2 |= (unsigned long long)peek16(0x6a) << 21;
		v2 |= (unsigned long long)((peek16(0x66) >> 8) & 0xf) << 37;
		if (m == 0x4500) {
			v2 |= (unsigned long long)((a & 0x3e) >> 5);
			v2 |= (unsigned long long)((b & 0xff00) >> 8) << 41;
			v2 |= (unsigned long long)((c & 0xf0) >> 4) << 49;
		}
		sbusunlock();
		printf("diodir=0x%llx\n", v);
		printf("dio_out=0x%llx\n", v2);
		sbuslock();
	}

	if (opt_getdio && (opt_setdiodir || opt_setdio)) {
		unsigned long long v;
		v = peek16(0x6e) << 5;
		v |= (unsigned long long)peek16(0x68) << 21;
		v |= (unsigned long long)((peek16(0x66) >> 12)) << 37;
		if (peek16(0x60) == 0x4500) { /* TS-4500 has more DIO */
			v |= (unsigned long long)(peek16(0x78) & 0x7c00) >> 10;
			v |= (unsigned long long)(peek16(0x7c) & 0xfff) << 41;
		}
		sbusunlock();
		printf("dio2=0x%llx\n", v);
		sbuslock();
	}

	if (opt_poke16) {
		poke16(opt_address, opt_pokeval);
	}

	if (opt_peek16) {
		i = peek16(opt_address);
		sbusunlock();
		printf("0x%x\n", i);
		sbuslock();
	}

	if (opt_getmac) {
		unsigned char a, b, c;
		read_tagmem(tagmem);
		a = tagmem[0] >> 24; 
		b = tagmem[0] >> 16; 
		c = tagmem[0] >> 8; 
		sbusunlock();
		printf("mac=00:d0:69:%02x:%02x:%02x\n", a, b, c);
		sbuslock();
	}

	if (opt_setmac) {
		unsigned int a, b, c;
		int r;
		r = sscanf(opt_mac, "%*x:%*x:%*x:%x:%x:%x",  &a, &b, &c);
		assert(r == 3); /* XXX: user arg problem */
		assert(a < 0x100);
		assert(b < 0x100);
		assert(c < 0x100);
		read_tagmem(tagmem);
		erase_tagmem();
		tagmem[0] &= ~0xffffff00;
		tagmem[0] |= (a<<24|b<<16|c<<8);
		write_tagmem(tagmem);
	}

	if (opt_reboot) {
		struct sched_param sched;
		sync();
		poke16(0x74, 0);
		sched.sched_priority = 99;
		sched_setscheduler(0, SCHED_FIFO, &sched);
		while(1);
	}

	if (opt_greenledoff) poke16(0x62, peek16(0x62) & ~0x8000);
	if (opt_greenledon) poke16(0x62, peek16(0x62) | 0x8000);
	if (opt_redledoff) poke16(0x62, peek16(0x62) & ~0x4000);
	if (opt_redledon) poke16(0x62, peek16(0x62) | 0x4000);

	if (opt_setrtc) {
		unsigned short x, led;
		unsigned int d, ack;
		unsigned char rtcdat[10];
		time_t now = time(NULL);
		struct tm *tm;
		int ppb;

		read_tagmem(tagmem);
		ppb = (signed char)(~tagmem[1] & 0xff) * 1000;
		if (ppb > (4068 * 31)) ppb = (4068 * 31);
		if (ppb < (-2034 * 31)) ppb = (-2034 * 31);
		if (ppb >= 0) {
			ppb = ppb / 4068;
			rtcdat[8] = (1<<5)|(ppb & 0x1f);
		} else {
			ppb = ppb / -2034;
			rtcdat[8] = (ppb & 0x1f);
		}
		now += 6; /* 6 seconds in future to accomodate sleep() below */
		sbusunlock();
		fprintf(stderr, "rtc_calibration_reg=0x%x\n", rtcdat[8]);
		fprintf(stderr, "rtc_calibration_ppm=%f\n", rtcdat[8]&(1<<5) ?
		  ((rtcdat[8] & 0x1f) * 4.068) : ((rtcdat[8] & 0x1f) * -2.034));
		tm = gmtime(&now);
		sbuslock();
		rtcdat[0] = 0;
		rtcdat[1] = tobcd(tm->tm_sec) | 0x80;
		rtcdat[2] = tobcd(tm->tm_min) | 0x80;
		rtcdat[3] = tobcd(tm->tm_hour);
		rtcdat[4] = tm->tm_wday + 1; 
		rtcdat[5] = tobcd(tm->tm_mday);
		rtcdat[6] = tobcd(tm->tm_mon + 1);
		assert(tm->tm_year >= 100);
		rtcdat[7] = tobcd(tm->tm_year % 100);

		led = peek16(0x62) & 0xc000;
resettime:
		poke16(0x62, led|0x0); /* tristate, scl/sda pulled up */
		poke16(0x62, led|0xe00); /* i2c, start (sda low) */
		x = 0xe00|led;
		for (d = 0xd0, i = 0; i < 8; i++, d <<= 1) {
			x &= ~0x200;
			poke16(0x62, x); /* scl low */
			if (d & 0x80) x |= 0x100;
			else x &= ~0x100;
			poke16(0x62, x); /* sda update */
			x |= 0x200;
			poke16(0x62, x); /* scl high */
		}
		poke16(0x62, led|0x800); /* scl low, tristate sda */
		poke16(0x62, led|0xa00); /* scl high, tristate sda */
		ack = (peek16(0x62) >> 12) & 0x1; /* sample ack */
		if (ack) {
			sbusunlock();
			fprintf(stderr, "rtc_present=0\n");
			sbuslock();
			goto skiprtc;
		}

		for (j = 0; j < 9; j++) {
			for (d = rtcdat[j], i = 0; i < 8; i++, d <<= 1) {
				x &= ~0x200;
				poke16(0x62, x); /* scl low */
				if (d & 0x80) x |= 0x100;
				else x &= ~0x100;
				poke16(0x62, x); /* sda update */
				x |= 0x200;
				poke16(0x62, x); /* scl high */
			}
			poke16(0x62, led|0x800); /* scl low, tristate sda */
			poke16(0x62, led|0xa00); /* scl high, tristate sda */
			ack = (peek16(0x62) >> 12) & 0x1; /* sample ack */
			assert(ack == 0);
		}

		poke16(0x62, led|0xc00); /* scl low, sda low */
		poke16(0x62, led|0xe00); /* scl high, sda low */
		poke16(0x62, led|0x0); /* tristate (i2c stop) */
		if (rtcdat[1] & 0x80) {
			rtcdat[1] &= ~0x80;
			goto resettime;
		}
		if (rtcdat[2] & 0x80) {
			sbusunlock();
			sleep(6);
			sbuslock();
			rtcdat[2] &= ~0x80;
			goto resettime;
		}
		sbusunlock();
		fprintf(stderr, "rtc_present=1\n");
		sbuslock();
	}

	if (opt_getrtc || opt_diffrtc) {
		unsigned short x, led;
		unsigned int d, ack;
		unsigned char rtcdat[9];
		time_t now;
		struct tm tm;
		struct timeval tv;
		int cdrift, rdrift;
		struct timex tmx;

		bzero(rtcdat, 9);
		read_tagmem(tagmem);
		cdrift = (signed char)((~tagmem[1] & 0xff00) >> 8) * 2;
		rdrift = (signed char)(~tagmem[1] & 0xff);
		led = peek16(0x62) & 0xc000;
		poke16(0x62, led|0x0); /* tristate, scl/sda pulled up */
		poke16(0x62, led|0xe00); /* i2c, start (sda low) */
		poke16(0x62, led|0xf00); /* i2c, stop (sda high) */
		poke16(0x62, led|0xe00); /* i2c, start (sda low) */
		x = led|0xe00;
		for (d = 0xd0, i = 0; i < 8; i++, d <<= 1) {
			x &= ~0x200;
			poke16(0x62, x); /* scl low */
			if (d & 0x80) x |= 0x100;
			else x &= ~0x100;
			poke16(0x62, x); /* sda update */
			x |= 0x200;
			poke16(0x62, x); /* scl high */
		}
		poke16(0x62, led|0x800); /* scl low, tristate sda */
		poke16(0x62, led|0xa00); /* scl high, tristate sda */
		ack = (peek16(0x62) >> 12) & 0x1; /* sample ack */
		sbusunlock();
		printf("rtc_present=%d\n", ack ? 0 : 1);
		printf("tagmem_cpudrift_ppm=%d\n", cdrift);
		if (opt_getrtc) {
			tmx.modes = ADJ_FREQUENCY;
			tmx.freq = cdrift * 65536;
			ret = adjtimex(&tmx);
			assert(ret != -1);
		}
		printf("tagmem_rtcdrift_ppm=%d\n", rdrift);
		sbuslock();
		if (ack) goto skiprtc;
		for (i = 0; i < 8; i++) {
			poke16(0x62, led|0xc00); /* scl low, sda low */
			poke16(0x62, led|0xe00); /* scl high */
		}
		poke16(0x62, led|0x800); /* scl low, tristate sda */
		poke16(0x62, led|0xa00); /* scl high, tristate sda */
		ack = (peek16(0x62) >> 12) & 0x1; /* sample ack */
		assert(ack == 0);
		poke16(0x62, led|0x800); /* scl low, tristate sda */
		poke16(0x62, led|0xd00); /* scl low, sda high */
		poke16(0x62, led|0xf00); /* scl high, sda high */
		poke16(0x62, led|0xe00); /* scl high, sda lo (repeated start) */
		x = led|0xe00;
		for (d = 0xd1, i = 0; i < 8; i++, d <<= 1) {
			x &= ~0x200;
			poke16(0x62, x); /* scl low */
			if (d & 0x80) x |= 0x100;
			else x &= ~0x100;
			poke16(0x62, x); /* sda update */
			x |= 0x200;
			poke16(0x62, x); /* scl high */
		}
		poke16(0x62, led|0x800); /* scl low, tristate sda */
		poke16(0x62, led|0xa00); /* scl high, tristate sda */
		ack = (peek16(0x62) >> 12) & 0x1; /* sample ack */
		assert(ack == 0);
		for (j = 0; j < 8; j++) {
			for (i = 0; i < 8; i++) {
				poke16(0x62, led|0x800); /* scl low, tri sda */
				poke16(0x62, led|0xa00); /* scl high, tri sda */
				rtcdat[j] <<= 1;
				rtcdat[j] |= (peek16(0x62) >> 12) & 0x1;
			}
			if (j != 7) {
				poke16(0x62, led|0x800); /* scl low, tri sda */
				poke16(0x62, led|0xc00); /* scl low, sda low */
				poke16(0x62, led|0xe00); /* scl high, sda low */
			} else {
				poke16(0x62, led|0x800); /* scl low, tri sda */
				poke16(0x62, led|0xd00); /* scl low, sda high */
				poke16(0x62, led|0xf00); /* scl high, sda hi */
			}
		}
		poke16(0x62, led|0x800); /* scl low, tristate sda */
		poke16(0x62, led|0xc00); /* scl low, sda low */
		poke16(0x62, led|0xe00); /* scl high, sda low */
		poke16(0x62, led|0x0); /* tristate (i2c stop) */
		sbusunlock();
		printf("rtc_oscillator_ok=%d\n", (rtcdat[1] & 0x80) ? 0 : 1);
		printf("rtc_calibration_reg=0x%x\n", rtcdat[7]);
		printf("rtc_calibration_ppm=%f\n", rtcdat[7]&(1<<5) ?
		  ((rtcdat[7] & 0x1f) * 4.068) : ((rtcdat[7] & 0x1f) * -2.034));
		sbuslock();
		// for (i = 0; i < 8; i++) printf("dat%d=0x%x\n", i, rtcdat[i]);
		if ((rtcdat[1] & 0x80) == 0) {
			tm.tm_sec = frombcd(rtcdat[0] & 0x7f);
			tm.tm_min = frombcd(rtcdat[1] & 0x7f);
			tm.tm_hour = frombcd(rtcdat[2] & 0x3f);
			tm.tm_mday = frombcd(rtcdat[4] & 0x3f);
			tm.tm_mon = frombcd(rtcdat[5] & 0x1f) - 1;
			tm.tm_year = frombcd(rtcdat[6]) + 100;
			sbusunlock();
			setenv("TZ", "UTC", 1);
			now = mktime(&tm);
			tv.tv_sec = now;
			tv.tv_usec = 0;
			if (opt_getrtc) settimeofday(&tv, NULL);
			sbuslock();
		}
	}

	if (opt_diffrtc) {
		struct sched_param sched;
		unsigned short x, led;
		unsigned int d, ack;
		int loops = 0;
		unsigned char rtcdat[8], prev;
		time_t now;
		struct tm tm;
		struct timeval tv0, tv;

		bzero(&tm, sizeof(tm));
		sched.sched_priority = 99;
		sched_setscheduler(0, SCHED_FIFO, &sched);
		sbusunlock();
		setenv("TZ", "UTC", 1);
		sbuslock();
		led = peek16(0x62) & 0xc000;
		gettimeofday(&tv0, NULL);

diffrtc_again:
		poke16(0x62, led|0x0); /* tristate, scl/sda pulled up */
		poke16(0x62, led|0xe00); /* i2c, start (sda low) */
		poke16(0x62, led|0xf00); /* i2c, stop (sda high) */
		poke16(0x62, led|0xe00); /* i2c, start (sda low) */
		x = led|0xe00;
		for (d = 0xd0, i = 0; i < 8; i++, d <<= 1) {
			x &= ~0x200;
			poke16(0x62, x); /* scl low */
			if (d & 0x80) x |= 0x100;
			else x &= ~0x100;
			poke16(0x62, x); /* sda update */
			x |= 0x200;
			poke16(0x62, x); /* scl high */
		}
		poke16(0x62, led|0x800); /* scl low, tristate sda */
		poke16(0x62, led|0xa00); /* scl high, tristate sda */
		ack = (peek16(0x62) >> 12) & 0x1; /* sample ack */
		assert(ack == 0);
		for (i = 0; i < 8; i++) {
			poke16(0x62, led|0xc00); /* scl low, sda low */
			poke16(0x62, led|0xe00); /* scl high */
		}
		poke16(0x62, led|0x800); /* scl low, tristate sda */
		poke16(0x62, led|0xa00); /* scl high, tristate sda */
		ack = (peek16(0x62) >> 12) & 0x1; /* sample ack */
		assert(ack == 0);
		poke16(0x62, led|0x800); /* scl low, tristate sda */
		poke16(0x62, led|0xd00); /* scl low, sda high */
		poke16(0x62, led|0xf00); /* scl high, sda high */
		poke16(0x62, led|0xe00); /* scl high, sda lo (repeated start) */
		x = led|0xe00;
		for (d = 0xd1, i = 0; i < 8; i++, d <<= 1) {
			x &= ~0x200;
			poke16(0x62, x); /* scl low */
			if (d & 0x80) x |= 0x100;
			else x &= ~0x100;
			poke16(0x62, x); /* sda update */
			x |= 0x200;
			poke16(0x62, x); /* scl high */
		}
		poke16(0x62, led|0x800); /* scl low, tristate sda */
		poke16(0x62, led|0xa00); /* scl high, tristate sda */
		ack = (peek16(0x62) >> 12) & 0x1; /* sample ack */
		assert(ack == 0);
		for (i = 0, prev = rtcdat[0]; i < 8; i++) {
			poke16(0x62, led|0x800); /* scl low, tri sda */
			poke16(0x62, led|0xa00); /* scl high, tri sda */
			rtcdat[0] <<= 1;
			rtcdat[0] |= (peek16(0x62) >> 12) & 0x1;
		}
		if (rtcdat[0] == prev || loops == 0) { /* no second hand movement? */
			poke16(0x62, led|0x800); /* scl low, tri sda */
			poke16(0x62, led|0xd00); /* scl low, sda high */
			poke16(0x62, led|0xf00); /* scl high, sda hi */
			poke16(0x62, led|0x800); /* scl low, tristate sda */
			poke16(0x62, led|0xc00); /* scl low, sda low */
			poke16(0x62, led|0xe00); /* scl high, sda low */
			poke16(0x62, led|0x0); /* tristate (i2c stop) */
			loops++;
			assert(loops < 10000);
			goto diffrtc_again;
		} else {
			gettimeofday(&tv, NULL);
			poke16(0x62, led|0x800); /* scl low, tri sda */
			poke16(0x62, led|0xc00); /* scl low, sda low */
			poke16(0x62, led|0xe00); /* scl high, sda low */
		}
		for (j = 1; j < 8; j++) {
			for (i = 0; i < 8; i++) {
				poke16(0x62, led|0x800); /* scl low, tri sda */
				poke16(0x62, led|0xa00); /* scl high, tri sda */
				rtcdat[j] <<= 1;
				rtcdat[j] |= (peek16(0x62) >> 12) & 0x1;
			}
			if (j != 7) {
				poke16(0x62, led|0x800); /* scl low, tri sda */
				poke16(0x62, led|0xc00); /* scl low, sda low */
				poke16(0x62, led|0xe00); /* scl high, sda low */
			} else {
				poke16(0x62, led|0x800); /* scl low, tri sda */
				poke16(0x62, led|0xd00); /* scl low, sda high */
				poke16(0x62, led|0xf00); /* scl high, sda hi */
			}
		}
		poke16(0x62, led|0x800); /* scl low, tristate sda */
		poke16(0x62, led|0xc00); /* scl low, sda low */
		poke16(0x62, led|0xe00); /* scl high, sda low */
		poke16(0x62, led|0x0); /* tristate (i2c stop) */
		if ((rtcdat[1] & 0x80) == 0) {
			long long us, p;
			tm.tm_sec = frombcd(rtcdat[0] & 0x7f);
			tm.tm_min = frombcd(rtcdat[1] & 0x7f);
			tm.tm_hour = frombcd(rtcdat[2] & 0x3f);
			tm.tm_mday = frombcd(rtcdat[4] & 0x3f);
			tm.tm_mon = frombcd(rtcdat[5] & 0x1f) - 1;
			tm.tm_year = frombcd(rtcdat[6]) + 100;
			sbusunlock();
			fprintf(stderr, "tm=\"%d %d %d %d %d %d\"\n", tm.tm_sec,
			  tm.tm_min, tm.tm_hour, tm.tm_mday, tm.tm_mon, 
			  tm.tm_year);
			now = mktime(&tm);
			p = us = tv.tv_sec * 1000000LL + tv.tv_usec;
			us -= now * 1000000LL;
			fprintf(stderr, "diffrtc=");
			if (us < 0) fputc('-', stderr);
#define LLABS(x)	((x) < 0 ? -(x) : (x))
			fprintf(stderr, "%lld.%06lld\n", LLABS(us) / 1000000LL,
			  LLABS(us) % 1000000LL);
			fprintf(stderr, "measured_when=%lld\n", 
			  (long long)tv.tv_sec);
			fprintf(stderr, "loops=%d\n", loops);
			p -= tv0.tv_sec * 1000000LL + tv0.tv_usec;
			fprintf(stderr, "precision_us=%lld\n", p / loops);
			fprintf(stderr, "now=%lld\n", (long long)now);
			fprintf(stderr, "rtcdat=\"%02x", rtcdat[0]);
			for (i = 1; i < 7; i++) fprintf(stderr, " %02x", rtcdat[i]);
			fprintf(stderr, "\"\n");
			sbuslock();
		}
	}

skiprtc:

	/* Should be last! */
	if (opt_watchdog) {
		struct sched_param sched;
		int fd, r;
		char c;
		sbusunlock();
		daemon(0, 0);
		signal(SIGHUP, SIG_IGN);
		sched.sched_priority = 98;
		sched_setscheduler(0, SCHED_FIFO, &sched);
		if (opt_autofeed) {
			unsigned int us = 60000000;
			if (wdog == 0) {
				us = 100000;
			} else if (wdog == 1) {
				us = 1000000;
			} else if (wdog == 2) {
				us = 5000000;
			}
			for (;;) {
				sbuslock();
				poke16(0x74, wdog);
				sbusunlock();
				usleep(us);
			}
		}
		/* Single character writes to /dev/watchdog keep it fed */
		mknod("/dev/watchdog", S_IFIFO|0666, 0);
		for(;;) {
			fd = open("/dev/watchdog", O_RDONLY);
			assert (fd != -1);
			do {
				r = read(fd, &c, 1);
				if (c > '3' || c < '0') c = '3';
				sbuslock();
				poke16(0x74, c - '0');
				sbusunlock();
			} while (r > 0);
			close(fd);
		}
	}

	sbusunlock();

	return 0;
}
