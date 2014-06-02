/*
 *@Author Frederico Gonçalves [frederico.lopes.goncalves@gmail.com]
 *
 * This file contains code which will use João's interruption module to gather samples
 * collected from the geophone and send them across the network. It already handles byte
 * order.
 *
 * In the enventuallity that the fpga.c code changes the way it stores samples in the
 * buffer, function read_nsamples will also need to be changed. This function is located
 * in interruption/proc_entry.c
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/inet.h>
#include <linux/in.h>
#include <linux/net.h>
#include <linux/delay.h>

#include "miavita_packet.h"
#include "proc_entry.h"

/*Kernel threads need to have a name. This may seem stupid but names with spaces cause kernel panics in 2.6.24.*/
#define KTHREAD_NAME "miavita-sender"

/*This was a workarround. I was using the macro SLEEP_TIME_MS which describes the time interval which the thread reads and
 *send data as a constant. However, later I wanted to make this a module argument, so being as lazy as I can be, I've changed the macro
 *to expand to the argument variable.
 */
#define SLEEP_TIME_MS read_t

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Frederico Gonçalves, [frederico.lopes.goncalves@gmail.com]");
MODULE_DESCRIPTION("This module spawns a thread which reads the buffer exported by João ands sends samples accross the network.");

/*Module argument variables. All are initialized to their default values.*/
char* bind_ip = "127.0.0.1";
char* sink_ip = "127.0.0.1";
uint16_t sport = 57843;
uint16_t sink_port = 57844;
uint16_t node_id = 0; //This is 16bit long, but it may only have 8 bits.
uint32_t read_t = 500;

module_param(bind_ip, charp, 0000);
MODULE_PARM_DESC(bind_ip, "This is the ip which the kernel thread will bind to. Default is localhost.");

module_param(sink_ip, charp, 0000);
MODULE_PARM_DESC(sink_ip, "This is the sink ip. Default is localhost.");

module_param(sport, ushort, 0000);
MODULE_PARM_DESC(sport, "This is the UDP port which the sender thread will bind to. Default is 57843.");

module_param(sink_port, ushort, 0000);
MODULE_PARM_DESC(sink_port, "This is the sink UDP port. Default is 57844.");

module_param(node_id, ushort, 0000);
MODULE_PARM_DESC(node_id, "This is the identifier of the node running this thread. Defaults to 0.");

module_param(read_t, uint, 0000);
MODULE_PARM_DESC(read_t, "The sleep time for reading the buffer. In miliseconds");

/*Global structures*/
static struct sockaddr_in my_addr;
static struct task_struct * sender = NULL;
static struct socket * udp_socket = NULL;

/*Send the packet structure using the kernel structures for io vectors.*/
static void send_it(packet_t* pkt)
{
  struct msghdr msg;
  mm_segment_t oldfs;
  struct iovec iov;
  struct sockaddr_in addr;
  int status;

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = in_aton(sink_ip);
  addr.sin_port = htons(sink_port);

  /*The message name is just the memory address of destination address...*/
  msg.msg_name = &addr;
  msg.msg_namelen = sizeof(struct sockaddr_in);
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = NULL;
  msg.msg_controllen = 0;
  msg.msg_flags = MSG_NOSIGNAL;

  iov.iov_base = pkt; /* Put the data in the message */
  iov.iov_len = (__kernel_size_t) sizeof(*pkt);

  /*Remove this and you'll regret it. I'm serious, removing this can corrupt the files on the sd card.
   *It can even corrupt the Debian installation.
   */
  oldfs = get_fs();
  set_fs(KERNEL_DS);

  if((status = sock_sendmsg(udp_socket, &msg, (size_t) sizeof(*pkt))) < 0)
    {
      printk(KERN_EMERG "FAILED TO SEND MESSAGE THROUGH SOCKET. ERROR %d\n", -status);
    }
  set_fs(oldfs);
}

/**
 * @brief Creates a packet containing a single sample
 *
 * @param sample
 *
 * @return The created packet. It has to be freed by the caller of this function
 */
static packet_t * prepare_packet(const sample_t * sample, uint32_t seq, uint8_t node_id)
{
  packet_t* pkt;
  unsigned int sample_size;

  pkt = kmalloc(sizeof(packet_t), GFP_ATOMIC); //we may use vmalloc, or GFP_KERNEL....
  if(!pkt){
      printk(KERN_EMERG "%s:%d Failed to allocate packet.\n", __FILE__, __LINE__);
      return NULL;
  }

  //Erase the memory for the new packet
  memset(pkt, 0, sizeof(packet_t));

  // Let's copy the timestamp from the sample to the packet. While doing that we put it in big endian
  pkt->timestamp = cpu_to_be64(sample->timestamp);

  // Let's put the packet timestamp also in big endian
  pkt->seq = cpu_to_be32(sample->seq_number);

  // Copying the sender's node id. Note that as it is 1Byte it is not necessary to put it in big endian
  pkt->id = node_id;

  // Now let's copy the samples into the packet
  sample_size = sizeof(sample->data); // This should be (24b * 4) or in other words (1char * 12)
  memcpy(pkt->samples, sample->data, sample_size);

  return pkt;
}

/*
 * This is the kthread function. Although I didn't used it you can always pass it values through the data variable.
 */
static int main_loop(void* data)
{
  sample_t *samples;
  packet_t * pkt;
  uint32_t len, i = 0;
  static uint32_t seq = 0;

  if (sock_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &udp_socket) < 0)
    {
      printk(KERN_EMERG "Unable to create socket.\n");
      return 0;
    }

  memset(&my_addr, 0, sizeof(my_addr));
  my_addr.sin_family = AF_INET;
  my_addr.sin_addr.s_addr = in_aton(bind_ip);
  my_addr.sin_port = htons(sport);

  if(udp_socket->ops->bind(udp_socket, (struct sockaddr*) &my_addr, sizeof(struct sockaddr)) < 0)
    {
      printk(KERN_EMERG "Unable to bind socket to %s:%d.\n", bind_ip, sport);
      return 0;
    }

  /*
   * If you need debug, just compile the code with -D__DEBUG__
   */
#ifdef __DEBUG__
  printk(KERN_INFO "Bound to %s:%u\n", bind_ip, sport);
#endif

  while (1)
    {
      if (kthread_should_stop())
        {
#ifdef __DEBUG__
          printk(KERN_INFO "Stopping sender thread...\n");
#endif
          break;
        }

      len = read_nsamples(&samples);
      if(len > 0)
        {

#ifdef __DEBUG__
          printk(KERN_INFO "Read %d samples:\n", len);
#endif

          for(i = 0; i < len; i += 1)
            {
              pkt = prepare_packet(&(samples[i]), seq++, node_id);
              if(pkt == NULL) {
                  printk(KERN_EMERG "Was not able to prepare the packet to send\n");
                  continue;
              }
              send_it(pkt);
              kfree(pkt);
            }

          kfree(samples); // Never forget samples was allocated in read_nsamples and so has to be freed
        }

      //    schedule(); //This is similar to kill a fly with a bazooka, but it works.
      msleep(SLEEP_TIME_MS);
    }
  return 0;
}


int __init init_module(void)
{
  sender = kthread_run(main_loop, NULL, KTHREAD_NAME);
  if (IS_ERR(sender))
    {
      printk(KERN_EMERG "Unable to create sender thread.\n");
      kfree(sender);
      sender = NULL;
      return -ENOMEM;
    }

  printk(KERN_INFO "Kernel sender module (sender kthread) initialized\n");
  return 0;
}

void __exit cleanup_module(void) {
    if (sender)
      {
        kthread_stop(sender);
        sock_release(udp_socket);
      }
}
