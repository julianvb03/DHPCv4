#ifndef DHCPV4_CLIENT_H
#define DHCPV4_CLIENT_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include "dhcpv4_utils.h"

#define SERVER_PORT 67
#define CLIENT_PORT 68
#define TIMEOUT_SECONDS 4
#define MAX_RETRIES 2

void handle_discover(struct dhcp_packet* packet, struct ifreq* ifr);
int send_with_retries(int sockfd, struct dhcp_packet *packet, size_t packet_len, struct sockaddr *dest_addr, socklen_t addr_len, void *buffer, size_t buffer_len, int max_retries);

#endif