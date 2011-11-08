#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/inet.h>
#include <linux/in.h>
#include <linux/net.h>

#include "miavita_packet.h"
#include "proc_entry.h"

#define KTHREAD_NAME "miavita-sender"

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Frederico Gonçalves, [frederico.lopes.goncalves@gmail.com]");
MODULE_DESCRIPTION("This module spawns a thread which reads the buffer exported by João ands sends samples accross the network.");

char* bind_ip = "127.0.0.1";
char* sink_ip = "127.0.0.1";
uint16_t sport = 57843;
uint16_t sink_port = 57843;

module_param(bind_ip, charp, 0000);
MODULE_PARM_DESC(bind_ip, "This is the ip which the kernel thread will bind to. Default is localhost.");

module_param(sink_ip, charp, 0000);
MODULE_PARM_DESC(sink_ip, "This is the sink ip. Default is localhost.");

module_param(sport, ushort, 0000);
MODULE_PARM_DESC(sport, "This is the UDP port which the sender thread will bind to. Default is 57843.");

module_param(sink_port, ushort, 0000);
MODULE_PARM_DESC(sink_port, "This is the sink UDP port. Default is 57843.");

static struct sockaddr_in my_addr;
static struct task_struct * sender = NULL;
static struct socket * udp_socket = NULL;

static void send_it(packet_t* pkt) {
  struct msghdr msg;
  mm_segment_t oldfs;
  struct iovec iov;
  struct sockaddr_in addr;
  int status;
  
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = in_aton(sink_ip);
  addr.sin_port = htons(sink_port);

  msg.msg_name = &addr;
  msg.msg_namelen = sizeof(struct sockaddr_in);
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = NULL;
  msg.msg_controllen = 0;
  msg.msg_flags = MSG_NOSIGNAL;

  iov.iov_base = pkt;
  iov.iov_len = (__kernel_size_t) sizeof(*pkt);

  oldfs = get_fs();
  set_fs(KERNEL_DS);
  if((status = sock_sendmsg(udp_socket, &msg, (size_t) sizeof(*pkt))) < 0){
    printk(KERN_EMERG "FAILED TO SEND MESSAGE THROUGH SOCKET. ERROR %d\n", -status);
  }
  set_fs(oldfs);
}

static int main_loop(void* data) {
  uint8_t samples[12];
  uint32_t offset = 0;

  if (sock_create(AF_INET, SOCK_RAW, IPPROTO_UDP, &udp_socket) < 0) {
    printk(KERN_EMERG "Unable to create socket.\n");
    return 0;
  }

  memset(&my_addr, 0, sizeof(my_addr));
  my_addr.sin_family = AF_INET;
  my_addr.sin_addr.s_addr = in_aton(bind_ip);
  my_addr.sin_port = htons(sport);

  if(udp_socket->ops->bind(udp_socket, (struct sockaddr*) &my_addr, sizeof(struct sockaddr)) < 0){
    printk(KERN_EMERG "Unable to bind socket to %s:%d.\n", bind_ip, sport);
    return 0;
  }

  while (1) {
    if (kthread_should_stop())
      break;
    if(read_4samples(samples, &offset)){
      //TODO: construir o pacote e enviar
      //      send_it(packet);
    }
 
    schedule(); //We need to let others do stuff!!
  }
  return 0;
}

int __init init_module(void) {
  sender = kthread_run(main_loop, NULL, KTHREAD_NAME);
  if (IS_ERR(sender)) {
    printk(KERN_EMERG "Unable to create sender thread.\n");
    kfree(sender);
    sender = NULL;
    return -ENOMEM;
  }
  return 0;
}

void __exit cleanup_module(void) {
  if (sender) {
    kthread_stop(sender);
  }
}
