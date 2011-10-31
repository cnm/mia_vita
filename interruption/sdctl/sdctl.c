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
/* To compile sdctl, use the appropriate cross compiler and run the
 * command:
 *
 *   gcc -Wall -O -mcpu=arm9 -o sdctl sdctl.c
 *
 * sdcore2.c should be in the same directory as sdctl.c
 *
 * On uclibc based initrd's, the following additional gcc options are
 * necessary: -Wl,--rpath,/slib -Wl,-dynamic-linker,/slib/ld-uClibc.so.0
 */

const char copyright[] = "Copyright (c) Technologic Systems - " __DATE__ ;

#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <linux/unistd.h>
#include <netdb.h>
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
#include <sys/resource.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>


#define NOCHECKSUM
#ifdef PPC
#define BIGENDIAN
#endif

#ifndef TEMP_FAILURE_RETRY
# define TEMP_FAILURE_RETRY(expression) \
  (__extension__                                                              \
    ({ long int __result;                                                     \
       do __result = (long int) (expression);                                 \
       while (__result == -1L && errno == EINTR);                             \
       __result; }))
#endif

#ifndef PPC
static void poke8(unsigned int, unsigned char);
static void poke16(unsigned int, unsigned short);
static void poke32(unsigned int, unsigned int);
static unsigned char peek8(unsigned int);
static unsigned short peek16(unsigned int);
static unsigned int peek32(unsigned int);
#define SDPOKE8(sd, x, y)	poke8((sd)->sd_regstart + (x), (y))
#define SDPOKE32(sd, x, y)	poke32((sd)->sd_regstart + (x), (y))
#define SDPOKE16(sd, x, y)	poke16((sd)->sd_regstart + (x), (y))
#define SDPEEK8(sd, x)		peek8((sd)->sd_regstart + (x)) 
#define SDPEEK32(sd, x)		peek32((sd)->sd_regstart + (x))
#define SDPEEK16(sd, x)		peek16((sd)->sd_regstart + (x))
#endif

static volatile unsigned int *cvspiregs, *cvgpioregs;
#include "sdcore2.c"

/* ep93xx DMA register constants */
#define DMA_CONTROL	0
#define DMA_INTERRUPT	1
#define DMA_STATUS	3
#define DMA_BCR0	4
#define DMA_BCR1	5
#define DMA_SAR_BASE0	6
#define DMA_SAR_BASE1	7
#define DMA_SAR_CURRENT0	9
#define DMA_SAR_CURRENT1	10
#define DMA_DAR_BASE0	11
#define DMA_DAR_BASE1	12
#define DMA_DAR_CURRENT0	13
#define DMA_DAR_CURRENT1	15

#define MEM_GETPGD       _IOR(0xaa, 1, unsigned long)
#ifndef PPC
#ifdef _syscall3
# define __NR_cacheflush __ARM_NR_cacheflush
static inline _syscall3(int,cacheflush,unsigned long,beg,unsigned long,end,int,flags);
#else
# include <sys/syscall.h>
# define cacheflush(a, b, c)	syscall(__ARM_NR_cacheflush, (a), (b), (c))
#endif
#endif
static void sdcore_irqwait2(void *arg, unsigned int n);

static unsigned char cavium_mbr[512] = {
0x28,0x31,0x9f,0xe5,0x08,0xd0,0x4d,0xe2,0x01,0x60,0xa0,0xe1,0x00,0x80,0xa0,0xe1,
0x03,0x00,0x93,0xe8,0x0d,0x30,0xa0,0xe1,0x03,0x00,0x83,0xe8,0x10,0x01,0x9f,0xe5,
0x02,0x70,0xa0,0xe1,0x00,0xa0,0x9d,0xe5,0x08,0x41,0x9f,0xe5,0x0f,0xe0,0xa0,0xe1,
0x06,0xf0,0xa0,0xe1,0x00,0x50,0xa0,0xe3,0x12,0x00,0x00,0xea,0x04,0x30,0xd4,0xe5,
0xda,0x00,0x53,0xe3,0x0e,0x00,0x00,0x1a,0xb8,0x10,0xd4,0xe1,0xbc,0xc0,0xd4,0xe1,
0xba,0x00,0xd4,0xe1,0xbe,0x20,0xd4,0xe1,0x08,0xe0,0x8d,0xe2,0x05,0x31,0x8e,0xe0,
0x00,0x08,0x81,0xe1,0x02,0x28,0x8c,0xe1,0x08,0x10,0x13,0xe5,0x0f,0xe0,0xa0,0xe1,
0x08,0xf0,0xa0,0xe1,0xb8,0x00,0x9f,0xe5,0x01,0x50,0x85,0xe2,0x0f,0xe0,0xa0,0xe1,
0x06,0xf0,0xa0,0xe1,0x10,0x40,0x84,0xe2,0xac,0x30,0x9f,0xe5,0x03,0x00,0x54,0xe1,
0xe9,0xff,0xff,0x1a,0x0f,0xe0,0xa0,0xe1,0x07,0xf0,0xa0,0xe1,0x01,0x00,0x55,0xe3,
0x98,0x30,0x9f,0xc5,0x78,0x20,0xe0,0xc3,0x06,0x20,0xc3,0xc5,0x90,0xc0,0x9f,0xe5,
0x01,0x1c,0xa0,0xe3,0x02,0x00,0x5c,0xe5,0x88,0xe0,0x9f,0xe5,0x01,0x20,0x5c,0xe5,
0x3f,0x30,0x00,0xe2,0x03,0x20,0xce,0xe7,0x80,0x00,0x10,0xe3,0x00,0x30,0xde,0x15,
0x00,0x20,0xa0,0x13,0x03,0xe1,0xa0,0x11,0x04,0x00,0x00,0x1a,0x06,0x00,0x00,0xea,
0x60,0x30,0x9f,0xe5,0x02,0x30,0xd3,0xe7,0x02,0x30,0xc1,0xe7,0x01,0x20,0x82,0xe2,
0x0e,0x00,0x52,0xe1,0xf9,0xff,0xff,0x3a,0x0e,0x10,0x81,0xe0,0x40,0x00,0x10,0xe3,
0x02,0xc0,0x8c,0xe2,0xea,0xff,0xff,0x0a,0x3c,0x30,0x9f,0xe5,0x00,0x00,0xa0,0xe3,
0x00,0x10,0x83,0xe5,0x00,0x00,0x81,0xe5,0x01,0x2c,0xa0,0xe3,0x2c,0x10,0x9f,0xe5,
0x0f,0xe0,0xa0,0xe1,0x0a,0xf0,0xa0,0xe1,0x08,0xd0,0x8d,0xe2,0x0e,0xf0,0xa0,0xe1,
0x54,0x41,0x00,0x00,0x5c,0x41,0x00,0x00,0xbe,0x41,0x00,0x00,0xfe,0x41,0x00,0x00,
0x60,0x41,0x00,0x00,0x62,0x41,0x00,0x00,0x00,0x42,0x00,0x00,0x14,0x42,0x00,0x00,
0xd1,0x07,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x01,0x2e,0x0d,0x0a,0x00,
0x00,0x05,0x04,0x01,0x06,0x41,0xc7,0x54,0x04,0x05,0x06,0x42,0x0b,0x01,0x0e,0x40,
0xcf,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0xda,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0xff,0x0f,0x00,0x00,0x00,0x00,
0x00,0x00,0xda,0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,
0x01,0x00,0x83,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,0xe0,0x07,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x55,0xaa,
};

static unsigned char ts735x_mbr[512] = {
0xf0,0xb5,0x43,0x4b,0x85,0xb0,0x01,0x90,0x5c,0x68,0x1b,0x68,0x00,0x91,0x03,0x93,
0x04,0x94,0x40,0x48,0x00,0xf0,0x8a,0xf8,0x00,0x22,0x13,0x1c,0xc0,0x33,0x1b,0x06,
0x1a,0x60,0x01,0x32,0x50,0x2a,0xf8,0xd1,0x3b,0x4a,0x80,0x23,0x5b,0x00,0x3b,0x48,
0x13,0x60,0x00,0x21,0x3f,0x23,0x0a,0x1c,0x1a,0x40,0x01,0x31,0x00,0x23,0x83,0x54,
0x14,0x29,0xf7,0xd1,0x36,0x48,0x02,0x24,0x00,0xf0,0x74,0xf8,0x00,0x23,0x1b,0x68,
0x48,0x2b,0x02,0xd1,0x00,0xf0,0x6e,0xf8,0x00,0xe0,0x04,0x30,0x01,0x3c,0x00,0x2c,
0xf2,0xd1,0xe0,0x23,0x1b,0x06,0x1c,0x68,0x2a,0x2c,0x01,0xd0,0x28,0x2c,0x0a,0xd1,
0x00,0xf0,0x60,0xf8,0x28,0x2c,0x03,0xd1,0x00,0xf0,0x5c,0xf8,0x06,0x1c,0x04,0xe0,
0x06,0x1c,0x08,0x36,0x01,0xe0,0x06,0x1c,0x10,0x36,0x6b,0x46,0x0c,0x33,0x02,0x93,
0x00,0x27,0x24,0x4b,0x3a,0x01,0xd5,0x18,0x2b,0x79,0xda,0x2b,0x1d,0xd1,0x02,0x9a,
0x02,0xca,0x02,0x92,0x68,0x7a,0x2b,0x7a,0x00,0x02,0x18,0x43,0xeb,0x7a,0xaa,0x7a,
0x1b,0x02,0x13,0x43,0x1b,0x04,0x6a,0x7b,0xc0,0x18,0x2b,0x7b,0x12,0x02,0x1a,0x43,
0xeb,0x7b,0xac,0x7b,0x1b,0x02,0x23,0x43,0x1b,0x04,0xd2,0x18,0x01,0x9b,0x00,0xf0,
0x2f,0xf8,0x10,0x48,0x00,0x9a,0x00,0xf0,0x2a,0xf8,0x01,0x37,0x04,0x2f,0xd8,0xd1,
0x02,0x9a,0x03,0xab,0xd3,0x1a,0x9b,0x10,0x01,0x2b,0x02,0xdd,0x30,0x1c,0x00,0xf0,
0x21,0xf8,0x09,0x4b,0x00,0x20,0x1a,0x68,0x00,0x23,0x13,0x60,0x80,0x23,0x80,0x22,
0x1b,0x02,0x09,0x49,0x52,0x00,0x00,0xf0,0x13,0xf8,0x05,0xb0,0xf0,0xbd,0x00,0x00,
0x74,0x11,0x00,0x00,0x7c,0x11,0x00,0x00,0x14,0x20,0x00,0x00,0x00,0x20,0x00,0x00,
0x80,0x11,0x00,0x00,0xbe,0x11,0x00,0x00,0x63,0x01,0x00,0x00,0x08,0x47,0x10,0x47,
0x18,0x47,0xc0,0x46,0x30,0xb5,0x0d,0x4b,0x0d,0x4d,0x1c,0x68,0x01,0x78,0x42,0x78,
0x3f,0x23,0x0b,0x40,0xea,0x54,0x80,0x23,0x02,0x30,0x19,0x42,0x09,0xd0,0x00,0x22,
0x03,0xe0,0x53,0x5d,0x01,0x32,0x23,0x70,0x01,0x34,0x2b,0x78,0x9b,0x00,0x9a,0x42,
0xf7,0xd3,0x4b,0x06,0xea,0xd5,0x01,0x4b,0x1c,0x60,0x30,0xbd,0x14,0x20,0x00,0x00,
0x00,0x20,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x01,0x2e,0x0d,0x0a,0x00,
0x00,0x05,0x04,0x01,0x06,0x41,0x87,0x54,0x00,0x04,0x04,0x02,0x8a,0x80,0xcf,0x01,
0x8f,0x02,0xcf,0x03,0x8f,0x04,0xcf,0x05,0x8f,0x06,0xcf,0x07,0x8f,0xe0,0x8f,0xe1,
0x8f,0xe4,0xcf,0xe5,0x8f,0xe2,0x8f,0xe3,0x8f,0xe6,0xcf,0xe7,0x04,0x05,0x06,0x42,
0x0a,0x00,0x0b,0x01,0x0e,0x40,0xcf,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,
0x01,0x00,0x0c,0x01,0x02,0x00,0x3d,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x01,
0x03,0x00,0xda,0x04,0x3b,0x05,0x3f,0x00,0x00,0x00,0x00,0x14,0x00,0x00,0x00,0x07,
0x15,0x08,0xda,0x09,0x2a,0x0b,0x3f,0x20,0x00,0x00,0x00,0x0c,0x00,0x00,0x00,0x0d,
0x27,0x10,0x83,0x01,0xe2,0xd6,0x3f,0x40,0x00,0x00,0x00,0x60,0x0e,0x00,0x55,0xaa
};


static int devmem, devkmem;
static unsigned long pgd;
static unsigned long l1tbl[4096];
static int l1idx_last = -1;
static int first_access;
static volatile unsigned int * msiregs;
static volatile unsigned int * doorbell;
static volatile unsigned int * mvdmaregs, * epdmaregs;
static volatile unsigned short * syscon7350;
static int nbdbufsz;
static char *nbdbuf;
static int die, killable=1;
static int opt_erasehint = 0;
volatile unsigned long long *sbus_shm;

struct nbd_request {
	unsigned int magic;
	char type[4];
	char handle[8];
	unsigned int fromhi;
	unsigned int fromlo;
	unsigned int len;
};

struct nbd_reply {
	char magic[4];
	unsigned int error;
	char handle[8];
};


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
	fclose(maps);
}


#ifndef PPC
static int semid = -1;
static int sbuslocked = 0;
static void sbusunlock(void);
static void sbuslock(void) {
	int r;
	struct sembuf sop;
	if (killable > 0 && die) exit(1);
	killable--;
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
	r = TEMP_FAILURE_RETRY(semop(semid, &sop, 1));
	assert (r == 0);
	cvgpioregs[0] = (1<<17|1<<3);
	assert((cvspiregs[0x5c/4] & 0xf) == 0);
	assert((cvspiregs[0x64/4] & 0xf) == 0);
	sbuslocked = 1;
}


static void sbusunlock(void) {
	struct sembuf sop = { 0, 1, SEM_UNDO};
	int r;
	if (!sbuslocked) return;
	r = semop(semid, &sop, 1);
	assert (r == 0);
	sbuslocked = 0;
	killable++;
	if (killable > 0 && die) exit(1);
	sched_yield();
}


static void sbuspreempt(void) {
	int r;
	if (killable == 0 && die) exit(1);
	r = semctl(semid, 0, GETNCNT);
	assert (r != -1);
	if (r) {
		sbusunlock();
		sched_yield();
		sbuslock();
	}
}


static void poke8(unsigned int adr, unsigned char dat) {
	if (!cvspiregs) *(unsigned char *)adr = dat;
	else {
		if (adr & 0x1) {
			cvgpioregs[0] = (1<<17);
			poke16(adr, dat << 8);
		} else {
			cvgpioregs[0] = (1<<3);
			poke16(adr, dat);
		}
		cvgpioregs[0] = (1<<17|1<<3);
	}

}


static void poke16(unsigned int adr, unsigned short dat) {
	unsigned int dummy = 0;

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
		: "+r"(dummy) : "r"(adr), "r"(dat), "r"(cvspiregs) : "r1","cc"
	);
}


static inline void 
poke16_stream(unsigned int adr, unsigned short *dat, unsigned int len) {
	volatile unsigned int *spi = cvspiregs;
	unsigned int cmd, ret, i, j;

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


static void poke32(unsigned int adr, unsigned int dat) {
	if (!cvspiregs) *(unsigned int *)adr = dat;
	else {
		poke16(adr, dat & 0xffff);
		poke16(adr + 2, dat >> 16);
	}
}


static unsigned char peek8(unsigned int adr) {
	unsigned char ret;
	if (!cvspiregs) ret = *(unsigned char *)adr;
	else {
		unsigned short x;
		x = peek16(adr);
		if (adr & 0x1) ret = x >> 8;
		else ret = x & 0xff;
	}
	return ret;
}


static unsigned short peek16(unsigned int adr) {
	unsigned short ret = 0;

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


static unsigned int peek32(unsigned int adr) {
	unsigned int ret;
	if (!cvspiregs) ret = *(unsigned int *)adr;
	else {
		unsigned short l, h;
		l = peek16(adr);
		h = peek16(adr + 2);
		ret = (l|(h<<16));
	}
	return ret;
}


static inline
void peek16_stream(unsigned int adr, unsigned short *dat, unsigned int len) {
	unsigned int dummy = 0;

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
		"ldr %0, [%4, #0x40]\n" /* XXX */
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
#else
static void sbuslock(void) { }
static void sbusunlock(void) { }
static void sbuspreempt(void) { }
static void peek16_stream(unsigned int a, unsigned short *d, unsigned int l) { }
static void poke16_stream(unsigned int a, unsigned short *d, unsigned int l) { }
static unsigned char peek8(unsigned int a) { }
static unsigned short peek16(unsigned int a) { }
static unsigned int peek32(unsigned int a) { }
static void poke16(unsigned int a, unsigned short v) { }
static void poke32(unsigned int a, unsigned int v) { }
static void poke8(unsigned int a, unsigned char v) { }
#endif


static unsigned int is_cavium(void) __attribute__((pure));
static unsigned int is_cavium(void) {
	FILE *cpuinfo;
	char buf[4096];
	static unsigned int checked = 0;
	static unsigned int iscavium = 0;

	if (!checked) {
		cpuinfo = fopen("/proc/cpuinfo", "r");
		if (cpuinfo == NULL) {
			perror("/proc/cpuinfo");
			exit(4);
		}
		bzero(buf, 4096);
		fread(buf, 1, 4095, cpuinfo);
		fclose(cpuinfo);
		if (strstr(buf, "FA526")) iscavium = 1;
		checked = 1;
	}
	return iscavium;
}

static int send_nbd_hup = 0;
static int is_nbd = 0;
static void xsd0(void) {
	int ret, nbdpid = sbus_shm[0];

	if (is_cavium()) sbuslock();
	if (!is_nbd && nbdpid) while ((sbus_shm[9] & 2) == 0) {
		/* tell NBD to finish up */
		killable--;
		ret = kill(nbdpid, SIGUSR1);
		assert(ret == 0);
		assert(nbdpid == sbus_shm[0]);
		if (is_cavium()) sbusunlock();
		usleep(10000);
		if (is_cavium()) sbuslock();
	}
}

static void xsdend(void) {
	int nbdpid = sbus_shm[0];

	if (!is_nbd && nbdpid) {
		killable++;
		kill(nbdpid, SIGUSR2);
		if (send_nbd_hup) kill(nbdpid, SIGHUP);
		send_nbd_hup = 0;
	}
	if (is_cavium()) sbusunlock();
	if (killable > 0 && die) {
		if (is_nbd) sbus_shm[0] = sbus_shm[1] = 0;
		exit(1);
	}
}

static int xsdreset(struct sdcore *sd) {
	int ret;
	xsd0();
	ret = sdreset(sd);
	xsdend();
	sbus_shm[6]++;
	return ret;
}

static int xsdread(struct sdcore *sd, unsigned int o, char *b, 
  int n) {
	int ret;
	xsd0();
	ret = sdread(sd, o, (unsigned char *)b, n);
	xsdend();
	return ret;
}

static int xsdwrite(struct sdcore *sd, unsigned int o, char *b,
  int n) {
	int ret;
	xsd0();
	ret = sdwrite(sd, o, (unsigned char *)b, n);
	if (o == 0) send_nbd_hup = 1; /* wrote MBR */
	xsdend();
	return ret;
}

int xsdsetwprot(struct sdcore *sd, unsigned int x) {
	int ret;
	xsd0();
	ret = sdsetwprot(sd, x);
	xsdend();
	return ret;
}

int xsdlockctl(struct sdcore *sd, unsigned int a, char *b, char *c) {
	int ret;
	xsd0();
	ret = sdlockctl(sd, a, (unsigned char *)b, (unsigned char *)c);
	xsdend();
	return ret;
}

static int sdtimeout;
static void alarmsig(int x) {
	sdtimeout = 1000000;
}

static int hup, usr1, usr2;
static void hupsig(int x) {
	hup = 1;
}
static void usr1sig(int x) {
	usr1 = 1;
}
static void usr2sig(int x) {
	usr2 = 1;
}
static void intsig(int x) {
	if (killable > 0) exit(1);
	die = 1;
}


int sdcore_timeout(void *v) {
	if ((sdtimeout & 0x3f) == 0x3f) {
		sbuspreempt();
	}
	if (sdtimeout == 0x3f) {
		alarm(1);
	}
	if (sdtimeout >= 1000000) {
		return 1;
	} else {
		sdtimeout++;
	}
	return 0;
}


int sdcore_reset_timeout(void *v) {
	if (sdtimeout >= 0x40) alarm(0);
	sdtimeout = 0;
	return 0;
}


int sdcore_cvdmastream(void *v, unsigned char *buf, unsigned int len) {
	struct sdcore *sd = (struct sdcore *)v;
	unsigned short *sbuf;
	static int i = 0;

	len = len / 2;
	sbuf = (unsigned short *)buf;
	assert(((unsigned int)buf & 0x1) == 0);
	assert((len & 0xff) == 0);

	if (sd->sd_state & SDDAT_RX) {
		while (len > 0) {
			peek16_stream(4, sbuf, 256); /* SDDAT2 */
			len -= 256;
			if ((i & 0xf) == 0xf) sbuspreempt();
			i++;
			sbuf += 256;
		}
	} else {
		while (len > 0) {
			poke16_stream(4, sbuf, 256); /* SDDAT2 */
			peek16(12); /* necessary to flush write buffer */
			len -= 256;
			sbuf += 256;
			i++;
			if (i & 0x1) sbuspreempt();
		}
	}

	return 0;
}


/* Re-copy L1 page table from /dev/kmem */
static void refresh_pgtbl(void) {
#ifndef PPC
	int ret;

	if (pgd == 0) return;
	reservemem();
	ret = lseek(devkmem, pgd, SEEK_SET);
	assert(ret != -1);
	ret = read(devkmem, l1tbl, sizeof(l1tbl));
	assert(ret == sizeof(l1tbl));
	l1idx_last = -1;
#endif
}


/* Page table walk */
static unsigned long virt2phys(unsigned long virt) {
	static unsigned long l2tbl[256];
	int l1idx = virt >> 20;
	unsigned long l1desc;
	unsigned long l2desc;

	if (l1idx_last != l1idx) {
		l1desc = l1tbl[l1idx];
		switch (l1desc & 0x3) {
		case 0x1: /* Coarse page table */
			lseek(devmem, (l1desc & ~0x3ff), SEEK_SET);
			read(devmem, l2tbl, sizeof(l2tbl));
			l1idx_last = l1idx;
			break;

		case 0x2: /* Section */
			return ((virt & 0xfffff) | (l1desc & ~0xfffff));

		case 0x0: /* Fault */
			assert(0);
		case 0x3: /* Fine page table (not used in Linux/BSD) */
			assert(0);
		};
	}

	l2desc = l2tbl[(virt & 0xff000) >> 12];

	return ((virt & 0xfff) | (l2desc & ~0xfff));
}


void sdcore_dmaprep(void *v, unsigned char *buf, unsigned int len) {
	unsigned long start = (unsigned long)buf;

#ifndef PPC
	cacheflush(start & ~0xfff, (start + len + 4096) & ~0xfff, 0);
#else
	char *ebuf = buf + len;
	while (buf < ebuf) {
		asm volatile ("dcbf 0,%0": :"r" (buf));
		buf += 32;
	}
#endif
	first_access = 1;
}


/* This version of os_dmastream callback is used for the TS7350 SD core */
int sdcore_dmastream2(void *v, unsigned char *buf, unsigned int len) {
	struct sdcore *sd = (struct sdcore *)v;
	unsigned long va;
	unsigned int ctrl = 0x01085200;
	unsigned int count = 0;
	volatile unsigned short *fpgaregs = (unsigned short *)sd->sd_regstart;

	assert(sd->hw_version == 3);

	va = (unsigned long)buf;
	assert((va & 0x3) == 0);

	/* Theres something going on with using DMA for SD writes that I
	 * don't have time to troubleshoot right now, so we use PIO for
	 * the time being on the TS-7350.  -JO
	 */
	if (sd->sd_state & SDDAT_TX) {
		int i;
		unsigned short *b = (unsigned short *)va;
		for (i = 0, len >>= 1; i < len; i++) {
			fpgaregs[2] = *b++;
			if ((i & 0xff) == 0xff) sdcore_irqwait2(sd->os_arg, 1);
		}
		return 0;
	} 

	while (((va + len - 1) & ~0xfff) != (va & ~0xfff)) {
		/* crosses page boundary */
		int l;

		l = ((va + 4096) & ~0xfff) - va; /* len to end of pg */
		sdcore_dmastream2(v, (unsigned char *)va, l);
		va += l;
		len -= l;
	}

	epdmaregs[DMA_CONTROL] = ctrl;
	fpgaregs[8] = (len/2)|(1<<12);
	epdmaregs[DMA_SAR_BASE0] = 0x600ff020;
	epdmaregs[DMA_DAR_BASE0] = (unsigned int)virt2phys(va);

	epdmaregs[DMA_BCR0] = len;
	epdmaregs[DMA_CONTROL] = ctrl | 0x8;

	while (!(epdmaregs[DMA_STATUS] & 0x40)) {
		if (first_access) sched_yield();
		assert (count++ < 10000000);
	}
	first_access = 0;
	epdmaregs[DMA_INTERRUPT] = 0x0;

	return 0;
}


int sdcore_dmastream(void *v, unsigned char *buf, unsigned int len) {
	unsigned int x;
	struct sdcore *sd = (struct sdcore *)v;
	int i = 0;
	unsigned long va;

	assert(sd->hw_version > 1);

	va = (unsigned long)buf;
	assert((va & 0x3) == 0);

	while (((va + len - 1) & ~0xfff) != (va & ~0xfff)) {
		/* crosses page boundary */
		int l;

		l = ((va + 4096) & ~0xfff) - va; /* len to end of pg */
		sdcore_dmastream(v, (unsigned char *)va, l);
		va += l;
		len -= l;
	}

	x = msiregs[1];
	mvdmaregs[1] = virt2phys(va);
	if (sd->sd_state & SDDAT_TX)
	  mvdmaregs[0] = (len/4)|(4<<16)|(2<<19);
	else 
	  mvdmaregs[0] = (len/4)|(1<<12)|(4<<16)|(2<<19);
	mvdmaregs[0];
	*doorbell = ~0x2;
	x |= 0x2;
	msiregs[1] = x;

	while (!(*doorbell & 0x2)) {
		i++;
		if (i >= 100 || first_access) {
			sched_yield();
		}
		/*
		if (i == 20 && !noprocirq) {
			int irqfd;
			int t;
			irqfd = open("/proc/irq/65/irq", O_RDONLY|O_SYNC);
			if (irqfd == -1) {
				noprocirq = 1;
				break;
			}
			read(irqfd, &t, sizeof(t));
			close(irqfd);
			return 0;
		}
		*/
	}

	x &= ~0x2;
	msiregs[1] = x;
	first_access = 0;
	return 0;
}


static void sdcore_delay(void *arg, unsigned int us) {
	if (is_cavium()) sbusunlock();
	usleep(us);
	if (is_cavium()) sbuslock();
}


/* This version of os_irqwait callback is used for the TS7350 SD core */
static void sdcore_irqwait2(void *arg, unsigned int n) {
	unsigned int i = 0;
	volatile unsigned short *p = &syscon7350[3];

	while (*p & 0x100) {
		i++;
		if (i >= 100) {
			sched_yield();
		}
			
		/*
		if (i == 100 && !noprocirq) {
			int irqfd;
			int t;
			*p |= 0x80;
			irqfd = open("/proc/irq/32/irq", O_RDONLY|O_SYNC);
			if (irqfd == -1) {
				noprocirq = 1;
				break;
			}
			read(irqfd, &t, sizeof(t));
			close(irqfd);
			*p &= ~0x80;
		}
		*/
	}
}


static void sdcore_irqwait(void *arg, unsigned int n) {
	unsigned int x, i = 0;

	x = msiregs[1];
	*doorbell = ~0x4;
	msiregs[1] = 0x4;
	while (!(*doorbell & 0x4)) {
		i++;
		if (i >= 100) sched_yield();
		/*
		if (i == 100 && !noprocirq) {
			int irqfd;
			int t;
			irqfd = open("/proc/irq/66/irq", O_RDONLY|O_SYNC);
			if (irqfd == -1) {
				noprocirq = 1;
				break;
			}
			read(irqfd, &t, sizeof(t));
			close(irqfd);
			return;
		}
		*/
	}
	assert (msiregs[1] == 0x4); /* check if somebody else touched reg */
	msiregs[1] = x;
}


void usage(char **argv) {
	fprintf(stderr, "Usage: %s [OPTION] ...\n"
	  "Technologic Systems SD core manipulation.\n"
	  "\n"
	  "General options:\n"
	  "  -R, --read=N            Read N blocks of SD to stdout\n"
	  "  -W, --write=N           Write N blocks to SD\n"
	  "  -x, --writeset=BYTE     Write BYTE as value (default 0)\n"
	  "  -i, --writeimg=FILE     Use FILE as file to write to SD\n"
	  "  -t, --writetest         Run write speed test\n"
	  "  -r, --readtest          Run read speed test\n"
	  "  -n, --random=SEED       Do random seeks for tests\n"
	  "  -o, --noparking         Disable write parking optimization\n"
	  "  -z, --blocksize=SZ      Use SZ bytes each sdread/sdwrite call\n"
	  "  -E, --erasehint[=SZ]    Use erase hint write optimization\n"
	  "  -b, --sdboottoken=TOK   Use TOK as the boot token (to quicken init)\n"
	  "  -a, --address=ADD       Use ADD address instead of 0x13000000\n"
	  "  -k, --seek=SECTOR       Seek to 512b sector number SECTOR\n"
	  "  -l, --lun=N             Use N as numbered card slot (default 0)\n"
	  "  -S, --scanluns          Scan all LUNs for cards\n"
	  "  -m, --nodma             Don't use DMA\n"
	  "  -d, --nbdserver=NBDSPEC Run NBD userspace block driver server\n"
	  "  -I, --bind=IPADDR       Bind NBD server to IPADDR\n"
	  "  -Q, --stats             Print NBD server stats\n"
	  "  -P, --printmbr          Print MBR and partition table\n"
	  "  -M, --setmbr            Write MBR from environment variables\n"
	  "  -f, --foreground        Run NBD server in foreground\n"
	  "  -N, --nomultiwrite      Use only single block write SD commands\n"
	  "  -h, --help              This help\n"
	  "\n"
	  "Security/SD lock options:\n"
	  "  -p, --password=PASS     Use PASS as password\n"
	  "  -c, --clear             Remove password lock\n"
	  "  -s, --set               Set password lock\n"
	  "  -u, --unlock            Unlock temporarily\n"
	  "  -e, --erase             Erase entire device (clears password)\n"
	  "  -w, --wprot             Enable permanent write protect\n"
	  "\n"
	  "When running a NBD server, NBDSPEC is a comma separated list of\n"
	  "devices and partitions for the NBD servers starting at port 7500.\n"
	  "e.g. \"lun0:part1,lun1:disc\" corresponds to 2 NBD servers, one at port\n"
	  "7500 serving the first partition of SD lun 0, and the other at TCP\n"
	  "port 7501 serving the whole disc device of SD lun #1.\n",
	  argv[0]
	);
}


static 
int get_ptbl_offs(struct sdcore *sd, int part, int *type, int *sz) {
	unsigned char mbr[512];
	int ret, ext = 0;
	
	if (hup) xsdreset(sd);
	ret = xsdread(sd, 0, (char *)mbr, 1);
	if (ret != 0) return -1;

	if (part > 4) { /* extended partition */
		int i, oext;
		for (i = 0; i < 4; i++) 
		  if (mbr[0x1be + (16 * i) + 4] == 5) break;

		if (i == 4) return -1;

		oext = mbr[0x1be + (16 * i) + 8];
		oext |= mbr[0x1be + (16 * i) + 9] << 8;
		oext |= mbr[0x1be + (16 * i) + 10] << 16;
		oext |= mbr[0x1be + (16 * i) + 11] << 24;
		ext = oext;

		ret = xsdread(sd, oext, (char *)mbr, 1);
		if (ret != 0) return -1;

		/* We now have the first EBR in 'mbr' from sector 'oext' */

		/* Skip over 'part' - 5 EBRs -- we don't want them */
		while (part > 5) {
			int n;
			n = mbr[0x1d6];
			n |= mbr[0x1d7] << 8;
			n |= mbr[0x1d8] << 16;
			n |= mbr[0x1d9] << 24;
			ext = oext + n;

			if (mbr[0x1d2] == 0) return -1;

			ret = xsdread(sd, ext, (char *)mbr, 1);
			if (ret != 0) return -1;

			part--;
		}

		part -= 4;
	} 

	part--;
	ret = mbr[0x1be + (16 * part) + 8];
	ret |= mbr[0x1be + (16 * part) + 9] << 8;
	ret |= mbr[0x1be + (16 * part) + 10] << 16;
	ret |= mbr[0x1be + (16 * part) + 11] << 24;
	ret += ext; 

	if (type) *type = mbr[0x1be + (16 * part) + 4];
	if (sz) {
		*sz = mbr[0x1be + (16 * part) + 12];
		*sz |= mbr[0x1be + (16 * part) + 13] << 8;
		*sz |= mbr[0x1be + (16 * part) + 14] << 16;
		*sz |= mbr[0x1be + (16 * part) + 15] << 24;
	}

	return ret;
}


static
int setmbr(struct sdcore *sd) {
	unsigned char mbr[512];
	unsigned int sz, offs;
	int ret, i;
	char var[32];
	char *e;
	unsigned char *x;
	
	ret = xsdread(sd, 0, (char *)mbr, 1);
	if (ret) return ret;
	if (is_cavium()) x = cavium_mbr; else x = ts735x_mbr;
	if (mbr[510] != 0x55 || mbr[511] != 0xaa) memcpy(mbr, x, 512);
	else memcpy(mbr, x, 446);

	for (i = 0; i < 4; i++) {
		sprintf(var, "part%d_offs", i+1);
		e = getenv(var);
		if (e) {
			offs = strtoul(e, NULL, 0);
			mbr[0x1be + (16 * i) + 8] = offs & 0xff;
			mbr[0x1be + (16 * i) + 9] |= (offs>>8) & 0xff;
			mbr[0x1be + (16 * i) + 10] |= (offs>>16) & 0xff;
			mbr[0x1be + (16 * i) + 11] |= offs>>24;
		}
		sprintf(var, "part%d_sz", i+1);
		e = getenv(var);
		if (e) {
			sz = strtoul(e, NULL, 0);
			mbr[0x1be + (16 * i) + 12] = sz & 0xff;
			mbr[0x1be + (16 * i) + 13] |= (sz>>8) & 0xff;
			mbr[0x1be + (16 * i) + 14] |= (sz>>16) & 0xff;
			mbr[0x1be + (16 * i) + 15] |= sz>>24;
		}
		sprintf(var, "part%d_type", i+1);
		e = getenv(var);
		if (e) mbr[0x1be + (16 * i) + 4] = strtoul(e, NULL, 0);
	}
	xsdwrite(sd, 0, (char *)mbr, 1);
	xsdread(sd, 0, (char *)mbr, 1); /* To ensure write is terminated */
	return 0;
}


static unsigned char *csumtbl_lun[8];
static inline
void nbdwrite(struct sdcore *sd, int fd, unsigned int sector, int len) {
	int ret;
	char *buf = nbdbuf;
#ifndef NOCHECKSUM
	int i;
	unsigned char *csumtbl = csumtbl_lun[sd->sd_lun];

	if (csumtbl == NULL) {
		csumtbl_lun[sd->sd_lun] = csumtbl = malloc(sdsize(sd));
		bzero(csumtbl, sdsize(sd));
	}
#endif

	while (len > nbdbufsz) {
		nbdwrite(sd, fd, sector, nbdbufsz);
		sector += nbdbufsz;
		len -= nbdbufsz;
	}

again:
	if (hup) xsdreset(sd);
	ret = xsdread(sd, sector, buf, len);
	if (ret != 0) {
		hup = 1;
		usleep(500000);
		goto again;
	}
#ifndef NOCHECKSUM
	for (i = 0; i < len; i++) {
		int j, csum;
		unsigned char *c = &buf[512 * i];
		for (csum = 0, j = 0; j < 512; j++) {
			csum += c[j];
		}
		if (csumtbl[i + sector] & 0x80) {
			if ((csumtbl[i + sector] & 0x7f) != (csum & 0x7f)) {
				fprintf(stderr, 
				  "checksum error sector %d!\n", sector);
			}
			assert((csumtbl[i + sector] & 0x7f) == (csum & 0x7f));
		}
		csumtbl[i + sector] = 0x80 | (csum & 0x7f);
	}
#endif

	len = len << 9;
	while (len) {
		ret = TEMP_FAILURE_RETRY(write(fd, buf, len));
		assert (ret > 0);
		len -= ret;
		buf += ret;
	}
}


static inline
void nbdread(struct sdcore *sd, int fd, unsigned int sector, int len) {
	int ret;
	char *buf = nbdbuf;
	int rem;
#ifndef NOCHECKSUM
	int i;
	unsigned char *csumtbl = csumtbl_lun[sd->sd_lun];

	if (csumtbl == NULL) {
		csumtbl_lun[sd->sd_lun] = csumtbl = malloc(sdsize(sd));
		bzero(csumtbl, sdsize(sd));
	}
#endif

	while (len > nbdbufsz) {
		nbdread(sd, fd, sector, nbdbufsz);
		sector += nbdbufsz;
		len -= nbdbufsz;
	}

	rem = len << 9;
	while (rem) {
		ret = TEMP_FAILURE_RETRY(read(fd, buf, rem));
		assert (ret > 0);
		rem -= ret;
		buf += ret;
	}

#ifndef NOCHECKSUM
	for (i = 0; i < len; i++) {
		int j, csum;
		unsigned char *c = &nbdbuf[512 * i];
		for (csum = 0, j = 0; j < 512; j++) {
			csum += c[j];
		}
		csumtbl[i + sector] = 0x80 | (csum & 0x7f);
	}
#endif

again:
	if (hup) xsdreset(sd);
	if (opt_erasehint) sd->sd_erasehint = len;
	ret = xsdwrite(sd, sector, nbdbuf, len);
	if (ret != 0) {
		hup = 1;
		usleep(500000);
		goto again;
	}
}


static inline
int xread(int fd, void *d, int len) {
	int olen = len;
	char *buf = d;

	if (buf == NULL) {
		buf = nbdbuf;
		while (len > (nbdbufsz << 9)) {
			xread(fd, buf, nbdbufsz);
			len -= nbdbufsz;
		}
		if (len) xread(fd, buf, len);
	}

	while (len > 0) {
		int x = TEMP_FAILURE_RETRY(read(fd, buf, len));
		if (x == 0 || x < 0) return 0;
		buf += x;
		len -= x;
	}
	return olen;
}


static inline
int xwrite(int fd, void *d, int len) {
	int olen = len;
	unsigned char *buf = d;

	while (len > 0) {
		int x = TEMP_FAILURE_RETRY(write(fd, buf, len));
		if (x == 0 || x < 0) return 0;
		buf += x;
		len -= x;
	}
	return olen;
}


static int opt_foreground = 0;
static 
void nbdserver(struct sdcore *sd, const char *arg, struct in_addr iface) {
	int j, i, n, maxfd = 0;
	int infd[16], remfd[16];
	int offs[16], luns[16];
	char parts[16];
	int numnbd;
	int read_prev = 0;
	unsigned int next_sect = 0;
	struct sdcore *sdcs[16], *sdc_wpending = NULL;
	struct sockaddr_in sa;
	struct nbd_request req;
	struct nbd_reply resp;
	unsigned int reqmagic;
	fd_set rfds;
	key_t shmkey;
	int shmid;
	struct sigaction siga;

	FD_ZERO(&rfds);

	numnbd = 0;
	while(*arg) {
		if (*arg == ',') arg++;
		if (strncmp(arg, "lun", 3) != 0) break;
		arg += 3;
		if (!isdigit(*arg)) break;
		n = *arg - '0';
		if (sd->sd_lun == n) sdcs[numnbd] = sd;
		else sdcs[numnbd] = NULL;
		luns[numnbd] = n;
		arg++;
		if (*arg != ':') break;
		arg++;
		if (strncmp(arg, "part", 4) == 0) {
			arg += 4;
			if (!isdigit(*arg)) break;
			offs[numnbd] = -(*arg - '0');
			parts[numnbd] = *arg - '0';
			arg++;
		} else if (strncmp(arg, "disc", 4) == 0) {
			arg += 4;
			offs[numnbd] = 0;
			parts[numnbd] = 0;
		} else break;
		numnbd++;
		assert (numnbd <= 16);
	}

	if (*arg != 0) {
		fprintf(stderr, "NBDSPEC parse error\n");
		exit(9);
	}

	bzero(csumtbl_lun, sizeof(csumtbl_lun));

	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = iface.s_addr;
	for (i = 0; i < numnbd; i++) {
		int r, sk, x = 1;
		/* TCP server socket */
		sa.sin_port = htons(7500 + i);
		sk = socket(PF_INET, SOCK_STREAM, 0);
		setsockopt(sk, SOL_SOCKET, SO_REUSEADDR, &x, 4);
		assert(sk != -1);
		r = bind(sk, (struct sockaddr *)&sa, sizeof(sa));
		if (r) {
			perror("bind");
			exit(1);
		}
		r = listen(sk, 5);
		assert(r != -1);
		r = fcntl(sk, F_SETFL, O_NONBLOCK);
		assert(r != -1);
		if (sk > maxfd) maxfd = sk;
		FD_SET(sk, &rfds);
		infd[i] = sk;

		remfd[i] = -1;
	}
	resp.magic[0] = 0x67;
	resp.magic[1] = 0x44;
	resp.magic[2] = 0x66;
	resp.magic[3] = 0x98;
	reqmagic = htonl(0x25609513);
	if (!opt_foreground) {
		daemon(1, 0);
		reservemem();
		ioctl(devmem, MEM_GETPGD, &pgd);
		refresh_pgtbl();
	}

	siga.sa_handler = hupsig;
	sigemptyset(&siga.sa_mask);
	siga.sa_flags = 0;
	sigaction(SIGHUP, &siga, NULL);
	siga.sa_handler = usr1sig;
	sigaction(SIGUSR1, &siga, NULL);
	siga.sa_handler = usr2sig;
	sigaction(SIGUSR2, &siga, NULL);
	siga.sa_handler = intsig;
	sigaction(SIGINT, &siga, NULL);
	sigaction(SIGTERM, &siga, NULL);
	sigaction(SIGQUIT, &siga, NULL);
	sigaction(SIGABRT, &siga, NULL);
	siga.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &siga, NULL);
	killable--;
	is_nbd = 1;

	shmkey = 0x75000000;
	shmid = shmget(shmkey, 0x1000, IPC_CREAT);
	assert(shmid != -1);
	sbus_shm = shmat(shmid, NULL, 0);
	sbus_shm[0] = getpid();

superloop_start:
	if (sdc_wpending) {
		struct timeval tv;
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		n = select(maxfd + 1, &rfds, NULL, NULL, &tv);
		if (n == -1 && errno == EINTR) {
			FD_ZERO(&rfds);
		} else if (n == 0 && !hup) {
			xsdread(sdc_wpending, 0, nbdbuf, 1);
			sdc_wpending = NULL;
		}
	} else {
		n = select(maxfd + 1, &rfds, NULL, NULL, NULL);
		if (n == -1 && errno == EINTR) FD_ZERO(&rfds);
	}

	if (usr1) {
		if (sdc_wpending) {
			xsdread(sdc_wpending, 0, nbdbuf, 1);
			sdc_wpending = NULL;
			sbus_shm[9] &= ~1;
		}
		usr1 = usr2 = die = 0;
		sbus_shm[9] |= 2; /* this bit says NBD is pausing */
		while (usr2 == 0 && !die) pause();
		if (die) {
			sbus_shm[0] = sbus_shm[9] = 0;
			exit(1);
		}
		usr1 = usr2 = 0;
		sbus_shm[9] &= ~2;
	}

	if (hup) {
		hup = 0;
		for (i = 0; i < numnbd; i++) {
			struct sdcore *sdt = sdcs[i];
			if (sdcs[i] != sd) free(sdcs[i]);
			sdcs[i] = NULL;
			offs[i] = -parts[i];
			for (j = 0; j < numnbd; j++) 
			  if (sdcs[j] == sdt) {
				sdcs[j] = NULL;
				offs[j] = -parts[j];
			}
		}
	}

	for (i = 0; i < numnbd; i++) {
		int r;
		if (remfd[i] == -1) continue;

		if (FD_ISSET(remfd[i], &rfds)) {
			unsigned int sec, len;
			req.magic = 0;
			r = xread(remfd[i], &req, sizeof(req));
			if (r == 0) goto eof;
			assert (r == sizeof(req));
			assert (req.magic == reqmagic);

			resp.error = 0;

			/* lazy SD init */
			if (sdcs[i] == NULL) {
				sdcs[i] = malloc(sizeof(struct sdcore)); 
				assert(sdcs[i] != NULL);
				memcpy(sdcs[i], sd, sizeof(struct sdcore));
				sdcs[i]->sd_lun = luns[i];
				sdcs[i]->os_arg = sdcs[i];
				if (xsdreset(sdcs[i]) == 0) {
					resp.error = htonl(1);
					free(sdcs[i]);
					sdcs[i] = NULL;
				} else for (j = 0; j < numnbd; j++) {
					if (luns[j] == luns[i])
					  sdcs[j] = sdcs[i];
				}
			}

			/* lazy SD partition table parse */
			if (offs[i] < 0 && !resp.error) {
				r = get_ptbl_offs(sdcs[i], -offs[i], NULL, NULL);
				if (r > 0) offs[i] = r;
				else hup = resp.error = htonl(1);
			}

			memcpy(resp.handle, req.handle, 8);

			len = ntohl(req.len);
			assert ((len & 0x1ff) == 0);
			len = len >> 9;
			sec = (ntohl(req.fromhi) & 0x1ff) << 23;
			sec |= ntohl(req.fromlo) >> 9;
			sec += offs[i];

			if ((sec + len) >= sdsize(sdcs[i])) {
				// access beyond end of device
				hup = resp.error = htonl(1);
				sbus_shm[5]++;
			}

			if (resp.error) {
				if (req.type[3] == 1) {
					xread(remfd[i], NULL, len << 9);
				} 
				r = xwrite(remfd[i], &resp, sizeof(resp));
				if (r == 0) goto eof;
				assert (r == sizeof(resp));
				continue;
			}

			if (req.type[3] == 0) { /* READ */
				r = xwrite(remfd[i], &resp, sizeof(resp));
				if (r == 0) goto eof;
				assert (r == sizeof(resp));
				nbdwrite(sdcs[i], remfd[i], sec, len);
				sbus_shm[1]++;
				sbus_shm[2] += len;
				if (!read_prev || sec != next_sect)
				  sbus_shm[7]++;
				read_prev = 1;
				next_sect = sec + len;
				sdc_wpending = NULL;
				sbus_shm[9] &= ~1;
			} else if (req.type[3] == 1) { /* WRITE */
				nbdread(sdcs[i], remfd[i], sec, len);
				sbus_shm[3]++;
				sbus_shm[4] += len;
				if (read_prev || sec != next_sect)
				  sbus_shm[8]++;
				read_prev = 0;
				next_sect = sec + len;
				sdc_wpending = sdcs[i];
				sbus_shm[9] |= 1;
				r = xwrite(remfd[i], &resp, sizeof(resp));
				if (r == 0) goto eof;
				assert (r == sizeof(resp));
			} else goto eof;

		} else FD_SET(remfd[i], &rfds);

		continue;

eof:
		shutdown(remfd[i], SHUT_RDWR);
		FD_CLR(remfd[i], &rfds);
		TEMP_FAILURE_RETRY(close(remfd[i]));
		remfd[i] = -1;
		
	}

	for (i = 0; i < numnbd; i++) {
		/* new connection requests pending accept? */
		if (FD_ISSET(infd[i], &rfds)) {
			int r, sk, tos = IPTOS_LOWDELAY;
			int x;
			char d[128] = 
			  {0x00, 0x00, 0x42, 0x02, 0x81, 0x86, 0x12, 0x53};
			struct linger l;
			if (remfd[i] != -1) {
				shutdown(remfd[i], SHUT_RDWR);
				FD_CLR(remfd[i], &rfds);
				TEMP_FAILURE_RETRY(close(remfd[i]));
				remfd[i] = -1;
			}
			sk = TEMP_FAILURE_RETRY(accept(infd[i], NULL, 0));
			if (sk == -1) {
				FD_SET(infd[i], &rfds);
				continue;
			}
			setsockopt(sk, IPPROTO_IP, IP_TOS, &tos, 4);
			x = 1;
			setsockopt(sk, IPPROTO_TCP, TCP_NODELAY, &x, 4);
			setsockopt(sk, SOL_SOCKET, SO_KEEPALIVE, &x, 4);
			setsockopt(sk, SOL_SOCKET, SO_OOBINLINE, &x, 4);
			l.l_onoff = 0;
			l.l_linger = 0;
			setsockopt(sk, SOL_SOCKET, SO_LINGER, &l, sizeof(l));
			if (sk > maxfd) maxfd = sk;
			FD_SET(sk, &rfds);
			remfd[i] = sk;
			r = xwrite(sk, "NBDMAGIC", 8);
			assert (r == 8);
			r = xwrite(sk, d, 8);
			assert (r == 8);
			/* size */
			d[0] = 0; d[1] = 0; d[2] = 0; d[3] = 0x80;
			d[4] = 0; d[5] = 0; d[6] = 0; d[7] = 0;
			r = xwrite(sk, d, 8);
			assert (r == 8);
			bzero(d, 128);
			r = xwrite(sk, d, 128);
			assert (r == 128);
		} else {
			FD_SET(infd[i], &rfds);
		}
	}

	goto superloop_start;
}


int main(int argc, char **argv) {
	int sz, i, ret, c, l, partseek;
	struct sdcore sd;
	int opt_writetest = 0, opt_blocksz = 1;
	int opt_readtest = 0, opt_read = 0, opt_readblks = 0;
	int opt_writeset = 0, opt_write = 0, opt_writeblks = 0;
	int opt_clear = 0, opt_set = 0, opt_unlock = 0, opt_erase = 0;
	int opt_wprot = 0, opt_lun = 0, opt_seek = 0, opt_scanluns = 0;
	int opt_nodma = 0, opt_nbdserver = 0, opt_stats = 0;
	int opt_printmbr = 0, opt_setmbr = 0, opt_nomultiwrite = 0;
	struct in_addr opt_bind = { INADDR_ANY };
	char *opt_password = NULL, *opt_nbdarg = NULL;
	char sdbootdat[20], pwddat[17];
	int opt_random = 0, opt_noparking = 0;
	unsigned int opt_address = 0x13000000;
	unsigned int sizemask, randomseed = 0;
	FILE *ifile = NULL;
	key_t shmkey;
	int shmid;
	struct sigaction siga;
	static struct option long_options[] = {
	  { "setmbr", 0, 0, 'M' },
	  { "printmbr", 0, 0, 'P' },
	  { "stats", 0, 0, 'Q' },
	  { "bind", 1, 0, 'I' },
	  { "writetest", 0, 0, 't' },
	  { "writeset", 1, 0, 'x' },
	  { "writeimg", 1, 0, 'i' },
	  { "seek", 1, 0, 'k' },
	  { "readtest", 0, 0, 'r' },
	  { "noparking", 0, 0, 'o' },
	  { "read", 1, 0, 'R' },
	  { "random", 1, 0, 'n' },
	  { "write", 1, 0, 'W' },
	  { "blocksize", 1, 0, 'z' },
	  { "erasehint", 2, 0, 'E' },
	  { "sdboottoken", 1, 0, 'b' },
	  { "lun", 1, 0, 'l' },
	  { "address", 1, 0, 'a' },
	  { "clear", 0, 0, 'c' },
	  { "set", 0, 0, 's' },
	  { "unlock", 0, 0, 'u' },
	  { "erase", 0, 0, 'e' },
	  { "password", 1, 0, 'p' },
	  { "wprot", 0, 0, 'w' },
	  { "scanluns", 0, 0, 'S' },
	  { "nodma", 0, 0, 'm' },
	  { "nbdserver", 1, 0, 'd' },
	  { "foreground", 0, 0, 'f' },
	  { "nomultiwrite", 0, 0, 'N' },
	  { "help", 0, 0, 'h' },
	  { 0, 0, 0, 0 }
	};
	
	bzero(&sd, sizeof(sd));
	bzero(pwddat, sizeof(pwddat));

	while((c = getopt_long(argc, argv, 
	  "hwz:E:rR:x:W:n:oa:b:csuep:tl:i:k:Smd:fI:QPMN", 
	  long_options, NULL)) != -1) {
		switch (c) {
		case 'N':
			opt_nomultiwrite = 1;
			break;
		case 'M':
			opt_setmbr = 1;
			break;
		case 'P':
			opt_printmbr = 1;
			break;
		case 'Q':
			opt_stats = 1;
			break;
		case 'I':
			i = inet_aton(optarg, &opt_bind);
			if (i == 0) {
				fprintf(stderr, "Bad arg: %s\n", optarg);
				return 3;
			}
			break;
		case 'f':
			opt_foreground = 1;
			break;
		case 'd':
			opt_nbdserver = 1;
			opt_nbdarg = strdup(optarg);
			break;
		case 'm':
			opt_nodma = 1;
			break;
		case 'S':
			opt_scanluns = 1;
			break;
		case 'k':
			if (strcmp(optarg, "kernel") == 0) 
			  opt_seek = -32;
			else if (strcmp(optarg, "initrd") == 0)
			  opt_seek = -33;
			else if (optarg[0] == 'p' && optarg[1] == 'a' && 
			  optarg[2] == 'r' && optarg[3] == 't') 
			  opt_seek = -1 * (optarg[4] - '0');
			else opt_seek = strtoul(optarg, NULL, 0);
			break;
		case 'i':
			i = strlen(optarg);
			if (strcmp("-", optarg) == 0) ifile = stdin;
			else if (strcmp(".bz2", &optarg[i - 4]) == 0) {
				char b[512];
				snprintf(b, 512, "exec bunzip2 -c '%s'", 
				  optarg);
				ifile = popen(b, "r");
			} else ifile = fopen(optarg, "r");
			if (!ifile) {
				perror(optarg);
				return 3;
			}
			if (opt_writeblks == 0) opt_writeblks = INT_MAX;
			opt_write = 1;
			break;
		case 'b':
			sd.sdboot_token = strtoul(optarg, NULL, 0);
			break;
		case 'a':
			opt_address = strtoul(optarg, NULL, 0);
			break;
		case 'o':
			opt_noparking = 1;
			break;
		case 'n':
			randomseed = strtoul(optarg, NULL, 0);
			fprintf(stderr, "randomseed=0x%x\n", randomseed);
			opt_random = 1;
			break;
		case 'x':
			opt_writeset = strtoul(optarg, NULL, 0);
			break;
		case 'W':
			opt_write = 1;
			opt_writeblks = strtoul(optarg, NULL, 0);
			break;
		case 'R':
			opt_read = 1;
			opt_readblks = strtoul(optarg, NULL, 0);
			break;
		case 'r':
			opt_readtest = 1;
			break;
		case 't':
			opt_writetest = 1;
			break;
		case 'E':
			if (optarg) opt_erasehint = strtoul(optarg, NULL, 0) / 512;
			else opt_erasehint = INT_MAX;
			break;
		case 'z':
			opt_blocksz = strtoul(optarg, NULL, 0) / 512;
			break;
		case 'e':
			opt_erase = 1;
			break;
		case 'c':
			opt_clear = 1;
			break;
		case 's':
			opt_set = 1;
			break;
		case 'u':
			opt_unlock = 1;
			break;
		case 'w':
			opt_wprot = 1;
			break;
		case 'l':
			opt_lun = strtoul(optarg, NULL, 0);
			break;
		case 'p':
			l = strlen(optarg);
			for (i = 0; i < 16; i++) pwddat[i] = optarg[i % l];
			pwddat[16] = 0;
			opt_password = pwddat;
			break;
		case 'h':
		default:
			usage(argv);
			return(1);
		}
	} 

	shmkey = 0x75000000;
	shmid = shmget(shmkey, 0x1000, IPC_CREAT);
	assert(shmid != -1);
	sbus_shm = shmat(shmid, NULL, 0);

	errno = 0;
	if (getpriority(PRIO_PROCESS, sbus_shm[0]) == -1 &&
	  errno == ESRCH) sbus_shm[9] = sbus_shm[0] = 0;

	if (opt_stats) {
		fprintf(stderr, "nbdpid=%lld\n", sbus_shm[0]);
		fprintf(stderr, "nbd_readreqs=%lld\n", sbus_shm[1]);
		fprintf(stderr, "nbd_read_blks=%lld\n", sbus_shm[2]);
		fprintf(stderr, "nbd_writereqs=%lld\n", sbus_shm[3]);
		fprintf(stderr, "nbd_write_blks=%lld\n", sbus_shm[4]);
		fprintf(stderr, "nbd_seek_past_eof_errs=%lld\n", sbus_shm[5]);
		fprintf(stderr, "sdcard_resets=%lld\n", sbus_shm[6]);
		fprintf(stderr, "read_seeks=%lld\n", sbus_shm[7]);
		fprintf(stderr, "write_seeks=%lld\n", sbus_shm[8]);

		return 0;
	}

	siga.sa_handler = intsig;
	sigemptyset(&siga.sa_mask);
	siga.sa_flags = 0;
	sigaction(SIGINT, &siga, NULL);
	sigaction(SIGTERM, &siga, NULL);
	sigaction(SIGQUIT, &siga, NULL);
	sigaction(SIGABRT, &siga, NULL);
	
	devmem = open("/dev/mem", O_RDWR|O_SYNC);
	assert(devmem != -1);
	devkmem = open("/dev/kmem", O_RDWR|O_SYNC);
	assert(devkmem != -1);
	epdmaregs = (unsigned int *) mmap(0, 4096, 
	  PROT_READ | PROT_WRITE, MAP_SHARED, devmem, 0x80000000);
	epdmaregs += 0x140 / sizeof(unsigned int);
	msiregs = (unsigned int *) mmap(0, 4096, 
	  PROT_READ | PROT_WRITE, MAP_SHARED, devmem, 0xe8000000);
	msiregs += 0x200 / sizeof(unsigned int);
	mvdmaregs = msiregs + (0x200 / sizeof(unsigned int));
	doorbell = (unsigned int *) mmap(0, 4096, 
	  PROT_READ | PROT_WRITE, MAP_SHARED, devmem, 0xf1020000);
	doorbell += 0x400 / sizeof(unsigned int);
	syscon7350 = (unsigned short *) mmap(0, 4096, 
	  PROT_READ | PROT_WRITE, MAP_SHARED, devmem, 0x600ff000);
	syscon7350 += 0x80 / sizeof(unsigned short);
	sd.sd_regstart = (unsigned int) mmap(0, 4096, 
	  PROT_READ | PROT_WRITE, MAP_SHARED, devmem, opt_address & ~0xfff);
	ioctl(devmem, MEM_GETPGD, &pgd);

	if (is_cavium()) {
		struct sigaction sa;
		sa.sa_handler = alarmsig;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = 0;
		sigaction(SIGALRM, &sa, NULL);

		cvspiregs = (unsigned int *) mmap(0, 4096,
		  PROT_READ | PROT_WRITE, MAP_SHARED, devmem, 0x71000000);
		cvgpioregs = (unsigned int *) mmap(0, 4096,
		  PROT_READ | PROT_WRITE, MAP_SHARED, devmem, 0x7c000000);

		sbuslock();
		cvgpioregs[0] = (1<<17|1<<3);
		cvspiregs[0x64 / 4] = 0x0; /* RX iRQ threahold 0 */
		cvspiregs[0x40 / 4] = 0x80000c02; /* 24-bit mode, no byte swap */
		cvspiregs[0x60 / 4] = 0x0; /* 0 clock inter-transfer delay */
		cvspiregs[0x6c / 4] = 0x0; /* disable interrupts */
		cvspiregs[0x4c / 4] = 0x4; /* deassert CS# */
		for (i = 0; i < 8; i++) cvspiregs[0x58 / 4];
		sbusunlock();
		opt_address = opt_address & 0x3f;
		sd.os_timeout = sdcore_timeout;
		sd.os_reset_timeout = sdcore_reset_timeout;
	}

	sd.sd_regstart += opt_address & 0xfff;
	fprintf(stderr, "address=0x%08x\n", opt_address);
	sd.os_delay = sdcore_delay;
	sd.os_arg = &sd;
	sd.os_dmastream = NULL;
	sd.os_dmaprep = NULL;
	sd.sd_writeparking = !opt_noparking;
	sd.sd_pwd = (unsigned char *)opt_password;
	if (opt_nomultiwrite) sd.sd_nomultiwrite = 1;
	if (opt_password) fprintf(stderr, "password=\"%s\"\n", opt_password);
	if (opt_scanluns) for (i = 0; i < 8; i++) {
		struct sdcore *sdtmp;
		char tmpbuf[512];
		sdtmp = malloc(sizeof(struct sdcore));
		memcpy(sdtmp, &sd, sizeof(struct sdcore));
		fprintf(stderr, "lun%d_mb=", i);
		sdtmp->sd_lun = i;
		sz = xsdreset(sdtmp);
		fprintf(stderr, "%d\n", sz / 2048);
		if (sz) xsdread(sdtmp, 0, tmpbuf, 1);
		if (sdtmp->hw_version < 2) break;
	} 
	sd.sd_lun = opt_lun;
	sz = xsdreset(&sd);

#ifndef PPC
	if (sd.hw_version == 2) {
		/* only used on version > 1 cores */
		sd.os_irqwait = sdcore_irqwait; 
	} else if (sd.hw_version == 3 && !is_cavium()) {
		sd.os_irqwait = sdcore_irqwait2;
	}
#endif
	fprintf(stderr, "lun=%d\n", opt_lun);
	fprintf(stderr, "locked=%d\n", sd.sd_locked);
	fprintf(stderr, "readonly=%d\n", sd.sd_wprot);
	fprintf(stderr, "cardsize_sectors=%d\n", sz);
	fprintf(stderr, "cardsize_mb=%d\n", sz / 2048);
	fprintf(stderr, "sd_hc=%d\n", ((sd.sd_state & SD_HC) != 0));
	fprintf(stderr, "sd_50mhz=%d\n", ((sd.sd_state & SD_HISPEED) != 0));
	fprintf(stderr, "hw_version=%d\n", sd.hw_version);
	fprintf(stderr, "card_blocksize=%d\n", sd.sd_blocksize);
	fprintf(stderr, "sdboot_token=0x%08x\n", sd.sdboot_token);
	if (opt_writetest || opt_readtest || opt_read || opt_write ||
	  opt_nbdserver)
	  fprintf(stderr, "sdcore_blocksize=%d\n", opt_blocksz * 512);
	if (opt_writetest || opt_write)
	  fprintf(stderr, "write_parking=%d\n", sd.sd_writeparking);
	sizemask = 0;
	for (i = 0; i < 32; i++) {
		if (sz > (1 << (i + 1))) sizemask = (sizemask << 1) | 0x1;
		else break;
	}
	if (opt_nodma) {
		sd.os_dmastream = NULL;
		sd.os_dmaprep = NULL;
		/* sd.os_irqwait = NULL; */
	} else if (is_cavium()) {
		sd.os_dmastream = sdcore_cvdmastream;
	} else if (pgd && sd.hw_version == 2) {
		/* XXX: Hopefully no NAND DMA activity takes place */
		mvdmaregs[2] = 0x80000104; /* SDDAT2 reg */
		sd.os_dmastream = sdcore_dmastream;
		sd.os_dmaprep = sdcore_dmaprep;
		mlockall(MCL_CURRENT|MCL_FUTURE);
	} else if (pgd && sd.hw_version == 3 && !is_cavium()) {
		for (i = 0; i < 4; i++)  /* flush DMA */
		  *(volatile unsigned short *)(sd.sd_regstart + 0x20);
		*(volatile unsigned short *)(sd.sd_regstart + 0x18) = 0x4;
		sd.os_dmastream = sdcore_dmastream2;
		sd.os_dmaprep = sdcore_dmaprep;
		mlockall(MCL_CURRENT|MCL_FUTURE);
	}
	fprintf(stderr, "using_dma=%d\n", sd.os_dmastream ? 1 : 0);
	refresh_pgtbl();

	if (opt_printmbr) {
		int ty, sz, offs, hasext;
		for (i = 0, hasext = 0; i < 4; i++) {
			offs = get_ptbl_offs(&sd, i+1, &ty, &sz);
			fprintf(stderr, "part%d_offs=0x%x\n", i+1, offs);
			fprintf(stderr, "part%d_sz=0x%x\n", i+1, sz);
			fprintf(stderr, "part%d_type=0x%x\n", i+1, ty);
			if (ty == 5) {
				fprintf(stderr, "has_extended_partition=1\n");
				hasext = 1;
			}
		}
		i = 4;
		if (hasext) 
		  while ((offs = get_ptbl_offs(&sd, i+1, &ty, &sz)) != -1) {
			fprintf(stderr, "part%d_offs=0x%x\n", i+1, offs);
			fprintf(stderr, "part%d_sz=0x%x\n", i+1, sz);
			fprintf(stderr, "part%d_type=0x%x\n", i+1, ty);
			i++;
		} 
	}

	if (opt_setmbr) {
		i = setmbr(&sd);
		fprintf(stderr, "setmbr_ok=%d\n", i ? 0 : 1);
	}
		
	if (opt_seek == -32) { /* kernel partition */
		int ty;
		partseek = 1;
		for (i = 0; i < 4; i++) {
			opt_seek = get_ptbl_offs(&sd, i, &ty, NULL);
			if (ty == 0xda) break;
		}
	} else if (opt_seek == -33) { /* initrd partition */
		int ty, np = 0;
		partseek = 1;
		for (i = 0; i < 4; i++) {
			opt_seek = get_ptbl_offs(&sd, i, &ty, NULL);
			if (ty == 0xda) np++;
			if (np == 2) break;
		}
	} else if (opt_seek < 0) {
		partseek = 1;
		opt_seek = get_ptbl_offs(&sd, -opt_seek, NULL, NULL);
	} else partseek = 0;
	if (opt_erase) {
		ret = xsdlockctl(&sd, SDLOCK_ERASE, NULL, NULL);
		if (ret) {
			xsdlockctl(&sd, SDLOCK_SETPWD, pwddat, NULL);
			sd.sdboot_token = 0;
			sd.sd_pwd = NULL;
			xsdreset(&sd);
			ret = xsdlockctl(&sd, SDLOCK_ERASE, NULL, NULL);
		}
		fprintf(stderr, "erase_ok=%d\n", !ret);
	}
	if (opt_unlock) {
		fprintf(stderr, "unlock_ok=%d\n", sz != 0);
	}
	
	if (opt_clear && opt_password) {
		i = xsdlockctl(&sd, SDLOCK_CLRPWD, opt_password, NULL);
		fprintf(stderr, "clear_ok=%d\n", !i);
	}
	if (opt_set && opt_password) {
		for (i = 0; i < 20; i++) sdbootdat[i] = 0;
		i = xsdlockctl(&sd, SDLOCK_SETPWD, opt_password, sdbootdat);
		fprintf(stderr, "set_ok=%d\n", !i);
		fprintf(stderr, "sdboot_lockdat=\"");
		for (i = 0; i < 20; i++) fprintf(stderr, "%02x", sdbootdat[i]);
		fprintf(stderr, "\"\n");
	} 

	if (opt_wprot) {
		i = xsdsetwprot(&sd, 1);
		fprintf(stderr,"wprot_ok=%d\n", !i);
	}

	if (opt_writetest) {
		time_t start;
		char *buf, *bufpg;
		unsigned int j;

		buf = malloc(opt_blocksz * 512 + 4095);
		bufpg = (char *)(((unsigned int)buf + 4095) & ~0xfff);
		memset(bufpg, opt_writeset, opt_blocksz * 512);
		refresh_pgtbl();
		srandom(randomseed);
		if (opt_erasehint == INT_MAX) 
		  fprintf(stderr, "write_erasehint=1\n");
		else 
		  fprintf(stderr, "write_erasehint=%d\n", opt_erasehint * 512);
		fprintf(stderr, "write_byte=0x%x\n", opt_writeset);
		i = 0;
		j = 0;
		/* Synchronize to the next second transition */
		start = time(NULL);
		while (start == time(NULL));
		start = time(NULL);
		while (time(NULL) - start < 10) {
			if (opt_erasehint == INT_MAX) sd.sd_erasehint = opt_blocksz; 
			else if ((opt_erasehint > 0) && ((i % opt_erasehint) == 0)) 
				sd.sd_erasehint = opt_erasehint;
			ret = xsdwrite(&sd, j, bufpg, opt_blocksz);
			assert(ret == 0);
			i += opt_blocksz;
			if (opt_random) j = random() & sizemask;
			else j += opt_blocksz;
		}
		
		/* This makes sure previous writes are committed */
		ret = xsdread(&sd, 0, bufpg, opt_blocksz);
		assert(ret == 0);

		fprintf(stderr, "writetest_kbps=%d\n", i / 20);
		fprintf(stderr, "writetest_xfrps=%d\n", 
		  i / 10 / opt_blocksz);
		free(buf);
	}

	if (opt_readtest) {
		time_t start;
		char *buf, *bufpg;
		unsigned int j;
		int l = opt_blocksz * 512;

		buf = malloc(l + 4095);
		bufpg = (char *)(((unsigned int)buf + 4095) & ~0xfff);
		srandom(randomseed);
		bzero(bufpg, l);
		refresh_pgtbl();
		i = 0;
		j = 0;
		/* Synchronize to the next second transition */
		start = time(NULL);
		while (start == time(NULL));
		start = time(NULL);
		while (time(NULL) - start < 10) {
			ret = xsdread(&sd, j, bufpg, opt_blocksz);
			assert(ret == 0);
			i += opt_blocksz;

			if (opt_random) j = random() & sizemask;
			else j += opt_blocksz;
		}

		fprintf(stderr, "readtest_kbps=%d\n", i / 20);
		fprintf(stderr, "readtest_xfrps=%d\n", 
		  i / 10 / opt_blocksz);
		free(buf);
	}

	if (opt_read) {
		char *buf, *bufpg;
		struct timeval tv1, tv2;
		unsigned int j;

		buf = malloc(opt_blocksz * 512 + 4095);
		bzero(buf, opt_blocksz * 512);
		bufpg = (char *)(((unsigned int)buf + 4095) & ~0xfff);
		refresh_pgtbl();
		srandom(randomseed);
		if (opt_readblks > (sz / opt_blocksz) || opt_readblks == -1)
		  opt_readblks = (sz / opt_blocksz);
		gettimeofday(&tv1, NULL);
		for (i = 0, j = opt_seek; i < opt_readblks; i++) {
			if ((j + opt_blocksz) > sz) {
			  ret = xsdread(&sd, j, bufpg, sz - j);
			} else
			  ret = xsdread(&sd, j, bufpg, opt_blocksz);
			assert(ret == 0);
			ret = fwrite(bufpg, opt_blocksz * 512, 1, stdout);
			if (ret != 1) break;

			if (opt_random) j = (unsigned int)random() & sizemask;
			else j += opt_blocksz;
		}
		gettimeofday(&tv2, NULL);
		j = (tv2.tv_sec - tv1.tv_sec) * 1000;
		j += tv2.tv_usec / 1000 - tv1.tv_usec / 1000;
		if (j <= 0) j = 1;
		i = i * opt_blocksz;
		fprintf(stderr, "read_kb=%d\n", i / 2);
		fprintf(stderr, "read_kbps=%d\n", (i * 500 / j));
		free(buf);
	}

	if (opt_write) {
		char *buf, *bufpg;
		struct timeval tv1, tv2;
		unsigned int j;
		int errs=0; 

		buf = malloc(opt_blocksz * 512 + 4095);
		bufpg = (char *)(((unsigned int)buf + 4095) & ~0xfff);
		memset(bufpg, opt_writeset, opt_blocksz * 512);
		refresh_pgtbl();
		srandom(randomseed);
		if (opt_writeblks > (sz / opt_blocksz) || opt_writeblks == -1)
		  opt_writeblks = (sz / opt_blocksz);
		gettimeofday(&tv1, NULL);
		if (opt_seek && ifile && !partseek) {
			ret = fseek(ifile, opt_seek * 512, SEEK_SET);
			if (ret == -1) for (i = 0; i < opt_seek; i++) {
				fread(bufpg, 512, 1, ifile);
			}
		}
		for (i = 0, j = opt_seek; i < opt_writeblks; i++) {
			if (ifile) {
				ret = fread(bufpg, opt_blocksz * 512, 1, ifile);
				if (ret == 0 && feof(ifile)) break;
				assert(ret == 1);
			} 
			if (opt_erasehint) sd.sd_erasehint = opt_blocksz;
			if ((j + opt_blocksz) > sz) {
				int t;
				if (opt_erasehint) sd.sd_erasehint = sz - j;
				ret = xsdwrite(&sd, j, bufpg, sz - j);
				t = xsdreset(&sd);
				assert (t == sz);
			} else ret = xsdwrite(&sd, j, bufpg, opt_blocksz);
			if (ret != 0) {
				fprintf(stderr, "error%d=%d\n", errs++, j);
				ret = xsdwrite(&sd, j, bufpg, opt_blocksz);
			} 

			if (ret != 0) {
				fprintf(stderr, "perm_write_error=1\n");
				exit(1);
			}
			if (opt_random) j = (unsigned int)random() & sizemask;
			else j += opt_blocksz;
		}
		gettimeofday(&tv2, NULL);
		j = (tv2.tv_sec - tv1.tv_sec) * 1000;
		j += tv2.tv_usec / 1000 - tv1.tv_usec / 1000;
		if (j <= 0) j = 1;
		i = i * opt_blocksz;
		fprintf(stderr, "write_kb=%d\n", i / 2);
		fprintf(stderr, "write_kbps=%d\n", (i * 500 / j));
		/* This makes sure previous writes are committed */
		ret = xsdread(&sd, 0, bufpg, opt_blocksz);
		assert(ret == 0);
		free(buf);
	}

	if (opt_nbdserver) {
		if (sbus_shm[0]) {
			fprintf(stderr, "nbderror=\"NBD server already running\"\n");
			return 1;
		}
		nbdbufsz = opt_blocksz;
		nbdbuf = malloc(opt_blocksz * 512 + 4095);
		nbdbuf = (char *)(((unsigned int)nbdbuf + 4095) & ~0xfff);
		bzero(nbdbuf, opt_blocksz * 512);
		reservemem();
		refresh_pgtbl();
		reservemem();
		nbdserver(&sd, opt_nbdarg, opt_bind);
	}

	return 0;
}
