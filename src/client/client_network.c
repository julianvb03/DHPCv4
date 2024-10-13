#include "dhcpv4_client.h"

int create_dgram_socket(const char *interface_name) {
    int sockfd;
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error al crear el socket");
        exit(1);
    }

    if(setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &(int){1}, sizeof(int)) < 0) {
        perror("Error al configurar SO_BROADCAST en el envío");
        close(sockfd);
        exit(1);
    }

    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) == -1) {
        close(sockfd);
        perror("Error al configurar SO_REUSEADDR");
        return 3;
    }

    if(setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, interface_name, strlen(interface_name)) < 0) {
        perror("Error al configurar bind to device");
        close(sockfd);
        exit(1);
    }

    return sockfd;
}

void handle_discover(struct dhcp_packet* packet, uint8_t *client_mac) {

    // NERVER USED options
    uint8_t sname[64] = {0};                                    // Empty server name
    uint8_t file[128] = {0};                                    // Empty boot file name
    uint8_t options[DHCP_OPTIONS_LEN] = {0};

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
        client_mac,   // Aquí usamos directamente la dirección MAC del cliente
        sname, 
        file, 
        options
    );
}

int send_dhcp_package(int sockfd, struct dhcp_packet *packet) {
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(SERVER_PORT);  // Puerto del servidor DHCP
    dest_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    if(sendto(sockfd, packet, sizeof(struct dhcp_packet), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        perror("Error al enviar el mensaje");
        return -1;
    }
    printf("DHCP Discover enviado\n");

    return 0;

}

int open_raw_socket(const char* ifname, uint8_t hwaddr[ETH_ALEN], int if_index) {
    // Crear un socket RAW para capturar paquetes (usamos SOCK_RAW y ETH_P_ALL)     ETH_P_ALL todo tipo de trafico TODO!!!!!!!!!!!!
    int s = socket(PF_PACKET, SOCK_RAW | SOCK_CLOEXEC, htons(ETH_P_IP));
    if (s < 0) {
        perror("Error al crear socket RAW");
        return -1;
    }

    // Asociar el socket a la interfaz de red
    struct sockaddr_ll bindaddr = {
        .sll_family = AF_PACKET,
        .sll_protocol = htons(ETH_P_IP),  // Captura todo tipo de tráfico _____ Solo ipv4
        .sll_ifindex = if_index,
        .sll_halen = ETH_ALEN,
    };
    memcpy(bindaddr.sll_addr, hwaddr, ETH_ALEN);

    // Enlazar el socket a la interfaz de red especificada
    if (bind(s, (struct sockaddr *)&bindaddr, sizeof(bindaddr)) < 0) {
        perror("No se pudo enlazar el socket a la interfaz de red");
        close(s);
        return -1;
    }

    return s;  // Retorna el descriptor del socket RAW
}

int handle_offer(struct dhcp_packet* packet, struct network_config* net_config) {
    // Puntero para recorrer las opciones
    uint8_t *options_ptr = packet->options;
    int options_len = DHCP_OPTIONS_LEN;

    // Verificar y saltar la Magic Cookie (99, 130, 83, 99)
    if (options_len < 4 || options_ptr[0] != 99 || options_ptr[1] != 130 || options_ptr[2] != 83 || options_ptr[3] != 99) {
        printf("Magic Cookie no encontrada en las opciones DHCP\n");
        return -1;
    }
    options_ptr += 4;  // Saltar la Magic Cookie
    options_len -= 4;

    // Inicializar la estructura network_config
    memset(net_config, 0, sizeof(struct network_config));

    // Almacenar la dirección IP ofrecida en la estructura
    net_config->ip_addr.s_addr = packet->yiaddr;

    // Recorrer las opciones DHCP
    while (options_len > 0) {
        uint8_t option_code = *options_ptr++;
        options_len--;

        if (option_code == 0) {
            // Opción de relleno (Padding), continuar al siguiente byte
            continue;
        } else if (option_code == 255) {
            // Opción de fin (End Option), salir del bucle
            break;
        } else {
            if (options_len == 0) {
                printf("Longitud de opción incorrecta\n");
                return -1;
            }
            uint8_t option_length = *options_ptr++;
            options_len--;

            if (option_length > options_len) {
                printf("La longitud de los datos de la opción supera el tamaño restante\n");
                return -1;
            }

            // Procesar la opción según su código
            switch (option_code) {
                case 1:  // Máscara de subred
                    if (option_length == 4) {
                        memcpy(&net_config->subnet_mask, options_ptr, 4);
                    } else {
                        printf("Longitud de máscara de subred inválida\n");
                    }
                    break;

                case 3:  // Gateway predeterminado
                    if (option_length >= 4) {
                        memcpy(&net_config->gateway, options_ptr, 4);
                    } else {
                        printf("Longitud de gateway predeterminado inválida\n");
                    }
                    break;

                case 6:  // Servidor DNS
                    if (option_length >= 4) {
                        memcpy(&net_config->dns_server, options_ptr, 4);
                    } else {
                        printf("Longitud de servidor DNS inválida\n");
                    }
                    break;

                default:
                    // Puedes manejar otras opciones aquí si lo deseas
                    break;
            }

            // Avanzar el puntero de opciones
            options_ptr += option_length;
            options_len -= option_length;
        }
    }

    // Verificar si se ofreció una dirección IP válida
    if (net_config->ip_addr.s_addr == 0) {
        printf("No se ofreció una dirección IP\n");
        return -1;
    }

    // Imprimir los valores ofrecidos (opcional)
    char ip_offer[INET_ADDRSTRLEN];
    char mask_offer[INET_ADDRSTRLEN];
    char dg_offer[INET_ADDRSTRLEN];
    char dns_offer[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &net_config->ip_addr, ip_offer, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &net_config->subnet_mask, mask_offer, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &net_config->gateway, dg_offer, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &net_config->dns_server, dns_offer, INET_ADDRSTRLEN);

    printf("IP Offer: %s\n", ip_offer);
    printf("Mask Offer: %s\n", mask_offer);
    printf("Default Gateway Offer: %s\n", dg_offer);
    printf("DNS Offer: %s\n", dns_offer);

    return 0;
}

int offer_validation(struct network_config* offer_config) {
    if (offer_config->ip_addr.s_addr == 0) {
        printf("No se ofreció una dirección IP\n");
        return -1;
    }

    if (offer_config->subnet_mask.s_addr == 0) {
        printf("La máscara de subred no está disponible\n");
        return -1;
    }

    if (offer_config->gateway.s_addr == 0) {
        printf("El gateway predeterminado no está disponible\n");
        return -1;
    }

    // Calcular las direcciones de red
    struct in_addr ip_network, dg_network;
    ip_network.s_addr = offer_config->ip_addr.s_addr & offer_config->subnet_mask.s_addr;
    dg_network.s_addr = offer_config->gateway.s_addr & offer_config->subnet_mask.s_addr;

    if (ip_network.s_addr == dg_network.s_addr) {
        printf("La IP ofrecida pertenece a la misma subred que el gateway predeterminado.\n");
    } else {
        printf("La IP ofrecida NO pertenece a la misma subred que el gateway predeterminado.\n");
        return -1;
    }

    return 0;
}

int handle_request(struct dhcp_packet* packet, struct network_config* net_config) {
    // NERVER USED options
    uint8_t sname[64] = {0};                                    // Empty server name
    uint8_t file[128] = {0};                                    // Empty boot file name
    uint8_t options[DHCP_OPTIONS_LEN] = {0};

    uint32_t magic_cookie = htonl(DHCP_MAGIC_COOKIE);           // Magic cookie distinguishing DHCP and BOOTP packets
    memcpy(options, &magic_cookie, sizeof(magic_cookie));
    options[4] = DHCP_OPTION_MESSAGE_TYPE;                      // Option: DHCP Message Type
    options[5] = 1;                                             // Length of the option data
    options[6] = DHCPREQUEST;                                   // DHCP Request message type

    options[7] = 51;                                            // Time to use the IP address
    options[8] = 4;                                             // Length of the option data
    options[9] = 255;                                          // Time in seconds
    
    options[13] = 54;                                           // Server IP address
    options[14] = 4;                                            // Length of the option data
    memcpy(&options[15], &net_config->ip_addr, 4);              // Server IP address

    options[19] = DHCP_OPTION_END;                              // End Option

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
        packet->chaddr,   // Aquí usamos directamente la dirección MAC del cliente
        sname, 
        file, 
        options
    );
}

int set_ip_address(const char* ifname, struct in_addr ip_addr, struct in_addr subnet_mask) {
    int fd;
    struct ifreq ifr;
    struct sockaddr_in* addr;

    // Crear un socket para realizar las operaciones ioctl
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("Error al crear el socket");
        return -1;
    }

    // Limpiar la estructura ifreq
    memset(&ifr, 0, sizeof(ifr));

    // Copiar el nombre de la interfaz
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);

    // Configurar la dirección IP
    addr = (struct sockaddr_in*)&ifr.ifr_addr;
    addr->sin_family = AF_INET;
    addr->sin_addr = ip_addr;

    if (ioctl(fd, SIOCSIFADDR, &ifr) < 0) {
        perror("Error al establecer la dirección IP");
        close(fd);
        return -1;
    }

    // Configurar la máscara de subred
    addr->sin_addr = subnet_mask;

    if (ioctl(fd, SIOCSIFNETMASK, &ifr) < 0) {
        perror("Error al establecer la máscara de subred");
        close(fd);
        return -1;
    }

    // Activar la interfaz
    if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
        perror("Error al obtener las banderas de la interfaz");
        close(fd);
        return -1;
    }

    ifr.ifr_flags |= IFF_UP | IFF_RUNNING;

    if (ioctl(fd, SIOCSIFFLAGS, &ifr) < 0) {
        perror("Error al activar la interfaz");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}