#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <pcap.h>
#define __FAVOR_BSD
#include <stdio.h>
#include <stdlib.h>
#include <pcap.h>
#include <arpa/inet.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <string.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <net/if_arp.h>
#include <netinet/if_ether.h>
#define MAC_ADDRSTRLEN 2*6+5+1
void dump_ethernet(u_int32_t length, const u_char *content);
void dump_ip(u_int32_t length, const u_char *content);
void dump_tcp(u_int32_t length, const u_char *content);
void dump_udp(u_int32_t length, const u_char *content);
void pcap_callback(u_char *arg, const struct pcap_pkthdr *header, const u_char *content);
char *mac_ntoa(u_char *d);
char *ip_ntoa(void *i);
char *ip_ttoa(u_int8_t flag);
char *ip_ftoa(u_int16_t flag);
char *tcp_ftoa(u_int8_t flag);
void dump_icmp(u_int32_t length,struct icmp *icmp,const u_char *content);
void pcap_callback(u_char *arg, const struct pcap_pkthdr *header, const u_char *content);
void dump_arp(u_int32_t length, const u_char *content);
int main(int argc, const char * argv[]) {
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle = NULL;
    char *device = "enp0s3";
    bpf_u_int32 net, mask;
    struct bpf_program fcode;
    pcap_dumper_t *dumper = NULL;
    int a=0;
    pcap_if_t *devices = NULL;
    char ntop_buf[256];
     if(-1 == pcap_findalldevs(&devices, errbuf)) {
        fprintf(stderr, "pcap_findalldevs: %s\n", errbuf);
        exit(1);
    }//end if

    //list all device
    for(pcap_if_t *d = devices ; d ; d = d->next) {
        printf("Interface: %s\n", d->name);
        if(d->description) {
            printf("\tDescription: %s\n", d->description);
        }//end if
        printf("\tLoopback: %s\n",(d->flags & PCAP_IF_LOOPBACK) ? "yes" : "no");

        //list all address
        for(struct pcap_addr *a = d->addresses ; a ; a = a->next) {
            sa_family_t family = a->addr->sa_family;

            if(family == AF_INET || family == AF_INET6) {
                if(a->addr) {
                    printf("\t\tAddress: %s\n",
                           inet_ntop(family, &((struct sockaddr_in *)a->addr)->sin_addr, ntop_buf, sizeof(ntop_buf)));
                }//end if
                if(a->netmask) {
                    printf("\t\tNetmask: %s\n",
                           inet_ntop(family, &((struct sockaddr_in *)a->netmask)->sin_addr, ntop_buf, sizeof(ntop_buf)));
                }//end if
                if(a->broadaddr) {
                    printf("\t\tBroadcast Address: %s\n",
                           inet_ntop(family, &((struct sockaddr_in *)a->broadaddr)->sin_addr, ntop_buf, sizeof(ntop_buf)));
                }//end if
                if(a->dstaddr) {
                    printf("\t\tDestination Address: %s\n",
                           inet_ntop(family, &((struct sockaddr_in *)a->dstaddr)->sin_addr, ntop_buf, sizeof(ntop_buf)));
                }//end if
            }//end else

            printf("\n");
        }//end for
    }//end for

    //free
    pcap_freealldevs(devices);
    
    printf("how many data you want to catch?\n");
    scanf("%d",&a);
    handle = pcap_open_live(device, 65535, 1, 1, errbuf);
    if(!handle) {
        fprintf(stderr, "pcap_open_live: %s\n", errbuf);
        exit(1);
    }//end if

    //ethernet only
    if(pcap_datalink(handle) != DLT_EN10MB) {
        fprintf(stderr, "Sorry, Ethernet only.\n");
        pcap_close(handle);
        exit(1);
    }//end if

    //get network and mask
    if(-1 == pcap_lookupnet("enp0s3", &net, &mask, errbuf)) {
        fprintf(stderr, "pcap_lookupnet: %s\n", errbuf);
        pcap_close(handle);
        exit(1);
    }//end if
    dumper = pcap_dump_open(handle, "./packet.pcap");
    if(!dumper) {
        fprintf(stderr, "pcap_dump_open: %s\n", pcap_geterr(handle));
        pcap_close(handle);
        exit(1);
    }//end if
    //compile filter
    if(-1 == pcap_compile(handle, &fcode,argv[1], 1, mask)) {
        fprintf(stderr, "pcap_compile: %s\n", pcap_geterr(handle));
        pcap_close(handle);
        exit(1);
    }//end if

    //set filter
    if(-1 == pcap_setfilter(handle, &fcode)) {
        fprintf(stderr, "pcap_pcap_setfilter: %s\n", pcap_geterr(handle));
        pcap_close(handle);
        exit(1);
    }

    //free code
    pcap_freecode(&fcode);


    //start capture
    pcap_loop(handle, a, pcap_callback, (u_char *)dumper);
    pcap_dump_flush(dumper);
    pcap_dump_close(dumper);

    //free
    pcap_close(handle);
    return 0;
}

void pcap_callback(u_char *arg, const struct pcap_pkthdr *header, const u_char *content) {
    
    pcap_dumper_t *dumper = (pcap_dumper_t *)arg;
    //dump to file
    pcap_dump(arg, header, content);
    //flush
    pcap_dump_flush(dumper);
    static int d = 0;
    struct tm *ltime;
    char timestr[16];
    time_t local_tv_sec;
    local_tv_sec = header->ts.tv_sec;
    ltime = localtime(&local_tv_sec);
    strftime(timestr, sizeof timestr, "%H:%M:%S", ltime);

    printf("No. %d\n", ++d);

    //print header
    printf("\tTime: %s.%.6ld\n", timestr, header->ts.tv_usec);
    printf("\tLength: %d bytes\n", header->len);
    printf("\tCapture length: %d bytes\n", header->caplen);

    //dump ethernet
    dump_ethernet(header->caplen, content);

    printf("\n");
}//end
char *mac_ntoa(u_char *d) {
    static char str[MAC_ADDRSTRLEN];

    snprintf(str, sizeof(str), "%02x:%02x:%02x:%02x:%02x:%02x", d[0], d[1], d[2], d[3], d[4], d[5]);

    return str;
}//end mac_ntoa

char *ip_ntoa(void *i) {
    static char str[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, i, str, sizeof(str));

    return str;
}//end ip_ntoa

void dump_ethernet(u_int32_t length, const u_char *content){
    struct ether_header *ethernet = (struct ether_header *)content;
    char dst_mac_addr[MAC_ADDRSTRLEN] = {};
    char src_mac_addr[MAC_ADDRSTRLEN] = {};
    u_int16_t type;
    strncpy(dst_mac_addr, mac_ntoa(ethernet->ether_dhost), sizeof(dst_mac_addr));
    strncpy(src_mac_addr, mac_ntoa(ethernet->ether_shost), sizeof(src_mac_addr));
    type = ntohs(ethernet->ether_type);
    //print
    if(type <= 1500)
        printf("IEEE 802.3 Ethernet Frame:\n");
    else
        printf("Ethernet Frame:\n");

    printf("\n");
    printf("Destination MAC Address:         %s\n", dst_mac_addr);
    printf("\n");
    printf("Source MAC Address:              %s\n", src_mac_addr);
    printf("\n");
    if (type < 1500)
        printf("Length:            %5u\n", type);
    else
        printf("Ethernet Type:    0x%04x\n", type);
    printf("+-------------------------+\n");
    switch (type) {
        case ETHERTYPE_ARP:
            dump_arp(length, content);
            break;

        case ETHERTYPE_IP:
            dump_ip(length, content);
            break;

        case ETHERTYPE_REVARP:
            printf("Next is RARP\n");
            break;

        case ETHERTYPE_IPV6:
            printf("Next is IPv6\n");
            break;

        default:
            printf("Next is %#06x", type);
            break;
    }
}
void dump_arp(u_int32_t length, const u_char *content) {
    struct ether_arp *arp = (struct ether_arp *)(content + ETHER_HDR_LEN);
    
    u_short hardware_type = ntohs(arp->ea_hdr.ar_hrd);
    u_short protocol_type = ntohs(arp->ea_hdr.ar_pro);
    u_char hardware_len = arp->ea_hdr.ar_hln;
    u_char protocol_len = arp->ea_hdr.ar_pln;
    u_short operation = ntohs(arp->ea_hdr.ar_op);

    static char *arp_op_name[] = {
        "Undefine",
        "(ARP Request)",
        "(ARP Reply)",
        "(RARP Request)",
        "(RARP Reply)"
    }; //arp option type

    if(operation < 0 || sizeof(arp_op_name)/sizeof(arp_op_name[0]) < operation)
        operation = 0;

    printf("Protocol: ARP\n");
    printf("+-------------------------+-------------------------+\n");
    printf("| Hard Type: %2u%-11s| Protocol:0x%04x%-9s|\n",
           hardware_type,
           (hardware_type == ARPHRD_ETHER) ? "(Ethernet)" : "(Not Ether)",
           protocol_type,
           (protocol_type == ETHERTYPE_IP) ? "(IP)" : "(Not IP)");
    printf("+------------+------------+-------------------------+\n");
    printf("| HardLen:%3u| Addr Len:%2u| OP: %4d%16s|\n",
           hardware_len, protocol_len, operation, arp_op_name[operation]);
    printf("+------------+------------+-------------------------+-------------------------+\n");
    printf("| Source MAC Address:                                        %17s|\n", mac_ntoa(arp->arp_sha));
    printf("+---------------------------------------------------+-------------------------+\n");
    printf("| Source IP Address:                 %15s|\n", ip_ntoa(arp->arp_spa));
    printf("+---------------------------------------------------+-------------------------+\n");
    printf("| Destination MAC Address:                                   %17s|\n", mac_ntoa(arp->arp_tha));
    printf("+---------------------------------------------------+-------------------------+\n");
    printf("| Destination IP Address:            %15s|\n", ip_ntoa(arp->arp_tpa));
    printf("+---------------------------------------------------+\n");
}//end dump_arp
void dump_ip(u_int32_t length, const u_char *content) {
    struct ip *ip =  (struct ip *)(content + ETHER_HDR_LEN);
    u_int version = ip->ip_v;
    u_int header_len = ip->ip_hl << 2;
    u_char tos = ip->ip_tos;
    u_int16_t total_len = ntohs(ip->ip_len);
    u_int16_t id = ntohs(ip->ip_id);
    u_int16_t offset = ntohs(ip->ip_off);
    u_char ttl = ip->ip_ttl;
    u_char protocol = ip->ip_p;
    u_int16_t checksum = ntohs(ip->ip_sum);
    printf("Protocol: IP\n");
    printf("\n");
    printf("IV:%1u| HL:%2u| Total Length: %10u\n",
           version, header_len, total_len);
    printf("\n");
    printf("Identifier:        %5u| FO:        %5u\n",
           id, offset & IP_OFFMASK);
    printf("\n");
    printf("TTL:    %3u| Pro:    %3u| Header Checksum:   %5u\n",
           ttl, protocol, checksum);
    printf("\n");
    printf("Source IP Address:                 %15s\n",  ip_ntoa(&ip->ip_src));
    printf("\n");
    printf("Destination IP Address:            %15s\n", ip_ntoa(&ip->ip_dst));
    printf("\n");
    printf("+-------------------------+\n");
    char *p = (char *)ip + (ip->ip_hl << 2);
    switch (protocol) {
        case IPPROTO_UDP:
            dump_udp(length, content);
            break;

        case IPPROTO_TCP:
            dump_tcp(length, content);
            break;

        case IPPROTO_ICMP:
            dump_icmp(length,(struct icmp *)p,content);
            break;

        default:
            printf("Next is %d\n", protocol);
            break;
    }//end switch
    
    
}
void dump_tcp(u_int32_t length, const u_char *content) {
    struct ip *ip = (struct ip *)(content + ETHER_HDR_LEN);
    struct tcphdr *tcp = (struct tcphdr *)(content + ETHER_HDR_LEN + (ip->ip_hl << 2));

    //copy header
    u_int16_t source_port = ntohs(tcp->th_sport);
    u_int16_t destination_port = ntohs(tcp->th_dport);
    u_int32_t sequence = ntohl(tcp->th_seq);
    u_int32_t ack = ntohl(tcp->th_ack);
    u_int8_t header_len = tcp->th_off << 2;
    u_int8_t flags = tcp->th_flags;
    u_int16_t window = ntohs(tcp->th_win);
    u_int16_t checksum = ntohs(tcp->th_sum);
    u_int16_t urgent = ntohs(tcp->th_urp);

    //print
    printf("Protocol: TCP\n");
    printf("\n");
    printf("Source Port:       %5u| Destination Port:  %5u\n", source_port, destination_port);
    printf("\n");
    printf("Sequence Number:            %10u\n", sequence);
    printf("\n");
    printf("Acknowledgement Number:                 %10u\n", ack);
    printf("\n");
    printf("Header_lenth:%2u| Window Size:       %5u\n", header_len, window);
    printf("\n");
    printf("Checksum:          %5u| Urgent Pointer:    %5u\n", checksum, urgent);
    printf("\n");
    printf("+-------------------------------------------------------+\n");
}//end dump_tcp
void dump_udp(u_int32_t length, const u_char *content) {
    struct ip *ip = (struct ip *)(content + ETHER_HDR_LEN);
    struct udphdr *udp = (struct udphdr *)(content + ETHER_HDR_LEN + (ip->ip_hl << 2));

    u_int16_t source_port = ntohs(udp->uh_sport);
    u_int16_t destination_port = ntohs(udp->uh_dport);
    u_int16_t len = ntohs(udp->uh_ulen);
    u_int16_t checksum = ntohs(udp->uh_sum);

    printf("Protocol: UDP\n");
    printf("\n");
    printf("Source Port:       %5u| Destination Port:  %5u\n", source_port, destination_port);
    printf("\n");
    printf("Length:            %5u| Checksum:          %5u\n", len, checksum);
    printf("\n");
    printf("+-------------------------------------------------------+\n");
}//end dump_udp
void dump_icmp(u_int32_t length,struct icmp *icmp,const u_char *content) {

    //copy header
    u_char type = icmp->icmp_type;
    u_char code = icmp->icmp_code;
    u_char checksum = ntohs(icmp->icmp_cksum);

    static char *type_name[] = {
        "Echo Reply",               /* Type  0 */
        "Undefine",                 /* Type  1 */
        "Undefine",                 /* Type  2 */
        "Destination Unreachable",  /* Type  3 */
        "Source Quench",            /* Type  4 */
        "Redirect (change route)",  /* Type  5 */
        "Undefine",                 /* Type  6 */
        "Undefine",                 /* Type  7 */
        "Echo Request",             /* Type  8 */
        "Undefine",                 /* Type  9 */
        "Undefine",                 /* Type 10 */
        "Time Exceeded",            /* Type 11 */
        "Parameter Problem",        /* Type 12 */
        "Timestamp Request",        /* Type 13 */
        "Timestamp Reply",          /* Type 14 */
        "Information Request",      /* Type 15 */
        "Information Reply",        /* Type 16 */
        "Address Mask Request",     /* Type 17 */
        "Address Mask Reply",       /* Type 18 */
        "Unknown"                   /* Type 19 */
    }; //icmp type
#define ICMP_TYPE_MAX (sizeof type_name / sizeof type_name[0])

    if (type < 0 || ICMP_TYPE_MAX <= type)
        type = ICMP_TYPE_MAX - 1;

    printf("Protocol: ICMP (%s)\n", type_name[type]);

    printf("+------------+------------+-------------------------+\n");
    printf("| Type:   %3u| Code:   %3u| Checksum:          %5u|\n", type, code, checksum);
    printf("+------------+------------+-------------------------+\n");

    if (type == ICMP_ECHOREPLY || type == ICMP_ECHO) {
        printf("| Identification:    %5u| Sequence Number:   %5u|\n", ntohs(icmp->icmp_id), ntohs(icmp->icmp_seq));
        printf("+-------------------------+-------------------------+\n");
    }//end if
    else if (type == ICMP_UNREACH) {
        if (code == ICMP_UNREACH_NEEDFRAG) {
            printf("| void:          %5u| Next MTU:          %5u|\n", ntohs(icmp->icmp_pmvoid), ntohs(icmp->icmp_nextmtu));
            printf("+-------------------------+-------------------------+\n");
        }//end if
        else {
            printf("| Unused:                                 %10lu|\n", (unsigned long) ntohl(icmp->icmp_void));
            printf("+-------------------------+-------------------------+\n");
        }//end else
    }//end if
    else if (type == ICMP_REDIRECT) {
        printf("| Router IP Address:                 %15s|\n", ip_ntoa(&(icmp->icmp_gwaddr)));
        printf("+---------------------------------------------------+\n");
    }//end if
    else if (type == ICMP_TIMXCEED) {
        printf("| Unused:                                 %10lu|\n", (unsigned long)ntohl(icmp->icmp_void));
        printf("+---------------------------------------------------+\n");
    }//end else

    //if the icmp packet carry ip header
    if (type == ICMP_UNREACH || type == ICMP_REDIRECT || type == ICMP_TIMXCEED) {
        struct ip *ip = (struct ip *)icmp->icmp_data;
        char *p = (char *)ip + (ip->ip_hl << 2);
        dump_ip(length,content);

        switch (ip->ip_p) {
            case IPPROTO_TCP:
                if(type == ICMP_REDIRECT) {
                    //dump_tcp_mini((struct tcphdr *)p);
                }//end if
                else {
                    dump_tcp(length,content);
                }//end else
                break;
            case IPPROTO_UDP:
                dump_udp(length,content);
                break;
        }//end switch
    }//end if
}//end dump_icmp