#include "dhcpv4_client.h"

int send_with_retries(int sockfd, const void *message, size_t message_len, struct sockaddr *dest_addr, socklen_t addr_len, void *buffer, size_t buffer_len, int timeout_seconds, int max_retries) {
    int retries = 0;

    while (retries < max_retries) {
        // Enviar el mensaje
        if (sendto(sockfd, message, message_len, 0, dest_addr, addr_len) < 0) {
            perror("Error sending message");
            return -1;
        }

        printf("Message sent, waiting for reply (attempt %d/%d)...\n", retries + 1, max_retries);

        struct timeval timeout;
        timeout.tv_sec = timeout_seconds;
        timeout.tv_usec = 0;
        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
            perror("Error setting timeout");
            return -1;
        }

        int recv_len = recvfrom(sockfd, buffer, buffer_len, 0, dest_addr, &addr_len);
        if (recv_len >= 0) {
            printf("Received response from server.\n");
            return recv_len;
        } else {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                printf("Timeout waiting for response from server, retrying...\n");
            } else {
                perror("Error receiving response");
                return -1;
            }
        }

        retries++;
    }

    printf("Unable to contact the server after %d attempts.\n", max_retries);
    return -1;
}

void handle_discover(struct dhcp_packet* packet, struct ifreq* ifr) {

    // Prepare the data for the DHCPDISCOVER message

    // Get the MAC address of the network interface and copy it to the chaddr field
    uint8_t chaddr[16] = {0};
    memcpy(chaddr, ifr->ifr_hwaddr.sa_data, ETH_ALEN);          // Copy the MAC address

    // NERVER USED options
    uint8_t sname[64] = {0};                                    // Empty server name
    uint8_t file[128] = {0};                                    // Empty boot file name
    
    // Initialize the options field with zeros
    uint8_t options[DHCP_OPTIONS_LEN] = {0};

    // Set the DHCP options, only one on this case
    uint32_t magic_cookie = htonl(DHCP_MAGIC_COOKIE);           // Magic cookie distinguishing DHCP and BOOTP packets
    memcpy(options, &magic_cookie, sizeof(magic_cookie));
    options[4] = DHCP_OPTION_MESSAGE_TYPE;                      // Option: DHCP Message Type
    options[5] = 1;                                             // Length of the option data
    options[6] = DHCPDISCOVER;                                  // DHCP Discover message type
    options[7] = DHCP_OPTION_END;                               // End Option

    // Use get_dhcp_struc_hton to fill the DHCP packet
    get_dhcp_struc_hton(
        packet, 
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

}