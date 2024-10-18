#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>  // Para obtener la dirección MAC
#include <netinet/ip.h>
#include <netinet/udp.h>

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
#define DHCP_OPTION_PAD 0

#define SERVER_PORT 67
#define CLIENT_PORT 68

#define DNS_OPTION 6
#define MASK_OPTION 1
#define GW_OPTION 4

// =================================================
// Funciones para el manejo de la estructura dhcp_packet
// =================================================

struct dhcp_packet {
    uint8_t op;        // Message op code / message type { 1 = START 2 = REPLY }
    uint8_t htype;     // Hardware address type
    uint8_t hlen;      // MAC address length
    uint8_t hops;      // Counter for the number of jumps on routers before reaching the server
    uint32_t xid;      // Transaction ID { Static on own case for debugging }
    uint16_t secs;     // Seconds elapsed after the client started the DHCP process
    uint16_t flags;    // Flags 1 = Broadcast 0 = Unicast
    uint32_t ciaddr;   // Client IP address { For renewing }
    uint32_t yiaddr;   // 'Your' IP address { IP offered to the client }
    uint32_t siaddr;   // Next server IP address { Optional and using only wen we work with auxiliary servers }             NERVER USED
    uint32_t giaddr;   // Relay agent IP address
    uint8_t chaddr[16]; // MAC address client
    uint8_t sname[64];  // Optional server host name                                                                        NEVER USED
    uint8_t file[128];  // Boot file name { Outburst loading files; using for aditionals files for the client or OS }       NEVER USED
    uint8_t options[DHCP_OPTIONS_LEN]; // Optional parameters field
} __attribute__((packed));  // For avoid padding

struct network_config {
    struct in_addr ip_addr;        // Dirección IP ofrecida
    struct in_addr subnet_mask;    // Máscara de subred
    struct in_addr gateway;        // Gateway predeterminado
    struct in_addr dns_server;     // Servidor DNS
};

void print_hex(const uint8_t *data, int len);  // Prototipo de función
void run_dhcp_test_cases();

void get_dhcp_struc_hton(struct dhcp_packet *packet, uint8_t op, uint8_t htype, uint8_t hlen, uint8_t hops, uint32_t xid, uint16_t secs, uint16_t flags, uint32_t ciaddr, uint32_t yiaddr, uint32_t siaddr, uint32_t giaddr, uint8_t chaddr[16], uint8_t sname[64], uint8_t file[128], uint8_t options[312]){
     // Set single-byte fields directly
    packet->op = op;
    packet->htype = htype;
    packet->hlen = hlen;
    packet->hops = hops;

    // Convert multi-byte fields to network byte order
    packet->xid = htonl(xid);      // Transaction ID
    packet->secs = htons(secs);    // Seconds elapsed
    packet->flags = htons(flags);  // Flags
    packet->ciaddr = htonl(ciaddr); // Client IP address
    packet->yiaddr = htonl(yiaddr); // 'Your' (client) IP address
    packet->siaddr = htonl(siaddr); // Next server IP address
    packet->giaddr = htonl(giaddr); // Relay agent IP address

    // Copy arrays directly (no byte order conversion needed)
    memcpy(packet->chaddr, chaddr, 16);
    memcpy(packet->sname, sname, 64);
    memcpy(packet->file, file, 128);
    memcpy(packet->options, options, 312);
}

void get_dhcp_struc_ntoh(struct dhcp_packet *net_packet, struct dhcp_packet *host_packet) {
    // Copy single-byte fields directly
    host_packet->op = net_packet->op;
    host_packet->htype = net_packet->htype;
    host_packet->hlen = net_packet->hlen;
    host_packet->hops = net_packet->hops;

    // Convert multi-byte fields from network to host byte order
    host_packet->xid = ntohl(net_packet->xid);
    host_packet->secs = ntohs(net_packet->secs);
    host_packet->flags = ntohs(net_packet->flags);
    host_packet->ciaddr = ntohl(net_packet->ciaddr);
    host_packet->yiaddr = ntohl(net_packet->yiaddr);
    host_packet->siaddr = ntohl(net_packet->siaddr);
    host_packet->giaddr = ntohl(net_packet->giaddr);

    // Copy arrays directly (no byte order conversion needed)
    memcpy(host_packet->chaddr, net_packet->chaddr, 16);
    memcpy(host_packet->sname, net_packet->sname, 64);
    memcpy(host_packet->file, net_packet->file, 128);
    memcpy(host_packet->options, net_packet->options, 312);
}

void print_dhcp_strct(struct dhcp_packet *packet) {
    struct in_addr ip_addr;

    printf("===== DHCP Packet =====\n");
    
    // Imprimir los campos de la estructura dhcp_packet
    printf("op: %d (1=Request, 2=Reply)\n", packet->op);
    printf("htype: %d (Hardware Type)\n", packet->htype);
    printf("hlen: %d (Hardware Address Length)\n", packet->hlen);
    printf("hops: %d (Hops)\n", packet->hops);
    
    printf("xid: 0x%x (Transaction ID)\n", ntohl(packet->xid));
    printf("secs: %d (Seconds elapsed)\n", ntohs(packet->secs));
    printf("flags: 0x%x (Flags)\n", ntohs(packet->flags));

    // Imprimir direcciones IP en formato legible
    ip_addr.s_addr = packet->ciaddr;
    printf("ciaddr: %s (Client IP Address)\n", inet_ntoa(ip_addr));

    ip_addr.s_addr = packet->yiaddr;
    printf("yiaddr: %s (Your IP Address - IP offered to the client)\n", inet_ntoa(ip_addr));

    ip_addr.s_addr = packet->siaddr;
    printf("siaddr: %s (Next Server IP Address)\n", inet_ntoa(ip_addr));

    ip_addr.s_addr = packet->giaddr;
    printf("giaddr: %s (Relay Agent IP Address)\n", inet_ntoa(ip_addr));

    // Imprimir la dirección MAC (los primeros 6 bytes son generalmente relevantes)
    printf("MAC Address (chaddr): ");
    for (int i = 0; i < 6; i++) {
        printf("%02x", packet->chaddr[i]);
        if (i < 5) printf(":");
    }
    printf("\n");

    printf("sname: \n");
    print_hex(packet->sname, 64);

    printf("file: \n");
    print_hex(packet->file, 128);

    printf("options: \n");
    print_hex(packet->options, DHCP_OPTIONS_LEN);

    printf("=========================\n");
}

void print_hex(const uint8_t *data, int len) {
    for (int i = 0; i < len; i++) {
        printf("%02x ", data[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    if (len % 16 != 0) {
        printf("\n");
    }
} 

void print_hex2(const unsigned char *data, int length) {
    for (int i = 0; i < length; i++) {
        printf("%02x ", data[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");  // Salto de línea cada 16 bytes
        }
    }
    printf("\n");
}

// Por probar

int find_dhcp_magic_cookie(uint8_t *buffer, int buffer_len) {
    // Buscar la Magic Cookie (4 bytes) en el buffer
    for (int i = 0; i <= buffer_len - 4; i++) {
        if (buffer[i] == 0x63 && buffer[i + 1] == 0x82 &&
            buffer[i + 2] == 0x53 && buffer[i + 3] == 0x63) {
            return i;  // Devolver el offset donde se encuentra la Magic Cookie
        }
    }
    return -1;  // No se encontró la Magic Cookie
}

// Por probar

void print_dhcp_message_type(unsigned char type) {
    switch (type) {
        case 1: printf("DHCP Discover\n"); break;
        case 2: printf("DHCP Offer\n"); break;
        case 3: printf("DHCP Request\n"); break;
        case 4: printf("DHCP Decline\n"); break;
        case 5: printf("DHCP ACK\n"); break;
        case 6: printf("DHCP NAK\n"); break;
        case 7: printf("DHCP Release\n"); break;
        case 8: printf("DHCP Inform\n"); break;
        default: printf("Unknown DHCP message type\n");
    }
}

// =================================================


// =================================================
// Funciones para el manejo de la interfaz de red y los sockets
// =================================================

int get_hwaddr(const char *ifname, uint8_t *hwaddr) {
    struct ifreq ifr;
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    
    if (sockfd < 0) {
        perror("Error al abrir socket");
        return -1;
    }

    // Copiar el nombre de la interfaz a la estructura ifreq
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);

    // Obtener la dirección MAC de la interfaz
    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) == -1) {
        perror("Error al obtener la dirección MAC");
        close(sockfd);
        return -1;
    }

    // Copiar la dirección MAC obtenida en el buffer de hwaddr
    memcpy(hwaddr, ifr.ifr_hwaddr.sa_data, ETH_ALEN);

    close(sockfd);
    return 0;
}

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

int process_packet(unsigned char *buffer, int len, struct dhcp_packet *dhcp_packet_main) {
    struct ethhdr *eth = (struct ethhdr *)(buffer);

    // Verificar si es un paquete IPv4
    if (ntohs(eth->h_proto) != ETH_P_IP) {
        printf("No es un paquete IPv4\n");
        return -1;
    }

    struct iphdr *iph = (struct iphdr *)(buffer + sizeof(struct ethhdr));

    if (iph->version != 4) {
        printf("No es un paquete IPv4\n");
        return -1;
    }

    if(iph->protocol != IPPROTO_UDP) {
        printf("No es un paquete UDP\n");
        return -1;
    }

    if(iph->protocol != IPPROTO_UDP) {
        printf("No es un paquete UDP\n");
        return -1;
    }

    struct udphdr *udph = (struct udphdr *)(buffer + sizeof(struct ethhdr) + iph->ihl * 4);
    if(udph->dest != htons(CLIENT_PORT)) {
        printf("No es un paquete DHCP Reply\n");
        return -1;
    }

    int udp_header_len = sizeof(struct udphdr);
    int ip_header_len = iph->ihl * 4;
    int dhcp_data_len = ntohs(udph->len) - udp_header_len;  // Longitud de los datos DHCP
    unsigned char *dhcp_data = buffer + sizeof(struct ethhdr) + ip_header_len + udp_header_len;
    struct dhcp_packet *dhcp_packet = (struct dhcp_packet *)dhcp_data;
    struct dhcp_packet local_packet;
    memset(&local_packet, 0, sizeof(local_packet));
    get_dhcp_struc_ntoh(dhcp_packet, &local_packet);

    //print_dhcp_strct(dhcp_packet);
    if(dhcp_packet->op == BOOTREPLY) {
        printf("DHCP Reply\n");
        unsigned char *options = dhcp_packet->options;
        int offset = find_dhcp_magic_cookie(options, DHCP_OPTIONS_LEN);
        if(offset < 0) {
            printf("Magic cookie not found\n");
            return -1;
        }
        unsigned char *dhcp_options = options + offset + 4;  // Skip the magic cookie
        unsigned char dhcp_message_type = 0;
        while(dhcp_options[0] != DHCP_OPTION_END) {
            if(dhcp_options[0] == DHCP_OPTION_MESSAGE_TYPE) {
                dhcp_message_type = dhcp_options[2];
                break;
            }
            dhcp_options += dhcp_options[1] + 2;
        }
        print_dhcp_message_type(dhcp_message_type);
        memcpy(dhcp_packet_main, &local_packet, sizeof(struct dhcp_packet));
        printf("DHCP packet copied\n");

        return dhcp_message_type;
    } else {
        printf("Not a DHCP Reply\n");
        return -1;
    } 

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
    options[9] = 10800;                                          // Time in seconds
    
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

// =================================================
// Funcion main
// =================================================

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


int main(int argc, char *argv[]) {
    if(argc < 2) {
        printf("Uso: <nombre_interfaz>\n");
        return 1;
    }

    uint8_t client_mac[ETH_ALEN];
    if(get_hwaddr(argv[1], client_mac) < 0) {
        return -1;
    }

    printf("Dirección MAC de %s: %02x:%02x:%02x:%02x:%02x:%02x\n", argv[1],
           client_mac[0], client_mac[1], client_mac[2], client_mac[3], client_mac[4], client_mac[5]);

    int if_index = if_nametoindex(argv[1]);
    if (if_index == 0) {
        perror("No se pudo obtener el índice de la interfaz");
        return -1;
    }

    struct dhcp_packet dhcp_discover;
    handle_discover(&dhcp_discover, client_mac);
    //print_dhcp_strct(&dhcp_discover);
    
    int sockfd_dgram = create_dgram_socket(argv[1]);
    if(sockfd_dgram < 0) {
        return -1;
    }

    if(send_dhcp_package(sockfd_dgram, &dhcp_discover) < 0) {
        close(sockfd_dgram);
        return -1;
    }

    int sockfd_raw = open_raw_socket(argv[1], client_mac, if_index);
    if(sockfd_raw < 0) {
        return -1;
    }

    //struct dhcp_packet local_net_packet;

    unsigned char buffer[65536]; // Buffer grande para capturar paquetes
    struct sockaddr saddr;
    socklen_t saddr_len = sizeof(saddr);

    printf("Capturando paquetes DHCPv4...\n");

    // Bucle para capturar paquetes continuamente
    struct dhcp_packet dhcp_packet;
    while (1) {
        printf("Esperando paquete...\n");
        int status;
        int packet_len = recvfrom(sockfd_raw, buffer, sizeof(buffer), 0, &saddr, &saddr_len);
        if (packet_len < 0) {
            perror("Error al recibir paquete");
            continue;
        }
        // Validar mensaje tio Offer
        status = process_packet(buffer, packet_len, &dhcp_packet);
        if(status == DHCPOFFER) {
            break;
        }
        else {
            printf("Not a DHCP Offer\n");
            close(sockfd_raw);
            close(sockfd_dgram);
            return -1;
        }
    }

    //print_dhcp_strct(&dhcp_packet);
    struct network_config net_config;
    if(handle_offer(&dhcp_packet, &net_config) < 0) {
        close(sockfd_raw);
        close(sockfd_dgram);
        return -1;
    }

    if(offer_validation(&net_config) < 0) {
        close(sockfd_raw);
        close(sockfd_dgram);
        return -1;
    }

    handle_request(&dhcp_packet, &net_config);

    if(send_dhcp_package(sockfd_dgram, &dhcp_packet) < 0) {
        close(sockfd_raw);
        close(sockfd_dgram);
        return -1;
    }

    struct dhcp_packet dhcp_packet2;
    while (1) {
        int status;
        int packet_len = recvfrom(sockfd_raw, buffer, sizeof(buffer), 0, &saddr, &saddr_len);
        if (packet_len < 0) {
            perror("Error al recibir paquete");
            continue;
        }
        // Validar mensaje tio Offer
        status = process_packet(buffer, packet_len, &dhcp_packet2);
        if(status == DHCPACK) {
            break;
        }
    }

    printf("DHCP ACK recibido\n");
    //print_dhcp_strct(&dhcp_packet2);
    if(set_ip_address(argv[1], net_config.ip_addr, net_config.subnet_mask) < 0) {
        printf("Error al configurar la dirección IP\n");
        close(sockfd_raw);
        close(sockfd_dgram);
        return -1;
    }

    printf("Dirección IP configurada correctamente\n");

    close(sockfd_raw);
    close(sockfd_dgram);

    //run_dhcp_test_cases();
    return 0;

}


// {

// // Enviar el mensaje DHCPDISCOVER
//     struct sockaddr_in dest_addr;
//     memset(&dest_addr, 0, sizeof(dest_addr));
//     dest_addr.sin_family = AF_INET;
//     dest_addr.sin_port = htons(SERVER_PORT);  // Puerto del servidor DHCP
//     dest_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

//     if(sendto(sockfd, &dhcp_discover, sizeof(struct dhcp_packet), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
//         perror("Error al enviar el mensaje");
//         close(sockfd);
//         exit(1);
//     }
//     printf("DHCPDISCOVER enviado\n");

// }

// =================================================
// Casos de prueba para las funciones
// =================================================