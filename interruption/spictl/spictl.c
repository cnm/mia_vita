/*
 * Command to work:
 *
 * ./spictl -l 1 -r 4
 *
 */

#include <stdio.h>
#include <getopt.h>
#include <stdlib.h> /*  For malloc  */
#include <string.h> /* For strlen */
#include <ctype.h> /* For tolower  */



#include "defbin.h"
#include "opt.h"
#include <netinet/in.h>


//Stream protocol:
#define SPI_CMD_MASK            b1100_0000
#define SPI_CS			b00xx_xxxx
#define SPI_CS_AMASK            b0010_0000
#define SPI_CS_ASSERT	        bxx1x_xxxx
#define SPI_CS_DEASSERT	        bxx0x_xxxx
#define SPI_CS_DOMASK           b0001_0000
#define SPI_CS_DOASSERT         bxxx1_0000
#define SPI_CS_EMASK1           b0000_1000
#define SPI_CS_EMASK2           b0000_0100
#define SPI_CS_CHEDGE           bxxxx_1xxx
#define SPI_CS_EDGE_POS         bxxxx_x1xx
#define SPI_CS_EDGE_NEG         bxxxx_x0xx
#define SPI_CS_NMASK            b0000_0011
#define SPI_READ		b01xx_xxxx
#define SPI_SIZE_MASK           b0011_1111

void print_octal(char * rbuf, unsigned int bytes);

struct instance_data {
    unsigned char *un;
    int bytes;
};

extern void (*buslock)(void);
extern void (*busunlock)(void);
extern void (*buspreempt)(void);
extern unsigned char xbuf1[512];

int cavium_spi_lun(int n);
void cavium_spi_read(int octets,char *buf,int de_cs);
void cavium_disable_cs();

int result = 0;

#define CAVIUM_SPI_LUN(lun) \
  cavium_spi_lun(lun);
#define CAVIUM_SPI_SPEED(clk,edge) \
  cavium_spi_speed(clk,edge);
#define CAVIUM_SPI_READ(len1,retbuf,de_cs)\
  cavium_spi_read(len1,retbuf,de_cs);
#define CAVIUM_SPI_WRITE(len1,buf,de_cs) \
  cavium_spi_write(len1,buf,de_cs);
#define CAVIUM_DISABLE_CS() \
  cavium_disable_cs();
#define DEBUGMSG(msg,...)

// replace the first occurence of <X> with X in str.
// if this character is upper-case, make it lower-case
char *unaccelerate(const char *str) {
  char *ret,*str2 = malloc(strlen(str)+1);
  int found = 0;
  
  ret = str2;
  while (*str) {
    if (found == 0 && *str == '<') {
      found = 1;
      str++;
    } else if (found == 1 && *str == '>') {
      found = 2;
      str++;
    } else {
      if (found != 1) {
	*str2++ = *str++;
      } else {
	*str2++ = tolower(*str++);
      }
    }
  }
  return ret;
}

// return X for the first occurence of <X> in str
// if there is none, return a special non-char value
int accelerate(const char *str) {
  static int nonchar = 0x1000;

  while (*str && *str != '<') {
    str++;
  }
  if (*str == '<') {
    return *(str+1);
  } else {
    return nonchar++;
  }
}

void process_options(int argc,char **argv,struct option2 *opts) {
  struct option *long_options;
  char *optstr;
  const char *prolog="",*epilog="";
  int i,n,j,c;

  for (n=0;
       opts[n].has_arg||opts[n].f||opts[n].target;
       n++);
  if (opts[n].name) {
    prolog = opts[n].name;
  }
  if (opts[n].help) {
    epilog = opts[n].help;
  }
  long_options = malloc(sizeof(struct option) * (n+2));
  optstr = malloc(2+3*n);
  j=0;
  optstr[j] = 0;
  for (i=0;i<n;i++) {
    long_options[i].name = unaccelerate(opts[i].name);
    long_options[i].has_arg = opts[i].has_arg;
    long_options[i].flag = 0;
    long_options[i].val = accelerate(opts[i].name);
    if (long_options[i].val && long_options[i].val < 0xFF) {
      optstr[j++] = long_options[i].val;
      if (long_options[i].has_arg > 0) {
	optstr[j++] = ':';
      } 
      if (long_options[i].has_arg > 1) {
	optstr[j++] = ':';
      }
      optstr[j] = 0;
    }
  }
  long_options[i].name = "help";
  long_options[i].has_arg = 0;
  long_options[i].flag = 0;
  long_options[i].val = 0xFFF;
  long_options[i+1].name = 0;
  long_options[i+1].has_arg = 0;
  long_options[i+1].flag = 0;
  long_options[i+1].val = 0;
  optstr[j++] = 'h';
  optstr[j] = 0;

  while ((c = getopt_long(argc,argv,optstr,long_options,NULL)) != -1) {
    for (i=0;i<=n;i++) {
      if (c == long_options[i].val) {
	if (c == 0xFFF) {
	  fprintf(stderr,"%s",prolog);
	  for (j=0;j<n;j++) {
	    if (opts[j].help) {
	      if (long_options[j].val < 0x100) {
		fprintf(stderr,"-%c | --%s%s%s\n",long_options[j].val,
			long_options[j].name, 
			(long_options[j].has_arg>0 ? "=" : " "),
			opts[j].help);
		/*
		  To do: if has_arg == 2, then the parameter is optional.
		  we should show that somehow.
		 */
	      } else {
		fprintf(stderr,"--%s  %s\n",long_options[j].name, opts[j].help);
	      }
	    }
	  }
	  fprintf(stderr,"%s",epilog);
	} else if (!opts[i].f(optarg,opts[i].target,c)) {
	  return;
	}
      }
    }
  }
  free(long_options);
  free(optstr);
}


// takes: length of buffer, and buffer with command stream
// and pointer to integer to put the length of buffered returned
// and pointer to integer to put number of bytes we processed
// returns that buffer
/*
   interpretting a command stream
   1. we must look at the first byte to see what the command is
   and determine the length of the command
   2. if we don't have all the data for the command we cannot execute
   the command
   3. commands are length denoted strings
   4. if we can look ahead to the next command and see that is it a
   de-assertion, we can optimize.
 */
/*
 * Executes the SPI commands present in the buf var
 * 
 * len = lenght in bytes ??
 * buf = buf of commands
 * n   = ???
 * did = ???
 */
unsigned char *interpret_spi_commandstream(int len,unsigned char *buf,int *n,int *did) {
    unsigned char *next = 0; //The next command
    unsigned char *retbuf=0,*end,*buf0=buf;
    int retlen=0,lun,clk,edge;
    int de_cs = 0;
    int len1 = 0;

    end = buf + len;
    if (did) *did = 0;
    buslock();
    lun = cavium_spi_lun(-1);

    printf("interpretting %d bytes, buf=%p, end=%p\n\n",len,buf,end);

    while (len > 0) {
/*        printf("\t%d bytes left, buf[0]=%02X\n",len,buf[0]);*/
        switch ((buf[0] & SPI_CMD_MASK) >> 6) { //buf[0] & b1100_0000


          case 0: // SPI_CS
            printf("\nSPI_CS\n");
            if (buf[0] & SPI_CS_AMASK) {
                next = buf + 3;
                if (next > end) { // insufficient length to hold args
                    printf("%p > %p + %d\n",next,buf,len1);
                    if (n) *n = retlen;
                    return retbuf;
                }
                if ((buf[0] & SPI_CS_DOMASK) == SPI_CS_DOASSERT) {
/*                    if (lun != (buf[0] & SPI_CS_NMASK)) {*/ // I'm forcing it
                        printf("\tSetting the lun ");
                        lun = buf[0] & SPI_CS_NMASK;
                        printf("lun=%d\n",lun);
                        CAVIUM_SPI_LUN(lun);
/*                    }*/
                }
                clk = ((((unsigned)buf[1]) << 8) + buf[2]) * 2048;
                if ((buf[0] & SPI_CS_EMASK1) == SPI_CS_CHEDGE) {
                    if ((buf[0] & SPI_CS_EMASK2) == SPI_CS_EDGE_POS) {
                        edge = 1;
                    } else {
                        edge = -1;
                    }
                } else {
                    edge = 0;
                }
                if (clk || edge) {
                    printf("Setting speed and/or edge");
                    printf("\tclk=%d, edge=%d\n",clk,edge);
                    CAVIUM_SPI_SPEED(clk,edge);
                }
                buf+=3;
                if (did) *did += 3;
                len-=3;

            } else {
                if (buf0 == buf) { // first command
                    printf("CMD 0 deassert (forced)\n");
                    CAVIUM_DISABLE_CS();
                } else {
                    printf("CMD 0 deassert\n");
                }
                buf++;
                if (did) *did += 1;
                len--;
            }
            break;

          case 1: // SPI_READ
            printf("\nSPI_READ\n");
            len1 = (1 << ((unsigned)buf[0] & SPI_SIZE_MASK));
            next = buf+1;
            de_cs = (len > 1) && ((next[0] & SPI_CMD_MASK) == 0) && ((next[0] & SPI_CS_AMASK) == 0);
            retbuf = realloc(retbuf,retlen+len1);
            printf("\tread %d bytes to %p, de_cs=%d\n",len1,retbuf+retlen,de_cs);
            /*            printf("----> BEFORE %08X\n", *(retbuf+retlen));*/
            CAVIUM_SPI_READ(len1,retbuf+retlen,de_cs);

            printf("TESTING CAVIUM \n");
            result = (cavium_peek16(0x0008) << 16) ;
            result |= cavium_peek16(0x0008);

            /*            printf("--->HERE %08X\n", *(retbuf+retlen));*/
            retlen += len1; //Updates the size of the return
            buf++;
            if (did) *did += 1;
            len--;
            break;

          case 2: // SPI_WRITE
            printf("\nSPI_WRITE\n");
            printf("\nERROR - Removed \n");
            exit(1);
            break;

          case 3: // SPI_READWRITE
            printf("\nSPI_READWRITE\n");
            printf("\nERROR - Removed \n");
            exit(1);
            break;
        }
    }
    busunlock();
    if (n) *n = retlen;

    printf("Ended SPI\n");
    return retbuf;
}


/*unsigned int cavium_peek32(unsigned int adr) {*/
/*  unsigned int ret;*/
/*  unsigned short l, h;*/
/*  l = cavium_peek16(adr);*/
/*  h = cavium_peek16(adr + 2);*/
/*  ret = (l|(h<<16));*/
/*  return ret;*/
/*}*/

int gotHUP = 0;

void do_hup() {
    gotHUP = 1;
}

int bufsize = 0,bufn = 0, expected=0;
char *buf = 0;
int server = -1;
int ext,clk,edge,lun;

void buf_largen(unsigned by) {
    if (bufn + by >= bufsize) {
        if (bufsize * 2 < bufn + by) {
            bufsize = bufn + by;
        } else {
            bufsize *= 2;
        }
        buf = realloc(buf,bufsize);
    }
}

void spi_init() {
    if (buf) {
        free(buf);
        buf = 0;
    }
    bufsize = bufn = expected = 0;
}

// clock=0 means to use existing clock
// edge>0 means use positive edge, edge<0 means use negative edge, 0=unchanged
// cs<0 means to use existing cs
int spi_assert_cs_config(int cs,int clock,int edge) {
    unsigned char edgelogic;
    buf_largen(2);
    if (cs > 3) return 0;
    if (clock > 2048*65535) return 0;
    edgelogic = (edge == 0) ? 0 : SPI_CS_CHEDGE | (edge>0 ? SPI_CS_EDGE_POS : SPI_CS_EDGE_NEG);
    buf[bufn++] = SPI_CS|SPI_CS_ASSERT|edgelogic|((cs>=0)?(cs|SPI_CS_DOASSERT):0);
    clock /= 2048;
    printf("Setted --clock to %d * 2048\n",clock);
    buf[bufn++] = clock >> 8;
    buf[bufn++] = clock & 0xFF;
    return 1;
}

int use_lun1() {
    int lun1 = 1;
    return spi_assert_cs_config(lun1,0,0);
}

/* Creates the buf command */
void spi_readstream(int bytes) {
    printf("Reading the spi stream??\n");
    int n,i,j;

    expected += bytes;
    while (bytes) {
        i = n = (bytes > 4096) ? 4096 : bytes;
        j = 1;
        while (i) {
            if (i & 1) {
                buf_largen(1);
                buf[bufn++] = SPI_READ|(j-1);
            }
            i >>= 1;
            j++;
        }
        bytes -= n;
    }

    printf("Ended creating the command to read spi stream??\n");
    printf("---------------------------\n\n");
}

unsigned char *spi_execute(int *n) {
    printf("Executing SPI\n");

    return interpret_spi_commandstream(bufn,buf,n,0);
}

/* See chapter 5.7 in the manual  */
int init_cavium();

int opt_int(char *arg,int *target,int opt) {
    // JONAS TO REMOVE */
    return 1;
}

int opt_spiseq(char *arg,unsigned *target,int opt) {
    int i;

    target[0]++;
    switch(opt) {
      case 'r':
        spi_readstream(atoi(arg));
        break;
      case 'c':
        i = atoi(arg);

        // this is a hack to get around the fact that we can only request
        // frequencies that are multiples of 2048 via our packet protocol
        // technically this means that we might get a slightly higher
        // frequency than we requested.  but if we don't do this, we can't
        // hit 75MHz, since we have a remainder and will get dropped to
        // 37.5MHz
/*        if (i % 2048 > 0) {*/
/*            i += 2048;*/
/*        }*/
        spi_assert_cs_config(-1,i,0);
        break;
      case 'e':
        spi_assert_cs_config(-1,0,atoi(arg));
        break;
    }
    return 1;
}


void print_octal(char * rbuf, unsigned int bytes){
    int i = 0;

    while(i<bytes){
        printf("##### %02X ", *(rbuf+i));
        /*        i += 4;*/
        i++;
    }

    printf("\n");

    return;
}

void print_int(char * rbuf){
    printf("## Normal %u\n", htonl(*((unsigned int*) rbuf)));
    printf("## HTONL: %u\n", *((unsigned int*) rbuf));
}

int main(int argc, char **argv) {
    int opt_doseq = 0;
    int bytes =0,ext=0;
    unsigned char *rbuf;

    struct option2 opts[] = {
          { 1, (opt_func)opt_spiseq ,&opt_doseq  ,"<c>lock", "frequency    SPI clock frequency" },
          { 1, (opt_func)opt_spiseq ,&opt_doseq  ,"<e>dge", "value         set clock edge (positive for > 0, negative for < 0)" },
          { 1, (opt_func)opt_spiseq ,&opt_doseq  ,"<r>eadstream", "bytes   read specified number of bytes from SPI to stdout" },
          { 1, (opt_func)opt_spiseq ,&opt_doseq  ,"<l>un", "id             Talk to specified chip number" },
          { 0,0,0,"Technologic Systems SPI controller manipulation.", "hex octets are hexadecimal bytes. for example,\n"}
    };
    printf("Initiating and locking\n");
    init_cavium();
    buslock();
    printf("Previous parameters ---> ");
    cavium_spi_getparms(&ext,&clk,&edge,&lun);
    busunlock();
    printf("Ended init ---- 2\n");
    printf("-------------------------------\n");

    use_lun1();
    process_options(argc,argv,opts);

    rbuf = spi_execute(&bytes);
    printf("#Read bytes:\n");
    print_octal(rbuf, (unsigned int) bytes);
    print_int(rbuf);
    printf("Teste: %u\n", result);

    printf("\n");

    return 0;
}
