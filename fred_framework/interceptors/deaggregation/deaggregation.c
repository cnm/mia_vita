#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/inetdevice.h>
#include <linux/netfilter.h> 
#include <linux/netfilter_ipv4.h> 
#include <linux/skbuff.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include "klist.h"
#include "interceptor.h"
#include "miavita_packet.h"
#include "utils.h"
#include "injection_thread.h"
#include "new_ip_protocols.h"

#define INTERCEPTOR_NAME "deaggregation"
#define MAX_SCATTERS 27

interceptor_descriptor deagg_desc;

//Should be the maximum for mtu. upperupperbound 
struct iphdr* scatters[MAX_SCATTERS] = {0};

static unsigned int l3_deaggregate(struct sk_buff* skb) {
  struct iphdr* iph, *first;
  uint16_t agg_len, acc_len = 0;
  uint64_t ts, first_ts, ts_acc = 0;
  packet_t *pdu;
  uint8_t scatter_index, first_time = 1;
  struct udphdr *udp_helper;

  iph = ip_hdr(skb);

  memset(scatters, 0, sizeof(scatters));

  //Size of aggregated data
  agg_len = ntohs(iph->tot_len) - (iph->ihl << 2);

  //jump to first iphdr
  iph = (struct iphdr*) (((char*) iph) + (iph->ihl << 2));

  switch(iph->protocol){
  case AGREGATED_APPLICATION_ENCAP_UDP_PROTO:
  case IPPROTO_UDP:
    pdu = (packet_t*) (((char*) iph) + (iph->ihl << 2) + sizeof(struct udphdr));
    first_ts = cpu_to_be64(pdu->timestamp);
    break;
  default:
    printk(KERN_EMERG "%s: Encapsulated packet Ip protocol not supported.\n", __FUNCTION__);
    return NF_ACCEPT;
  }

  scatter_index = MAX_SCATTERS - 1;
  
  ts = first_ts;

  for (acc_len = 0, first = iph; acc_len < agg_len; scatter_index--) {
    debug("Deaggregating network aggregated packet.\n");
    acc_len += ntohs(first->tot_len);
    scatters[scatter_index] = first;
    switch(first->protocol){
    case AGREGATED_APPLICATION_ENCAP_UDP_PROTO:
    case IPPROTO_UDP:
      udp_helper = (struct udphdr*) (((char*) first) + (first->ihl << 2));
      udp_helper->check = 0;
      pdu = (packet_t*) (((char*) first) + (first->ihl << 2) + sizeof(struct udphdr));
      break;
    default:
      printk(KERN_EMERG "%s: Encapsulated packet Ip protocol not supported. Possibly there was an error in the aggregation process. Packet will be dropped.\n", __FUNCTION__);
      return NF_DROP;
    }
 
    pdu->timestamp = ts - ts_acc;
    if(!first_time)
      ts_acc += pdu->timestamp;
    else
      first_time = 0;
    pdu->timestamp = cpu_to_be64(pdu->timestamp);

    first = (struct iphdr*) (((char*) first) + ntohs(first->tot_len));
  }
  
  for(scatter_index++; scatter_index < MAX_SCATTERS; scatter_index++){
    inject_packet(scatters[scatter_index]);
  }

  kfree_skb(skb);
  return NF_STOLEN;
}

static unsigned int app_deagregate(struct sk_buff* skb) {
  struct iphdr* iph, *new;
  struct udphdr* udph;
  packet_t* first, *pdu;
  uint64_t ts, first_ts, ts_acc = 0;
  uint8_t scatter_index, first_time = 1;
  uint32_t number_of_packets = 0;

  iph = ip_hdr(skb);

  switch(iph->protocol){
  case AGREGATED_APPLICATION_ENCAP_UDP_PROTO:
    //First UDP
    udph = (struct udphdr*) (((char*) iph) + (iph->ihl << 2));
    
    first = (packet_t*) (((char*) udph) + sizeof(struct udphdr));

    memset(scatters, 0, sizeof(scatters));

    //Number of aggregated packets (always multiple of sizeof(udp) + sizeof(packet_t))
    debug("Computation of number of packets -> (%d - %d) / (%d + %d)\n", ntohs(iph->tot_len), iph->ihl << 2, sizeof(struct udphdr), sizeof(packet_t));
    number_of_packets = (ntohs(iph->tot_len) - (iph->ihl << 2)) / (sizeof(struct udphdr) + sizeof(packet_t));
    debug("Received an application aggregated packet, which should contain %d packets.\n", number_of_packets);

    //Get first ts
    first_ts = ts = be64_to_cpu(first->timestamp);

    scatter_index = MAX_SCATTERS - 1;
  
    //Iterate the aggregated packet and keep track of time stamp
    for (; number_of_packets; scatter_index--, number_of_packets--) {
      debug("Deaggregating UDP packet. Scatter index at %d.\n", scatter_index);

      //We need to invalidate udp checksum to avoid errors
      udph->check = 0;

      first->timestamp = ts - ts_acc;
      if(!first_time)
	ts_acc += pdu->timestamp;
      else
	first_time = 0;
      first->timestamp = cpu_to_be64(first->timestamp);
    
      //prepend iphdr to new
      new = kmalloc((iph->ihl << 2)  + ntohs(udph->len), GFP_ATOMIC);
      memcpy(new, iph, iph->ihl << 2);
      memcpy((((char*) new) + (new->ihl << 2)), udph, ntohs(udph->len));

      new->tot_len = htons((new->ihl << 2) + ntohs(udph->len));
      new->protocol = IPPROTO_UDP;
      new->check = 0;
      new->check = csum((uint16_t*) new, new->ihl << 1);

      scatters[scatter_index] = new;
      udph = (struct udphdr*) (((char*) udph) + ntohs(udph->len));
    }

    for(scatter_index++; scatter_index < MAX_SCATTERS; scatter_index++){
      inject_packet(scatters[scatter_index]);
      kfree(scatters[scatter_index]);
    }
    break;
  default:
    printk(KERN_EMERG "%s: Unable to deaggregate packet. Protocol IP not supported.\n", __FUNCTION__);
    return NF_ACCEPT;
  }

  kfree_skb(skb);
  return NF_STOLEN;
}

//Deaggregation
unsigned int deaggregation_local_in_hook(filter_specs* sp, unsigned int hooknum,
    struct sk_buff* skb, const struct net_device* in,
    const struct net_device *out, int(*okfn)(struct sk_buff*)) {
  struct iphdr* iph = ip_hdr(skb);

  if (!skb){
    debug("SKB is null.\n");
    return NF_ACCEPT;
  }

  if (!skb_network_header(skb)){
    debug("SKB does not have a network header.\n");
    return NF_ACCEPT;
  }

  switch(iph->protocol){
  case AGREGATED_APPLICATION_ENCAP_UDP_PROTO:
    debug("Deaggregating udp encapsulated in ip.\n");
    return app_deagregate(skb);
  case AGREGATED_IP_ENCAP_IP_PROTO:
    debug("Deaggregating ip encapsulated in ip.\n");
    return l3_deaggregate(skb);
  default:
    debug("Packet is already deaggregated.\n");
    break;
  }

  return NF_ACCEPT;
}

rule* create_rule_for_specs(filter_specs* sp) {
  rule* r;
  filter *local_in_filter = NULL;

  printk("Calling create rule from deaggregation module.\n");

  r = make_rule(1);
  if (!r)
    return NULL;

  local_in_filter = create_filter(FILTER_PRIO_FIRST, sp, r, FILTER_AT_LOCAL_IN, deaggregation_local_in_hook);
  (r->filters)[0] = local_in_filter;

  r->interceptor = &deagg_desc;
  return r;
}

int __init init_module() {
  if (!start_injection_thread()) {
    printk(KERN_EMERG "Could not create injection thread.\n");
    return -1;
  }

  deagg_desc.name = INTERCEPTOR_NAME;
  deagg_desc.create_rule_for_specs_cb = create_rule_for_specs;

  if (!register_interceptor(&deagg_desc)) {
    printk(KERN_EMERG "Failed to register deaggregation filter.\n");
    return -1;
  }

  printk(KERN_INFO "Deaggregation module loaded.\n");

  return 0;
}

void __exit cleanup_module() {
  stop_injection_thread();
  if (!unregister_interceptor(INTERCEPTOR_NAME))
    printk("Unable to unregister deaggregation interceptor. Interceptor framework will panic soon.\n");

  printk(KERN_INFO "Deaggregation module unloaded.\n");
}

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Frederico Gonçalves, [frederico.lopes.goncalves@gmail.com]");
MODULE_DESCRIPTION("Deaggregation interceptor.");
