#include "dhcpv4_client.h"

int main() {
    int sockfd;
    struct sockaddr_in their_addr, my_addr;
    struct dhcp_packet packet;
    struct ifreq ifr;

    //volver esto variable
    char interface[] = "lo"; // Interface name

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // This option able the posibility to do broadcast over a network
    memset(&ifr, 0, sizeof(ifr));
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), interface);
    if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0) {
        perror("Error binding socket to interface");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Configuring my_addr struct
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(CLIENT_PORT);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
        perror("Error binding socket");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Configuring their_addr struct
    memset(&their_addr, 0, sizeof(their_addr));
    their_addr.sin_family = AF_INET;
    their_addr.sin_port = htons(SERVER_PORT);
    their_addr.sin_addr.s_addr = INADDR_BROADCAST;

    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &(int){1}, sizeof(int)) < 0) {
        perror("Error enabling broadcast");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Temporal socket to get the MAC address
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

    handle_discover(&packet, &ifr);

    // Print the packet before sending for debugging purposes
    print_dhcp_struc((const char*)&packet, sizeof(packet));

    if(send_with_retries(sockfd, &packet, sizeof(packet), (struct sockaddr *)&their_addr, sizeof(their_addr), &packet, sizeof(packet), TIMEOUT_SECONDS, MAX_RETRIES) < 0) {
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    close(sockfd);
    return EXIT_SUCCESS;
}
