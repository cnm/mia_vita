/* previous update: 2009-02-03

  Single Connection Server
  =================================
  int socket = create_server_socket(PORT);
  run_server(socket,service_function,data);

  The "service_function" parameter is a pointer to a function to call
  when something connects to the port.  This function should process
  data on the port until it closes, after which it should return TRUE
  if the server should continue running; otherwise FALSE.

  data is a pointer that will be passed to all calls to the service
  function.  an example use is to store global data for all instance to
  share (for instance, pointers to memory mapper hardware).

  When service function is called, the socket and the data pointer is passed.

  Multi Connection Server
  =================================
  int socket = create_server_socket(PORT);
  run_multi_server(socket,multi_service_function,new_conn_function,data);

  The multi-connection model keeps a structure (SC) for each connection.
  In this connection is a pointer to instance data for the application,
  as well as a read buffer and the minimum number of bytes the application
  requires to be ready before it gets called to process the data.   
  The "new_conn_function" provided by the application is responsible for 
  initializing the SC structure.

  The list of connections are maintained as a linked list within this
  structure.

  The multi service function is called with a pointer to its own
  connection structure, the "data" pointer passed on initialization,
  and a pointer to the head of the connection list.  An example of
  how this is used is to let a connection "block" on an event triggered
  by another connection.  The server returns from the service call with 
  a flag set its data structure which other service calls check and if
  they have caused the event they generate the required reply.

  Typically the "used" field is set to 0 to indicate all the data was
  consumed.
 */
#ifndef __SOCK_H
#define __SOCK_H
#include <stdio.h>		/* Basic I/O routines          */
#include <sys/types.h>		/* standard system types       */
#include <netinet/in.h>		/* Internet address structures */
#include <sys/socket.h>		/* socket interface functions  */
#include <netdb.h>		/* host to IP resolution       */
#include <string.h>
#include <stdlib.h>

typedef int (*service)(int,void *);
int create_server_socket(int port);
int run_server(int s,service svc,void *);
int create_client_socket(char *host,int port);

typedef struct server_connection {
  int s; // server descriptor
  char *buf; // must be malloced
  int len,  // total size of buf
    used,   // current number of bytes in buf
    want;   // call service fcn when used >= want
  void *inst_data;
  char *wbuf;
  int wlen,wwr;
  fd_set *wfd;
  struct server_connection *next; // for server use only
} SC;
typedef SC *(*SC_new)(int);
typedef int (*multi_service)(SC *,void *,SC *);
int run_multi_server(int s,multi_service svc,SC_new newSC,void *data,SC *);
int bwrite(int fd,char *buf,int len,int *written);
#endif
