#ifndef DHCPV4_CLIENT_H
#define DHCPV4_CLIENT_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <net/if.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <string.h>
#include <stdio.h>
#include "dhcpv4_utils.h"

struct network_config {
    struct in_addr ip_addr;        // IP address offered
    struct in_addr subnet_mask;    // Subnet mask
    struct in_addr gateway;        // Default gateway
    struct in_addr dns_server;     // DNS server
};

int get_hwaddr(const char *ifname, uint8_t *hwaddr);
int create_dgram_socket(const char *interface_name);
int offer_validation(struct network_config* offer_config);
int send_dhcp_package(int sockfd, struct dhcp_packet *packet);
int handle_offer(struct dhcp_packet* packet, struct network_config* net_config);
int open_raw_socket(const char* ifname, uint8_t hwaddr[ETH_ALEN], int if_index);
int handle_request(struct dhcp_packet* packet, struct network_config* net_config);
int process_packet(unsigned char *buffer, int len, struct dhcp_packet *dhcp_packet_main);
int set_ip_address(const char* ifname, struct in_addr ip_addr, struct in_addr subnet_mask);

void print_dhcp_message_type(unsigned char type);
void handle_discover(struct dhcp_packet* packet, uint8_t *client_mac);

#endif