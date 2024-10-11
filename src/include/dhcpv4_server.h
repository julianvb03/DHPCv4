#ifndef DHCPV4_SERVER_H
#define DHCPV4_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include "dhcpv4_utils.h"

#define SERVER_PORT 8067
#define CLIENT_PORT 8068

struct ip_in_poll {
    char ip_str[INET_ADDRSTRLEN];
    u_int8_t state;
    char mac_str[18];
    u_int64_t arr_time;             // Time on seconds
};

struct dhcp_thread_data {
    char buffer[1024];
    int numbytes;
    struct sockaddr_in client_addr;
};

void *handle_dhcp_generic(void *arg);
char* assign_ip(struct ip_in_poll* ips, char* mac_str, u_int32_t time, int num_ips);
int create_poll(struct ip_in_poll* ips, int num_ips, const char* ip_base, const char* ip_mask);

#endif