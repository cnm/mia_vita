/* Copyright 2008-2009, Unpublished Work of Technologic Systems
 * All Rights Reserved.
 *
 * THIS WORK IS AN UNPUBLISHED WORK AND CONTAINS CONFIDENTIAL,
 * PROPRIETARY AND TRADE SECRET INFORMATION OF TECHNOLOGIC SYSTEMS.
 * ACCESS TO THIS WORK IS RESTRICTED TO (I) TECHNOLOGIC SYSTEMS 
 * EMPLOYEES WHO HAVE A NEED TO KNOW TO PERFORM TASKS WITHIN THE SCOPE
 * OF THEIR ASSIGNMENTS AND (II) ENTITIES OTHER THAN TECHNOLOGIC
 * SYSTEMS WHO HAVE ENTERED INTO APPROPRIATE LICENSE AGREEMENTS.  NO
 * PART OF THIS WORK MAY BE USED, PRACTICED, PERFORMED, COPIED, 
 * DISTRIBUTED, REVISED, MODIFIED, TRANSLATED, ABRIDGED, CONDENSED, 
 * EXPANDED, COLLECTED, COMPILED, LINKED, RECAST, TRANSFORMED, ADAPTED
 * IN ANY FORM OR BY ANY MEANS, MANUAL, MECHANICAL, CHEMICAL, 
 * ELECTRICAL, ELECTRONIC, OPTICAL, BIOLOGICAL, OR OTHERWISE WITHOUT
 * THE PRIOR WRITTEN PERMISSION AND CONSENT OF TECHNOLOGIC SYSTEMS.
 * ANY USE OR EXPLOITATION OF THIS WORK WITHOUT THE PRIOR WRITTEN
 * CONSENT OF TECHNOLOGIC SYSTEMS COULD SUBJECT THE PERPETRATOR TO
 * CRIMINAL AND CIVIL LIABILITY.
 */

#define TXRXSTAT	18	
#define TXRXIRQ		16
#define RADR		20
#define TXCFG		22

#define XUTXMEM(xu, n, offs)	\
	(unsigned short *)((xu)->xu_memstart + 0x1000 + ((n) * 0x200) + ((offs)*2)) 
#define XUTXMEM_SHADOW(xu, n, offs)	&(xu)->txmem_shadow[n * 0x100 + offs]

/* Expected worst-case IRQ latency in microseconds */
#ifndef IRQLAT
#define IRQLAT 1000
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifdef XUDEBUG
#include <stdio.h>
extern FILE *logfile;
#endif

struct xuart {
	int remain;
	int baudrate;
	int busy;
	unsigned short *xtbl;
	unsigned char txput;
	unsigned char txspc;
	unsigned char rxcnt;
	unsigned char rxirq;
	unsigned char nbits;
	unsigned char txovr;
	unsigned char tirqpos;
	unsigned char rmsk;
	unsigned short nspecials;
	unsigned short tirqdat;
	unsigned short rfifo_put;
	unsigned short rfifo_get;
	unsigned short tfifo_put;
	unsigned short tfifo_get;
	unsigned short rfifo[2048];
	unsigned short tfifo[2048];
};

struct xuartcore {
	/* xu_regstart must be initialized before calling xu_reset() and must
	 * be a pointer to the start of XUART IO space.  xu_memstart must
	 * point to the start of the XUART memory.
	 */
	unsigned int xu_regstart;
	unsigned int xu_memstart;

	/* xu_osarg is supplied as the first argument to all callbacks */
	void *xu_osarg;

	/* xu_atomic_set is called upon to atomically set the value and
	 * return the previous state of the value pointed to by the arg.  If
	 * this is NULL, a default, non-atomic version will be used and the
	 * XUART API calls should not be considered thread-safe.
	 */
	int (*xu_atomic_set)(int *);

	/* xu_maskirq is called to request masking/unmasking of the XUART
	 * IRQ.  This is to prevent asynchronous entry into the xu_irq()
	 * function when internal data structures are being manipulated.
	 * While IRQ's are unmasked, it is guaranteed xu_irq() is able to
	 * complete its function without blocking.  If NULL, and xu_irq()
	 * is called asynchronously from another thread preempted from
	 * another XUART API call, xu_atomic_set, xu_wait, and xu_signal MUST 
	 * be implemented as xu_irq() may end up having to sleep to wait
	 * for locks. 
	 *
	 * The function's second parameter is 1 to request masking, and 0
	 * to request unmasking.  The return value is the previous state
	 * of interrupt masking (1 - masked, 0 - unmasked);
	 */
	int (*xu_maskirq)(void *, int);

	/* xu_wait is called when the API is blocked waiting either for
	 * a hardware event or another thread to release a lock.  xu_signal is
	 * called to unblock another thread waiting in xu_wait().  The arg
	 * is a number from 0-24 representing the wait channel:
	 *
	 * 0 - TX wait channel for serial port #0
	 * 1 - TX wait channel for serial port #1
	 * ...
	 * 7 - TX wait channel for serial port #7
	 * 8 - RX wait channel for serial port #0
	 * 9 - RX wait channel for serial port #1
	 * ...
	 * 15 - RX wait channel for serial port #7
	 * 16-24 - internal mutex lock wait channels
	 *
	 * TX wait channels are slept on in xu_write() or xu_writec() whenever
	 * there is not enough buffer space to accept the user's full amount
	 * of TX bytes.  TX wait channels are also slept on in xu_draintx and
	 * xu_close to block until all pending TX data has been transmitted. 
	 * xu_open may also block here so that the receiver can be enabled 
	 * only after the baud rate has changed.
	 *
	 * RX wait channels are slept on in xu_read() and xu_readc() if there
	 * are not enough received bytes to completely satisfy the read
	 * request.
	 *
	 * xu_wait() may refuse to sleep on a given wait channel by returning
	 * -1.  In this way, client code can individual enable non-blocking
	 * modes for each serial channel in either the TX or RX direction.
	 * Also, the wait could have a time-out in which xu_wait will return -1
	 * after some time has elapsed without the wait channel having been
	 * signaled.
	 *
	 * If xu_wait is NULL, the XUART API always runs non-blocking and 
	 * xu_signal may then also be NULL.
	 *
	 * While being called upon to potentially sleep in xu_wait() on a 
	 * channel < 16, it is guaranteed no lock is held and the XUART IRQ
	 * will be unmasked.
	 *
	 * There may be multiple sleepers at a time on a given wait channel.
	 * It is ok to wake up all of them or only one of them when xu_signal
	 * is called.
	 *
	 * Wait channels 16-24 will never be slept on if there is never
	 * contention for internal locks.  Contention can be avoided if xu_irq()
	 * is only called when the IRQ is unmasked (as signaled by the 
	 * xu_maskirq() callback) and that no more than 1 thread is executing
	 * inside the XUART API functions at a time.  If called upon to wait
	 * on one of these wait channels, blocking cannot be refused (e.g.
	 * cannot return -1).  If blocking is refused on these channels or
	 * xu_wait is NULL, the API will purposefully attempt to crash by 
	 * derefencing NULL.
	 *
	 */
	int (*xu_wait)(void *, int);
	void (*xu_signal)(void *, int);

	/* xu_rxbreak() may be called from within the context of xu_readc()
	 * when a RX break is detected.  The first argument passed is the 
	 * xu_osarg, the second argument is the serial channel number, the
	 * third is the length of the break in bit times at the current
	 * baudrate, and the 4th is the index in the xu_readc buffer the 
	 * break immediately precedes. (e.g. 0 means before the first
	 * character, 1 means after the first character)  If the function
	 * returns -1, the rest of the xu_readc() is aborted and a short
	 * read may be returned.
	 *
	 * xu_rxidle() is similar to the xu_rxbreak() callback except that it
	 * signifies a period of idle detected.
	 *
	 * This function will be called with no locks held.
	 */
	int (*xu_rxbreak)(void *, int, int, int);
	int (*xu_rxidle)(void *, int, int, int);

	int status;
	int busy;
	unsigned short nchan;
	unsigned short txrxstat;
	unsigned short txcfg;
	unsigned short rget;
	unsigned short hwcts;
	unsigned short txmem_shadow[2048];
	struct xuart xu[8];
};


/* Public XUART API functions at a glance:
 * * xu_reset() -- resets and initializes FPGA core and xuartcore struct
 * * xu_open() -- initializes a particular serial channel
 * * xu_close() -- closes a particular serial channel
 * * xu_readc() -- reads characters (8-bits) from a particular channel
 * * xu_read() -- reads raw XUART RX works from channel
 * * xu_writec() -- writes characters to channel
 * * xu_write() -- write raw XUART TX opcodes to channel
 * * xu_stoptx() -- suspend/resume TX (without having to wait to drain buffers)
 * * xu_txbreak() -- initiate/terminate continuous serial BREAK condition
 * * xu_draintx() -- sleep until all TX completes
 * * xu_irq() -- client code should call this when XUART IRQ detected
 */

/* This is the first function that should be called.  It requires at the very
 * least that xu_regstart and xu_memstart members be setup ahead of time as
 * pointers to XUART io space and memory space, respectively.  Unutilized 
 * callback functions should be set to NULL.  xu_osarg is always the first
 * argument sent to callbacks (with the exception of xu_atomic_set) and should
 * also be set up before the xu_reset() call.
 *
 * xu_reset() returns the number of serial channels detected, or 0 if there
 * was no XUART core detected.
 */
int xu_reset(struct xuartcore *xu);


/* xu_open() initializes the given XUART serial channel baudrate and serial
 * parameters.  The "mode" argument is a string representing serial mode such
 * as "8n1" or "7e2" -- supported modes are listed below:
 *
 * * 8n1
 * * 8n2
 * * dmx - when in DMX mode, baudrate arg is not used (hardcoded 250 kbaud)
 * * 8e1
 * * 8o1
 * * 8e2
 * * 8o2
 * * 7n1
 * * 7n2
 * * 7e1
 * * 7o1
 * * 7e2
 * * 7o2
 * * 9n1 - in 9-bit mode, the character read/write routines xu_readc/xu_writec
 *   probably should not be used.
 *
 * If ",hwcts" is appended to the mode (e.g. "8n1,hwcts") hardware flow control
 * is used.  Returns -1 on error and 0 on success.
 */
int xu_open(struct xuartcore *xu, int uartn, char *mode, int baudrate);


/* xu_writec (write characters) takes a byte array and tries to enqueue up 
 * to 'n' bytes for transmission.  If n is 0 or buf is NULL, nothing is
 * written, and the current number of TX bytes pending is returned.  Up to
 * 2 kbytes is enqueued in the XUART software API and 256 bytes can be 
 * enqueued in the hardware.  If there is not enough space to satisfy 
 * the request and TX blocking mode is enabled (see discussion on blocking
 * modes under the xu_wait() callback above), xu_writec will block until
 * all bytes are enqueued.  If TX is configured as non-blocking, xu_writec
 * will return however many bytes were successfully accepted which may be
 * zero.
 *
 * xu_write and xu_writec are not considered thread-safe if called from 
 * multiple threads for the same UART channel at the same time.
 *
 * xu_write is just like xu_writec except that it takes raw XUART TX opcodes
 * as described in the XUART hardware documentation.  This interface should
 * be used in >8 bit modes or when periods of timed breaks and idle
 * periods are necessary.
 */
int xu_writec(struct xuartcore *xu, int uartn, unsigned char *buf, int n);
int xu_write(struct xuartcore *xu, int uartn, unsigned short *buf, int n);


/* xu_readc returns up to "n" received bytes in "buf" and returns how many
 * were read.  If buf is NULL, xu_readc returns how many bytes are waiting
 * to be read without actually reading them.  The XUART API can buffer
 * 2 kbytes.  Note that the XUART hardware counts idle words and break words,
 * but will not return them in xu_readc.
 *
 * As a read optimization feature, when called with buf == NULL, "n" is
 * supplied to the API as a hint as to how many bytes the user would like
 * to read next.  This is used to optimally schedule a RX interrupt when that
 * size request could be met.  Normally, after a read request is 100%
 * completed, the next RX irq will be delayed as much as possible to reduce CPU
 * load.  If the read request is blocking, this will always be the case, but
 * if the read request is non-blocking, the RX IRQ will be scheduled to 
 * precisely interrupt at the point the original request would have been
 * completed.  If "n" is 0, the next RX IRQ is pushed back as far as possible
 * (255 bytes in the future)
 *
 * xu_read and xu_readc are not considered thread-safe if called from 
 * multiple threads for the same UART channel at the same time.
 *
 * xu_read is just like xu_readc except that it returns raw XUART RX words.
 * Refer to the XUART hardware documentation for further information.
 */
int xu_readc(struct xuartcore *xu, int uartn, unsigned char *buf, int n);
int xu_read(struct xuartcore *xu, int uartn, unsigned short *buf, int n);


/* xu_stoptx() either suspends TX ("stop" parameter == 1) or resumes TX
 * ("stop" parameter == 0).  The current byte being transmitted will
 * complete and possibly one more after that after being suspended.  Returns
 * 1 if the transmitter is currently shifting bits out, 0 if the transmitter
 * is idle, and -1 on error (uart not open).
 *
 * If the channel is closed while suspended, all pending TX data is canceled.
 */
int xu_stoptx(struct xuartcore *xu, int uartn, int stop);


/* xu_txbreak() either starts/cancels sending of a continuous break condition.
 * if "brk" == 1, current TX activity is suspended similar to xu_stoptx() and
 * followed by the continuous assertion of the break condition on the line.
 * This is not a timed break -- those must be sent with proper TX opcodes 
 * sent with xu_write().
 *
 * Returns 0 on success, or -1 on error.
 */ 
int xu_txbreak(struct xuartcore *xu, int uartn, int brk);


/* xu_draintx() will block the current thread until all pending TX is completed
 * provided TX blocking mode is enabled.  If TX blocking is not enabled,
 * xu_draintx() will only return the count of TX bytes remaining to be sent.
 *
 * Even with blocking enabled, draintx may still return a value of "1"
 * representing the very last byte as it shifts out of the transmitter. If
 * interested in synchronizing with the completion of all activity on the TX
 * line, xu_draintx() should be called in a loop until 0 is returned.
 */
int xu_draintx(struct xuartcore *xu, int uartn); 


/* xu_close() will disable a channel and block until all pending TX is
 * completed.  If TX is non-blocking, xu_close() will return a count of bytes
 * remaining for TX but still initiate XUART channel shutdown.  After closing,
 * xu_read() and xu_readc() may still be called to retreive pending RX bytes,
 * but upon re-opening, they will be lost.
 *
 * Returns 0 when closing is completed, -1 on error.
 */ 
int xu_close(struct xuartcore *xu, int uartn);


/* xu_irq() must be called by client code when the XUART asserts an IRQ.
 * It will return 0 if there was nothing to do, non-zero if new bytes were
 * RX/TX'ed, and -1 when an overflow is detected.
 *
 * If something was done, the return status is a 16 bit number:
 *   bits 15-8: TX activity (bit 8 -> channel 0)
 *   bits 7-0: RX activity (bit 0 -> channel 0)
 * This return status can be used in upper layer IRQ handlers to skip 
 * further processing of UART channels without activity.
 */
int xu_irq(struct xuartcore *xu);

static unsigned short xtbl_8n2[256];
static unsigned short xtbl_8e2[256];
static unsigned short xtbl_8o2[256];
static unsigned short xtbl_8e1[256];
static unsigned short xtbl_8o1[256];
static unsigned short xtbl_7n2[256];
static unsigned short xtbl_7e2[256];
static unsigned short xtbl_7o2[256];
static unsigned short xtbl_7e1[256];
static unsigned short xtbl_7o1[256];

static int do_write(struct xuartcore *, int, unsigned short *, int);
int xu_reset(struct xuartcore *);

static int xbits(unsigned short x) {
	int bt = x & 0x3ff;
	if (x & (1<<10)) bt = bt << 6;
	return bt;
}


static void fill_xtbl(void) {
	int i;
	static int filled = 0;
	if (filled) return;
	filled = 1;

	for (i = 0; i < 256; i++) {
		int j = i;
		int p = 0;
		while (j) {
			if (j & 0x1) p++;
			j >>= 1;
		}
		
		xtbl_7n2[i] = 0x80 | i;
		xtbl_8n2[i] = 0x100 | i;
		if (p & 0x1) {
			xtbl_8e1[i] = 0x100 | i; 
			xtbl_8o1[i] = i;
			xtbl_8e2[i] = 0x300 | i;
			xtbl_8o2[i] = 0x200 | i;
		} else {
			xtbl_8o1[i] = 0x100 | i; 
			xtbl_8e1[i] = i;
			xtbl_8o2[i] = 0x300 | i;
			xtbl_8e2[i] = 0x200 | i;
		}

		if (i >= 128) {
			xtbl_7o1[i] = xtbl_7o1[i-128];
			xtbl_7e1[i] = xtbl_7e1[i-128];
			xtbl_7o2[i] = xtbl_7o2[i-128];
			xtbl_7e2[i] = xtbl_7e2[i-128];
		} else if (p & 0x1) {
			xtbl_7e1[i] = 0x80 | i;
			xtbl_7o1[i] = i & 0x7f;
			xtbl_7o2[i] = 0x100 | (i & 0x7f);
			xtbl_7e2[i] = 0x180 | i;
		} else {
			xtbl_7o1[i] = 0x80 | i;
			xtbl_7e1[i] = i & 0x7f;
			xtbl_7e2[i] = 0x100 | (i & 0x7f);
			xtbl_7o2[i] = 0x180 | i;
		}
	}

}


static int nonatomic_set(int *x) {
	int prev = *x;
	*x = 1;
	return prev;	
}


static inline int
inc(int start, int end, int pos) {
	if (end > start) {
		return (pos >= start && pos < end);
	} else {
		return (pos < end || pos >= start);
	}
}


static inline char *
xstrcasestr(char *big, char *little) {
	char aa, bb;
	while (*big) {
		char *x = little;
		char *y = big;
		do {
			aa = *x++;
			bb = *y++;
			if (aa >= 'A' && aa <= 'Z') aa = 'a' + (aa - 'A');
			if (bb >= 'A' && bb <= 'Z') bb = 'a' + (bb - 'A');
		} while (aa == bb && aa && bb);
		if (aa == 0) return big;
		else big++;
	}
	return NULL;
}


static inline int
xstrcasecmp(const char *a, const char *b) {
	char aa, bb;
	do {
		aa = *a++;
		bb = *b++;
		if (aa >= 'A' && aa <= 'Z') aa = 'a' + (aa - 'A');
		if (bb >= 'A' && bb <= 'Z') bb = 'a' + (bb - 'A');
	} while (aa == bb && aa && bb);
	if (aa == bb) return 0; else return 1;
}


static inline void
lock(struct xuartcore *xu, int uartn) {
	struct xuart *xuu = &xu->xu[uartn];
	int r;
	while (xu->xu_atomic_set(&xuu->busy)) {
		/* If client code didn't bother setting up a xu_wait() callback,
		 * this will crash here and it will not be our fault.
		 */
		r = xu->xu_wait(xu->xu_osarg, uartn + 16);
		if (r == -1) *((int *)NULL) = 0; /* crash me */
	}
}


static inline void
glock(struct xuartcore *xu) {
	int r;
	while (xu->xu_atomic_set(&xu->busy)) {
		/* If client code didn't bother setting up a xu_wait() callback,
		 * this will crash here and it will not be our fault.
		 */
		r = xu->xu_wait(xu->xu_osarg, 24);
		if (r == -1) *((int *)NULL) = 0; /* crash me */
	}
}


static inline void
unlock(struct xuartcore *xu, int uartn) {
	struct xuart *xuu = &xu->xu[uartn];
	xuu->busy = 0;
	if (xu->xu_signal) xu->xu_signal(xu->xu_osarg, uartn + 16);
}


static inline void
unglock(struct xuartcore *xu) {
	xu->busy = 0;
	if (xu->xu_signal) xu->xu_signal(xu->xu_osarg, 24);
}


#ifndef XUPEEK16
# define XUPEEK16(x, y) xupeek16((x), (y))
#endif
#ifndef XUPOKE16
# define XUPOKE16(x, y, z) xupoke16((x), (y), (z))
#endif
#ifndef PEEK16
# define PEEK16(x) peek16((x))
#endif
#ifndef POKE16
# define POKE16(x, y) poke16((x), (y))
#endif


static unsigned short peek16(volatile unsigned short *addr) {
#ifdef BIGENDIAN
	unsigned short r = *addr;
	return ((r << 8) | (r >> 8));
#else
	return (*addr);
#endif
}


static void poke16(volatile unsigned short *addr, unsigned short val) {
#ifdef BIGENDIAN
	val = (val << 8) | (val >> 8);
#endif
	*addr = val;
}


static unsigned short
xupeek16(struct xuartcore *xu, unsigned int offs) {
	return peek16((unsigned short *)(xu->xu_regstart + offs));
}
	

static void
xupoke16(struct xuartcore *xu, unsigned int offs, unsigned short val) {
	poke16((unsigned short *)(xu->xu_regstart + offs), val);
}


int xu_reset(struct xuartcore *xu) {
	int i, nchan;
	int tmout = 0;
	unsigned short *mem;

	/* probe how many channels we have */
	XUPOKE16(xu, TXRXSTAT, 0xff00);
	i = XUPEEK16(xu, TXRXSTAT) >> 8;
	nchan = 0;
	while (i) {
		nchan++;
		i = i >> 1;
	}
	if (nchan == 0) return 0;
	xu->nchan = nchan;

	/* disable receivers */
	XUPOKE16(xu, TXRXSTAT, 0);
	XUPOKE16(xu, TXCFG, 0);
	xu->rget = (XUPEEK16(xu, RADR) & 0xfff) >> 1;

	for (i = 0; i < nchan; i++) {
		unsigned short t = XUPEEK16(xu, i*2);
		xu->xu[i].baudrate = 0;
		xu->xu[i].nbits = 0;
		xu->xu[i].txput = t & 0xff;
		xu->xu[i].txspc = 0xff;
		xu->xu[i].rxcnt = (t >> 8);
		xu->xu[i].rxirq = (t >> 8);
		xu->xu[i].rfifo_get = xu->xu[i].rfifo_put = 0;
		xu->xu[i].tfifo_get = xu->xu[i].tfifo_put = 0;
		xu->xu[i].nspecials = 0;
		xu->xu[i].busy = 0;

		XUPOKE16(xu, i*2, xu->xu[i].txput | (xu->xu[i].rxirq << 8));
	}

	/* In case there are any sleepers */
	if (xu->xu_signal) for (i = 0; i < 25; i++) 
	  xu->xu_signal(xu->xu_osarg, i);
	if (xu->xu_atomic_set == NULL) xu->xu_atomic_set = nonatomic_set;
	fill_xtbl();
  
	/* only continue once TX has completely stopped */
	while ((XUPEEK16(xu, TXRXSTAT) & 0xff) && tmout < 10000000) tmout++;
	if (tmout == 10000000) return 0;

	if (xu->xu_maskirq) xu->xu_maskirq(xu->xu_osarg, 0);

	mem = (unsigned short *)xu->xu_memstart;
	for (i = 0; i < 0x1000; i++) POKE16(&mem[i], 0xffff);

	xu->status = 0;
	xu->busy = 0;
	xu->txrxstat = 0;
	xu->hwcts = 0;
	return xu->nchan;
}


static
int do_write(struct xuartcore *xu, int uartn, unsigned short *buf, int n) {
	struct xuart *xuu = &xu->xu[uartn];
	int txpos, x, y, orign = n, s = 0;
	unsigned char txput, txspc, recomputed = 0;
	volatile unsigned short *txmem;
	unsigned short *txmem_shadow;

	if (xu->xu_maskirq) s = xu->xu_maskirq(xu->xu_osarg, 1);
	lock(xu, uartn);
	txput = xuu->txput;
	txspc = xuu->txspc;
	txmem = XUTXMEM(xu, uartn, 0);
	txmem_shadow = XUTXMEM_SHADOW(xu, uartn, 0);
txagain:
	/* Recompute avail space */
	if (txspc == 0) { 
		txpos = XUPEEK16(xu, uartn*2) & 0xff;
		txspc = 0xff - ((txput - txpos) & 0xff);
		if (xuu->tirqdat && inc(txput, txpos, xuu->tirqpos)) {
			xuu->tirqdat = 0;
		}
		recomputed = 1;
	}
	while (txspc && xuu->tfifo_put != xuu->tfifo_get) {
		unsigned short v = xuu->tfifo[xuu->tfifo_get++];
		txmem_shadow[txput] = v;
		POKE16(&txmem[txput], v);
		txput++;
		xuu->tfifo_get &= 0x7ff;
		txspc--;
	}
	if (buf) while (n && txspc) {
		unsigned short v = *buf++;
		txmem_shadow[txput] = v;
		POKE16(&txmem[txput], v);
		txput++;
		n--;
		txspc--;
	}
	/* Have we run out of tx space but have more to send? */
	if (txspc == 0 && (n || xuu->tfifo_put != xuu->tfifo_get)) {
		if (!recomputed) goto txagain;
		/* schedule tx irq accomodating irq latency */
		x = (txput - xuu->txovr) & 0xff;
		y = txmem_shadow[x] | 0x8000; /* set irq flag */
		txmem_shadow[x] = y;
		POKE16(&txmem[x], y);
		if (xuu->tirqdat && (xuu->tirqpos != x)) {
			unsigned short v = xuu->tirqdat & 0x7fff;
			txmem_shadow[xuu->tirqpos] = v;
			POKE16(&txmem[xuu->tirqpos], v); 
		}
		xuu->tirqpos = x;
		xuu->tirqdat = y;
	} else if (txspc != 0xff && !(xu->txrxstat & (1<<(uartn + 8)))) {
		/* We're closing and have some remaining bytes in flight,
		   schedule a tx irq on the last one */
		unsigned short v;
		x = (txput - 1) & 0xff;
		v = txmem_shadow[x] | 0x8000;
		txmem_shadow[x] = v;
		POKE16(&txmem[x], v);
		/* Just in case we weren't quick enough setting irq bit... */
		txpos = XUPEEK16(xu, uartn*2) & 0xff;
		txspc = 0xff - ((txput - txpos) & 0xff);
	}
	xuu->txspc = txspc;
	if (txput != xuu->txput) {
		xuu->txput = txput;
		XUPOKE16(xu, uartn*2, xuu->txput | (xuu->rxirq << 8));
	}
	unlock(xu, uartn);

	if (buf) while (n && ((xuu->tfifo_get - 1) & 0x7ff) != xuu->tfifo_put) {
		xuu->tfifo[xuu->tfifo_put++] = *buf++;
		n--;
		xuu->tfifo_put &= 0x7ff;
	}

	glock(xu); /* XXX: this could be done with atomic ops */
	if (((xuu->tfifo_get - 1) & 0x7ff) == xuu->tfifo_put) /* fifo full? */
	  xu->status &= ~(1<<(uartn + 8));
	else xu->status |= 1<<(uartn + 8);
	unglock(xu);
	if (xu->xu_maskirq) xu->xu_maskirq(xu->xu_osarg, s);

	if (buf) return orign - n;
	else return ((xuu->tfifo_put - xuu->tfifo_get) & 0x7ff)+(0xff - txspc);
}


int xu_writec(struct xuartcore *xu, int uartn, unsigned char *buf, int n) {
	struct xuart *xuu = &xu->xu[uartn];
	int x = n;

	if (!(xu->txrxstat & (1<<(uartn + 8)))) goto ret;
	if (buf == NULL) return do_write(xu, uartn, NULL, n);
	
again:
	while (n && ((xuu->tfifo_get - 1) & 0x7ff) != xuu->tfifo_put) {
		if (xuu->xtbl) xuu->tfifo[xuu->tfifo_put++] = xuu->xtbl[*buf];
		else xuu->tfifo[xuu->tfifo_put++] = *buf;
		buf++;
		n--;
		xuu->tfifo_put &= 0x7ff;
	}
	if (xuu->txspc) do_write(xu, uartn, NULL, 1);
	if (n && xu->xu_wait) {
		int r;
		r = xu->xu_wait(xu->xu_osarg, 8 + uartn);
		if (r != -1) goto again;
	}
ret:
	return x - n;
}


int xu_write(struct xuartcore *xu, int uartn, unsigned short *buf, int n) {
	int i, x = n;

	if (!(xu->txrxstat & (1<<(uartn + 8)))) goto ret;
	if (buf == NULL) return do_write(xu, uartn, NULL, n);

again:
	i = do_write(xu, uartn, buf, n);
	buf += i;
	n -= i;
	if (n && xu->xu_wait) {
		int r;
		r = xu->xu_wait(xu->xu_osarg, 8 + uartn);
		if (r != -1) goto again;
	}
ret:
	return x - n;
}


/* When called with buf == NULL, will return number of bytes waiting in
 * buffer and n becomes a hint as too how many bytes the client is looking
 * for.  The hint is used for strategic scheduling of the RX irq.  By default,
 * the RX irq is delayed as much as possible to reduce IRQ load, but this
 * may not be desired for lowest latency.
 */
static
int do_read(struct xuartcore *xu, int uartn, unsigned short *buf, int n) {
	struct xuart *xuu = &xu->xu[uartn];
	int i, put, get, remain = n, signal, s;
	volatile unsigned short *hwfifo;
	unsigned int x, nbuf, nspecials;

	if (buf && n == 0) buf = NULL;
	else if (buf) while (xuu->rfifo_get != xuu->rfifo_put) {
		*buf = xuu->rfifo[xuu->rfifo_get++];
		if ((*buf >> 11) & 0x3) xuu->nspecials--;
		buf++;
		xuu->rfifo_get &= 0x7ff;
		if (xuu->rfifo_get == xuu->rfifo_put) {
			glock(xu);
			xu->status &= ~(1<<uartn);
			unglock(xu);
		}
		remain--;
		if (remain == 0) return n;
	}
	
	hwfifo = (unsigned short *)xu->xu_memstart;
	s = 0;
	if (xu->xu_maskirq) s = xu->xu_maskirq(xu->xu_osarg, 1);
	glock(xu);
	lock(xu, uartn);
	signal = 0;
	get = xu->rget;
	if (remain >= 0) put = (XUPEEK16(xu, RADR) & 0xfff) >> 1;
	else put = get;
again:
	while (put != get) {
		unsigned short d = PEEK16(&hwfifo[get++]);
		i = d >> 13;
		get &= 0x7ff;
		xu->xu[i].rxcnt++;
		if (i == uartn && remain > 0 && buf) {
			*buf++ = d;
			remain--;
		} else { 
			if ((d >> 11) & 0x3) xu->xu[i].nspecials++;
			xu->xu[i].rfifo[xu->xu[i].rfifo_put++] = d;
			xu->xu[i].rfifo_put &= 0x7ff;
			if (xu->xu[i].rfifo_put == xu->xu[i].rfifo_get) {
				int t = xu->xu[i].rfifo_get;
				d = xu->xu[i].rfifo[t];
				if ((d >> 11) & 0x3) xu->xu[i].nspecials--;
				t = (t + 1) & 0x7ff;
				xu->xu[i].rfifo_get = t;
			}
			signal |= (1 << i);
		}
	}
	/* RX IRQ scheduling: 
	 * case 1 - passed a non-NULL buf arg to read function
	 *  case 1a - req. fully completed and ...
	 *   case 1a1 - all chars from sw buf: retain prev. RX IRQ state
	 *   case 1a2 - needed chars from hw buf: delay IRQ as long as possible 
	 *  case 1b - req. incomplete: no IRQ until would-be completion point
	 * case 2 - passed a NULL buf: service hw and return num bytes waiting
	 *  case 2a - n arg < 0: hardcode delay for -n chars
	 *  case 2b - n arg > 0: n interpreted as next read op size "hint" ...
	 *   case 2b1 - chars waiting >= n: delay IRQ as long as possible
	 *   case 2b2 - chars waiting < n: delay to would-be completion point
	 */
	nspecials = xuu->nspecials;
	nbuf = (xuu->rfifo_put - xuu->rfifo_get) & 0x7ff;
	if (buf) x = xuu->rxcnt + remain + xuu->remain;
	else if (remain <= -256) x = xuu->rxirq + 1;
	else if (remain < 0) x = xuu->rxcnt - remain;
	else if (nbuf >= remain) x = xuu->rxcnt;
	else x = xuu->rxcnt + remain - nbuf;
	x = x - 1;
	if (x > (xuu->rxcnt + 0xff)) x = xuu->rxcnt - 1;
	x &= 0xff;
	if (xuu->rxirq != x) { /* did we adjust the RX IRQ? */
		
		xuu->rxirq = x;
		XUPOKE16(xu, uartn*2, xuu->txput | (xuu->rxirq << 8));
		x = (XUPEEK16(xu, RADR) & 0xfff) >> 1;
		/* If the RADR changed since we looked at it last, we have to
		 * start all over again since there is a chance we set the RX 
		 * IRQ to a point we already passed.
		 */
		if (put != x) {
			put = x;
			goto again;
		}
	}
	xu->rget = put;
	unlock(xu, uartn);
	xu->status |= signal;
	unglock(xu);
	if (xu->xu_maskirq) xu->xu_maskirq(xu->xu_osarg, s);

	if (xu->xu_signal && signal) for (i = 0; i < 8; i++) {
		if (signal & (1<<i)) xu->xu_signal(xu->xu_osarg, i);
	}

	if (buf) return n - remain;
	else return (nspecials << 16 | nbuf);
}


int xu_read(struct xuartcore *xu, int uartn, unsigned short *buf, int n) {
	int r, orign = n;
again:
	r = do_read(xu, uartn, buf, n);
	if (buf == NULL) return r & 0xffff;
	buf += r;
	n -= r;
	if (n && xu->xu_wait) {
		int z;
		z = xu->xu_wait(xu->xu_osarg, uartn);
		if (z != -1) goto again;
	}
		
	return orign - n;
}


int xu_readc(struct xuartcore *xu, int uartn, unsigned char *buf, int n) {
	unsigned short *sbuf;
	unsigned char *obuf = buf;
	struct xuart *xuu = &xu->xu[uartn];
	int r, i, shift, rmsk, abt = 0;
	unsigned short dum;

	if (buf == NULL) {
		int nspecials;
		int ret;
		ret = do_read(xu, uartn, NULL, n);
		nspecials = ret >> 16;
		ret = (ret & 0xffff) - nspecials;
		//if (ret == 0 && nspecials) ret = 1;
		return ret;
	}

	shift = 2 + (9 - xuu->nbits);
	rmsk = xuu->rmsk;

again:
	if (n == 1 || (n == 2 && ((unsigned)buf & 0x1))) {
		sbuf = &dum;
		xuu->remain = n - 1;
		r = xu_read(xu, uartn, sbuf, 1);
	} else {
		sbuf = (unsigned short *)(((unsigned)buf + 1) & ~0x1);
		xuu->remain = n - (n >> 1);
		r = xu_read(xu, uartn, sbuf, n >> 1);
	}

	for (i = 0; i < r; i++) {
		unsigned short d = sbuf[i];
		int t = (d >> 11) & 0x3;
		if (t == 0) *buf++ = (d >> shift) & rmsk;
		else if (t == 1 && xu->xu_rxbreak) {
			abt = xu->xu_rxbreak(xu->xu_osarg, uartn, xbits(d),
			  buf - obuf);
			n++;
		} else if (t == 2 && xu->xu_rxidle) {
			abt = xu->xu_rxidle(xu->xu_osarg, uartn, xbits(d),
			  buf - obuf);
			n++;
		} else n++;
	}
	n -= r;
	if (n && r && abt != -1) goto again;

	xuu->remain = 0;

	return buf - obuf;
}


int xu_stoptx(struct xuartcore *xu, int uartn, int stop) {
	int s = 0;

	if (!(xu->txrxstat & (1<<(uartn + 8)))) return -1; /* open? */

	if (xu->xu_maskirq) s = xu->xu_maskirq(xu->xu_osarg, 1);
	glock(xu);
	xu->txcfg &= ~(0x3 << (uartn * 2));
	if (stop == 0) {
		if (xu->hwcts & (1 << uartn))
		  xu->txcfg |= (0x3 << (uartn * 2));
		else
		  xu->txcfg |= (0x2 << (uartn * 2));
	}
	XUPOKE16(xu, TXCFG, xu->txcfg);
	unglock(xu);
	if (xu->xu_maskirq) xu->xu_maskirq(xu->xu_osarg, s);

	/* returns whether or not TX is active right now */
	return ((XUPEEK16(xu, TXCFG) >> (uartn * 2)) & 0x1);
}


int xu_txbreak(struct xuartcore *xu, int uartn, int brk) {
	int s = 0;
	if (!(xu->txrxstat & (1<<(uartn + 8)))) return -1; /* open? */

	if (!brk) {
		xu_stoptx(xu, uartn, 0);
		return 0;
	}

	if (xu->xu_maskirq) s = xu->xu_maskirq(xu->xu_osarg, 1);
	glock(xu);
	xu->txcfg &= ~(0x3 << (uartn * 2));
	xu->txcfg |= (0x1 << (uartn * 2));
	XUPOKE16(xu, TXCFG, xu->txcfg);
	unglock(xu);
	if (xu->xu_maskirq) xu->xu_maskirq(xu->xu_osarg, s);

	return 0;
}


int xu_irq(struct xuartcore *xu) {
	int put, get, i;
	int signal = 0;
	int overflow = 0;
	unsigned short *hwfifo;
	unsigned short txrxirq;
#ifdef XUDEBUG
	unsigned int nrx = 0;
	static unsigned int nirq = 0;
#endif

	txrxirq = XUPEEK16(xu, TXRXIRQ);	

#ifdef XUDEBUG
	if (txrxirq == 0) fprintf(logfile, "irq w/nothing to do\n");
#endif

	if (!(txrxirq & 0xff00)) goto tx;

	hwfifo = (unsigned short *)xu->xu_memstart;
	glock(xu);
	get = xu->rget;
	xu->rget = put = XUPEEK16(xu, RADR) >> 1;
	while (put != get) {
		struct xuart *xuu;
		unsigned short d = PEEK16(&hwfifo[get++]);
		i = (d >> 13) & 0x7;
		get &= 0x7ff;
		xuu = &xu->xu[i];
		xuu->rxcnt++;
		if ((d >> 11) & 0x3) xuu->nspecials++;
		xuu->rfifo[xuu->rfifo_put++] = d;
		xuu->rfifo_put &= 0x7ff;
		if (xuu->rfifo_put == xuu->rfifo_get) {
			int t = xuu->rfifo_get;
			d = xuu->rfifo[t];
			if ((d >> 11) & 0x3) xuu->nspecials--;
			t = (t + 1) & 0x7ff;
			xuu->rfifo_get = t;
		}
		signal |= (1 << i); /* RX activity */
#ifdef XUDEBUG
		nrx++;
#endif
	}
	xu->status |= signal;
	unglock(xu);
#ifdef XUDEBUG
	fprintf(logfile, "rx irq %d processed %d\n", nirq++, nrx); 
#endif

tx:
	if (!(txrxirq & 0xff)) goto end;
	txrxirq &= 0xff;
	glock(xu);
	for (i = 0; i < xu->nchan; i++, txrxirq >>= 1) {
		unsigned short x, y;
		struct xuart *xuu;
		int txspc;
		unsigned char txput, rxcnt;
		volatile unsigned short *txmem;
		unsigned short *txmem_shadow;

		if (!(txrxirq & 0x1) && !(xu->txrxstat & (1<<(i + 8))))
		  continue;
		xuu = &xu->xu[i];
		txmem = XUTXMEM(xu, i, 0);
		txmem_shadow = XUTXMEM_SHADOW(xu, i, 0);
		lock(xu, i);
		x = XUPEEK16(xu, i*2);
		rxcnt = x >> 8;
 		// if rxcnt is not what we expect, overflow happened
		if (xuu->rxcnt != rxcnt) {
			xuu->rxcnt = rxcnt;
			overflow = 1;
		}
		x &= 0xff;
		txput = xuu->txput;
		txspc = 0xff - ((txput - x) & 0xff);
		if (xuu->tirqdat && inc(txput, x, xuu->tirqpos)) 
		  xuu->tirqdat = 0;
		while (txspc && xuu->tfifo_put != xuu->tfifo_get) {
			unsigned short v = xuu->tfifo[xuu->tfifo_get++];
			txmem_shadow[txput] = v;
			POKE16(&txmem[txput], v);
			txput++;
			xuu->tfifo_get &= 0x7ff;
			txspc--;
		}
		if (txspc == 0) {
			// schedule TX IRQ accounting for IRQ latency
			x = (txput - xuu->txovr) & 0xff;
			y = txmem_shadow[x] | 0x8000;
			txmem_shadow[x] = y;	
			POKE16(&txmem[x], y);
			if (xuu->tirqdat && (x != xuu->tirqpos)) {
				unsigned short v = xuu->tirqdat & 0x7fff;
				txmem_shadow[xuu->tirqpos] = v;
			  	POKE16(&txmem[xuu->tirqpos], v);
			}
			xuu->tirqpos = x;
			xuu->tirqdat = y;
		}

		if (xuu->txspc != txspc) {
			xuu->txspc = txspc;
			signal |= (1 << (i + 8)); // TX activity
		}
		if (xuu->txput != txput) {
			xuu->txput = txput;
			XUPOKE16(xu, i*2, txput | (xuu->rxirq << 8));
			signal |= (1 << (i + 8)); // TX activity
			xu->status |= (1 << (i + 8));
		} 
		unlock(xu, i);
	}
	unglock(xu);
#ifdef XUDEBUG
	fprintf(logfile, "tx irq %d processed\n", nirq); 
#endif

end:
	if (signal && xu->xu_signal) for (i = 0; i < 16; i++) {
		if (signal & (1<<i)) xu->xu_signal(xu->xu_osarg, i);
	}

	if (overflow) return -1;
	return xu->status;
}


int xu_close(struct xuartcore *xu, int uartn) {
	int remain;

	if (xu->txrxstat & (1<<(uartn + 8))) {
		int s = 0;
		if (xu->xu_maskirq) s = xu->xu_maskirq(xu->xu_osarg, 1);
		glock(xu);
		xu->txrxstat &= ~(1<<(uartn + 8));
		/* immediately disable rx-- this also disables further writes */
		XUPOKE16(xu, TXRXSTAT, xu->txrxstat);
		unglock(xu);
		if (xu->xu_signal) xu->xu_signal(xu->xu_osarg, uartn);
		if (xu->xu_maskirq) xu->xu_maskirq(xu->xu_osarg, s);
	}

	/* return true only when all tx done */
	while ((remain = do_write(xu, uartn, NULL, 0)) != 0) {
		int r;
		r = -1;
		if (xu->xu_wait) r = xu->xu_wait(xu->xu_osarg, uartn + 8);
		if (r == -1) break;
	} 
	return remain;
	/* NOTE: you can still read from a closed uart if bytes remain */
}


int xu_draintx(struct xuartcore *xu, int uartn) {
	struct xuart *xuu = &xu->xu[uartn];
	int remain;
	volatile unsigned short *txmem;
	unsigned short *txmem_shadow;
	txmem = XUTXMEM(xu, uartn, 0);
	txmem_shadow = XUTXMEM_SHADOW(xu, uartn, 0);

	while ((remain = do_write(xu, uartn, NULL, 0)) != 0) {
		int r, x;
		int s = 0;
		if (xu->xu_maskirq) s = xu->xu_maskirq(xu->xu_osarg, 1);
		glock(xu);
		lock(xu, uartn);
		x = (xuu->txput - 1) & 0xff;
		xuu->txspc = 0; /* forces the next do_write() to check hw */
		txmem_shadow[x] = txmem_shadow[x] | 0x8000;
		POKE16(&txmem[x], txmem_shadow[x]);
		unlock(xu, uartn);
		unglock(xu);
		if (xu->xu_maskirq) xu->xu_maskirq(xu->xu_osarg, s);
		r = -1;
		if (xu->xu_wait) r = xu->xu_wait(xu->xu_osarg, uartn + 8);
		if (r == -1) break;
	} 
	/* is active right now? */
	remain += ((XUPEEK16(xu, TXCFG) >> (uartn * 2)) & 0x1);
	return remain;
}


int xu_open(struct xuartcore *xu, int uartn, char *mode, int baudrate) {
	struct xuart *xuu = &xu->xu[uartn];
	unsigned short baud, buf[3];
	int i, match, s = 0;
	char lmode[16];
	char *opts = NULL;
	static struct { 
		char *mode;
		unsigned short *xtbl;
		char nbits;
		char rmsk;
	} modes[] = {
		{"8n1", NULL, 8, 0xff},
		{"raw", NULL, 8, 0xff},
		{"8n2", xtbl_8n2, 9, 0xff},
		{"dmx", xtbl_8n2, 9, 0xff},
		{"8e1", xtbl_8e1, 9, 0xff},
		{"8o1", xtbl_8o1, 9, 0xff},
		{"8e2", xtbl_8e2, 10, 0xff},
		{"8o2", xtbl_8o2, 10, 0xff},
		{"7n1", NULL, 7, 0x7f},
		{"7n2", xtbl_7n2, 8, 0x7f},
		{"7e1", xtbl_7e1, 8, 0x7f},
		{"7o1", xtbl_7o1, 8, 0x7f},
		{"7e2", xtbl_7e2, 9, 0x7f},
		{"7o2", xtbl_7o2, 9, 0x7f},
		{"9n1", NULL, 9, 0xff},
	};

	if (xu->txrxstat & (1<<(uartn + 8))) return -1; /* already open? */

	for (i = 0; i < sizeof(lmode); i++) {
		if (mode[i] == ',') {
			lmode[i] = 0;
			opts = &mode[i + 1];
			break;
		} else lmode[i] = mode[i];
		if (mode[i] == 0) break;
	}
	
	for (i = 0, match = -1; i < sizeof(modes); i++) {
		if (xstrcasecmp(lmode, modes[i].mode) == 0) {
			match = i;
			break;
		}
	}	
	if (match == -1) return -1;

	/* DMX is always 250kbaud */
	if (xstrcasecmp(lmode, "dmx") == 0) baudrate = 250000;

	baud = 100000000 / (baudrate * 8) - 1;
	buf[0] = 0x1f00 | (baud >> 8);
	buf[1] = 0x1800 | (baud & 0xff); 
	if (xu->xu_maskirq) s = xu->xu_maskirq(xu->xu_osarg, 1);
	glock(xu);
	lock(xu, uartn);
	xuu->baudrate = baudrate;
	xuu->nbits = modes[match].nbits;
	buf[2] = 0x990c | (xuu->nbits - 7); // irq, 32 bit times idle threshold
	xuu->rmsk = modes[match].rmsk;
	xuu->xtbl = modes[match].xtbl;
	xuu->txovr = (IRQLAT / (1000000 / (baudrate / (xuu->nbits + 2)))) + 1;
	xuu->tirqdat = 0;
	xuu->tfifo_put = 0;
	xuu->tfifo_get = 0;
	xuu->remain = 0;
	if (((xu->txcfg >> (uartn*2 + 1)) & 0x1) == 0) {
		/* uart was suspended, cancel all pending tx */
		xuu->txput = XUPEEK16(xu, uartn*2) & 0xff;
		XUPOKE16(xu, uartn*2, xuu->txput|(xuu->rxirq<<8));
		xuu->txspc = 0xff;
	}
	xu->txcfg &= ~(0x3 << (uartn * 2));
	xu->txcfg |= (0x2 << (uartn * 2));
	xu->hwcts &= ~(1<<uartn); 
	XUPOKE16(xu, TXCFG, xu->txcfg); /* temporarily disable hwcts */
	if (opts) {
		char *x;
		if (xstrcasestr(opts, "hwcts")) {
			xu->hwcts |= (1<<uartn);
			xu->txcfg |= (0x1 << (uartn * 2));
		}
		if ((x = xstrcasestr(opts, "ithr="))) {
			int ithr;
			x += 5;
			ithr = *x - '0';
			if (ithr <= 0) ithr = 0;
			else if (ithr > 3) ithr = 3;
			buf[2] &= ~0xc;
			buf[2] |= (ithr << 2);
		}
	}
	unlock(xu, uartn);
	unglock(xu);
	if (xu->xu_maskirq) xu->xu_maskirq(xu->xu_osarg, s);
	do_write(xu, uartn, buf, 3);

	/* only continue once TX has completely stopped */
	while (do_write(xu, uartn, NULL, 0) != 0) {
		if (xu->xu_wait) xu->xu_wait(xu->xu_osarg, i + 8);
	} 
	XUPOKE16(xu, TXCFG, xu->txcfg); /* commit pending hwcts change */

	/* enable receiver */
	if (xu->xu_maskirq) s = xu->xu_maskirq(xu->xu_osarg, 1);
	glock(xu);
	xu->txrxstat |= (1<<(uartn + 8));
	XUPOKE16(xu, TXRXSTAT, xu->txrxstat);
	unglock(xu);
	if (xu->xu_maskirq) xu->xu_maskirq(xu->xu_osarg, s);

	return 0;
}
