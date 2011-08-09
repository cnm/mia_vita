/*
 * Command to work:
 *
 * ./spictl -l 1 -r 4
 *
 */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "defbin.h"
#include "opt.h"
#include "sock.h"
#include "file.h"
//Stream protocol:
#define SPI_CMD_MASK            b1100_0000
#define SPI_CS			b00xx_xxxx
#define SPI_CS_AMASK            b0010_0000
# define SPI_CS_ASSERT	        bxx1x_xxxx
# define SPI_CS_DEASSERT	bxx0x_xxxx
#define SPI_CS_DOMASK           b0001_0000
# define SPI_CS_DOASSERT        bxxx1_0000
#define SPI_CS_EMASK1           b0000_1000
#define SPI_CS_EMASK2           b0000_0100
#define SPI_CS_CHEDGE           bxxxx_1xxx
#define SPI_CS_EDGE_POS         bxxxx_x1xx
#define SPI_CS_EDGE_NEG         bxxxx_x0xx
#define SPI_CS_NMASK            b0000_0011
# define SPI_CS_0               bxxxx_xx00
# define SPI_CS_1               bxxxx_xx01
# define SPI_CS_2               bxxxx_xx10
# define SPI_CS_3               bxxxx_xx11
#define SPI_READ		b01xx_xxxx
#define SPI_WRITE		b10xx_xxxx
#define SPI_READWRITE		b11xx_xxxx
# define SPI_SIZE_1             bxx00_0000
# define SPI_SIZE_2             bxx00_0001
# define SPI_SIZE_4             bxx00_0010
# define SPI_SIZE_8             bxx00_0011
# define SPI_SIZE_16            bxx00_0100
# define SPI_SIZE_512           bxx00_1001
# define SPI_SIZE_4096          bxx00_1100
#define SPI_SIZE_MASK           b0011_1111

#define NOSIG(expression)			\
  (__extension__				\
   ({ long int __result;			\
    do __result = (long int) (expression);	\
    while (__result == -1L && errno == EINTR);	\
    __result; }))

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
    int pending=0;

    end = buf + len;
    if (did) *did = 0;
    buslock();
    lun = cavium_spi_lun(-1);

    printf("interpretting %d bytes, buf=%p, end=%p\n\n",len,buf,end);

    while (len > 0) {
        printf("\t%d bytes left, buf[0]=%02X\n",len,buf[0]);
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
                    if (lun != (buf[0] & SPI_CS_NMASK)) {
                        lun = buf[0] & SPI_CS_NMASK;
                        printf("lun=%d\n",lun);
                        CAVIUM_SPI_LUN(lun);
                    }
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
                printf("clk=%d, edge=%d\n",clk,edge);
                if (clk || edge) {
                    CAVIUM_SPI_SPEED(clk,edge);
                }
                buf+=3;
                if (did) *did += 3;
                len-=3;

            } else {
                if (buf0 == buf) { // first command
                    printf("CMD 0 deassert (forced)\n",0);
                    CAVIUM_DISABLE_CS();
                } else {
                    printf("CMD 0 deassert\n",0);
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
            printf("read %d to %p, de_cs=%d\n",len1,retbuf+retlen,de_cs);
            /*            printf("----> BEFORE %08X\n", *(retbuf+retlen));*/
            CAVIUM_SPI_READ(len1,retbuf+retlen,de_cs);
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

void spi_start(char *ip,int port) {
    server = create_client_socket(ip,port);
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
    edgelogic = (edge == 0) ? 0
      : SPI_CS_CHEDGE | (edge>0 ? SPI_CS_EDGE_POS : SPI_CS_EDGE_NEG);
    buf[bufn++] = SPI_CS|SPI_CS_ASSERT|edgelogic|((cs>=0)?(cs|SPI_CS_DOASSERT):0);
    clock /= 2048;
    printf("--clock=%d\n",clock);
    buf[bufn++] = clock >> 8;
    buf[bufn++] = clock & 0xFF;
    return 1;
}

int spi_assert_cs(int cs) {
    return spi_assert_cs_config(cs,0,0);
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

void spi_deassert_cs(int cs) {
    buf_largen(1);
    buf[bufn++] = SPI_CS|SPI_CS_DEASSERT|SPI_CS_3;
}

unsigned char *spi_execute(int *n) {
    char *buf1;
    unsigned ms = 1000;
    int got;
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
    char *buf;
    int i,n;

    target[0]++;
    switch(opt) {
      case 'l':
        spi_assert_cs(atoi(arg));
        break;
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
        if (i % 2048 > 0) {
            i += 2048;
        }
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
        printf(" %08X ", *(rbuf+i));
        /*        i += 4;*/
        i++;
    }

    printf("\n");

    return;
}

int main(int argc, char **argv) {
    unsigned opt_bytes=512;
    int opt_read=-1,opt_write=-1, opt_doseq = 0, opt_holdcs=0 , opt_lun=0;
    int opt_server = 0, opt_client = -1, opt_verbose = 0;
    int opt_ce = 0, opt_se = -1;
    int manu=-1, dev=-1, bytes, total=0,ext=0;
    unsigned char buf[512],*rbuf;

    struct option2 opts[] = {
          { 1, (opt_func)opt_spiseq ,&opt_doseq  ,"<c>lock", "frequency    SPI clock frequency" },
          { 1, (opt_func)opt_spiseq ,&opt_doseq  ,"<e>dge", "value         set clock edge (positive for > 0, negative for < 0)" },
          { 1, (opt_func)opt_spiseq ,&opt_doseq  ,"<w>ritestream", "data   write colon delimited hex octets to SPI" },
          { 1, (opt_func)opt_spiseq ,&opt_doseq  ,"rea<d>write", "data     write colon delimited hex octets to SPI while reading to stdout" },
          { 1, (opt_func)opt_spiseq ,&opt_doseq  ,"<r>eadstream", "bytes   read specified number of bytes from SPI to stdout" },
          { 0, (opt_func)opt_bool   ,&opt_holdcs ,"h<o>ldcs", "            don't de-assert CS# when done" },
          { 1, (opt_func)opt_spiseq ,&opt_doseq  ,"<l>un", "id             Talk to specified chip number" },
          { 2, (opt_func)opt_int    ,&opt_server ,"<s>erver", "<port>      Daemonize and run as server listening on port" },
          { 1, (opt_func)opt_spiseq ,&opt_client ,"<p>ort", "<host><:port> Talk to spictl server" },
          { 0,0,0,"Technologic Systems SPI controller manipulation.\n\nGeneral options:\n",
            "hex octets are hexadecimal bytes. for example,\n"
              "this command reads 32 bytes of CS#1 SPI flash from address 8192:\n"
              "./spictl -l 1 -w 0B:00:20:00:00 -r 32\n"
              /*
                 TS-4500 + TS-8200
                 ./spictl -e 1 -c 2000000 -l 0 -w 04:00 -d 08:00:14:00:18:00:24:00:28:00:34:00:38:00 -r 2 | hexdump -C
                 should return something close to this:
                 6e 0a 76 0a 35 0d 29 06  78 0a aa 0b 70 0a 00 00
               */
          }
    };
    printf("Initiating and locking\n");
    init_cavium();
    buslock();
    cavium_spi_getparms(&ext,&clk,&edge,&lun);
    busunlock();
    printf("Ended init ---- 2\n");
    printf("-------------------------------\n");


    process_options(argc,argv,opts);

    rbuf = spi_execute(&bytes);
    printf("Read bytes:\n");
    print_octal(rbuf, bytes);
    printf("\n");
}
