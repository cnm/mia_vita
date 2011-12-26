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

/*
 * Define new protocol numbers, which are currently unassigned.
 */
#define AGREGATED_APPLICATION_ENCAP_UDP_PROTO 143
#define AGREGATED_IP_ENCAP_IP_PROTO 144

#define INTERCEPTOR_NAME "deaggregation"
#define MAX_SCATTERS 27

interceptor_descriptor deagg_desc;

//Should be the maximum for mtu. upperupperbound 
struct iphdr* scatters[MAX_SCATTERS] = {0};

static unsigned int l3_deaggregate(struct sk_buff* skb) {
  struct iphdr* iph, *first;
  uint16_t agg_len, acc_len = 0;
  uint64_t ts, first_ts, ts_acc = 0, in_time;
  packet_t *pdu;
  uint8_t scatter_index, first_time = 1;

  iph = ip_hdr(skb);

  memset(scatters, 0, sizeof(scatters));

  //Size of aggregated data
  agg_len = ntohs(iph->tot_len) - (iph->ihl << 2) - sizeof(struct udphdr)
      - sizeof(control_byte);
  //jump to first iphdr
  iph = (struct iphdr*) (((char*) iph) + (iph->ihl << 2) + sizeof(struct udphdr)
      + sizeof(control_byte));
  pdu = (__tp(pdu)*) (((char*) iph) + (iph->ihl << 2) + sizeof(struct udphdr)
      + sizeof(control_byte));
  swap_pdu_byte_order(pdu);
  first_ts = ts = pdu->timestamp;
  swap_pdu_byte_order(pdu);

  scatter_index = MAX_SCATTERS - 1;
  
  ts = first_ts;

  for (acc_len = 0, first = iph; acc_len < agg_len; scatter_index--) {
    acc_len += ntohs(first->tot_len);
    scatters[scatter_index] = first;
    pdu = (__tp(pdu)*) (((char*) first) + (first->ihl << 2)
         + sizeof(struct udphdr) + sizeof(control_byte));
    cb = (control_byte*) (((char*) first) + (first->ihl << 2)
          + sizeof(struct udphdr));
    cb->synched = 1;
    swap_pdu_byte_order(pdu);
    pdu->timestamp = ts - ts_acc;
    if(!first_time)
      ts_acc += pdu->timestamp;
    else
      first_time = 0;
    swap_pdu_byte_order(pdu);
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
  __tp(pdu)* first, *pdu;
  control_byte* cb;
  uint32_t agg_len, acc_len = 0;
  uint64_t ts, first_ts, ts_acc = 0, in_time;
  __be32 preserve_saddr, preserve_daddr;
  __be16 preserve_sport, preserve_dport;
  uint8_t scatter_index, first_time = 1;

  iph = ip_hdr(skb);
  preserve_daddr = iph->daddr;
  preserve_saddr = iph->saddr;

  udph = (struct udphdr*) (((char*) iph) + (iph->ihl << 2));
  preserve_dport = udph->dest;
  preserve_sport = udph->source;

  first = pdu_skb(skb);
  if (!first)
    return NF_ACCEPT;

  memset(scatters, 0, sizeof(scatters));

  //Size of aggregated data
  agg_len = ntohs(iph->tot_len) - (iph->ihl << 2) - sizeof(struct udphdr)
      - sizeof(control_byte);
  //jump to first pdu
  pdu = first;
  swap_pdu_byte_order(first);
  first_ts = ts = first->timestamp;
  swap_pdu_byte_order(first);

  scatter_index = MAX_SCATTERS - 1;
  
  ts = first_ts;
  //Iterate the aggregate packet and keep track of time stamp
  for (acc_len = 0, first = pdu; acc_len < agg_len; scatter_index--) {
    acc_len += n_pdu_len(first);
    swap_pdu_byte_order(first);
    first->timestamp = ts - ts_acc;
    if(!first_time)
      ts_acc += pdu->timestamp;
    else
      first_time = 0;
    swap_pdu_byte_order(first);
    
    //prepend iphdr, udphdr and control byte to new
    new = kmalloc(
        sizeof(struct iphdr) + sizeof(struct udphdr) + sizeof(control_byte)
            + n_pdu_len(first), GFP_ATOMIC);
    iph = new;
    udph = (struct udphdr*) (((char*) new) + sizeof(struct iphdr));
    cb = (control_byte*) (((char*) new) + sizeof(struct iphdr)
        + sizeof(struct udphdr));
    memcpy(
        (((char*) new) + sizeof(struct iphdr) + sizeof(struct udphdr)
            + sizeof(control_byte)), first, n_pdu_len(first));

    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = htons(
        sizeof(struct iphdr) + sizeof(struct udphdr) + sizeof(control_byte)
            + n_pdu_len(first));
    iph->id = 0;
    iph->frag_off = 0;
    iph->ttl = 60;
    iph->protocol = IPPROTO_UDP;
    iph->check = 0;
    iph->saddr = preserve_saddr;
    iph->daddr = preserve_daddr;
    iph->check = csum((uint16_t*) iph, (iph->ihl << 2) >> 1);

    udph->len = htons(
        sizeof(control_byte) + n_pdu_len(first) + sizeof(struct udphdr));
    udph->dest = preserve_dport;
    udph->source = preserve_sport;

    udph->check = 0;

    memset(cb, 0, sizeof(control_byte));
    cb->synched = 1;
    cb->deaggregated = 1;
    scatters[scatter_index] = new;
    first = (__tp(pdu)*) (((char*) first) + n_pdu_len(first));
  }

  for(scatter_index++; scatter_index < MAX_SCATTERS; scatter_index++){
    inject_packet(scatters[scatter_index]);
    kfree(scatters[scatter_index]);
  }

  kfree_skb(skb);
  return NF_STOLEN;
}

//Deaggregation
unsigned int deaggregation_local_in_hook(filter_specs* sp, unsigned int hooknum,
    struct sk_buff* skb, const struct net_device* in,
    const struct net_device *out, int(*okfn)(struct sk_buff*)) {
  control_byte* cb;

  if (!skb)
    return NF_ACCEPT;

  if (!skb_network_header(skb))
    return NF_ACCEPT;

  cb = control_byte_skb(skb);
  if (!cb)
    return NF_ACCEPT;

  if (cb->deaggregated) {
    return NF_ACCEPT;
  }

  if (cb->l3_aggregated)
    return l3_deaggregate(skb);

  if (cb->app_aggregated)
    return app_deagregate(skb);

  return NF_ACCEPT;
}

rule* create_rule_for_specs(filter_specs* sp) {
  rule* r;
  filter *local_in_filter = NULL;

  printk("Calling create rule from deaggregation module.\n");

  r = make_rule(1);
  if (!r)
    return NULL;

  local_in_filter = create_filter(FILTER_PRIO_FIRST, sp, r, FILTER_AT_LOCAL_IN,
      deaggregation_local_in_hook);
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
		printk(
				"Unable to unregister deaggregation interceptor. Interceptor framework will panic soon.\n");

printk (KERN_INFO "Deaggregation module unloaded.\n");
}

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Frederico Gonçalves, [frederico.lopes.goncalves@gmail.com]");
MODULE_DESCRIPTION("Deaggregation interceptor.");
