/*
 * raw_udp.c
 * Copyright (C) 2016 alex <alex@alex>
 *
 * Distributed under terms of the MIT license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <string.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <netdb.h>

/* 
 * pesudo header is used to calculate the checksum mainly
 * and is not sent in the packet.
 */

#define MAX_LEN 1024
struct pesudoHeader {
    u_int32_t srcAddr;
    u_int32_t dstAddr;
    u_int8_t placeHolder;
    u_int8_t protocol;
    u_int16_t Len;
};

/*
 * checkSum calculates the checksum of the pesudo header.
 * It split the pesudo header into unsigned short, and
 * added it together, and added the carries to it.
 * And last, it get the "~" of the value.
 */
unsigned short checkSum(unsigned short *ptr, int nbytes) {
    long sum;
    unsigned short oddbyte;
    short answer;

    sum = 0;
    while (nbytes > 1) {
        sum += *ptr++;
        nbytes -= 2;
    }
    if (nbytes == 1) {
        oddbyte = 0;
        *((u_char*)&oddbyte) = *(u_char*)ptr;
        sum += oddbyte;
    }

    // carry.
    sum = (sum >> 16) + (sum & 0xffff);
    // in case of carry.
    sum += sum >> 16;
    answer = (short)~sum;
    return answer;
}

void udpPacketSend(struct sockaddr_in *srcHost, struct sockaddr_in *dstHost, char *udpData, int udpDataLen) {
    int st;
    int ipLen;
    int pseLen;
    const int on = 1;
    //char buf[MAX_LEN] = {0};
    int length = MAX_LEN + sizeof(struct ip) + sizeof(struct udphdr);
    char *buf = malloc(length * sizeof(char));
    bzero(buf, length);

    char *pse;
    struct ip *ipHeader;
    struct udphdr *udpHeader;
    struct pesudoHeader psh;

    if ((st = socket(AF_INET, SOCK_RAW, IPPROTO_UDP)) < 0) {
        perror("CREATE SOCKET ERROR");
        exit(1);
    }

    // construct the ip packet on our own.
    if (setsockopt(st, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0) {
        perror("HDRINCL ERROR");
        exit(1);
    }

    ipLen = sizeof(struct ip) + sizeof(struct udphdr) + udpDataLen;

    ipHeader = (struct ip*) buf;
    ipHeader->ip_v = IPVERSION;
    ipHeader->ip_hl = sizeof(struct ip) >> 2;
    ipHeader->ip_tos = 0;
    ipHeader->ip_len = htons(ipLen);
    ipHeader->ip_id = 0;
    ipHeader->ip_off = 0;
    ipHeader->ip_ttl = MAXTTL;
    ipHeader->ip_p = IPPROTO_UDP;
    ipHeader->ip_sum = 0;
    ipHeader->ip_dst = dstHost->sin_addr;
    ipHeader->ip_src = srcHost->sin_addr;

    udpHeader = (struct udphdr*)(buf + sizeof(struct ip));
    udpHeader->source = srcHost->sin_port;
    udpHeader->dest = dstHost->sin_port;
    udpHeader->len = htons(ipLen - sizeof(struct ip));
    udpHeader->check = 0;

    if (udpDataLen > 0) {
        memcpy(buf+sizeof(struct ip) + sizeof(struct udphdr), udpData, udpDataLen);
    }

    psh.srcAddr = srcHost->sin_addr.s_addr;
    psh.dstAddr = dstHost->sin_addr.s_addr;
    psh.placeHolder = 0;
    psh.protocol = IPPROTO_UDP;
    psh.Len = htons(sizeof(struct udphdr) + udpDataLen);
    pseLen = sizeof(struct pesudoHeader) + sizeof(struct udphdr) + udpDataLen;
    pse = malloc(pseLen);
    memcpy(pse, (char*)&psh, sizeof(struct pesudoHeader));
    memcpy(pse+sizeof(struct pesudoHeader), udpHeader, sizeof(struct udphdr) + udpDataLen);

    udpHeader->check = checkSum((unsigned short*)pse, pseLen);
    sendto(st, buf, ipLen, 0, (struct sockaddr*)dstHost, sizeof(struct sockaddr_in));
    close(st);
}

int raw_udp_send(char *src_addr, char *src_port, char *dst_addr, char *dst_port) {
    struct sockaddr_in srcHost, dstHost;
    struct hostent *host, *host2;

    char Data[MAX_LEN];
    char c = 0;
    int count = 0;
    while ((c != '\n') && (count < MAX_LEN)) {
        int n = scanf("%c", &c);
        if (n == EOF) {
            break;
        }
        Data[count++] = c;
    }
    
    bzero(&srcHost, sizeof(struct sockaddr_in));
    bzero(&dstHost, sizeof(struct sockaddr_in));

    dstHost.sin_family = AF_INET;
    dstHost.sin_port = htons(atoi(dst_port));
    if (inet_aton(dst_addr, &dstHost.sin_addr) != 1) {
        host = gethostbyname(dst_addr);
        if (host == NULL) {
            printf("ERROR");
            exit(1);
        }
        dstHost.sin_addr = *(struct in_addr*)(host->h_addr_list[0]);
    }

    srcHost.sin_family = AF_INET;
    srcHost.sin_port = htons(atoi(src_port));
    
    if (inet_aton(src_addr, &srcHost.sin_addr) != 1) {
        host2 = gethostbyname(src_addr);
        if (host == NULL) {
            printf("ERROR");
            exit(1);
        }
        srcHost.sin_addr = *(struct in_addr*)(host2->h_addr_list[0]);
    }
    udpPacketSend(&srcHost, &dstHost, Data, count);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 5) {
        printf("usage: ./raw_udp src_addr src_port dst_addr dst_port\n");
        exit(1);
    }

    raw_udp_send(argv[1], argv[2], argv[3], argv[4]);
}
