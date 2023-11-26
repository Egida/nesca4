/*
 * NCSOCK & NESCA 4
 * by oldteam & lomaster
 * license GPL-2.0
 *   Сделано от души 2023.
*/

#include "include/base.h"
#include <stdio.h>

#include <stdlib.h>
#include "include/dns.h"
int this_is(const char* node)
{
  int len = strlen(node);
  if (len >= 7 && strncmp(node, "http://", 7) == 0) {
    return _URL_;
  }
  else if (len >= 8 && strncmp(node, "https://", 8) == 0) {
      return _URL_;
  }

  char* cidr_symbol = strchr(node, '/');
  if (cidr_symbol != NULL) {
    int mask = atoi(cidr_symbol + 1);
    if (mask >= 0 && mask <= 32) {
      return CIDR;
    }
  }

  char* ip_range_delimiter = strchr(node, '-');
  if (ip_range_delimiter != NULL) {
    if (ip_range_delimiter != node && ip_range_delimiter[1] != '\0') {
      return RANGE;
    }
  }

  if (dns_or_ip(node) == THIS_IS_DNS) {
    return DNS;
  }
  else {
    return IPv4;
  }

  return -1;
}

const char* get_this_is(int type)
{
  switch (type)
  {
    case _URL_: return "URL";
    case DNS: return "DNS";
    case CIDR: return "CIDR";
    case RANGE: return "RANGE";
    case IPv4: return "IPv4";
  }

  return "-1";
}
int check_root_perms()
{
  return (geteuid() == 0);
}

#include <time.h>

void delayy(int ms)
{
  struct timespec ts;
  ts.tv_sec = ms / 1000;
  ts.tv_nsec = (ms % 1000) * 1000000;
  nanosleep(&ts, NULL);
}

const char* get_time()
{
  time_t rawtime; struct tm * timeinfo;
  static char time_str[9];

  time(&rawtime);
  timeinfo = localtime(&rawtime);

  sprintf(time_str, "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
  return time_str;
}

void get_current_date(char* formatted_date, size_t max_length)
{
  time_t current_time = time(NULL);
  struct tm* local_time = localtime(&current_time);

  int year = local_time->tm_year + 1900;
  int month = local_time->tm_mon + 1;
  int day = local_time->tm_mday;

  snprintf(formatted_date, max_length, "%d-%02d-%02d", year, month, day);
}

int calculate_timeout(double rtt, int speed)
{
  const int timeout_values[] = {6, 5, 4, 3, 2};
  int timeout = -1;

  if (speed >= 1 && speed <= 5) {
    timeout = timeout_values[speed - 1];
    timeout *= rtt;
  }

  return timeout;
}

int calculate_ping_timeout(int speed)
{
  const int timeouts[] = {3000, 2000, 1000, 600, 400};
  const int speed_type_index = speed - 1;

  int result = (speed_type_index >= 0 && speed_type_index < 5) ? timeouts[speed_type_index] : 0;
  return result;
}

int calculate_threads(int speed, int len)
{
  const int sizes[] = {100, 500, 1000, 1500, 2000};
  const int speed_type_index = speed - 1;
  const int max_threads = (speed_type_index >= 0 && speed_type_index < 5) ? sizes[speed_type_index] : 0;

  int result = (max_threads < len) ? max_threads : len;
  return result;
}

#include <stdlib.h>
string_vector* create_string_vector()
{
  string_vector* vec = (string_vector*)malloc(sizeof(string_vector));
  if (vec == NULL) {
    printf("create_string_vector/memory-malloc!\n");
    return NULL;
  }

  vec->data = NULL;
  vec->size = 0;
  vec->capacity = 0;
  return vec;
}

void string_vector_add(string_vector* vec, const char* str)
{
  if (vec->size >= vec->capacity) {
    vec->capacity = (vec->capacity == 0) ? 1 : vec->capacity * 2;
    vec->data = (char**)realloc(vec->data, vec->capacity * sizeof(char*));
    if (vec->data == NULL) {
      printf("string_vector_add/memory-realloc!\n");
      return;
    }
  }

  vec->data[vec->size] = strdup(str);
  vec->size++;
}

void string_vector_free(string_vector* vec)
{
  for (size_t i = 0; i < vec->size; i++) {
    free(vec->data[i]);
  }
  free(vec->data);
  vec->size = 0;
  vec->capacity = 0;
}

int_vector* create_int_vector()
{
  int_vector* vec = (int_vector*)malloc(sizeof(int_vector));
  if (vec == NULL) {
    printf("create_int_vector/memory-malloc!\n");
    return NULL;
  }

  vec->data = NULL;
  vec->size = 0;
  vec->capacity = 0;
  return vec;
}

void int_vector_add(int_vector* vec, int value)
{
  if (vec->size >= vec->capacity) {
    vec->capacity = (vec->capacity == 0) ? 1 : vec->capacity * 2;
    vec->data = (int*)realloc(vec->data, vec->capacity * sizeof(int));
    if (vec->data == NULL) {
      printf("int_vector_add/memory-realloc!\n");
    }
  }

  vec->data[vec->size] = value;
  vec->size++;
}

void int_vector_free(int_vector* vec)
{
  free(vec->data);
  free(vec);
}
