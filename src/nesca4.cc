/*
 *
 *  d8b   db d88888b .d8888.  .o88b.  .d8b.    NESCA4 - Многопоточный linux сканер портов,
 *  888o  88 88'     88'  YP d8P  Y8 d8' `8b   с точностью nmap-а, обработкой nesca3 брутфорсом
 *  88V8o 88 88ooooo `8bo.   8P      88ooo88   самых популярных протоколов и сканированием DNS.
 *  88 V8o88 88~~~~~   `Y8b. 8b      88~~~88
 *  88  V888 88.     db   8D Y8b  d8 88   88   Был сделан с вдохновением от nesca3 & nmap.
 *  VP   V8P Y88888P `8888Y'  `Y88P' YP   YP   by oldteam & lomaster.
 *
 *  license GPL-3.0                            Github - [https://github.com/oldteamhost/nesca4]
 *  Сделано от души 2023.                      Telegram - [https://t.me/+Ft1ABe8P8Tc1YzZi]
 *
 *  PS: Перечислять все возможности тут я не буду.
 *  Прочитать о том что умеет nesca4 и как это разрабатывалось
 *  читайте в resources/do_not_read.txt
 *
 *  Вся работа с сетью находится в ncsock/
 *    ^ (даже брутфорс: RVI, HTTP, FTP, SMTP, RTSP)
*/

#include "../include/nesca4.h"
#include "../include/nescautils.h"
#include "../include/portscan.h"
#include "../ncsock/include/icmp.h"
#include "../ncsock/include/igmp.h"
#include "../ncsock/include/readpkt.h"
#include "../ncsock/include/strbase.h"
#include "../ncsock/include/tcp.h"
#include "../ncsock/include/utils.h"
#include <arpa/inet.h>
#include <bits/getopt_core.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <netinet/in.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <string.h>

nesca_prints np;
html_output ho;
services_nesca sn;
arguments_program argp;
readfiler rf;
NESCADATA n;

const char* run;
std::mutex ls;
const char* short_options = "s:hl:vd:T:p:aS:";

int main(int argc, char** argv)
{
  run = argv[0];

  if (argc <= 1)
    usage();

  char* templocalip = get_local_ip();
  std::string __temp = templocalip;
  argp.source_ip = __temp.c_str();
  free(templocalip);

  parse_args(argc, argv);
  pre_check();
  importfile();

  /* set tcp flags */
  struct tcp_flags tf;
  memset(&tf, 0, sizeof(struct tcp_flags));

  if (!argp.custom_tcpflags)
    tf = set_flags(argp.type);
  else
    tf = str_set_flags(argp.custom_res_tcpflags);
  argp.tcpflags = set_tcp_flags(&tf);

  if (argp.custom_tcpflags){
    np.golder_rod_on();
    printf("-> syn: %d, ack: %d, rst: %d, fin: %d, psh: %d, urg: %d, cwr: %d, ece: %d\n",
            tf.syn, tf.ack, tf.rst, tf.fin, tf.psh, tf.urg, tf.cwr, tf.ece);
    reset_colors;
  }
  if (argp.frag_mtu > 16) {
    np.golder_rod_on();
    printf("-> NOTE: Note that if (ihl * 4) < (mtu) then fragmentation will not be performed, it is better to use mtu not more than 16.\n");
    reset_colors;
  }
  if (argp.data_string.length() > 1400) {
    np.golder_rod_on();
    puts("-> NOTE: Usually packages that have a payload greater than 1400 are rejected.");
    reset_colors;
  }
  if ((argp.tcp_ddos || argp.icmp_ddos) && argp.data_string.empty()) {
    np.golder_rod_on();
    puts("-> NOTE: To enhance DDOS you need to use (-data-len) to add payload.");
    reset_colors;
  }

  /*Определение цели.*/
  if (optind < argc) {
    std::vector<std::string> temp_ips;
    temp_ips = split_string_string(argv[optind], ',');
    resolv_hosts(temp_ips);
  }
  if (argp.random_ip) {
    int i;
    for (i = 1; i <= argp.random_ip_count; i++) {
      std::string random_temp = generate_ipv4();
      n.add_ip(random_temp);
    }
  }

  std::vector<std::string> temp_vector = n.get_all_ips();

  if (temp_vector.size() > 50000 && argp.speed_type != 5) {
    np.golder_rod_on();
    std::cout << "-> NOTE: With your number of IPs - (" << temp_vector.size()
      << "), it is better to use speed (-S5), otherwise scanning may take longer.\n";
    reset_colors;
  }

  /*Расчёт количества потоков и таймаута для пинга.*/
  if (!argp.custom_threads) {
    if (argp.my_life_my_rulez) {
      argp.threads_ping = temp_vector.size();
      argp.ping_timeout = 250;
    }
    else {
      argp.threads_ping = calculate_threads(argp.speed_type, temp_vector.size());
      argp.ping_timeout = calculate_ping_timeout(argp.speed_type);
    }
  }

  /*Установка методов пинга.*/
  if (!argp.custom_ping) {
    if (argp.max_ping == true) {
      argp.syn_ping = true;
      argp.ack_ping = true;
      argp.echo_ping = true;
      argp.info_ping = true;
      argp.timestamp_ping = true;
    }
    switch (argp.speed_type)
    {
      case 5: {
        argp.ack_ping = true;
        break;
      }
      case 4: {
        argp.echo_ping = true;
        argp.ack_ping = true;
        break;
      }
      case 3:
      case 2: {
        argp.ack_ping = true;
        argp.syn_ping = true;
        argp.echo_ping = true;
        break;
      }
      case 1: {
        argp.syn_ping = true;
        argp.ack_ping = true;
        argp.echo_ping = true;
        argp.info_ping = true;
        argp.timestamp_ping = true;
        break;
      }
    }
  }

  auto start_time_ping = std::chrono::high_resolution_clock::now();

  /*Вырез одиниковых IP.*/
  n.remove_duplicates();

  /*Удаление не тех хостов.*/
  n.negatives_hosts(argp.exclude);

  /*Пинг сканирования*/
  std::vector<std::string> result_main;
  if (!argp.ping_off) {
    thread_pool ping_pool(argp.threads_ping);
    int threads_ping = argp.threads_ping;
    std::vector<std::future<std::pair<bool, std::string>>> futures;
    auto result_iter = temp_vector.begin();
    int ip_count_ping = 0;

    while (result_iter != temp_vector.end()) {
      int remaining_tasks = std::distance(result_iter, temp_vector.end());
      int tasks_to_execute = (threads_ping < remaining_tasks) ? threads_ping : remaining_tasks;
      for (int i = 0; i < tasks_to_execute; ++i) {
        std::string ip = *result_iter;
        futures.push_back(ping_pool.enqueue([ip]() -> std::pair<bool, std::string> {
          bool ping = nesca_ping(ip.c_str());
          return std::make_pair(ping, ip);
        }));
        ++result_iter;
      }
      for (auto& future : futures) {
        auto result = future.get();
        if (result.first) {
          result_main.push_back(result.second);
        }
      }

      futures.clear();

      for (auto& future : futures) {future.wait();}
      ip_count_ping += tasks_to_execute;

      if (ip_count_ping % argp.ping_log == 0) {
        double procents = (static_cast<double>(ip_count_ping) / temp_vector.size()) * 100;
        std::string _result = format_percentage(procents);
        std::cout << np.main_nesca_out("#", "Ping "+std::to_string(ip_count_ping)+" out of "+
        std::to_string(temp_vector.size()) + " IPs", 6, "", "", _result+"%", "", "") << std::endl;
      }
    }

    temp_vector = result_main;
    n.update_data_from_ips(temp_vector);
  }

  int count_success_ips = temp_vector.size();
  auto end_time_ping = std::chrono::high_resolution_clock::now();
  auto duration_ping = std::chrono::duration_cast<std::chrono::microseconds>(end_time_ping - start_time_ping);
  argp.ping_duration = duration_ping.count() / 1000000.0;

  /*Расчёт количеста потоков для DNS resolv.*/
  if (!argp.custom_threads_resolv)
    argp.dns_threads = calculate_threads(argp.speed_type, temp_vector.size());
  if (argp.my_life_my_rulez)
    argp.dns_threads = temp_vector.size();

  auto start_time_dns = std::chrono::high_resolution_clock::now();

  /* DNS сканирование */
  if (!argp.no_get_dns) {
    int complete = 0;
    int total = temp_vector.size();
    std::vector<std::future<void>> futures_dns;
    thread_pool dns_pool(argp.dns_threads);

    for (const auto& ip : temp_vector) {
      futures_dns.emplace_back(dns_pool.enqueue(get_dns_thread, ip)); complete++;
      if (futures_dns.size() >= static_cast<long unsigned int>(argp.dns_threads)) {
        for (auto& future : futures_dns)
          future.get();

        futures_dns.clear();
      }

      if (complete % 300 == 0) {
        double procents = (static_cast<double>(complete) / total) * 100;
        std::cout << np.main_nesca_out("#", "Resolv "+std::to_string(complete)+" out of "+
        std::to_string(total) + " IPs", 6, "", "", std::to_string(procents)+"%", "", "") << std::endl;
      }
    }
    for (auto& future : futures_dns)
      future.wait();

    futures_dns.clear();
  }

  auto end_time_dns = std::chrono::high_resolution_clock::now();
  auto duration_dns = std::chrono::duration_cast<std::chrono::microseconds>(end_time_dns - start_time_dns);
  argp.dns_duration = duration_dns.count() / 1000000.0;

  if (argp.no_scan) {
    for (const auto& ip : temp_vector)
      std::cout << np.main_nesca_out("READY", ip, 5, "rDNS", "RTT", n.get_new_dns(ip), std::to_string(n.get_rtt(ip))+"ms","") << std::endl;

    putchar('\n');
    nescaend(count_success_ips, argp.ping_duration+argp.dns_duration);
    return 0;
  }

  if (argp.icmp_ddos || argp.tcp_ddos || argp.udp_ddos || argp.ip_ddos) {
    u32 saddr, daddr;
    saddr = inet_addr(argp.source_ip);
    u8 proto;
    u16 port;

    if (argp.icmp_ddos) {
      proto = IPPROTO_ICMP;
      port = 0;
    }
    else if (argp.tcp_ddos) {
      proto = IPPROTO_TCP;
      port = argp.ports[0];
    }
    else if (argp.udp_ddos) {
      proto = IPPROTO_UDP;
      port = argp.ports[0];
    }
    else if (argp.ip_ddos) {
      proto = argp.ip_ddos_proto;
      port = 0;
    }

    std::vector<std::future<void>> futures_ddos;
    thread_pool ddos_pool(argp.ddos_threads);

    int ipc = 0;
    int totalsuccess = 0;

    auto start_time_ddos = std::chrono::high_resolution_clock::now();
    for (const auto& ip : temp_vector) {
      double temprtt;
      totalsuccess += argp.success_packet_ddos;
      argp.success_packet_ddos = 0;
      ipc++;
      if (ipc > 1)
        putchar('\n');

      temprtt = n.get_rtt(ip);
      std::cout << np.main_nesca_out("RUN", ip, 5, "rDNS", "RTT", n.get_new_dns(ip), std::to_string(temprtt)+"ms","") << std::endl;
      daddr = inet_addr(ip.c_str());

      for (int i = 1; i <= argp.ddos_packets; i++){
        futures_ddos.emplace_back(ddos_pool.enqueue(nesca_ddos, proto, argp.icmp_ddos_type, daddr, saddr, port, argp.ip_ddos));

        if (futures_ddos.size() >= static_cast<long unsigned int>(argp.ddos_threads)) {
          for (auto& future : futures_ddos)
            future.get();

          futures_ddos.clear();
        }
        if (argp.ddos_packets < 20) {
          std::cout << np.main_nesca_out("REPORT", ip, 3, "packets", "out of",
              std::to_string(i), std::to_string(argp.ddos_packets),"") << std::endl;
        }
        else {
          if (i % (argp.ddos_packets / 5) == 0) {
            std::cout << np.main_nesca_out("REPORT", ip, 3, "packets", "out of",
                std::to_string(i), std::to_string(argp.ddos_packets),"") << std::endl;
          }
        }
      }
    }
    for (auto& future : futures_ddos)
      future.wait();

    futures_ddos.clear();

    auto end_time_ddos = std::chrono::high_resolution_clock::now();
    auto duration_ddos = std::chrono::duration_cast<std::chrono::microseconds>(end_time_ddos - start_time_ddos);
    putchar('\n');

    np.golder_rod_on();
    std::cout << "-> NESCA finished " << totalsuccess << " packets (success) in "
      << std::fixed << std::setprecision(2) << duration_ddos.count() / 1000000.0 << " seconds\n";
    reset_colors;

    return 0;
  }

  /*Иницилизация брутфорса.*/
  init_bruteforce();

  n.group_size = GROUP_MIN_SIZE_DEFAULT;

  /*Установка настроек группы и потоков.*/
  if (!argp.custom_g_rate){
    switch (argp.speed_type) {
      case 5:
        n.group_rate = 100;
        break;
      case 4:
        n.group_rate = 50;
        break;
      case 3:
        n.group_rate = 20;
        break;
      case 2:
        n.group_rate = 10;
        break;
      case 1:
        n.group_rate = 5;
        break;
    }
  }
  if (!argp.custom_g_max)
    n.max_group_size = temp_vector.size();

  if (argp.my_life_my_rulez)
    n.group_rate = 1000;

  /*Потоки для сканирования портов.*/
  argp._threads = n.max_group_size;

  long long size = temp_vector.size();
  std::vector<std::future<int>> futures;

  if (argp.json_save)
    nesca_json_start_array(argp.json_save_path);

  /*Создание пула потоков.*/
  thread_pool pool(argp._threads);

  /*Сканирование по группам*/
  int group_start = 0, ip_count = 0;
  bool first = false;

  while (group_start < size) {
    auto start_time_scan = std::chrono::high_resolution_clock::now();
    int group_end = (group_start + static_cast<int>(n.group_size) <
        static_cast<int>(size)) ? group_start + static_cast<int>(n.group_size) : static_cast<int>(size);

    n.create_group();

    /*Сканирование текущей группы*/
    for (const auto& ip : n.current_group) {
      int log_set;
      ip_count++;

      if (argp.custom_log_set)
        log_set = argp.log_set;
      else
        log_set = n.group_size;

      if (ip_count % log_set == 0 && !first) {
        first = true;
        std::cout << std::endl;
      }

      if (ip_count % log_set == 0) {
        double procents = (static_cast<double>(ip_count) / size) * 100;
        std::string result = format_percentage(procents);

        std::cout << np.main_nesca_out("#", "SCAN "+std::to_string(ip_count)+" out of "+
        std::to_string(size) + " IPs", 6, "", "", result+"%", "", "") << std::endl;

        std::cout << np.main_nesca_out("# rate", "GROUP "+std::to_string(n.group_size)+" out of "+
        std::to_string(n.max_group_size), 6, "", "", std::to_string(n.group_rate), "", "") << std::endl << std::endl;
      }

      auto fut = pool.enqueue(nesca_scan, ip, argp.ports, argp.timeout_ms);
      futures.push_back(std::move(fut));

      if (futures.size() >= static_cast<std::vector<std::future<int>>::size_type>(argp._threads)) {
        futures.front().get();
        futures.erase(futures.begin());
      }
    }
    for (auto& fut : futures)
      fut.wait();
    for (auto& fut : futures)
      fut.get();

    futures.clear();

    /*Увелечение группы.*/
    n.increase_group();
    group_start = group_end;

    auto end_time_scan = std::chrono::high_resolution_clock::now();
    auto duration_scan = std::chrono::duration_cast<std::chrono::microseconds>(end_time_scan - start_time_scan);
    argp.scan_duration += duration_scan.count() / 1000000.0;
    auto start_time_proc = std::chrono::high_resolution_clock::now();

    /*Обработка результатов для текущей группы*/
    for (const auto& ip : n.current_group) {
      if (n.find_port_status(ip, PORT_OPEN)
        || argp.debug || n.find_port_status(ip, PORT_OPEN_OR_FILTER)
        || n.find_port_status(ip, PORT_NO_FILTER)){

        std::cout << np.main_nesca_out("READY", ip, 5, "rDNS", "RTT", n.get_new_dns(ip), std::to_string(n.get_rtt(ip))+"ms","") << std::endl;

        if (argp.json_save) {
          nesca_host_details nhd;
          nhd.ip_address = ip.c_str();
          nhd.rtt = n.get_rtt(ip);
          nhd.dns_name = n.get_new_dns(ip).c_str();
          nesca_json_save_host(argp.json_save_path, &nhd);
        }

        process_port(ip, n.get_port_list(ip, PORT_OPEN), PORT_OPEN);
        process_port(ip, n.get_port_list(ip, PORT_FILTER), PORT_FILTER);
        process_port(ip, n.get_port_list(ip, PORT_OPEN_OR_FILTER), PORT_OPEN_OR_FILTER);
        process_port(ip, n.get_port_list(ip, PORT_NO_FILTER), PORT_NO_FILTER);
        process_port(ip, n.get_port_list(ip, PORT_CLOSED), PORT_CLOSED);
        process_port(ip, n.get_port_list(ip, PORT_ERROR), PORT_ERROR);

        std::cout << std::endl;

        if (argp.json_save) {
          nesca_json_close_info(argp.json_save_path);
          nesca_json_set_comma(argp.json_save_path);
          nesca_json_skip_line(argp.json_save_path);
        }
      }
    }

    n.clean_ports();
    n.clean_group();

    auto end_time_proc = std::chrono::high_resolution_clock::now();
    auto duration_proc = std::chrono::duration_cast<std::chrono::microseconds>(end_time_proc - start_time_proc);
    argp.proc_duration += duration_proc.count() / 1000000.0;
  }

  if (argp.json_save) {
    nesca_json_fix_file(argp.json_save_path);
    nesca_json_skip_line(argp.json_save_path);
    nesca_json_close_array(argp.json_save_path);
  }

  double elapsed_result = argp.ping_duration+argp.dns_duration+argp.scan_duration+argp.proc_duration;
  nescaend(count_success_ips, elapsed_result);

  return 0;
}

void nescaend(int success, double res)
{
  np.golder_rod_on();
  std::cout << "-> NESCA finished " << success << " up IPs (success) in " << std::fixed << std::setprecision(2) << res << " seconds\n";
  reset_colors;
}

/* Главная функция для сканировния портов */
int nesca_scan(const std::string& ip, std::vector<int>ports, const int timeout_ms)
{
  u32 seq, saddr, daddr, datalen = 0;
  int source_port, res, sock, recvtime = 600;
  u8* buffer;
  u16 ttl;
  double rtt_ping;
  u8 portstat = PORT_ERROR;
  const char* data;
  bool df = true;

  if (argp.frag_mtu)
    df = false;

  saddr = inet_addr(argp.source_ip);
  daddr = inet_addr(ip.c_str());
  seq = generate_seq();

  if (argp.custom_source_port)
    source_port = argp._custom_source_port;
  else
    source_port = generate_rare_port();

  if (argp.custom_recv_timeout_ms)
    recvtime = argp.recv_timeout_ms;
  else {
    rtt_ping = n.get_rtt(ip);
    if (rtt_ping != -1)
      recvtime = calculate_timeout(rtt_ping, argp.speed_type);
  }

  if (!argp.data_string.empty()) {
    data = argp.data_string.c_str();
    datalen = strlen(data);
  }

  for (const auto& port : ports) {
    sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (sock == -1)
      continue;

    if (argp.custom_ttl)
      ttl = argp._custom_ttl;
    else
      ttl = random_num(29, 255);

    res = send_tcp_packet(sock, saddr, daddr, ttl, df, 0, 0, source_port, port, seq, 0, 0, argp.tcpflags, 1024, 0, 0, 0, data, datalen, argp.frag_mtu);

    ls.lock();
    close(sock);
    ls.unlock();

    if (res == -1) {
      ls.lock();
      if (argp.print_errors)
        n.add_port(ip, port, PORT_ERROR);
      ls.unlock();
      continue;
    }

    ls.lock();
    buffer = (u8*)calloc(RECV_BUFFER_SIZE, sizeof(u8));
    ls.unlock();

    rf.dest_ip = ip.c_str();
    rf.protocol = IPPROTO_TCP;

    res = read_packet(&rf, recvtime, &buffer);

    if (res == -1) {
      ls.lock();
      free(buffer);
      ls.unlock();

      if (argp.type != SYN_SCAN && argp.type != ACK_SCAN && argp.type != WINDOW_SCAN) {
        ls.lock();
        n.add_port(ip, port, PORT_OPEN_OR_FILTER);
        ls.unlock();
      }
      else {
        ls.lock();
        if (argp.debug)
          n.add_port(ip, port, PORT_FILTER);
        ls.unlock();
      }
      continue;
    }

    portstat = get_port_status(buffer, argp.type);

    ls.lock();
    free(buffer);
    n.add_port(ip, port, portstat);
    if (portstat)
      argp.count_success_ports++;
    ls.unlock();
  }

  return 0;
}

/* Главная функция для пинг сканирования */
bool nesca_ping(const char* ip)
{
  double rtt;
  u8 ttl = random_num(29, 255);
  u16 source_port = generate_rare_port();
  u32 datalen = 0;
  const char* data = "";

  if (!argp.data_string.empty()) {
    data = argp.data_string.c_str();
    datalen = strlen(argp.data_string.c_str());
  }

  if (argp.custom_source_port)
    source_port = argp._custom_source_port;

  if (argp.custom_ttl)
    ttl = argp._custom_ttl;

  if (argp.echo_ping) {
    rtt = icmp_ping(ip, argp.source_ip, argp.ping_timeout, ICMP_ECHO, 0, 1, ttl, data, datalen, argp.frag_mtu);
    if (rtt != -1)
      goto ok;
  }
  if (argp.syn_ping) {
    rtt = tcp_ping(SYN_PACKET, ip, argp.source_ip, argp.syn_dest_port, source_port, argp.ping_timeout, ttl, data,
        datalen, argp.frag_mtu);
    if (rtt != -1)
      goto ok;
  }
  if (argp.ack_ping) {
    rtt = tcp_ping(ACK_PACKET, ip, argp.source_ip, argp.syn_dest_port, source_port, argp.ping_timeout, ttl, data,
        datalen, argp.frag_mtu);
    if (rtt != -1)
      goto ok;
  }
  if (argp.info_ping) {
    rtt = icmp_ping(ip, argp.source_ip, argp.ping_timeout, ICMP_INFO_REQUEST, 0, 1, ttl, data, datalen, argp.frag_mtu);
    if (rtt != -1)
      goto ok;
  }
  if (argp.timestamp_ping) {
    rtt = icmp_ping(ip, argp.source_ip, argp.ping_timeout, ICMP_TIMESTAMP, 0, 1, ttl, data, datalen, argp.frag_mtu);
    if (rtt != -1)
      goto ok;
  }

  return 0;
ok:
  n.set_rtt(ip, rtt);
  return 1;
}

void nesca_ddos(u8 proto, u8 type, const u32 daddr, const u32 saddr, const int port, bool ip_ddos)
{
  u32 seq, datalen = 0;
  int source_port, sock;
  u16 ttl;
  const char* data;
  bool df = true;

  if (argp.frag_mtu)
    df = false;

  if (argp.icmp_ddos || argp.tcp_ddos)
    seq = generate_seq();

  if (proto == IPPROTO_TCP || proto == IPPROTO_UDP) {
    if (argp.custom_source_port)
      source_port = argp._custom_source_port;
    else
      source_port = generate_rare_port();
  }

  if (!argp.data_string.empty()) {
    data = argp.data_string.c_str();
    datalen = strlen(data);
  }

  if (argp.custom_ttl)
    ttl = argp._custom_ttl;
  else
    ttl = random_num(29, 255);

  sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
  if (sock == -1)
    return;

  if (proto == IPPROTO_TCP)
    send_tcp_packet(sock, saddr, daddr, ttl, df, 0, 0, source_port, port, seq, 0, 0, argp.tcpflags, 1024,
        0, 0, 0, data, datalen, argp.frag_mtu);
  else if (proto == IPPROTO_ICMP)
    send_icmp_packet(sock, saddr, daddr, ttl, df, 0, 0, seq, 0, type, data, datalen, argp.frag_mtu);
  else if (proto == IPPROTO_UDP)
    send_udp_packet(sock, saddr, daddr, ttl, generate_ident(), NULL, 0, source_port, port, df, data, datalen, argp.frag_mtu);
  else if (ip_ddos)
    send_ip_empty(sock, saddr, daddr, ttl, proto, df, 0, 0, NULL, 0, argp.frag_mtu);

  ls.lock();
  close(sock);
  ls.unlock();
}

void process_port(const std::string& ip, std::vector<uint16_t> ports, int port_type)
{
  int total_ports_to_process = ports.size();
  int port_count_on_this_ip = 0;

  for (const auto& port : ports) {
    processing_tcp_scan_ports(ip, port, port_type);

    if (port_count_on_this_ip != total_ports_to_process - 1 && argp.json_save) {
      nesca_json_set_comma(argp.json_save_path);
      nesca_json_skip_line(argp.json_save_path);
    }
    port_count_on_this_ip++;
  }
}

void fix_time(double time)
{
  std::cout << std::fixed << std::setprecision(2) << time << "s";
}

std::vector<std::string> resolv_hosts(std::vector<std::string> hosts)
{
  char ipbuf[1024];
  std::vector<std::string> result;
  int temp;
  char* clean;

  for (const auto& t : hosts) {
    temp = this_is(t.c_str());
    if (temp == CIDR) {
      std::vector<std::string> temp = cidr_to_ips({t});
      for (auto& tt : temp) {
        n.add_ip(tt);
        result.push_back(tt);
      }
    }
    if (temp == RANGE) {
      std::vector<std::string> temp = range_to_ips({t});
      for (auto& tt : temp) {
        n.add_ip(tt);
        result.push_back(tt);
      }
    }
    if (temp == _URL_) {
      clean = clean_url(t.c_str());
      get_ip(clean, ipbuf, sizeof(ipbuf)); result.push_back(ipbuf);
      n.add_ip(ipbuf);
      n.set_dns(ipbuf, clean);
      if (clean)
        free(clean);
      memset(ipbuf, 0, 1024);
    }
    if (temp == DNS) {
      get_ip(t.c_str(), ipbuf, sizeof(ipbuf)); result.push_back(ipbuf);
      n.add_ip(ipbuf);
      n.set_dns(ipbuf, t);
    }
    else {
      n.add_ip(t);
      result.push_back(t);
    }
  }
  return result;
}

void importfile(void)
{
  if (!argp.ip_scan_import)
    return;

  if (check_file(argp.path_ips)) {
    np.golder_rod_on();
    std::cout << "-> Import file (" + std::string(argp.path_ips) + ") loaded " + std::to_string(get_count_lines(argp.path_ips)) + " entries\n";
    reset_colors;
  }
  else {
    np.nlog_error("Failed to import file, check path (" + std::string(argp.path_ips) + ")\n");
    exit(1);
  }

  std::vector<std::string> temp_ips = write_file(argp.path_ips);
  resolv_hosts(temp_ips);
}

void
processing_tcp_scan_ports(std::string ip, int port, int result)
{
  argp.result_success_ports++;
  argp.result_success_ip++;
  std::stringstream stream;
  stream << std::fixed << std::setprecision(2) << n.get_rtt(ip);
  std::string rtt_log = stream.str(); std::string protocol = sn.probe_service(port);
  std::string result1 = ip + ":" + std::to_string(port);

  /*Класс с обработками.*/
  std::unique_ptr<ports_strategy> ports_strategy_;

  /*Открытый порт.*/
  if (result == PORT_OPEN) {
    if (argp.no_proc) {
      print_port_state(PORT_OPEN, port, sn.probe_service(port), np);
      return;
    }
    if (sn.probe_service(port) == "HTTP") {
      print_port_state(PORT_OPEN, port, "HTTP", np);
      ports_strategy_ = std::make_unique<http_strategy>();
    }
    else if (port == 20 || port == 21) {
      print_port_state(PORT_OPEN, port, "FTP", np);
      ports_strategy_ = std::make_unique<ftp_strategy>();
    }
    else if (port == 554) {
      print_port_state(PORT_OPEN, port, "RTSP", np);
      ports_strategy_ = std::make_unique<rtsp_strategy>();
    }
    else if (port == 37777) {
      print_port_state(PORT_OPEN, port, "RVI", np);
      ports_strategy_ = std::make_unique<rvi_strategy>();
    }
    else if (port == 8000) {
      print_port_state(PORT_OPEN, port, "HIKVISION", np);
      ports_strategy_ = std::make_unique<hikvision_strategy>();
    }
    else if (port == 443) {
      print_port_state(PORT_OPEN, port, "HTTPS", np);
      ports_strategy_ = std::make_unique<https_strategy>();
    }
    else if (port == 25) {
      print_port_state(PORT_OPEN, port, "SMTP", np);
      ports_strategy_ = std::make_unique<smtp_strategy>();
    }
    else {
      print_port_state(PORT_OPEN, port, sn.probe_service(port), np);
      ports_strategy_ = std::make_unique<else_strategy>();
    }
    /*Запуск стратегий.*/
    if (ports_strategy_)
      ports_strategy_->handle(ip, result1, rtt_log, protocol, port, argp, np, n, sn);

    if (argp.json_save) {
      nesca_port_details npd;
      npd.port = port;
      npd.protocol = protocol.c_str();
      npd.http_title = ports_strategy_->http_title.c_str();
      npd.screenshot = ports_strategy_->screenshot_base64.c_str();
      npd.content = "";
      npd.passwd = ports_strategy_->brute_temp.c_str();
      npd.type_target = ports_strategy_->type_target.c_str();
      nesca_json_save_port(argp.json_save_path, &npd);
    }
  }

  /*Ошибочный порт.*/
  else if (result == PORT_ERROR) {
    if (argp.print_errors)
      print_port_state(PORT_ERROR, port, sn.probe_service(port), np);
  }
  /*Закрытый порт.*/
  else if (result == PORT_CLOSED) {
    if (argp.debug)
      print_port_state(PORT_CLOSED, port, sn.probe_service(port), np);
  }
  /*Фильтруемый порт.*/
  else if (result == PORT_FILTER) {
    if (argp.debug)
      print_port_state(PORT_FILTER, port, sn.probe_service(port), np);
  }
  /*Открыт или фильтруеться.*/
  else if (result == PORT_OPEN_OR_FILTER)
    print_port_state(PORT_OPEN_OR_FILTER, port, sn.probe_service(port), np);

  /*Не фильтруеться.*/
  else if (result == PORT_NO_FILTER)
    print_port_state(PORT_NO_FILTER, port, sn.probe_service(port), np);
}

void usage(void)
{
  puts("d8b   db d88888b .d8888.  .o88b.  .d8b.         j88D ");
  puts("888o  88 88'     88'  YP d8P  Y8 d8' `8b       j8~88 "); 
  puts("88V8o 88 88ooooo `8bo.   8P      88ooo88      j8' 88 ");
  puts("88 V8o88 88~~~~~   `Y8b. 8b      88~~~88      V88888D");
  puts("88  V888 88.     db   8D Y8b  d8 88   88          88 ");
  puts("VP   V8P Y88888P `8888Y'  `Y88P' YP   YP          VP \n");

  np.gray_nesca_on();
  std::cout << "[VERSION]:";
  np.green_html_on();
  std::cout << _VERSION << std::endl;
  np.gray_nesca_on();

  np.gray_nesca_on();
  std::cout << "[USAGE]:";
  np.green_html_on();
  std::cout << run << " [target(s) 1,2,3] [flags]" << std::endl;
  np.gray_nesca_on();

  np.golder_rod_on();
  std::cout << "\nTARGET SPECIFICATION:" << std::endl;
  reset_colors;
  std::cout << "  -import <inputfilename>: Set target(s) from file.\n";
  std::cout << "  -random-ip <num hosts>: Choose random target(s)\n";
  std::cout << "  -exclude <host1[,host2][,host3],...>: Exclude host(s).\n";
  std::cout << "  -excludefile <exclude_file>: Exclude list from file\n";
  np.golder_rod_on();
  std::cout << "PORT SCAN OPTIONS:" << std::endl;
  reset_colors;
  std::cout << "  -fin, -xmas, -null: Use one of these scanning methods.\n";
  std::cout << "  -ack, -windows -maimon: Use ack or window or maimon scan method.\n";
  std::cout << "  -scanflags <flags>: Customize TCP scan flag (ACKSYN).\n";
  std::cout << "  -p <port ranges>: Only scan specified port(s) \n    Ex: -p 80; -p 22,80; -p 1-65535;\n";
  std::cout << "  -delay, -d <ms>: Set delay for scan.\n";
  std::cout << "  -scan-timeout <ms>: Edit timeout for getting packet on port.\n";
  std::cout << "  -no-scan: Disable port scan(skip).\n";
  np.golder_rod_on();
  std::cout << "PING SCAN OPTIONS:" << std::endl;
  reset_colors;
  std::cout << "  -TP <num>: Set max thread(s) for ping.\n";
  std::cout << "  -ping-timeout <ms>: Set recv timeout for ping.\n";
  std::cout << "  -PS, -PA <port>: On TCP ping SYN|ACK and edit dest port.\n";
  std::cout << "  -PE, -PI, -PM: On ICMP ping ECHO|INFO|TIMESTAMP\n";
  std::cout << "  -max-ping: Using all ping methods ICMP and TCP.\n";
  std::cout << "  -no-ping: Skip ping scan.\n";
  np.golder_rod_on();
  std::cout << "HOST RESOLUTION:" << std::endl;
  reset_colors;
  std::cout << "  -TD <num>: Set max thread(s) for dns-resolution.\n";
  std::cout << "  -resol-port <port>: Edit source port for dns-resolution.\n";
  std::cout << "  -resol-delay <ms>: Set delay for dns-resolution.\n";
  std::cout << "  -no-resolv: Skip dns-resolution.\n";
  np.golder_rod_on();
  std::cout << "BRUTEFORCE OPTIONS:" << std::endl;
  reset_colors;
  std::cout << "  -brute-login <ss,path>: Set path for <ss> logins.\n";
  std::cout << "  -brute-pass <ss,path>: Set path for <ss> passwords.\n";
  std::cout << "  -brute-timeout <ms>: Edit brute timout.\n";
  std::cout << "  -no-brute <ss,2>: Disable <ss> bruteforce.\n";
  std::cout << "  -brute-verbose <ss,2>: Display bruteforce <ss> all info.\n";
  std::cout << "  -brute-log <ss,2>: Display bruteforce <ss> info.\n";
  np.golder_rod_on();
  std::cout << "OTHER OPTIONS:" << std::endl;
  reset_colors;
  std::cout << "  -negatives <path>: Set custom path for negatives.\n";
  std::cout << "  -source-ip <ip>: Set custom source_ip.\n";
  std::cout << "  -source-port <port>: Set custom source_port.\n";
  std::cout << "  -data-string <string>: Append a custom ASCII string to packets.\n";
  std::cout << "  -data-len <num>: Append random data to sent packets.\n";
  std::cout << "  -frag <mtu>: fragment all packets.\n";
  std::cout << "  -ttl <num>: Set custom ip_header_ttl.\n";
  np.golder_rod_on();
  std::cout << "NETWORK TEST/DDOS:" << std::endl;
  reset_colors;
  std::cout << "  -TDD: Set max thread(s) for testing.\n";
  std::cout << "  -ip <proto>: Set a IP protocol for send, and enable test mode.\n";
  std::cout << "  -icmp <type>: Set a ICMP protocol for send, and enable test mode. \n    Ex types: ECHO=8, INFO=15, TIMESTAMP=13\n";
  std::cout << "  -tcp: Set a TCP protocol for send, and enable test mode.\n";
  std::cout << "  -udp: Set a UDP protocol for send, and enable test mode.\n";
  std::cout << "  -count: Set the number of packages for the host.\n";
  np.golder_rod_on();
  std::cout << "PORT SCAN GROUPS:" << std::endl;
  reset_colors;
  std::cout << "  -max-group <num>: Edit max size group & threads for port scan.\n";
  std::cout << "  -min-group <num>: Edit min size group & threads for port scan.\n";
  std::cout << "  -rate-group <num>: Edit the value by which the group is incremented.\n";
  np.golder_rod_on();
  std::cout << "DELISEARCH SCAN:" << std::endl;
  reset_colors;
  std::cout << "  -find <target1[,target2][,target3],...>: Search for keywords on the host.\n";
#ifdef HAVE_NODE_JS
  np.golder_rod_on();
  std::cout << "SAVE SCREENSHOTS:" << std::endl;
  reset_colors;
  std::cout << "  -screenshot, -s <folder>: Save screenshot on pages.\n";
  std::cout << "  -ss-timeout <ms>: Set timeout on save screenshots.\n";
#endif
  np.golder_rod_on();
  std::cout << "SPEED OPTIONS:" << std::endl;
  reset_colors;
  std::cout << "  -speed, -S <1-5>: Set timing template (higher is faster).\n";
  std::cout << "  -my-life-my-rulez: Using very MAX speed settings.\n";
  np.golder_rod_on();
  std::cout << "SAVE OUTPUT:" << std::endl;
  reset_colors;
  std::cout << "  -html, -l <path file>: Classic nesca save, write on html page.\n";
  std::cout << "  -json <path file>: Save on json file.\n";
  np.golder_rod_on();
  std::cout << "VERBOSE OUTPUT:" << std::endl;
  reset_colors;
  std::cout << "  -db, -debug: On debug mode, save and display not even working hosts.\n";
  std::cout << "  -er, -error: On display errors.\n";
  np.golder_rod_on();
  std::cout << "PRINT OUTPUT:" << std::endl;
  reset_colors;
  std::cout << "  -no-proc: Skip main processing.\n";
  std::cout << "  -no-get-path: Disable getting paths.\n";
  std::cout << "  -log-set <num>: Change the value of ips after which % will be output.\n";
  std::cout << "  -http-response: Display HTTP response.\n";
  std::cout << "  -sitemap: Get /sitemap.xml.\n";
  std::cout << "  -robots: Get /robots.txt.\n";
  np.golder_rod_on();
  std::cout << "COLOR:" << std::endl;
  reset_colors;
  std::cout << "  -no-color: Disable all colors in nesca4.\n";
  std::cout << "  -import-color <path>: Import color scheme from file.\n";
  std::cout << "  -print-color <path>: Check color scheme.\n";
  np.golder_rod_on();
  std::cout << "EXAMPLES:" << std::endl;
  reset_colors;
  std::cout << "  ./nesca4 google.com -p 80,443\n";
  std::cout << "  ./nesca4 72.230.205.0/24 -p 80,8080,81 -S5\n";
  std::cout << "  ./nesca4 https://www.youtube.com\n";
  exit(0);
}

void init_bruteforce(void)
{
  argp.ftp_logins          = write_file(argp.path_ftp_login);
  argp.ftp_passwords       = write_file(argp.path_ftp_pass);
  argp.rtsp_logins         = write_file(argp.path_rtsp_login);
  argp.rtsp_passwords      = write_file(argp.path_rtsp_pass);
  argp.http_logins         = write_file(argp.path_http_login);
  argp.http_passwords      = write_file(argp.path_http_pass);
  argp.hikvision_logins    = write_file(argp.path_hikvision_login);
  argp.hikvision_passwords = write_file(argp.path_hikvision_pass);
  argp.smtp_logins         = write_file(argp.path_smtp_login);
  argp.smtp_passwords      = write_file(argp.path_smtp_pass);
  argp.rvi_logins          = write_file(argp.path_rvi_login);
  argp.rvi_passwords       = write_file(argp.path_rvi_pass);
}

std::string format_percentage(double procents)
{
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(1) << procents << "%";
  std::string result = oss.str();
  return result;
}

void get_dns_thread(std::string ip)
{
  char dnsbuf[1024];
  std::string temp_dns;

  delayy(argp.resol_delay);
  get_dns(ip.c_str(), argp.resol_source_port, dnsbuf, sizeof(dnsbuf));
  temp_dns = dnsbuf;

  ls.lock();
  n.set_new_dns(ip, temp_dns);
  ls.unlock();
}

void
pre_check(void)
{
  if (check_ansi_support() != true) {
    std::cout << "You terminal don`t support ansi colors!\n";
    std::cout << "Colors disable!\n\n";
    np.disable_colors();
  }

  if (argp.import_color_scheme) {
    np.import_color_scheme(argp.path_color_scheme, np.config_values);
    np.processing_color_scheme(np.config_values);
  }

  char formatted_date[11];
  np.golder_rod_on();
  get_current_date(formatted_date, sizeof(formatted_date));
  std::cout << "-> Running NESCA [v" + std::string(_VERSION) + "] # " +
  std::string(get_time()) + " at " + formatted_date << std::endl; 
  reset_colors;

  if (argp.info_version)
    version_menu();

  if (argp.print_help_menu)
    usage();

  if (!check_root_perms()) {
    np.nlog_error("UNIX requires root permissions, to use raw sockets (sudo).\n");
    exit(1);
  }

  if (argp.ns_track) {
    std::filesystem::path directory_path= "ns_track";
    if (!std::filesystem::exists(directory_path))
      std::filesystem::create_directory(directory_path);

    argp.json_save = true;
    argp.save_screenshots = true;
    argp.json_save_path = "ns_track/data.json";
    argp.screenshots_save_path = "ns_track/";
  }

  if (np.html_save) {
    std::vector<std::string> temp = write_file("resources/data_html");
    auto it = std::find(temp.begin(), temp.end(), np.html_file_path);
    if (it == temp.end() || temp[0] == "-1")
      ho.html_main_init(np.html_file_path);
    else {
      if (!check_file(np.html_file_path.c_str()))
        ho.html_main_init(np.html_file_path);
    }
    ho.html_pre_init(np.html_file_path);
    write_temp(np.html_file_path, "resources/data_html");
  }

  if (argp.json_save) {
    std::vector<std::string> temp = write_file("resources/data_json");
    auto it = std::find(temp.begin(), temp.end(), argp.json_save_path);
    if (it != temp.end() && temp[0] != "-1") {
      if (check_file(argp.json_save_path)) {
        nesca_json_set_comma(argp.json_save_path);
        nesca_json_skip_line(argp.json_save_path);
      }
    }
    write_temp(argp.json_save_path, "resources/data_json");
  }

  if (!check_file("./resources/nesca-services"))
    std::cout << np.main_nesca_out("NESCA4", "SERVICES_DATA", 5, "status", "", "FAILED","","") << std::endl;
  else
    sn.init_services();

  if (!check_file(argp.negatives_path.c_str()))
    std::cout << np.main_nesca_out("NESCA4", "NEGATIVES_LOAD", 5, "status", "", "FAILED","","") << std::endl;
  else
    argp.nesca_negatives = get_negatives(argp.negatives_path);
}

void version_menu(void)
{
  logo();
  np.gray_nesca_on();
  std::cout << print_get_time(get_time());
  std::cout << "[VERSION]:";
  np.green_html_on();
  std::cout << _VERSION << std::endl;
  np.gray_nesca_on();
  std::cout << print_get_time(get_time());
  std::cout << "[INFO]:";
  np.sea_green_on();
  std::cout << "https://github.com/oldteamhost/nesca4" << std::endl;
  reset_colors;
  np.gray_nesca_on();
  std::cout << print_get_time(get_time());
  std::cout << "[NB]:";
  np.golder_rod_on();
  std::cout << "Don`t read \"do_not_read.txt\"" << std::endl;
  reset_colors;
  std::cout << std::endl;
  exit(0);
}

int count_map_vector(const std::unordered_map<std::string, std::vector<int>>& map, const std::string& key)
{
  auto it = map.find(key);
  if (it != map.end())
    return it->second.size();

  return 0;
}

std::string randomstr(int len)
{
  int i;
  std::string res;
  const std::string characters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  const int chlen = characters.length(); /* член */

  std::srand(static_cast<unsigned int>(std::time(0)));
  for (i = 0; i < len; ++i) {
    char _char = characters[std::rand() % chlen];
    res += _char;
  }
  return res;
}

void parse_args(int argc, char** argv)
{
  int rez;
  int option_index = 0;

  while ((rez = getopt_long_only(argc, argv, short_options, long_options, &option_index)) != EOF) {
    switch (rez)
    {
      case 'h':
        argp.print_help_menu = true;
        break;
      case 'v':
        argp.info_version = true;
        break;
      case 'S':
        argp.speed_type = atoi(optarg);
        break;
      case 'p': {
        argp.ports_temp = optarg;
        argp.ports = write_ports(argp.ports_temp);
        if (argp.ports[0] == EOF) {
          size_t pos1 = argp.ports_temp.find("-");
          if (pos1 != std::string::npos) {
            argp.ports = parse_range(optarg);
          }
          else {
            argp.ports = split_string_int(optarg, ',');
          }
        }
        break;
      }
      case 12: {
        std::vector<std::string> what = split_string_string(optarg, ',');
        what[0] = to_lower_case(what[0]);
        what[1] = to_lower_case(what[1]);

        const char* what_convert = what[0].c_str();
        if (what[1] == "ftp") {
          argp.path_ftp_login = what_convert;
        }
        else if (what[1] == "rtsp") {
          argp.path_rtsp_login = what_convert;
        }
        else if (what[1] == "http") {
          argp.path_http_login = what_convert;
        }
        else if (what[1] == "smtp") {
          argp.path_smtp_login = what_convert;
        }
        else if (what[1] == "hikvision") {
          argp.path_hikvision_login = what_convert;
        }
        else if (what[1] == "rvi") {
          argp.path_rvi_login = what_convert;
        }
        else if (what[1] == "all") {
          argp.path_rvi_login = what_convert;
          argp.path_ftp_login = what_convert;
          argp.path_http_login = what_convert;
          argp.path_rtsp_login = what_convert;
          argp.path_smtp_login = what_convert;
          argp.path_hikvision_login = what_convert;
        }
        break;
      }
      case 11: {
        std::vector<std::string> what = split_string_string(optarg, ',');
        what[0] = to_lower_case(what[0]);
        what[1] = to_lower_case(what[1]);

        const char* what_convert = what[0].c_str();
        if (what[1] == "ftp") {
          argp.path_ftp_pass = what_convert;
        }
        else if (what[1] == "rtsp") {
          argp.path_rtsp_pass = what_convert;
        }
        else if (what[1] == "http") {
          argp.path_http_pass = what_convert;
        }
        else if (what[1] == "smtp") {
          argp.path_smtp_pass = what_convert;
        }
        else if (what[1] == "hikvision") {
          argp.path_hikvision_pass = what_convert;
        }
        else if (what[1] == "rvi") {
          argp.path_rvi_pass = what_convert;
        }
        else if (what[1] == "all") {
          argp.path_rvi_pass = what_convert;
          argp.path_ftp_pass = what_convert;
          argp.path_smtp_pass = what_convert;
          argp.path_rtsp_pass = what_convert;
          argp.path_http_pass = what_convert;
          argp.path_hikvision_pass = what_convert;
        }
        break;
      }
      case 30: {
        std::vector<std::string> what = split_string_string(optarg, ',');
        for (int i = 0; i < static_cast<int>(what.size()); i++) {
          what[i] = to_lower_case(what[i]);
          if (what[i] == "ftp") {
            argp.ftp_brute_log = true;
          }
          else if (what[i] == "rtsp") {
            argp.rtsp_brute_log = true;
          }
          else if (what[i] == "http") {
            argp.http_brute_log = true;
          }
          else if (what[i] == "hikvision") {
            argp.hikvision_brute_log = true;
          }
          else if (what[i] == "smtp") {
            argp.smtp_brute_log = true;
          }
          else if (what[i] == "rvi") {
            argp.rvi_brute_log = true;
          }
          else if (what[i] == "all") {
            argp.ftp_brute_log = true;
            argp.smtp_brute_log = true;
            argp.rvi_brute_log = true;
            argp.rtsp_brute_log = true;
            argp.http_brute_log = true;
            argp.hikvision_brute_log = true;
          }
        }
        break;
      }
      case 31: {
        std::vector<std::string> what = split_string_string(optarg, ',');
        for (int i = 0; i < static_cast<int>(what.size()); i++) {
          what[i] = to_lower_case(what[i]);
          if (what[i] == "ftp") {
            argp.ftp_brute_verbose = true;
          }
          else if (what[i] == "rtsp") {
            argp.rtsp_brute_verbose = true;
          }
          else if (what[i] == "http") {
            argp.http_brute_verbose = true;
          }
          else if (what[i] == "smtp") {
            argp.smtp_brute_verbose = true;
          }
          else if (what[i] == "all") {
            argp.ftp_brute_verbose = true;
            argp.smtp_brute_verbose = true;
            argp.rtsp_brute_verbose = true;
            argp.http_brute_verbose = true;
          }
        }
        break;
      }
      case 44: {
        std::vector<std::string> what = split_string_string(optarg, ',');

        for (int i = 0; i < static_cast<int>(what.size()); i++) {
          what[i] = to_lower_case(what[i]);
          if (what[i] == "ftp") {
            argp.off_ftp_brute = true;
          }
          else if (what[i] == "rtsp") {
            argp.off_rtsp_brute = true;
          }
          else if (what[i] == "http") {
            argp.off_http_brute = true;
          }
          else if (what[i] == "hikvision") {
            argp.off_hikvision_brute = true;
          }
          else if (what[i] == "smtp") {
            argp.off_smtp_brute = true;
          }
          else if (what[i] == "rvi") {
            argp.off_rvi_brute = true;
          }
          else if (what[i] == "all") {
            argp.off_ftp_brute = true;
            argp.off_rvi_brute = true;
            argp.off_smtp_brute = true;
            argp.off_rtsp_brute = true;
            argp.off_http_brute = true;
            argp.off_hikvision_brute = true;
          }
        }
        break;
      }
      case 47:
        argp.brute_timeout_ms = atoi(optarg);
        break;
      case 48:
        argp.thread_on_port = true;
        break;
      case 5:
        argp.random_ip = true;
        argp.random_ip_count = atoi(optarg);
        break;
      case 7:
        argp.debug = true;
        break;
      case 'd':
        argp.timeout = true;
        argp.timeout_ms = atoi(optarg);
        break;
      case 19:
        argp.find = true;
        argp.find_target = split_string_string(optarg, ',');
        break;
      case 20:
        argp.frag_mtu = atoi(optarg);
        if (argp.frag_mtu >! 0 && argp.frag_mtu % 8 != 0) {
          np.nlog_error("Data payload MTU must be > 0 and multiple of 8: (8,16,32,64)\n");
          exit(1);
        }
        break;
      case 21:
        argp.custom_tcpflags = true;
        argp.custom_res_tcpflags = optarg;
        break;
      case 79:
      {
        u32 datalen = atoi(optarg);
        argp.data_string = randomstr(datalen);
        break;
      }
      case 23:
        argp.ip_scan_import = true;
        argp.path_ips = optarg;
        break;
      case 15:
        argp.tcp_ddos = true;
        break;
      case 16:
        argp.icmp_ddos = true;
        argp.icmp_ddos_type = atoi(optarg);
        break;
      case 24:
        argp.custom_log_set = true;
        argp.log_set = atoi(optarg);
        break;
      case 25:
        argp.print_errors = true;
        break;
      case 26:
        np.disable_colors();
        break;
      case 27:
        argp.debug = true;
        break;
      case 28:
        argp.print_errors = true;
        break;
      case 29:
        argp.ping_off = true;
        break;
      case 34:
        argp.source_ip = optarg;
        break;
      case 35:
        argp.save_camera_screens = true;
        argp.screenshots_save_path_cam = optarg;
        break;
      case 36:
        argp.custom_source_port = true;
        argp._custom_source_port = atoi(optarg);
        break;
      case 37:
        argp.custom_ttl = true;
        argp._custom_ttl = atoi(optarg);
        break;
      case 38:
        argp.custom_g_max = true;
        n.max_group_size = atoi(optarg);
        break;
#ifdef HAVE_NODE_JS
      case 's':
        argp.save_screenshots = true;
        argp.screenshots_save_path = optarg;
        break;
      case 41:
        argp.timeout_save_screenshots = atoi(optarg);
        break;
      case 39:
        argp.ns_track = true;
        break;
#endif
      case 45:
        argp.data_string = optarg;
        break;
      case 60:
        argp.custom_g_min = true;
        n.group_size = atoi(optarg);
        break;
      case 61:
        argp.custom_g_rate = true;
        n.group_rate = atoi(optarg);
        break;
      case 49:
        argp.ping_timeout = atoi(optarg);
        break;
      case 33:
        argp.resol_source_port = atoi(optarg);
        break;
      case 32:
        argp.random_dns = true;
        argp.random_dns_count = atoi(optarg);
        break;
      case 40:
        argp.resol_delay = atoi(optarg);
        break;
      case 50:
        argp.no_get_path = true;
        break;
      case 51:
        argp.get_response = true;
        break;
      case 46:
        argp.no_scan = true;
        break;
      case 67:
        argp.robots_txt = true;
        break;
      case 68:
        argp.sitemap_xml = true;
        break;
      case 52:
        argp.custom_threads_resolv = true;
        argp.dns_threads = atoi(optarg);
        break;
      case 62: {
        std::vector<std::string> temp_ips = split_string_string(optarg, ',');
        argp.exclude = resolv_hosts(temp_ips);
        break;
      }
      case 63: {
        std::vector<std::string> temp_ips = write_file(optarg);
        argp.exclude = resolv_hosts(temp_ips);
        break;
      }
      case 53:
        argp.my_life_my_rulez = true;
        argp.speed_type = 5;
        break;
      case 10:
        argp.ddos_packets = atoi(optarg);
        break;
      case 78:
        np.import_color_scheme(optarg, np.config_values);
        np.processing_color_scheme(np.config_values);
        np.printcolorscheme();
        exit(1);
        break;
      case 54:
        argp.import_color_scheme = true;
        argp.path_color_scheme = optarg;
        break;
      case 'l':
        np.html_save = true;
        np.html_file_path = std::string(optarg);
        break;
      case 57:
        argp.custom_threads = true;
        argp.threads_ping = atoi(optarg);
        break;
      case 59:
        argp.no_get_dns = true;
        break;
      case 76:
        argp.negatives_path = std::string(optarg);
        break;
      case 80:
        argp.custom_ping = true;
        argp.syn_ping = true;
        argp.syn_dest_port = atoi(optarg);
        break;
      case 81:
        argp.custom_ping = true;
        argp.ack_ping = true;
        argp.ack_dest_port = atoi(optarg);
        break;
      case 43:
        argp.json_save = true;
        argp.json_save_path = optarg;
        break;
      case 82:
        argp.custom_ping = true;
        argp.echo_ping = true;
        break;
      case 86:
        argp.custom_ping = true;
        argp.info_ping = true;
        break;
      case 87:
        argp.custom_ping = true;
        argp.timestamp_ping= true;
        break;
      case 88:
        argp.max_ping = true;
        break;
      case 89:
        argp.ack_scan = true;
        argp.type = ACK_SCAN;
        break;
      case 9:
        argp.ddos_threads = atoi(optarg);
        break;
      case 90:
        argp.ping_log = atoi(optarg);
        break;
      case 91:
        argp.null_scan = true;
        argp.type = NULL_SCAN;
        break;
      case 8:
        argp.udp_ddos = true;
        break;
      case 92:
        argp.fin_scan = true;
        argp.type = FIN_SCAN;
        break;
      case 93:
        argp.xmas_scan = true;
        argp.type = XMAS_SCAN;
        break;
      case 94:
        argp.window_scan = true;
        argp.type = WINDOW_SCAN;
        break;
      case 95:
        argp.no_proc = true;
        break;
      case 6:
        argp.ip_ddos = true;
        argp.ip_ddos_proto = atoi(optarg);
        break;
      case 97:
        argp.maimon_scan = true;
        argp.type = MAIMON_SCAN;
        break;
      case 101:
        argp.custom_recv_timeout_ms = true;
        argp.recv_timeout_ms = atoi(optarg);
        break;
      default:
        usage();
      }
    }
}
