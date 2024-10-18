#ifndef DHCPV4_SERVER_H
#define DHCPV4_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include "dhcpv4_utils.h"

/*
    Structure for the IP addresses pool
        @param ip_str: IP address
        @param state: State of the IP address (0 = Free, 1 = Used)
        @param mac_str: MAC address of the client
        @param arr_time: Time on seconds
*/
struct ip_in_poll {
    char ip_str[INET_ADDRSTRLEN];
    u_int8_t state;
    char mac_str[18];
    u_int32_t arr_time;
};

/*
    Structure for the configuration of the DHCP server
        @param ip_base: Base IP address
        @param ip_mask: Mask of the network
        @param num_ips: Number of IP addresses on the array
        @param time_tp: Time of arrendation
        @param dns_server: DNS server
*/
struct configuration {
    char ip_base[INET_ADDRSTRLEN];
    char ip_mask[INET_ADDRSTRLEN];
    int num_ips;
    u_int32_t time_tp;
    char dns_server[INET_ADDRSTRLEN];
};

/*
    Structure for the thread data (for avoid the use of global variables, and got race conditions)
        @param buffer: Buffer with the data
        @param numbytes: Number of bytes
        @param client_addr: Client address
*/
struct dhcp_thread_data {
    char buffer[1024];
    int numbytes;
    struct sockaddr_in client_addr;
    struct configuration *config;
    struct ip_in_poll *ips;
};

int create_socket_dgram();
int fill_dhcp_ack(struct dhcp_packet *packet);
int send_dhcp_package(struct dhcp_packet *packet);
int handle_discover(struct dhcp_packet* packet, struct configuration* config, char* ip_offer);
int create_poll(struct ip_in_poll* ips, int num_ips, const char* ip_base, const char* ip_mask);
int assign_ip(struct ip_in_poll* ips, char* mac_str, u_int32_t time_tp, int num_ips, char *ip_str_offer);
int fill_dhcp_offer(struct dhcp_packet *packet, u_int8_t* clien_mac, const char* ip_offer, struct configuration* config);

void get_dns(char* dns_server);
void *handle_dhcp_generic(void *arg);

#endif