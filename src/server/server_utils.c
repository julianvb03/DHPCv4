// #include "dhcpv4_server.h"

// void handle_discover(struct dhcp_packet* packet, struct sockaddr_in* client_addr) {
//     // Prepare the data for the DHCPOFFER message

//     // Get the MAC address of the network interface and copy it to the chaddr field
//     client_addr->sin_port = htons(CLIENT_PORT); // Puerto DHCP del cliente
//     client_addr->sin_addr.s_addr = htonl(INADDR_BROADCAST);
//     uint8_t chaddr[16] = {0};
//     memcpy(chaddr, packet->chaddr, ETH_ALEN);          // Copy the MAC address

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
//     options[6] = DHCPOFFER;                                     // DHCP Offer message type
//     options[7] = DHCP_OPTION_END;                               // End Option

//     // Prepare the DHCP packet
//     struct dhcp_packet offer_packet;
//     get_dhcp_struc_hton(&offer_packet, BOOTREPLY, HTYPE_ETHERNET, ETH_ALEN, 0, packet->xid, 0, 0, 0, 0, 0, 0, chaddr, sname, file, options);

//     // Send the DHCPOFFER message
//     int sockfd;
//     if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
//         perror("Error al crear el socket");
//         exit(1);
//     }

//     if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &(int){1}, sizeof(int)) < 0) {
//         perror("Error al configurar SO_BROADCAST en el envío");
//         close(sockfd);
//         exit(1);
//     }

//     if (sendto(sockfd, &offer_packet, sizeof(offer_packet), 0, (struct sockaddr *)client_addr, sizeof(*client_addr)) < 0) {
//         perror("Error al enviar el mensaje");
//         close(sockfd);
//         exit(1);
//     }
//     printf("DHCPOFFER enviado\n");

//     close(sockfd);
// }

// void *handle_dhcp_generic(void *arg) {
//     struct dhcp_thread_data *data = (struct dhcp_thread_data *)arg;

//     struct dhcp_packet local_net_packet;
//     struct dhcp_packet local_host_packet;

//     memcpy(&local_net_packet, data->buffer, sizeof(struct dhcp_packet));
//     get_dhcp_struc_ntoh(&local_net_packet, &local_host_packet);

//     printf("Paquete DHCP recibido y procesado en el hilo %ld:\n", (unsigned long int)pthread_self());
//     print_dhcp_struc((const char*)&local_host_packet, sizeof(local_host_packet));

//     printf("Opciones del paquete DHCP:\n");

//     int message_type;
//     for (int iter = 0; iter < DHCP_OPTIONS_LEN && local_host_packet.options[iter] != DHCP_OPTION_END; iter++) {
//         if(local_host_packet.options[iter] == DHCP_OPTION_MESSAGE_TYPE) {
//             int len = local_host_packet.options[iter + 1];
//             if (len != 1) {
//                 printf("Error: Invalid DHCP message type option length\n");
//                 break;
//             }
//             message_type = local_host_packet.options[iter + 2];
//         }
//     }
    
//     if(message_type == DHCPDISCOVER) {
//         printf("DHCPDISCOVER\n");
//         handle_discover(&local_host_packet, &data->client_addr);
//     } else if(message_type == DHCPREQUEST) {
//         printf("DHCPREQUEST\n");
//         //handle_request(&local_host_packet, data->client_addr);
//     } else if(message_type == DHCPRELEASE) {
//         printf("DHCPRELEASE\n");
//         //handle_release(&local_host_packet, data->client_addr);
//     } else {
//         //printf("Unknown message type\n");
//     }

//     usleep(1000000);
//     free(data);
//     return NULL;
// }