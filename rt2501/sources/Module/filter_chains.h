/*
 *@author: Frederico Gonçalves
 */

#ifndef __FILTER_CHAINS_H__
#define __FILTER_CHAINS_H__

#ifdef CONFIG_SYNCH_ADHOC

#include <linux/kernel.h>
#include <linux/ip.h>

typedef struct{
  uint8_t proto;
  uint32_t dst_addr;
  uint32_t src_addr;
  uint16_t dst_port;
  uint16_t src_port;
}filter;

#define MAX_FILTERS 256

extern filter* chains[MAX_FILTERS];
extern uint8_t nfilters;

extern void register_filter(filter* f);
extern void unregister_filter(uint8_t index);
extern uint8_t match_filter(struct iphdr* ip, filter* f);
extern void clean_filters(void);
#endif

#endif
