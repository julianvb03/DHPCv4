#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>

// DHCP Operation Codes
#define BOOTREQUEST 1
#define BOOTREPLY 2

// DHCP Hardware Types
#define HTYPE_ETHERNET 1

// DHCP Options
#define DHCP_OPTION_MESSAGE_TYPE 53
#define DHCP_OPTION_END 255
#define DHCP_MAGIC_COOKIE 0x63825363

// DHCP Message Types
#define DHCPDISCOVER 1
#define DHCPOFFER 2
#define DHCPREQUEST 3
#define DHCPDECLINE 4
#define DHCPACK 5
#define DHCPNAK 6
#define DHCPRELEASE 7
#define DHCPINFORM 8

// Lengths
#define ETH_ALEN 6   // Length of an Ethernet address (MAC address)
#define DHCP_OPTIONS_LEN 312


struct prueba {
    int color;
    int numero;
};

int main() {
    char options[312] = {0};
    uint32_t magic_cookie = htonl(DHCP_MAGIC_COOKIE);  
    memcpy(options, &magic_cookie, sizeof(magic_cookie));
    options[4] = DHCP_OPTION_MESSAGE_TYPE;                      // Option: DHCP Message Type
    options[5] = 1;                                             // Length of the option data
    options[6] = DHCPDISCOVER;                                  // DHCP Discover message type
    options[7] = DHCP_OPTION_END;                                // End Option

    for (int i = 0; i < 312; i++) {
            printf("%02x ", (unsigned char)options[i]);
            if ((i + 1) % 16 == 0) {
                printf("\n");
            }
        }
        printf("\n\n");

    printf("Magic cookie: %x has a size of %ld bytes\n", magic_cookie, sizeof(magic_cookie));

    return 0;
}

