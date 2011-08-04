#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include "sock.h"

#define NOSIG(expression)			\
  (__extension__				\
   ({ long int __result;			\
     do __result = (long int) (expression);	\
     while (__result == -1L && errno == EINTR);	\
     __result; }))

static const char *version = "$Id: sock.c,v 1.2 2010/04/05 18:48:48 michael Exp $";

inline int fileSetBlocking(int fd,int on) {
  return fcntl(fd, F_SETFL, on ? 0 : (FNDELAY | O_NONBLOCK)) != -1;
}

int create_server_socket(int port) {
  int			rc;       /* system calls return value storage  */
  int			s;        /* socket descriptor                  */
  int			cs;       /* new connection's socket descriptor */
  struct sockaddr_in	sa;       /* Internet address struct            */
  int x = 1;

  memset(&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);
  sa.sin_addr.s_addr = INADDR_ANY;
  
  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0) {
    //perror("socket: allocation failed");
    return -1;
  }
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &x, 4);
  setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &x, 4);
  if (!fileSetBlocking(s,0)) {
    printf("ERROR: Unable to set server socket non-blocking (%m)!\n");
    NOSIG(close(s));
    return -1;
  }
  rc = bind(s, (struct sockaddr *)&sa, sizeof(sa));
  if (rc) {
    //perror("bind");
    return -1;
  }
  rc = listen(s, 5);
  if (rc) {
    //perror("listen");
    return -1;
  }  
  return s;
}

SC *SC_tail(SC *conn) {
  while (conn->next) conn = conn->next;
  return conn;
}

SC *SC_append(SC *head,SC *tail) {
  SC *last_tail;

  if (head == 0) {
    return tail;
  }
  last_tail = SC_tail(head);
  last_tail->next = tail;
  tail->next = 0; // just to be sure
  return head;
}

SC *SC_delete(SC *head,SC *e) {
  SC *cur = head, *ret;

  if (e == head) {
    ret = e->next;
    free(e->buf);
    return ret;
  }
  while (cur->next != e) {
    cur = cur->next;
  }
  cur->next = e->next;
  free(e->buf);
  return head;
}

// bwrite: buffered write
// write as much data as possible from the buffer
int bwrite(int fd,char *buf,int len,int *written) {
  int rc=0;

  if (len - *written > 0) {
    rc = write(fd,buf+*written,len - *written);
  }
  if (rc > 0) {
    *written += rc;
    return 1;
  } else {
    *written = len; // pretend we sent the rest
  }
  return 0;
}

// bread: buffered read
// returns false if an error occured (or EOF)
int bread(int fd,char *buf,int *len,int *used) {
  int rc=0;

  if (*len - *used > 0) {
    rc = read(fd,buf+*used,*len-*used);
  }
  if (rc >= 0) {
    if (rc != 0) {
      //printf("%d:read %d\n",fd,rc);
    } else {
      return 0; // end of file
    }
    *used += rc;
    return 1;
  } else {
    //perror("read:");
    return 0;
  }
}

int run_multi_server(int s,multi_service svc,SC_new newSC,void *data,SC *init) {
  fd_set             rfd;           /* set of open sockets                */
  fd_set             c_rfd;         /* set of sockets waiting to be read  */
  struct sockaddr_in csa;           /* client's address struct            */
  unsigned           size_csa;      /* size of client's address struct    */
  int running = 1;
  int dmax = s+1;
  int i,cs,rc;
  SC *head = init, *cur;

  FD_ZERO(&rfd);
  if (head && head->wfd) FD_ZERO(head->wfd);
  FD_SET(s,&rfd);
  cur = init;
  while (cur) {
    FD_SET(cur->s, &rfd); 
    if (cur->s + 1 > dmax) {
      dmax = cur->s + 1;
    }
    cur = cur->next;
  }
  size_csa = sizeof(csa);
  while (running) {
    do {
      c_rfd = rfd;
      rc = NOSIG(select(dmax, &c_rfd, head ? head->wfd : NULL, NULL, NULL));
    } while (rc == -1 && errno == EINTR);
    assert(rc != -1);
    if (FD_ISSET(s, &c_rfd)) {
      cs = NOSIG(accept(s, (struct sockaddr *)&csa, &size_csa));
      if (cs < 0) {
	continue;
      }
      FD_SET(cs, &rfd);
      if (cs + 1 > dmax) {
	dmax = cs + 1;
      }
      head = SC_append(head,newSC(cs));
      continue;
    }
    cur = head;
    while (cur) {
      if (cur->s && FD_ISSET(cur->s, &c_rfd)) {
	rc = bread(cur->s,cur->buf,&cur->len,&cur->used);
	if (rc == 0) {
	  //fprintf(stderr,"closing %d\n",cur->s);
	  NOSIG(close(cur->s));
	  FD_CLR(cur->s, &rfd);
	  head = SC_delete(head, cur);
	} else {
	  if (cur->used >= cur->want) {
	    running = svc(cur,data,head);
	  }
	}
      }
      if (head && head->wfd && cur->s && FD_ISSET(cur->s, head->wfd)) {
	bwrite(cur->s,cur->wbuf,cur->wlen,&cur->wwr);
	if (cur->wwr == cur->wlen) {
	  FD_CLR(cur->s,head->wfd);
	}
      }
      cur = cur->next;
    }
  }
}

int run_server(int s,service svc,void *data) {
  unsigned size_csa; /* size of client's address struct    */
  struct sockaddr_in	csa;      /* client's address struct            */
  int cs,running = 1,n=0;

  while (running) {
    size_csa = sizeof(struct sockaddr_in);
    cs = NOSIG(accept(s, (struct sockaddr *)&csa, &size_csa));
    if (cs < 0) {
      continue;
    }
    n++;
    if (svc) {
      running = svc(cs,data);
    }
    NOSIG(close(cs));
  }
  return n;
}

int create_client_socket(char *host,int port) {
  int			rc;            /* system calls return value storage */
  int            	s;             /* socket descriptor */
  struct sockaddr_in	sa;            /* Internet address struct */
  struct hostent*     hen; 	       /* host-to-IP translation */

  hen = gethostbyname(host);
  if (!hen) {
    //perror("couldn't resolve host name");
    return -1;
  }

  memset(&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);
  memcpy(&sa.sin_addr.s_addr, hen->h_addr_list[0], hen->h_length);
  
  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0) {
    //perror("socket: allocation failed");
    return -1;
  }

  rc = connect(s, (struct sockaddr *)&sa, sizeof(sa));
  
  if (rc) {
    //perror("connect");
    return -1;
  }
  return s;
}
