#include <assert.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sched.h>
#include <stdlib.h>
#include <limits.h>
#include "peekpoke.h"

#define DEGUG 1

#ifdef DEBUG
#define DEBUGMSG(msg,...)\
  printf(msg,__VA_ARGS__);
#else
#define DEBUGMSG(msg,...)
#endif

#ifdef REMOTE_ONLY
int init_cavium() {
  printf("No Cavium support in this binary");
  exit(3);
}
#else
void (*buslock)(void);
void (*busunlock)(void);
void (*buspreempt)(void);
void cavium_poke8(unsigned int, unsigned char);
void cavium_poke16(unsigned int, unsigned short);
void cavium_poke32(unsigned int, unsigned int);
unsigned char cavium_peek8(unsigned int);
unsigned short cavium_peek16(unsigned int);
unsigned int cavium_peek32(unsigned int);
void cavium_poke16_stream(unsigned int adr, unsigned short *dat, unsigned int len);
void cavium_peek16_stream(unsigned int adr, unsigned short *dat, unsigned int len);
void sbuslock(void);
void sbusunlock(void);

extern int gotHUP;
static volatile unsigned int *cvspiregs, *cvgpioregs;
int maxspeed = 1;
unsigned last_freq=0,last_edge=0;

static int spi_f[] = {
  75000000,37500000,18750000,12500000,9375000,
  7500000,6250000,5360000,4680000,4170000,
  3750000,3409090,3125000,2884615,2678571,
  2500000,2343750,2205882,2083333,1973684,
  1875000,1785714,1704545,1630434,1562500,
  1500000,1442307,1388888,1339285,1293103,
  1250000,0 // 1209677
};

//===========================================================================
// memoization of FPGA register 0
unsigned short spiR0 = 0, spiR0valid = 0;

unsigned short getR0(int force) {
  if (spiR0valid && !force) return spiR0;
  spiR0 = cavium_peek16(0);
  spiR0valid = 1;
  return spiR0;
}

void setR0(unsigned short val) {
  if (spiR0valid && spiR0 == val) return;
  spiR0valid = 1;
  spiR0 = val;
  cavium_poke16(0,val);
}

// functions accessing register 0
int cavium_spi_get_clk_state() {
}

// val = -1 means to invert it from its current value
int cavium_spi_set_clk_state(int val) {
}

//===========================================================================

void cavium_disable_cs() {
  unsigned short val = getR0(0);

  if (val & (1<<7)) {
    DEBUGMSG("cavium_disable_cs:%04X\n",val);
    setR0(val & ~(1<<7));
    setR0((val & ~(1<<7)) ^ (1 << 14));
  } else {
    DEBUGMSG("cavium_disable_cs:%04X (NOP)\n",val);
  }
}

int cavium_spi_detect_extsup() {
  unsigned short val;
  int ext = 0;

  val = getR0(0);
  setR0(val ^ (1 << 7));
  ext = (getR0(1) & (1 << 7)) != (val & (1 << 7));
  setR0(val);
  return ext;
}

void cavium_spi_getparms(int *ext,int *clock,int *edge,int *lun) {
  unsigned short val,val2,val3;
  int divisor;

  val = getR0(0);
  divisor = ((val & 1) << 4) + ((val >> 10) & 0xF);
  if (divisor == 0) {
    *clock = 75000000;
    maxspeed = 1;
    *edge = 1;
  } else {
    *clock = 75000000 / (2*divisor);
    maxspeed = 0;
    *edge = ((val >> 14) & 1) ? 1 : -1;
  }
  *lun = (val >> 8) & 0x3;
  *ext = cavium_spi_detect_extsup();
  printf("cavium_spi_getparms(%02X): ext=%d, divisor=%d, clock=%d, edge=%d, lun=%d\n",val,*ext,divisor,*clock,*edge,*lun);
}

void cavium_spi_speed(unsigned freq,int edge) {
  int i=0;
  unsigned short val;

  val = getR0(0);
  if (freq > 0) {
    last_freq = freq;
    while (spi_f[i] > freq) i++;
    DEBUGMSG("cavium_spi_speed(%d): [%d]=%d\n",freq,i,spi_f[i]);
    maxspeed = (i == 0);
    val &= ~(15<<10); // mask out existing speed[3:0] bits
    val |= (i& 15) << 10; // or in new speed[3:0] bits
    val &= ~(1 << 0); // mask out existing speed[4] bit
    val |= ((i >> 4) & 1); // or in new speed[4] bit
  }
  if (edge != 0) {
    last_edge = edge;
    val &= ~(1<<14); // mask out existing clock polarity bit
    DEBUGMSG("also: %d,%d",edge>0,maxspeed);
    val |= ((edge>0||maxspeed)?0:1) << 14; // or in new clock polarity bit
  } // 
  setR0(val);
}

/*
void cavium_flash_fastread(unsigned int adr, unsigned char *dat, unsigned int len) {
  if (len == 0) return;
  cavium_poke16(0x8, 0xb00|(adr>>16));
  cavium_poke16(0x8, adr);
  cavium_poke8(0x8, 0x0);
  assert (((unsigned int)dat & 0x1) == 0);
  assert ((len & 0x1) == 0);
  cavium_peek16_stream(0xa, (unsigned short *)dat, (len / 2) - 1);
  *(unsigned short *)(&dat[len - 2]) = cavium_peek16(0xc);
}
*/

int cavium_spi_lun(int n) {
  if (n < 0 || n > 3) return 3 & (getR0(0) >> 8);
  setR0((getR0(0) & (~(3<<8))) | (n << 8));
  return n;
}

void cavium_spi_read(int octets,char *buf,int de_cs) {
  unsigned s;
  int i,n = octets;

  if (n > 16 && maxspeed) {
    if (n % 2 == 0) {
      DEBUGMSG("peek16_stream %X for %d\n",0xA,(n-2)/2);
      cavium_peek16_stream(0x0A, (unsigned short *)buf, (n-2)/2);
      for (i=0;i<n-2;i+=2) { // swap byte order
	unsigned char b;

	b = buf[i];
	buf[i] = buf[i+1];
	buf[i+1] = b;
      }
      buf += (n-2);
      n = 2;
    } else {
      DEBUGMSG("peek16_stream %X for %d\n",0xA,(n-3)/2);
      cavium_peek16_stream(0x0A, (unsigned short *)buf, (n-3)/2);
      for (i=0;i<n-2;i+=2) { // swap byte order
	unsigned char b;

	b = buf[i];
	buf[i] = buf[i+1];
	buf[i+1] = b;
      }
      buf += (n-3);
      n = 3;
    }
  }
  while (n >= 4) {
    s = cavium_peek16(0x0A); // pipelined read
/*    printf("s1=%X\n",s);*/
    *buf++ = s & 0xff;
    *buf++ = s >> 8;
    n -= 2;
  }
  if (n > 2) { // n == 3
    s = cavium_peek16(0x08); // read, leave CS# asserted
    printf("s2=%X\n",s);
    *buf++ = s & 0xff;
    *buf++ = s >> 8;
    n -= 2;
    cavium_poke8(de_cs?0xC:0x8,0);
    *buf = cavium_peek16(0x2) >> 8;
  } else if (n == 2) {
    s = cavium_peek16(de_cs?0x0C:0x8);
    printf("s3=%X\n",s);
    *buf++ = s & 0xff;
    *buf++ = s >> 8;
    n -= 2;
  } else if (n == 1) {
    cavium_poke8(de_cs?0xC:0x8,0);
    *buf = cavium_peek16(0x2) >> 8;
  }
}



unsigned char xbuf1[512];

void _init_cavium() {
  int i;

  cvspiregs[0x64 / 4] = 0x0; /* RX IRQ threahold 0 */
  cvspiregs[0x40 / 4] = 0x80000c02; /* 24-bit mode, no byte swap */
  cvspiregs[0x60 / 4] = 0x0; /* 0 clock inter-transfer delay */
  cvspiregs[0x6c / 4] = 0x0; /* disable interrupts */
  cvspiregs[0x4c / 4] = 0x4; /* deassert CS# */
  for (i = 0; i < 8; i++) cvspiregs[0x58 / 4];
  cvgpioregs[0] = (2<<15|1<<17|1<<3);
  cavium_spi_speed(last_freq,last_edge);
  cavium_disable_cs(); // force CS# deassertion just in case
}


int init_cavium() {
  int devmem=-1;
  int spi_ext=0;

  cvspiregs  = map_phys(0x71000000,&devmem);
  assert(devmem != -1);
  cvgpioregs = map_phys(0x7c000000,&devmem);
  sbuslock();
  _init_cavium();
  sbusunlock();

  buslock = sbuslock;
  busunlock = sbusunlock;
  return 1;
}

void cavium_poke8(unsigned int adr, unsigned char dat) {
  if (adr & 0x1) {
    cvgpioregs[0] = (0x2<<15|1<<17);
    cavium_poke16(adr, dat << 8);
  } else {
    cvgpioregs[0] = (0x2<<15|1<<3);
    cavium_poke16(adr, dat);
  }
  cvgpioregs[0] = (0x2<<15|1<<17|1<<3);
}

void cavium_poke16(unsigned int adr, unsigned short dat) {
  unsigned int dummy;
  unsigned int d = dat;
  volatile unsigned int *spi = cvspiregs;


  printf("POKE16 dat=%X,adr=%X\n",dat,adr);
  asm volatile (
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

void cavium_poke32(unsigned int adr, unsigned int dat) {
  cavium_poke16(adr, dat & 0xffff);
  cavium_poke16(adr + 2, dat >> 16);
}

unsigned char cavium_peek8(unsigned int adr) {
  unsigned char ret;
  unsigned short x;
  x = cavium_peek16(adr);
  if (adr & 0x1) ret = x >> 8;
  else ret = x & 0xff;
  return ret;
}


unsigned short cavium_peek16(unsigned int adr) {
  unsigned short ret;
  volatile unsigned int *spi = cvspiregs;

  asm volatile (
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
  printf("PEEK16 adr=%X,dat=%X\n",adr,ret);
  return ret;
}

unsigned int cavium_peek32(unsigned int adr) {
  unsigned int ret;
  unsigned short l, h;
  l = cavium_peek16(adr);
  h = cavium_peek16(adr + 2);
  ret = (l|(h<<16));
  return ret;
}

void cavium_poke16_stream(unsigned int adr, unsigned short *dat, unsigned int len) {
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

void cavium_peek16_stream(unsigned int adr, unsigned short *dat, unsigned int len) {
  unsigned int dummy;
  volatile unsigned int *spi = cvspiregs;
  

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

void reservemem(void) {
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
		unsigned int s, e, i;
		char m[PATH_MAX + 1];
		char perm[16];
		int r = fscanf(maps, "%x-%x %s %*x %*x:%*x %*d",
		  &s, &e, perm);
		if (r == EOF) break;
		assert (r == 3);

		i = 0;
		while ((r = fgetc(maps)) != '\n') {
			if (r == EOF) break;
			m[i++] = r;
		}
		m[i] = '\0';
		assert(s <= e && (s & 0xfff) == 0);
		if (perm[0] == 'r') while (s < e) {
			unsigned char *ptr = (unsigned char *)s;
			*ptr;
			s += pgsize;
		}
	}
}

static int semid = -1;
static int sbuslocked = 0;
int canadr;
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
	cvgpioregs[0] = (2<<15|1<<17|1<<3);
	sbuslocked = 1;
	canadr = -1;
	if (gotHUP) {
	  _init_cavium();
	  gotHUP = 0;
	}
}

void sbusunlock(void) {
	struct sembuf sop = { 0, 1, SEM_UNDO};
	int r;
	if (!sbuslocked) return;
	r = semop(semid, &sop, 1);
	assert (r == 0);
	sbuslocked = 0;
}


#endif
