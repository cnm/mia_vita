/*
 *@author: Frederico Gonçalves
 */

#ifdef CONFIG_SYNCH_ADHOC

#include <linux/kernel.h>
#include <linux/if_ether.h> /*ETH_P_IP*/
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/udp.h>
#include <linux/byteorder/generic.h>

#include "sync_proto.h"
#include "rt73.h"
#include "mlme.h"
#include "filter_chains.h"
#include "miavita_packet.h"

#define SNAP 0xAA
#define SEC_2_NSEC 1000000000L
#define USEC_2_NSEC 1000L
#define __ieee80211b_diffs__ 50 

#define udph_from_iph(ip)					\
  ((struct udphdr*) (((uint8_t*) (ip)) + ((ip)->ihl << 2)))

#define application_payload_from_iph(ip)				\
  ((packet_t*) (((uint8_t*) (ip)) + ((ip)->ihl << 2) + sizeof(struct udphdr)))

int64_t get_kernel_current_time(void) {
  struct timeval t;
  memset(&t, 0, sizeof(struct timeval));
  do_gettimeofday(&t);
  return ((int64_t) t.tv_sec) * SEC_2_NSEC + ((int64_t) t.tv_usec)
    * USEC_2_NSEC;
}

void dump_data_to_syslog(char* label, uint8_t* data, uint32_t len){
  uint32_t i;
  printk("START DUMP %s\n", label);
  for(i = 0; i < len; i++){
    if(i != 0 && i % 7 == 0)
      printk("\n");
    printk("%02X ", data[i]);
  }
  printk("\nEND DUMP %s\n", label);
}

static void check_udp(struct udphdr* udp, __be32 saddr, __be32 daddr){
  uint16_t* pseudo_hdr;
  uint32_t pseudo_hdr_len, i;
  uint16_t udp_data_len = ntohs(udp->len) - sizeof(struct udphdr);
  uint16_t proto = IPPROTO_UDP;
  uint32_t sum = 0;

  pseudo_hdr_len = (udp_data_len % 2)? (udp_data_len + 21) / 2 : (udp_data_len + 20) / 2;

  pseudo_hdr = kmalloc(pseudo_hdr_len * sizeof(uint16_t), GFP_ATOMIC);
  if(!pseudo_hdr){
    printk(KERN_EMERG "%s:%d: kmalloc failed. Unable to calculate udp checksum.\n", __FILE__, __LINE__);
    return;
  }
  udp->check = 0;

  memset(pseudo_hdr, 0, pseudo_hdr_len * sizeof(uint16_t));
  memcpy(pseudo_hdr, &saddr, sizeof(saddr));
  memcpy(((uint8_t*)pseudo_hdr) + 4, &daddr, sizeof(daddr));
  memcpy(((uint8_t*)pseudo_hdr) + 9, &proto, sizeof(proto));
  memcpy(((uint8_t*)pseudo_hdr) + 10, &(udp->len), sizeof(udp->len));
  memcpy(((uint8_t*)pseudo_hdr) + 12, udp, ntohs(udp->len));
  
  for (i = 0; i < pseudo_hdr_len; i++){
    sum += ntohs(pseudo_hdr[i]);
    while(sum & 0x00010000){ //overflow - need to "readd" carry
      sum &= 0x0000FFFF;
      sum += 0x00000001;
    }
  }
  
  udp->check = ~sum;
  if(udp->check == 0x0000)
    udp->check = 0xFFFF;

  kfree(pseudo_hdr);
}

void synch_out_data_packet(struct urb* bulk_out, PRTMP_ADAPTER pad){
  if(ADHOC_ON(pad)){
    uint8_t* raw_packet = (uint8_t*) (bulk_out->transfer_buffer);
    uint32_t filter_it;
    uint16_t pid;
    uint8_t* raw_llc_header;
    struct iphdr* ip;
    struct udphdr* udp; 
    packet_t* pdu;
    int64_t incomming_ts;

    //I'll just keep track of the low part for now.
    static uint32_t last_fails = 0, last_retries = 0;

    //jump ralink's control data
    HEADER_802_11* _80211 = (HEADER_802_11*) (raw_packet + sizeof(TXD_STRUC)); 

    //Check if logical link control header will have 8 bytes
    raw_llc_header = (uint8_t*) (((uint8_t*) _80211) + sizeof(HEADER_802_11));

#ifdef DBG
    dump_data_to_syslog("LLC HEADER", raw_llc_header, 8);
#endif

    if(raw_llc_header[0] == (uint8_t) SNAP
       &&
       raw_llc_header[1] == (uint8_t) SNAP){

#ifdef DBG
      printk("dest mac: %02X:%02X:%02X:%02X:%02X:%02X\n", _80211->Addr1[0], _80211->Addr1[1], _80211->Addr1[2], _80211->Addr1[3], _80211->Addr1[4], _80211->Addr1[5]);
      printk("source mac: %02X:%02X:%02X:%02X:%02X:%02X\n", _80211->Addr2[0], _80211->Addr2[1], _80211->Addr2[2], _80211->Addr2[3], _80211->Addr2[4], _80211->Addr2[5]);
#endif

      memcpy(&pid, raw_llc_header + 6, sizeof(uint16_t)); 
      pid = ntohs(pid);
      if(pid == (uint16_t) ETH_P_IP){
	ip = (struct iphdr*) (raw_llc_header + 8);
	udp = (struct udphdr*) (((char*) ip) + (ip->ihl << 2));
	pdu = application_payload_from_iph(ip);
	
#ifdef DBG
	dump_data_to_syslog("IP HEADER", (uint8_t*) ip, ip->ihl << 2);
#endif

	if(ip->protocol == IPPROTO_UDP){
#ifdef DBG
	  dump_data_to_syslog("UDP HEADER", (uint8_t*) udp, ntohs(udp->len));
	  dump_data_to_syslog("PDU HEADER", (uint8_t*) pdu, sizeof(packet_t));
#endif

	  for(filter_it = 0; filter_it < nfilters; filter_it++)
	    if(match_filter(ip, chains[filter_it])){
	      incomming_ts = be64_to_cpu(pdu->timestamp);	      
	      pdu->timestamp = get_kernel_current_time() - incomming_ts;
	      pdu->timestamp = cpu_to_be64(pdu->timestamp);

	      pdu->air = cpu_to_be64(be64_to_cpu(pdu->air) + __ieee80211b_diffs__ + RTMPCalcDuration(pad, pad->PortCfg.TxRate, sizeof(HEADER_802_11) + 8 + ntohs(ip->tot_len)));

	      printk("THIS IS THE ESTIMATION TIME FOR THE PACKET %lldus", be64_to_cpu(pdu->air));
	      
	      if(last_fails != pad->WlanCounters.FailedCount.vv.LowPart){
		pdu->fails = last_fails -  pad->WlanCounters.FailedCount.vv.LowPart;
		last_fails = pad->WlanCounters.FailedCount.vv.LowPart;
	      }
	      if(last_retries != pad->WlanCounters.RetryCount.vv.LowPart){
		pdu->retries = last_retries -  pad->WlanCounters.RetryCount.vv.LowPart;
		last_retries = pad->WlanCounters.RetryCount.vv.LowPart;
	      }

	      check_udp(udp, ntohs(ip->saddr), ntohs(ip->daddr));
	      break;
	    }
	}
      }
    }
  }
}

void synch_in_data_packet(IN PUCHAR _8023hdr, IN PUCHAR data, IN ULONG len, PRTMP_ADAPTER pad){
  struct iphdr* ip;
  struct udphdr* udp;
  packet_t* pdu;
  int64_t other_ts;
  uint32_t filter_it;
  uint16_t pid;

  if(ADHOC_ON(pad)){

    memcpy(&pid, _8023hdr + 12, sizeof(uint16_t)); 
    pid = ntohs(pid);
    if(pid == (uint16_t) ETH_P_IP){
      ip = (struct iphdr*) data;
      udp = (struct udphdr*) (((char*) ip) + (ip->ihl << 2));
      pdu = application_payload_from_iph(ip);

#ifdef DBG
      dump_data_to_syslog("IP HEADER", (uint8_t*) ip, ip->ihl << 2);
#endif
      
      if(ip->protocol == IPPROTO_UDP){
#ifdef DBG
	dump_data_to_syslog("UDP HEADER", (uint8_t*) udp, ntohs(udp->len));
	dump_data_to_syslog("PDU HEADER", (uint8_t*) pdu, sizeof(packet_t));
#endif
	
	for(filter_it = 0; filter_it < nfilters; filter_it++)
	  if(match_filter(ip, chains[filter_it])){
	    other_ts = be64_to_cpu(pdu->timestamp);	      
	    pdu->timestamp = get_kernel_current_time() - other_ts;
	    pdu->timestamp = cpu_to_be64(pdu->timestamp);
	    
	    check_udp(udp, ntohs(ip->saddr), ntohs(ip->daddr));
	    break;
	  }
      }    
    }
  }
}

#endif
