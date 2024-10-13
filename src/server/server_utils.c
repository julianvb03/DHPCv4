#include "dhcpv4_server.h"

/*
    Function to assign an IP to a MAC address (0 = Free, 1 = Used)
        @param ips: Array of IP addresses
        @param mac_str: MAC address of the client
        @param time_tp: Time of arrendation
        @param num_ips: Number of IP addresses on the array
        
        @return: The IP address assigned or NULL if there is no IP available
*/
int assign_ip(struct ip_in_poll* ips, char* mac_str, u_int32_t time_tp, int num_ips, char *ip_str_offer) {
    time_t current_time = time(NULL);
    for (int i = 0; i < num_ips; i++) {
        if(ips[i].state == 1 && current_time >= ips[i].arr_time) {
            ips[i].state = 0;
            memset(ips[i].mac_str, 0, sizeof(ips[i].mac_str));
        }

        if (ips[i].state == 0) {
            ips[i].state = 1;
            strcpy(ips[i].mac_str, mac_str);
            ips[i].arr_time = current_time + time_tp;           // Time on seconds of arrendation
            strcpy(ip_str_offer, ips[i].ip_str);
            return 0;
        }
    }
    return .1;
}

/*
    Functiof for create the ips Pool with a base IP addresses
        @param ips: Array of IP addresses
        @param num_ips: Number of IP addresses on the array
        @param ip_base: Base IP address
        @param ip_mask: Mask of the network

        @return: 0 if the pool was created successfully, -1 if there was an error
*/
int create_poll(struct ip_in_poll* ips, int num_ips, const char* ip_base, const char* ip_mask) {
    struct in_addr base_addr, mask_addr;

    // Convertir la IP base y la máscara de red de string a formato binario
    if (inet_pton(AF_INET, ip_base, &base_addr) != 1) {
        perror("Error converting IP base");
        return -1;
    }
    if (inet_pton(AF_INET, ip_mask, &mask_addr) != 1) {
        perror("Error converting IP mask");
        return -1;
    }

    // Calcular la dirección de red (AND entre IP base y máscara)
    uint32_t network = ntohl(base_addr.s_addr) & ntohl(mask_addr.s_addr);
    uint32_t mask = ntohl(mask_addr.s_addr);

    // Calcular el número máximo de hosts en la subred
    uint32_t max_hosts = ~mask - 1; // Restamos 1 porque la dirección de broadcast no es asignable

    // Verificar si el número de IPs solicitado no supera el número de hosts disponibles menos las primeras 50
    if (num_ips > max_hosts - 50) {
        printf("Number of IPs requested (%d) exceeds available range (%u) after reserving 50 static addresses.\n", num_ips, max_hosts - 50);
        return -1;
    }

    // Generar las direcciones IP para el pool, empezando desde network + 51 para omitir las primeras 50
    for (int i = 0; i < num_ips; i++) {
        // Incrementar la parte del host en el rango de la red, comenzando desde la IP 51
        uint32_t ip_host = network + 51 + i;

        // Convertir la dirección IP a formato de red
        struct in_addr ip_addr;
        ip_addr.s_addr = htonl(ip_host);

        // Convertir la dirección IP a formato string y guardarla en la estructura
        inet_ntop(AF_INET, &ip_addr, ips[i].ip_str, INET_ADDRSTRLEN);

        // Inicializar los otros campos de la estructura
        ips[i].state = 0;  // Inicialmente desocupada (o en el estado deseado)
        ips[i].arr_time = 0;   // Tiempo inicial

        // Dejar el campo de MAC vacío por ahora, ya que no hay una MAC asociada inicialmente
        memset(ips[i].mac_str, 0, sizeof(ips[i].mac_str));

        printf("Assigned IP: %s\n", ips[i].ip_str); // Para ver el proceso (puedes quitarlo)
    }

    return 0;
}

/*
    Function to fill the DHCP Discover message
        @param packet: DHCP packet structure
        @param clien_mac: MAC address of the client
        @param ip_offer: Offered IP address
        @param default_gateway: Default gateway
        @param dns_server: DNS server

        @return: void

    We ned implement wen the discovir is received with an Agent Relay and subnets
*/
int fill_dhcp_offer(struct dhcp_packet *packet, u_int8_t* clien_mac, const char* ip_offer, struct configuration* config) {
    // DHCP options
    uint8_t sname[64] = {0};                                    // Never used
    uint8_t file[128] = {0};                                    // Never used
    uint8_t options[DHCP_OPTIONS_LEN] = {0};

    uint32_t magic_cookie = htonl(DHCP_MAGIC_COOKIE);           // Magic cookie distinguishing DHCP and BOOTP packets
    memcpy(options, &magic_cookie, sizeof(magic_cookie));
    options[4] = DHCP_OPTION_MESSAGE_TYPE;                      // Option: DHCP Message Type
    options[5] = 1;                                             // Length of the option data
    options[6] = DHCPOFFER;                                     // DHCP Offer message type

    options[7] = 1;                                             // Option: Bubnet mask
    options[8] = 4;                                             // Length of the option data
    inet_pton(AF_INET, config->ip_mask, &options[9]);               // Server IP address

    options[13] = 3;                                            // Option: DG
    options[14] = 4;                                            // Length of the option data
    inet_pton(AF_INET, config->ip_base, &options[15]);          // Default gateway

    options[19] = 6;                                            // Option: DNS
    options[20] = 4;                                            // Length of the option data
    inet_pton(AF_INET, config->dns_server, &options[21]);               // DNS server

    //51 (Lease Time) : 58 (Renewal Time) : 59 (Rebinding Time)     Time is still pending to implement: config->time_tp

    options[25] = DHCP_OPTION_END;

    uint32_t yiaddr_net;                                        // Convert the offered IP address to network byte order
    if (inet_pton(AF_INET, ip_offer, &yiaddr_net) != 1) {
        printf("Error al convertir la dirección IP\n");
        return -1;
    }

    get_dhcp_struc_hton(
        packet, 
        BOOTREPLY, 
        HTYPE_ETHERNET, 
        ETH_ALEN, 
        0, 
        0x12345678,                                             // Transaction ID Fixef for trace
        0, 
        0x8000,                                                 // Flags: Broadcast
        0, yiaddr_net, 0, 0,                                    // IP addresses (ciaddr, yiaddr, siaddr, giaddr)
        clien_mac,                                              // Aquí usamos directamente la dirección MAC del cliente
        sname, 
        file, 
        options
    );

    return 0;
}

/*
    Function to fill the DHCP ACK message
        @param packet: DHCP packet structure (pointer)

        @return: void
*/
int fill_dhcp_ack(struct dhcp_packet *packet) {
    // Implement the logic for filling the DHCP ACK message we need to and binding time
    packet->op = BOOTREPLY;
    packet->options[6] = DHCPACK;
    return 0;
}

/*
    Function to handle the DHCP Discover message
        @param packet: DHCP packet structure
        @param config: Configuration of the DHCP server
        @param ip_offer: Offered IP address

        @return: 0 if the process was successful, -1 if there was an error
*/
int handle_discover(struct dhcp_packet* packet, struct configuration* config, char* ip_offer) {
    // Implement the logic for each case of DISCOVER
    uint8_t chaddr[16] = {0};
    memcpy(chaddr, packet->chaddr, ETH_ALEN);                    // Copy the MAC address

    if (fill_dhcp_offer(packet, chaddr, ip_offer, config) < 0) {
        printf("Error al llenar el paquete DHCP Offer\n");
        return -1;
    }

    printf("Se va a ofertar la IP: %s\n", ip_offer);
    //print_dhcp_strct(packet);

    usleep(1000000);
    if (send_dhcp_package(packet) < 0) {
        printf("Error al enviar el paquete DHCP Offer\n");
        return -1;
    }

    printf("Se envió la oferta de IP despues de esperar\n");
    return 0;
}

/*
    Function to handle the DHCP ACK message
        @param packet: DHCP packet structure
        @param client_addr: Client address

        @return: 0 if the process was successful, -1 if there was an error
*/
int handle_ack(struct dhcp_packet* packet, struct sockaddr_in* client_addr) {
    // Implement the logic for each case of ACK
     if(fill_dhcp_ack(packet) < 0) {
        printf("Error al llenar el paquete DHCP ACK\n");
        return -1;
    }

    //printf("Se confirmo el uso de la ip con un ACK: %s\n");
    //print_dhcp_strct(packet);

    struct dhcp_packet packet_host;
    get_dhcp_struc_ntoh(packet, &packet_host);

    usleep(1000000);
    if(send_dhcp_package(packet) < 0) {
        printf("Error al enviar el paquete DHCP ACK\n");
        return -1;
    }

    return 0;
}

/*
    Function to get the DNS server
        @param dns_server: pointer to the dns_server string that will be filled

        @return: void
*/
void get_dns(char* dns_server) {
    strcpy(dns_server, "8.8.8.8");
}

/*
    Function to create a thread for handle the DHCP messages
        @param arg: Thread data

        @return: NULL
*/
void *handle_dhcp_generic(void *arg) {
    struct dhcp_thread_data *data = (struct dhcp_thread_data *)arg;

    struct dhcp_packet local_net_packet;
    struct dhcp_packet local_host_packet;
    struct configuration *config = data->config;

    memcpy(&local_net_packet, data->buffer, sizeof(struct dhcp_packet));
    get_dhcp_struc_ntoh(&local_net_packet, &local_host_packet);

    printf("Paquete DHCP recibido y procesado en el hilo %ld:\n", (unsigned long int)pthread_self());
    //print_dhcp_strct(&local_host_packet);

    printf("Opciones del paquete DHCP:\n");

    int message_type;
    for (int iter = 0; iter < DHCP_OPTIONS_LEN && local_host_packet.options[iter] != DHCP_OPTION_END; iter++) {
        if(local_host_packet.options[iter] == DHCP_OPTION_MESSAGE_TYPE) {
            int len = local_host_packet.options[iter + 1];
            if (len != 1) {
                printf("Error: Invalid DHCP message type option length\n");
                break;
            }
            message_type = local_host_packet.options[iter + 2];
        }
    }
    
    if(message_type == DHCPDISCOVER) {
        // Posible race condition
        char ip_offer[INET_ADDRSTRLEN];
        if(assign_ip(data->ips, local_host_packet.chaddr, data->config->time_tp, data->config->num_ips, ip_offer) < 0) {
            strcpy(ip_offer, "0.0.0.0");
            //this is temporal the real case is to send a NAK
            //handle_nak(&local_host_packet, data->client_addr);
        }
        printf("DHCPDISCOVER\n");
        handle_discover(&local_host_packet, config, ip_offer);
    } else if(message_type == DHCPREQUEST) {
        printf("DHCPREQUEST\n");
        handle_ack(&local_host_packet, &data->client_addr);
    } else if(message_type == DHCPRELEASE) {
        printf("DHCPRELEASE\n");
        //handle_release(&local_host_packet, data->client_addr);
    } else {
        print_dhcp_strct(&local_host_packet);
        printf("Unknown message type\n");
    }

    // usleep(1000000);
    free(data);
    return NULL;
}