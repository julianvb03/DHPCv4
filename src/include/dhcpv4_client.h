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
#include "dhcpv4_utils.h"

#define SERVER_PORT 8067
#define CLIENT_PORT 8068
#define TIMEOUT_SECONDS 5
#define MAX_RETRIES 1

void handle_discover(struct dhcp_packet* packet, struct ifreq* ifr);
int send_with_retries(int sockfd, const void *message, size_t message_len, struct sockaddr *dest_addr, socklen_t addr_len, void *buffer, size_t buffer_len, int timeout_seconds, int max_retries);

#endif