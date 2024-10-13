// #include "dhcpv4_client.h"

// int send_with_retries(int sockfd, struct dhcp_packet *packet, size_t packet_len, struct sockaddr *dest_addr, socklen_t addr_len, void *buffer, size_t buffer_len, int max_retries) {
//     int retries = 0;

//     while (retries < max_retries) {
//         // Enviar el paquete DHCP
//         if (sendto(sockfd, packet, packet_len, 0, dest_addr, addr_len) < 0) {
//             perror("Error sending DHCP DISCOVER");
//             return -1;
//         }

//         printf("DHCP DISCOVER sent, waiting for DHCP OFFER (attempt %d/%d)...\n", retries + 1, max_retries);

//         // Intentar recibir una respuesta
//         int recv_len = recvfrom(sockfd, buffer, buffer_len, 0, NULL, NULL);
//         if (recv_len >= 0) {
//             // Verificar que el paquete es un DHCP OFFER válido
//             struct ethhdr *eth_header = (struct ethhdr *)buffer;
//             struct iphdr *ip_header = (struct iphdr *)(buffer + sizeof(struct ethhdr));
//             struct udphdr *udp_header = (struct udphdr *)(buffer + sizeof(struct ethhdr) + sizeof(struct iphdr));

//             // Asegurarse de que el paquete es UDP y que el puerto de destino es 68
//             if (ip_header->protocol == IPPROTO_UDP && ntohs(udp_header->dest) == CLIENT_PORT) {
//                 // Verificar si el paquete es un DHCP OFFER
//                 struct dhcp_packet *dhcp_response = (struct dhcp_packet *)(buffer + sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct udphdr));

//                 if (dhcp_response->options[0] == DHCP_OPTION_MESSAGE_TYPE &&
//                     dhcp_response->options[1] == 1 &&
//                     dhcp_response->options[2] == DHCPOFFER) {
//                     printf("Received DHCP OFFER from server.\n");
//                     return recv_len;
//                 } else {
//                     printf("Received a non-DHCP OFFER packet, ignoring.\n");
//                 }
//             } else {
//                 printf("Received a packet that is not DHCP or not for the right port, ignoring.\n");
//             }
//         } else {
//             if (errno == EWOULDBLOCK || errno == EAGAIN) {
//                 printf("Timeout waiting for DHCP OFFER, retrying...\n");
//             } else {
//                 perror("Error receiving DHCP OFFER");
//                 return -1;
//             }
//         }

//         retries++;
//     }

//     printf("Unable to receive DHCP OFFER after %d attempts.\n", max_retries);
//     return -1;
// }

// void handle_discover(struct dhcp_packet* packet, struct ifreq* ifr) {

//     // Prepare the data for the DHCPDISCOVER message

//     // Get the MAC address of the network interface and copy it to the chaddr field
//     uint8_t chaddr[16] = {0};
//     memcpy(chaddr, ifr->ifr_hwaddr.sa_data, ETH_ALEN);          // Copy the MAC address

//     // NERVER USED options
//     uint8_t sname[64] = {0};                                    // Empty server name
//     uint8_t file[128] = {0};                                    // Empty boot file name
    
//     // Initialize the options field with zeros
//     uint8_t options[DHCP_OPTIONS_LEN] = {0};

//     // Set the DHCP options, only one on this case
//     uint32_t magic_cookie = htonl(DHCP_MAGIC_COOKIE);           // Magic cookie distinguishing DHCP and BOOTP packets
//     memcpy(options, &magic_cookie, sizeof(magic_cookie));
//     options[4] = DHCP_OPTION_MESSAGE_TYPE;                      // Option: DHCP Message Type
//     options[5] = 1;                                             // Length of the option data
//     options[6] = DHCPDISCOVER;                                  // DHCP Discover message type
//     options[7] = DHCP_OPTION_END;                               // End Option

//     // Use get_dhcp_struc_hton to fill the DHCP packet
//     get_dhcp_struc_hton(
//         packet, 
//         BOOTREQUEST, 
//         HTYPE_ETHERNET, 
//         ETH_ALEN, 
//         0, 
//         0x12345678,  // Transaction ID (can be random)
//         0, 
//         0x8000,  // Flags: Broadcast
//         0, 0, 0, 0,  // IP addresses (ciaddr, yiaddr, siaddr, giaddr)
//         chaddr, 
//         sname, 
//         file, 
//         options
//     );

// }