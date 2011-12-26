#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/inetdevice.h>
#include <linux/netfilter.h> 
#include <linux/netfilter_ipv4.h> 
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/in.h>
#include "klist.h"
#include "interceptor.h"
#include "miavita_packet.h"
#include "utils.h"
#include "byte_buffer.h"

/*
 * Define new protocol numbers, which are currently unassigned.
 */
#define AGREGATED_APPLICATION_ENCAP_UDP_PROTO 143
#define AGREGATED_IP_ENCAP_IP_PROTO 144

static unsigned short aggregate_app_packets = 0;
module_param(aggregate_app_packets, ushort, 0000);
MODULE_PARM_DESC(aggregate_app_packets,
    "Aggregate packets generated by the same application. Default is no.");

static unsigned short aggregate_ip_packets = 1;
module_param(aggregate_ip_packets, ushort, 0000);
MODULE_PARM_DESC(aggregate_ip_packets, "Aggregate ip packets. Default is yes.");
static unsigned short aggregate_app_packets_buffer_size = 256;
module_param(aggregate_app_packets_buffer_size, ushort, 0000);
MODULE_PARM_DESC(
    aggregate_app_packets_buffer_size,
    "Size in bytes of the buffer used to store application generated packets. In other words, how many bytes should be buffered before sending the aggregate packet. Default is 256 Bytes.");

static unsigned short aggregate_ip_buffer_size = 512;
module_param(aggregate_ip_buffer_size, ushort, 0000);
MODULE_PARM_DESC(
    aggregate_ip_buffer_size,
    "Size in bytes of the buffer used to store ip packets. In other words, how many bytes should be buffered before sending the aggregate packet. Default is 512 Bytes.");

static int flush_timeout = 1000; //ms
module_param(flush_timeout, int, 0000);
MODULE_PARM_DESC(
    flush_timeout,
    "Time out in milliseconds to flush aggregated buffers. Default is 1 second.");

#define INTERCEPTOR_NAME "aggregation"

interceptor_descriptor agg_desc;

klist *app_buffers = NULL, *ip_buffers = NULL;

static void flush_buffer_app(struct sk_buff* skb, aggregate_buffer* b,
    const struct net_device* out) {
  struct iphdr* iph;
  struct in_device* ipa;
  char *begin_of_buffered_data;
  uint16_t len;
  
  iph = ip_hdr(skb);
  len = ntohs(iph->tot_len);

  //Remove iphdr
  skb_pull(skb, (iph->ihl << 2));

  //put into skb buffered data
  if (skb_tailroom(skb) < buffer_data_len(b))
    if (pskb_expand_head(skb, 0, buffer_data_len(b) - skb_tailroom(skb), GFP_ATOMIC)) {
      printk(KERN_EMERG	"%s in %s:%d: Failed to expand skb tail. Reseting aggregate buffer anyway\n",
	     __FUNCTION__, __FILE__, __LINE__);
      reset_buffer(b);
      return;
    }
  begin_of_buffered_data = skb_put(skb, buffer_data_len(b));
  memcpy(begin_of_buffered_data, b->head, buffer_data_len(b));
  //--------------------------

  ipa = (struct in_device*) out->ip_ptr;

  if (skb_headroom(skb) < sizeof(struct iphdr))
    if (pskb_expand_head(skb, sizeof(struct iphdr) - skb_headroom(skb), 0, GFP_ATOMIC)) {
      printk (KERN_EMERG "%s in %s:%d: Failed to expand skb head. Reseting aggregate buffer anyway\n",
	      __FUNCTION__, __FILE__, __LINE__);
      reset_buffer(b);
      return;
    }

  iph = (struct iphdr*) skb_push(skb, sizeof(struct iphdr));

  iph->ihl = 5;
  iph->version = 4;
  iph->tos = 0;
  iph->tot_len = htons(sizeof(struct iphdr) + buffer_data_len(b) + len);
  iph->id = 0;
  iph->frag_off = 0;
  iph->ttl = 60;
  iph->protocol = AGREGATED_APPLICATION_ENCAP_UDP_PROTO;
  iph->check = 0;
  iph->saddr = ipa->ifa_list->ifa_address;
  iph->daddr = b->ip;
  iph->check = csum((uint16_t*) iph, (iph->ihl << 2) >> 1);
  skb_set_network_header(skb, 0);
  skb_set_transport_header(skb, sizeof(struct iphdr));

  reset_buffer(b);
  return;
}

static void flush_buffer_ip(struct sk_buff* skb, aggregate_buffer* b,
    const struct net_device* out) {
  struct iphdr* iph;
  struct in_device* ipa;
  char *begin_of_buffered_data;
  uint16_t len;
  
  iph = ip_hdr(skb);
  len = ntohs(iph->tot_len);

  //Remove iphdr
  skb_pull(skb, (iph->ihl << 2));

  //put into skb buffered data
  if (skb_tailroom(skb) < buffer_data_len(b))
    if (pskb_expand_head(skb, 0, buffer_data_len(b) - skb_tailroom(skb), GFP_ATOMIC)) {
      printk(KERN_EMERG	"%s in %s:%d: Failed to expand skb tail. Reseting aggregate buffer anyway\n",
	     __FUNCTION__, __FILE__, __LINE__);
      reset_buffer(b);
      return;
    }
  begin_of_buffered_data = skb_put(skb, buffer_data_len(b));
  memcpy(begin_of_buffered_data, b->head, buffer_data_len(b));
  //--------------------------

  ipa = (struct in_device*) out->ip_ptr;

  if (skb_headroom(skb) < sizeof(struct iphdr))
    if (pskb_expand_head(skb, sizeof(struct iphdr) - skb_headroom(skb), 0, GFP_ATOMIC)) {
      printk (KERN_EMERG "%s in %s:%d: Failed to expand skb head. Reseting aggregate buffer anyway\n",
	      __FUNCTION__, __FILE__, __LINE__);
      reset_buffer(b);
      return;
    }

  iph = (struct iphdr*) skb_push(skb, sizeof(struct iphdr));

  iph->ihl = 5;
  iph->version = 4;
  iph->tos = 0;
  iph->tot_len = htons(sizeof(struct iphdr) + buffer_data_len(b) + len);
  iph->id = 0;
  iph->frag_off = 0;
  iph->ttl = 60;
  iph->protocol = AGREGATED_IP_ENCAP_IP_PROTO;
  iph->check = 0;
  iph->saddr = ipa->ifa_list->ifa_address;
  iph->daddr = b->ip;
  iph->check = csum((uint16_t*) iph, (iph->ihl << 2) >> 1);
  skb_set_network_header(skb, 0);
  skb_set_transport_header(skb, sizeof(struct iphdr));

  reset_buffer(b);
  return;
}

/*
 * This function is always called from local out hook, so there is no danger of having IP aggregated packets, or even application aggregated packets.
 */
static unsigned int __push_app_packet(aggregate_buffer* b, struct sk_buff* skb,
    const struct net_device* out) {
  struct iphdr* iph = ip_hdr(skb);
  char* to_push;
  packet_t* pdu = NULL, *push_packet = NULL;
  uint16_t len;

  switch(iph->protocol){
  case IPPROTO_UDP:
    push_packet = (packet_t*) (((char*) iph) + (iph->ihl << 2) + sizeof(struct udphdr));
    break;
  default:
    printk(KERN_EMERG "%s: Unable to aggregate packet. Ip protocol not supported.\n", __FUNCTION__);
    return NF_ACCEPT;
  }

  to_push = (((char*) iph) + (iph->ihl << 2));
  len = ntohs(iph->tot_len) - (iph->ihl << 2);

  debug("Pushing %d bytes to buffer\n", len);

  pdu = (packet_t*) peek_packet(b);
  if (pdu) 
    pdu->timestamp = cpu_to_be64(be64_to_cpu(push_packet->timestamp) - be64_to_cpu(pdu->timestamp));

  if (buffer_data_len(b) + len >= buffer_len(b)) {
    flush_buffer_app(skb, b, out);
    return NF_ACCEPT;
  }

  if (push_bytes(b, to_push, len)) {
    panic(
        "Called push_bytes on a buffer that has no space for them. Please verify this before asking to push bytes.");
  }

  kfree_skb(skb);
  return NF_STOLEN;
}

static unsigned int __push_ip_packet(aggregate_buffer* b, struct sk_buff* skb,
    const struct net_device* out) {
  struct iphdr* iph = ip_hdr(skb), *seeker;
  uint16_t tot_len = ntohs(iph->tot_len);
  packet_t* pdu = NULL, *to_push = NULL;

  //Timestamp previous packet relative to the new one
  seeker = (struct iphdr *) peek_packet(b);
  if(seeker){
    switch(seeker->protocol){
      /*These cases are the same*/
    case AGREGATED_APPLICATION_ENCAP_UDP_PROTO:
    case IPPROTO_UDP:
      pdu = (packet_t*) (((char*) seeker) + (seeker->ihl << 2) + sizeof(struct udphdr));
      break;
    default:
      printk(KERN_EMERG "%s: Ip protocol not supported. Error occurred while timestamping a previous packet.", __FUNCTION__);
      return NF_ACCEPT;
    }
  }

  //Get new packet to be pushed
  switch(iph->protocol){
    /*These cases are the same*/
  case AGREGATED_APPLICATION_ENCAP_UDP_PROTO:
  case IPPROTO_UDP:
    to_push = (packet_t*) (((char*) iph) + (iph->ihl << 2) + sizeof(struct udphdr));
    break;
  case AGREGATED_IP_ENCAP_IP_PROTO:
    debug("%s: Packet already aggregated at network level.\n", __FUNCTION__);
    return NF_ACCEPT;
  default:
    printk(KERN_EMERG "%s: Ip protocol not supported. Error occurred while timestamping a previous packet.", __FUNCTION__);
    return NF_ACCEPT;
  }

  if(pdu && to_push)
    pdu->timestamp = cpu_to_be64(be64_to_cpu(to_push->timestamp) - be64_to_cpu(pdu->timestamp));

  if (buffer_data_len(b) + tot_len >= buffer_len(b)) {
    flush_buffer_ip(skb, b, out);
    return NF_ACCEPT;
  }

  if (push_bytes(b, (char*) iph, tot_len)) {
    panic(
        "Called push_bytes on a buffer that has no space for them. Please verify this before asking to push bytes.");
  }
  kfree_skb(skb);
  return NF_STOLEN;
}

static void init_buffers(void) {
  if (aggregate_app_packets) {
    app_buffers = make_klist();
  }
  if (aggregate_ip_packets) {
    ip_buffers = make_klist();
  }
}

static void __free_buffer(void* data) {
  free_buffer((aggregate_buffer*) data);
}

static void free_buffers(void) {
  if (aggregate_app_packets) {
    free_klist(app_buffers, __free_buffer);
  }
  if (aggregate_ip_packets) {
    free_klist(ip_buffers, __free_buffer);
  }
}

static aggregate_buffer* get_app_buffer_for(__be32 ip) {
  aggregate_buffer* ab = NULL;
  klist_iterator* kit = make_klist_iterator(app_buffers);
  while (klist_iterator_has_next(kit)) {
    ab = (aggregate_buffer*) (klist_iterator_next(kit))->data;
    if (ab->ip == ip) {
      free_klist_iterator(kit);
      return ab;
    }
  }
  free_klist_iterator(kit);
  ab = create_aggregate_buffer(aggregate_app_packets_buffer_size, ip);
  add_klist_node_to_klist(app_buffers, make_klist_node(ab));
  return ab;
}

static aggregate_buffer* get_ip_buffer_for(__be32 ip) {
  aggregate_buffer* ab = NULL;
  klist_iterator* kit = make_klist_iterator(ip_buffers);
  while (klist_iterator_has_next(kit)) {
    ab = (aggregate_buffer*) (klist_iterator_next(kit))->data;
    if (ab->ip == ip) {
      free_klist_iterator(kit);
      return ab;
    }
  }
  free_klist_iterator(kit);
  ab = create_aggregate_buffer(aggregate_ip_buffer_size, ip);
  add_klist_node_to_klist(ip_buffers, make_klist_node(ab));
  return ab;
}

unsigned int aggregation_post_routing_hook(filter_specs* sp,
    unsigned int hooknum, struct sk_buff* skb, const struct net_device* in,
    const struct net_device *out, int(*okfn)(struct sk_buff*)) {
  struct iphdr* iph;
  struct udphdr* udph;
  aggregate_buffer* ab;

  if (!skb)
    return NF_ACCEPT;

  if (!skb_network_header(skb))
    return NF_ACCEPT;

  iph = ip_hdr(skb);
  switch (iph->protocol) {
  case IPPROTO_UDP:
    udph = (struct udphdr*) (((char*) iph) + (iph->ihl << 2));
    break;
  default:
    printk("Aggregation hook does not support ip protocol.\n");
    return NF_ACCEPT;
  }

  ab = get_ip_buffer_for(iph->daddr);
  if (!ab)
    return NF_ACCEPT;
  return __push_ip_packet(ab, skb, out);
}

unsigned int aggregation_local_out_hook(filter_specs* sp, unsigned int hooknum,
    struct sk_buff* skb, const struct net_device* in,
    const struct net_device *out, int(*okfn)(struct sk_buff*)) {
  struct iphdr* iph;
  struct udphdr* udph;
  aggregate_buffer* ab;

  if (!skb)
    return NF_ACCEPT;

  if (!skb_network_header(skb))
    return NF_ACCEPT;

  iph = ip_hdr(skb);
  switch (iph->protocol) {
  case IPPROTO_UDP:
    udph = (struct udphdr*) (((char*) iph) + (iph->ihl << 2));
    break;
  default:
    printk("Aggregation hook does not support ip protocol.\n");
    return NF_ACCEPT;
  }

  ab = get_app_buffer_for(iph->daddr);
  if (!ab)
    return NF_ACCEPT;
  return __push_app_packet(ab, skb, out);
}

rule* create_rule_for_specs(filter_specs* sp) {
  rule* r;
  int32_t nfilters = 0;
  filter *post_routing_filter = NULL, *local_out_filter = NULL;

  if (aggregate_app_packets)
    nfilters++;
  if (aggregate_ip_packets)
    nfilters++;

  //Sanity check!!!
  if (!nfilters) {
    printk(KERN_EMERG "Trying to register aggregation rule with no filters.\n");
    return NULL;
  }

  r = make_rule(nfilters);
  if (!r)
    return NULL;

  if (aggregate_app_packets) {
    local_out_filter = create_filter(FILTER_PRIO_FIRST, sp, r,
        FILTER_AT_LOCAL_OUT, aggregation_local_out_hook);
  }
  if (aggregate_ip_packets) {
    post_routing_filter = create_filter(FILTER_PRIO_FIRST, sp, r,
        FILTER_AT_POST_RT, aggregation_post_routing_hook);
  }

  if (aggregate_app_packets && aggregate_ip_packets) {
    (r->filters)[0] = post_routing_filter;
    (r->filters)[1] = local_out_filter;
  } else {
    if (aggregate_app_packets) {
      (r->filters)[0] = local_out_filter;
    }
    if (aggregate_ip_packets) {
      (r->filters)[0] = post_routing_filter;
    }
  }

  r->interceptor = &agg_desc;
  return r;
}

int __init init_module() {
  init_buffers();

  agg_desc.name = INTERCEPTOR_NAME;
  agg_desc.create_rule_for_specs_cb = create_rule_for_specs;

  register_interceptor(&agg_desc);

  printk(KERN_INFO "Aggregation module loaded.\n");

  return 0;
}

void __exit cleanup_module() {
  free_buffers();
  if (!unregister_interceptor(INTERCEPTOR_NAME))
    printk(
        "Unable to unregister aggregation interceptor. Interceptor framework will panic soon.\n");

  printk (KERN_INFO "Aggregation module unloaded.\n");
}

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Frederico Gonçalves, [frederico.lopes.goncalves@gmail.com]");
MODULE_DESCRIPTION("Aggregation interceptor.");
