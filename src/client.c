#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include "tdhcp_package.h"

#define SERVER_PORT 8067
#define CLIENT_PORT 8068

int main() {
    int sockfd;
    struct sockaddr_in their_addr, my_addr;
    struct dhcp_packet packet;
    struct ifreq ifr;
    char interface[] = "lo"; // Replace with your network interface name

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    memset(&ifr, 0, sizeof(ifr));
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), interface);
    if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0) {
        perror("Error binding socket to interface");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(CLIENT_PORT);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
        perror("Error binding socket");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    memset(&their_addr, 0, sizeof(their_addr));
    their_addr.sin_family = AF_INET;
    their_addr.sin_port = htons(SERVER_PORT);
    their_addr.sin_addr.s_addr = INADDR_BROADCAST;

    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &(int){1}, sizeof(int)) < 0) {
        perror("Error enabling broadcast");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Get the MAC address of the specified interface
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("Error opening socket for MAC address retrieval");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);
    if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
        perror("Error retrieving MAC address");
        close(fd);
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    close(fd);

    // Prepare the data for the DHCPDISCOVER message
    uint8_t chaddr[16] = {0};
    memcpy(chaddr, ifr.ifr_hwaddr.sa_data, ETH_ALEN);  // Copy the MAC address
    uint8_t sname[64] = {0};  // Empty server name
    uint8_t file[128] = {0};  // Empty boot file name
    uint8_t options[DHCP_OPTIONS_LEN] = {0};

    // Set the DHCP options
    uint32_t magic_cookie = htonl(DHCP_MAGIC_COOKIE);
    memcpy(options, &magic_cookie, sizeof(magic_cookie));
    options[4] = DHCP_OPTION_MESSAGE_TYPE;  // Option: DHCP Message Type
    options[5] = 1;                         // Length of the option data
    options[6] = DHCPDISCOVER;              // DHCP Discover message type
    options[7] = DHCP_OPTION_END;           // End Option

    // Use get_dhcp_struc_hton to fill the DHCP packet
    get_dhcp_struc_hton(
        &packet, 
        BOOTREQUEST, 
        HTYPE_ETHERNET, 
        ETH_ALEN, 
        0, 
        0x12345678,  // Transaction ID (can be random)
        0, 
        0x8000,  // Flags: Broadcast
        0, 0, 0, 0,  // IP addresses (ciaddr, yiaddr, siaddr, giaddr)
        chaddr, 
        sname, 
        file, 
        options
    );

    // Print the packet before sending for debugging purposes
    print_dhcp_struc((const char*)&packet, sizeof(packet));

    // Send the DHCPDISCOVER message
    if (sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&their_addr, sizeof(their_addr)) < 0) {
        perror("Error sending DHCPDISCOVER message");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("DHCPDISCOVER message sent successfully.\n");

    close(sockfd);
    return EXIT_SUCCESS;
}
