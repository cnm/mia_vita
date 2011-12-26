#ifndef __UTILS_H__
#define __UTILS_H__

#define SEC_2_NSEC 1000000000L
#define USEC_2_NSEC 1000L

#define print_error(format, ...)					\
  do{									\
    printk(KERN_EMERG "%s - %s:%d: ", __FUNCTION__, __FILE__, __LINE__); \
    printk(format, ## __VA_ARGS__);					\
  }while(0)

#ifdef DBG
#define debug(format, ...)			\
    printk(format, ## __VA_ARGS__)
#else 
#define debug(format, ...)
#endif

static inline uint16_t csum(uint16_t* buff, int nwords) {
  uint32_t sum;
  for (sum = 0; nwords > 0; nwords--)
    sum += *buff++;
  sum = (sum >> 16) + (sum & 0xffff);
  sum += (sum >> 16);

  return ((uint16_t) ~sum);
}

static inline void dump_ip_skb(struct sk_buff *skb) {
  struct iphdr* iph = ip_hdr(skb);
  int32_t i;
  for (i = 0; i < ntohs(iph->tot_len); i++)
    printk("%02X ", ((uint8_t*) iph)[i]);
  printk("\n");
}

#endif
