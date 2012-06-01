/*
 * This file will spwan thread responsible for flushing buffers.
 * This will avoid problems such as flushing buffers only when they're full.
 *
 * Buffers are internally synchronized with spinlocks. Therefore they're interrupt and thread safe.
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/udp.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/net.h>
#include <linux/sched.h>
#include <linux/inet.h>

#include "new_ip_protocols.h"
#include "utils.h"
#include "consuela.h"
#include "byte_buffer.h"
#include "klist.h"

static struct task_struct * consuela_kthread = NULL;
static struct socket * raw_socket = NULL;
static klist *app_buffers = NULL, *net_buffers = NULL;
static __be32 source_ip;

//Declared as a module parameter in aggregation.c
extern int flush_timeout;
extern char* bind_ip;

//Called when aggregate packet is built
static void send_it(struct iphdr* ip) {
  struct msghdr msg;
  mm_segment_t oldfs;
  struct iovec iov;
  struct sockaddr_in addr;
  int status;

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = ip->daddr;

  msg.msg_name = &addr;
  msg.msg_namelen = sizeof(struct sockaddr_in);
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = NULL;
  msg.msg_controllen = 0;
  msg.msg_flags = MSG_NOSIGNAL;

  iov.iov_base = ip;
  iov.iov_len = (__kernel_size_t) ntohs(ip->tot_len);

  oldfs = get_fs();
  set_fs(KERNEL_DS);
  debug("Consuela is sending %d bytes.\n", ntohs(ip->tot_len));
  if((status = sock_sendmsg(raw_socket, &msg, (size_t) ntohs(ip->tot_len))) < 0){
    print_error(KERN_EMERG "FAILED TO SEND MESSAGE THROUGH SOCKET. ERROR %d\n", -status);
  }
  set_fs(oldfs);
}

void build_and_send_packets(klist* buffs, uint8_t proto){
  klist_iterator* it;
  aggregate_buffer* b;
  char* buffered = NULL, *packet = NULL;
  int len;
  struct iphdr iph;

  it = make_klist_iterator(buffs);
  while (klist_iterator_has_next(it)) {
    b = (aggregate_buffer*) (klist_iterator_next(it))->data;

    len = mv_data(&buffered, b);

    if(buffered){
      debug("Copied data from buffer\n");
      iph.ihl = 5;
      iph.version = 4;
      iph.tos = 0;
      iph.tot_len = htons(sizeof(struct iphdr) + len);
      iph.id = 0;
      iph.frag_off = 0;
      iph.ttl = 60;
      iph.protocol = proto;
      iph.check = 0;
      iph.saddr = source_ip;
      iph.daddr = b->ip;
      iph.check = csum((uint16_t*) &iph, (iph.ihl << 2) >> 1);
      
      packet = kmalloc(ntohs(iph.tot_len), GFP_ATOMIC);
      if(!packet){
	printk(KERN_EMERG "Could not allocate packet.\n");
	kfree(buffered);
	continue;
      }

      memcpy(packet, &iph, iph.ihl << 2);
      memcpy(packet + (iph.ihl << 2), buffered, len);

      send_it((struct iphdr*) packet);
	      
      kfree(buffered);
      buffered = NULL;
    }
  }
  free_klist_iterator(it);
}

static int main_loop(void* data) {
  char __user one = 1;
  char __user *val = &one;

  if (sock_create(AF_INET, SOCK_RAW, IPPROTO_UDP, &raw_socket) < 0) {
    print_error(KERN_EMERG "Unable to create socket.\n");
    return 0;
  }

  if (raw_socket->ops->setsockopt(raw_socket, IPPROTO_IP, IP_HDRINCL, val,
				  sizeof(val)) < 0) {
    print_error(KERN_EMERG "Unable to set socket option.\n");
    return 0;
  }

  while (1) {
    if (kthread_should_stop())
      break;
    
    build_and_send_packets(app_buffers, AGREGATED_APPLICATION_ENCAP_UDP_PROTO);
    build_and_send_packets(net_buffers, AGREGATED_IP_ENCAP_IP_PROTO);

    msleep(flush_timeout);
  }
  return 0;
}

void start_consuela(klist* application_buffers, klist* ip_buffers){
  consuela_kthread = kthread_run(main_loop, NULL, "consuela");
  if (IS_ERR(consuela_kthread)) {
    print_error("Unable to start consuela.\n");
    kfree(consuela_kthread);
    consuela_kthread = NULL;
  }

  app_buffers = application_buffers;
  net_buffers = ip_buffers;
  source_ip = in_aton(bind_ip);
  debug("Consuela is ready to clean!\n");
}

void stop_consuela(void){
  if(consuela_kthread){
    kthread_stop(consuela_kthread);
    kfree(consuela_kthread);
  }
}
