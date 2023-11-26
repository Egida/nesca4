/*
 * LIBNCSOCK & NESCA4
 *   Сделано от души 2023.
 * Copyright (c) [2023] [lomaster]
 * SPDX-License-Identifier: BSD-3-Clause
*/

#ifndef READPKT_HEADER
#define READPKT_HEADER
#include "types.h"
#include "ip.h"
#include "tcp.h"
#include "udp.h"
#include <sys/cdefs.h>

#define RECV_BUFFER_SIZE 65535

__BEGIN_DECLS

struct readfiler
{
  const char* dest_ip;
  u8          protocol;
};

int read_packet(struct readfiler *rf, int recv_timeout_ms, u8 **buffer);

struct ip_header*   ext_iphdr(u8 *buf);
struct tcp_header*  ext_tcphdr(u8 *buf);
struct udp_header*  ext_udphdr(u8 *buf);
struct icmp4_header* ext_icmphdr(u8 *buf);
int                 ext_payload(u8 *buf, u8 *rbuf);

void print_ipdr(const struct ip_header *iphdr);
void print_tcphdr(const struct tcp_header *tcphdr);
void print_udphdr(const struct udp_header *udphdr);

__END_DECLS


#endif


