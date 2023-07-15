/*
 * NESCA4
 * by oldteam & lomaster
 * license GPL-3.0
 * - Сделано от души 2023.
*/

#ifndef SYN_PING_H
#define SYN_PING_H
#include "../../include/portscan.h"
#include "../../include/generation.h"
#include "../../ncsock/include/ncread.h"

double
tcp_syn_ping(const char* ip, const char* source_ip, int dest_port, int source_port, int timeout_ms, int ttl);

#endif
