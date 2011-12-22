/*
 *@author: Frederico Gonçalves
 */

#ifdef CONFIG_SYNCH_ADHOC

#include <linux/slab.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include "proc_filters.h"
#include "filter_chains.h"

filter* chains[MAX_FILTERS] = {0};
uint8_t nfilters = 0;

void register_filter(filter* f){ 
  chains[nfilters++] = f;
  mess_proc_entry();
}

void unregister_filter(uint8_t index){
  if(!chains[index])
    return;

  kfree(chains[index]);

  nfilters--;
  memmove(&(chains[index]), &(chains[index + 1]), nfilters - index);
  mess_proc_entry();
}

void clean_filters(void){
  uint32_t i;
  for(i = 0; i < nfilters; i++)
    kfree(chains[i]);
}

/*
 * Try to match registered filters against an udp packet
 */
static uint8_t match_filter_against_udp_ports(struct udphdr* udp, filter* f){
  if(f->dst_port != 0 && udp->dest != f->dst_port)
    return 0;
  if(f->src_port != 0 && udp->source != f->src_port)
    return 0;
  return 1;
}

/*
 * Try to match registered filters against an ip packet
 */
static uint8_t match_filter_against_ip_ips(struct iphdr* ip, filter* f){
  if(f->dst_addr != 0 && ip->daddr != f->dst_addr)
    return 0;
  if(f->src_addr != 0 && ip->saddr != f->src_addr)
    return 0;
  return 1;
}

/*
 * Match filters against ip packet
 */
uint8_t match_filter(struct iphdr* ip, filter* f){
  return ( match_filter_against_ip_ips(ip, f) | match_filter_against_udp_ports((struct udphdr*) (((uint8_t*) ip) + (ip->ihl << 2)), f) );
}

#endif
