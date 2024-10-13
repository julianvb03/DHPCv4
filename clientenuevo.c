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

// void process_packet(unsigned char *buffer, int len) {
//     struct iphdr *iph = (struct iphdr *)(buffer); // Cabecera IP
//     if (iph->protocol == IPPROTO_UDP) { // Si es un paquete UDP
//         struct udphdr *udph = (struct udphdr *)(buffer + iph->ihl * 4); // Cabecera UDP

//         // Filtrar paquetes DHCP por puerto UDP 67 (servidor) y 68 (cliente)    ntohs(udph->source) == DHCP_SERVER_PORT || 
//         if (ntohs(udph->dest) == CLIENT_PORT) {
//             printf("Paquete DHCP capturado! Origen: %d, Destino: %d\n",
//                    ntohs(udph->source), ntohs(udph->dest));
//             // Aquí puedes procesar el contenido del paquete DHCP
//         }
//     }
// }

// void process_packet(unsigned char *buffer, int len) {
//     struct iphdr *iph = (struct iphdr *)(buffer); // Cabecera IP

//     // Imprimir los campos básicos de la cabecera IP
//     printf("===== Cabecera IP =====\n");
//     printf("Version: %d\n", iph->version);
//     printf("Longitud de la cabecera: %d\n", iph->ihl * 4);  // ihl en número de palabras de 32 bits
//     printf("Longitud total: %d\n", ntohs(iph->tot_len));
//     printf("Protocolo: %d\n", iph->protocol);
//     printf("IP origen: %s\n", inet_ntoa(*(struct in_addr *)&iph->saddr));
//     printf("IP destino: %s\n", inet_ntoa(*(struct in_addr *)&iph->daddr));

//     // Si es un paquete UDP, seguir con la impresión
//     if(iph->protocol == IPPROTO_UDP) {
//         struct udphdr *udph = (struct udphdr *)(buffer + iph->ihl * 4); // Cabecera UDP

//         printf("===== Cabecera UDP =====\n");
//         printf("Puerto origen: %d\n", ntohs(udph->source));
//         printf("Puerto destino: %d\n", ntohs(udph->dest));
//         printf("Longitud UDP: %d\n", ntohs(udph->len));

//         // Filtrar paquetes DHCP (puertos 67 y 68)
//         if(ntohs(udph->dest) == CLIENT_PORT) {
//             printf("===== Paquete DHCP capturado =====\n");
//             printf("Origen: %d, Destino: %d\n", ntohs(udph->source), ntohs(udph->dest));

//             // Imprimir la parte de datos (payload) en formato hexadecimal
//             int udp_header_len = sizeof(struct udphdr);
//             int ip_header_len = iph->ihl * 4;
//             int dhcp_data_len = ntohs(udph->len) - udp_header_len;  // Longitud de los datos DHCP
//             unsigned char *dhcp_data = buffer + ip_header_len + udp_header_len;

//             printf("===== Contenido del paquete DHCP (hexadecimal) =====\n");
//             print_hex2(dhcp_data, dhcp_data_len);
//         }
//     }
// }


void process_packet(unsigned char *buffer, int len) {
    struct iphdr *iph = (struct iphdr *)(buffer); // Cabecera IP

    // Verificar si es IPv4 (versión debe ser 4)
    if (iph->version != 4) {
        printf("No es un paquete IPv4\n");
        return;
    }

    // Imprimir los campos básicos de la cabecera IP
    printf("===== Cabecera IP =====\n");
    printf("Version: %d\n", iph->version);
    printf("Longitud de la cabecera: %d\n", iph->ihl * 4);
    printf("Longitud total: %d\n", ntohs(iph->tot_len));
    printf("Protocolo: %d\n", iph->protocol);
    printf("IP origen: %s\n", inet_ntoa(*(struct in_addr *)&iph->saddr));
    printf("IP destino: %s\n", inet_ntoa(*(struct in_addr *)&iph->daddr));

    // Verificar si es un paquete UDP (protocolo 17)
    if (iph->protocol == IPPROTO_UDP) {
        struct udphdr *udph = (struct udphdr *)(buffer + iph->ihl * 4); // Cabecera UDP

        printf("===== Cabecera UDP =====\n");
        printf("Puerto origen: %d\n", ntohs(udph->source));
        printf("Puerto destino: %d\n", ntohs(udph->dest));
        printf("Longitud UDP: %d\n", ntohs(udph->len));

        // Imprimir el contenido del paquete DHCP en hexadecimal
        printf("===== Paquete DHCP (hexadecimal) =====\n");
        int udp_header_len = sizeof(struct udphdr);
        int ip_header_len = iph->ihl * 4;
        int dhcp_data_len = ntohs(udph->len) - udp_header_len;  // Longitud de los datos DHCP
        unsigned char *dhcp_data = buffer + ip_header_len + udp_header_len;

        // Imprimir todo el paquete en formato hexadecimal
        print_hex2(buffer, len);
    } else {
        printf("No es un paquete UDP\n");
    }
}

// =================================================
// Funcion main
// =================================================

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
    print_dhcp_strct(&dhcp_discover);
    
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
    while (1) {
        int packet_len = recvfrom(sockfd_raw, buffer, sizeof(buffer), 0, &saddr, &saddr_len);
        if (packet_len < 0) {
            perror("Error al recibir paquete");
            continue;
        }

        // Procesar el paquete recibido
        process_packet(buffer, packet_len);
    }

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
void run_dhcp_test_cases() {
    // Caso de prueba 1: DHCP Discover
    struct dhcp_packet dhcp_discover;
    memset(&dhcp_discover, 0, sizeof(struct dhcp_packet)); // Limpiar la estructura
    dhcp_discover.op = 1;  // Request
    dhcp_discover.htype = 1; // Ethernet
    dhcp_discover.hlen = 6;  // Longitud de dirección MAC
    dhcp_discover.xid = htonl(0x12345678); // ID de transacción
    dhcp_discover.secs = htons(10);  // Segundos transcurridos
    dhcp_discover.flags = htons(0x8000);  // Bandera de broadcast
    // Dirección MAC ficticia
    dhcp_discover.chaddr[0] = 0x00;
    dhcp_discover.chaddr[1] = 0x0c;
    dhcp_discover.chaddr[2] = 0x29;
    dhcp_discover.chaddr[3] = 0x3e;
    dhcp_discover.chaddr[4] = 0x53;
    dhcp_discover.chaddr[5] = 0x2a;

    // Opción de tipo de mensaje DHCP: DHCPDISCOVER
    dhcp_discover.options[0] = DHCP_OPTION_MESSAGE_TYPE;
    dhcp_discover.options[1] = 1; // Longitud de la opción
    dhcp_discover.options[2] = 1; // DHCPDISCOVER
    dhcp_discover.options[3] = DHCP_OPTION_END; // Fin de opciones

    printf("==== Caso de Prueba 1: DHCP Discover ====\n");
    print_dhcp_strct(&dhcp_discover);
    printf("=========================================\n\n");

    // Caso de prueba 2: DHCP Offer
    struct dhcp_packet dhcp_offer;
    memset(&dhcp_offer, 0, sizeof(struct dhcp_packet)); // Limpiar la estructura
    dhcp_offer.op = 2;  // Reply
    dhcp_offer.htype = 1; // Ethernet
    dhcp_offer.hlen = 6;  // Longitud de dirección MAC
    dhcp_offer.xid = htonl(0x87654321); // ID de transacción
    dhcp_offer.secs = htons(20);  // Segundos transcurridos
    dhcp_offer.flags = htons(0x0000);  // No es broadcast
    // Dirección MAC ficticia
    dhcp_offer.chaddr[0] = 0x00;
    dhcp_offer.chaddr[1] = 0x0c;
    dhcp_offer.chaddr[2] = 0x29;
    dhcp_offer.chaddr[3] = 0x4f;
    dhcp_offer.chaddr[4] = 0x1e;
    dhcp_offer.chaddr[5] = 0x7b;

    // Opción de tipo de mensaje DHCP: DHCPOFFER
    dhcp_offer.options[0] = DHCP_OPTION_MESSAGE_TYPE;
    dhcp_offer.options[1] = 1; // Longitud de la opción
    dhcp_offer.options[2] = 2; // DHCPOFFER
    dhcp_offer.options[3] = DHCP_OPTION_END; // Fin de opciones

    printf("==== Caso de Prueba 2: DHCP Offer ====\n");
    print_dhcp_strct(&dhcp_offer);
    printf("=========================================\n\n");

    // Caso de prueba 3: DHCP Request
    struct dhcp_packet dhcp_request;
    memset(&dhcp_request, 0, sizeof(struct dhcp_packet)); // Limpiar la estructura
    dhcp_request.op = 1;  // Request
    dhcp_request.htype = 1; // Ethernet
    dhcp_request.hlen = 6;  // Longitud de dirección MAC
    dhcp_request.xid = htonl(0xabcdef12); // ID de transacción
    dhcp_request.secs = htons(30);  // Segundos transcurridos
    dhcp_request.flags = htons(0x8000);  // Bandera de broadcast
    // Dirección MAC ficticia
    dhcp_request.chaddr[0] = 0x00;
    dhcp_request.chaddr[1] = 0x0a;
    dhcp_request.chaddr[2] = 0x95;
    dhcp_request.chaddr[3] = 0x9d;
    dhcp_request.chaddr[4] = 0x68;
    dhcp_request.chaddr[5] = 0x16;

    // Opción de tipo de mensaje DHCP: DHCPREQUEST
    dhcp_request.options[0] = DHCP_OPTION_MESSAGE_TYPE;
    dhcp_request.options[1] = 1; // Longitud de la opción
    dhcp_request.options[2] = 3; // DHCPREQUEST
    dhcp_request.options[3] = DHCP_OPTION_END; // Fin de opciones

    printf("==== Caso de Prueba 3: DHCP Request ====\n");
    print_dhcp_strct(&dhcp_request);
    printf("=========================================\n\n");

    // Caso de prueba 4: DHCP ACK
    struct dhcp_packet dhcp_ack;
    memset(&dhcp_ack, 0, sizeof(struct dhcp_packet)); // Limpiar la estructura
    dhcp_ack.op = 2;  // Reply
    dhcp_ack.htype = 1; // Ethernet
    dhcp_ack.hlen = 6;  // Longitud de dirección MAC
    dhcp_ack.xid = htonl(0x123abc45); // ID de transacción
    dhcp_ack.secs = htons(40);  // Segundos transcurridos
    dhcp_ack.flags = htons(0x0000);  // No es broadcast
    // Dirección MAC ficticia
    dhcp_ack.chaddr[0] = 0x00;
    dhcp_ack.chaddr[1] = 0x1a;
    dhcp_ack.chaddr[2] = 0x92;
    dhcp_ack.chaddr[3] = 0x56;
    dhcp_ack.chaddr[4] = 0x78;
    dhcp_ack.chaddr[5] = 0xbc;

    // Opción de tipo de mensaje DHCP: DHCPACK
    dhcp_ack.options[0] = DHCP_OPTION_MESSAGE_TYPE;
    dhcp_ack.options[1] = 1; // Longitud de la opción
    dhcp_ack.options[2] = 5; // DHCPACK
    dhcp_ack.options[3] = DHCP_OPTION_END; // Fin de opciones

    printf("==== Caso de Prueba 4: DHCP ACK ====\n");
    print_dhcp_strct(&dhcp_ack);
    printf("=========================================\n\n");

    // Caso de prueba 5: DHCP NAK
    struct dhcp_packet dhcp_nak;
    memset(&dhcp_nak, 0, sizeof(struct dhcp_packet)); // Limpiar la estructura
    dhcp_nak.op = 2;  // Reply
    dhcp_nak.htype = 1; // Ethernet
    dhcp_nak.hlen = 6;  // Longitud de dirección MAC
    dhcp_nak.xid = htonl(0xdeadbeef); // ID de transacción
    dhcp_nak.secs = htons(50);  // Segundos transcurridos
    dhcp_nak.flags = htons(0x0000);  // No es broadcast
    // Dirección MAC ficticia
    dhcp_nak.chaddr[0] = 0x00;
    dhcp_nak.chaddr[1] = 0x50;
    dhcp_nak.chaddr[2] = 0x56;
    dhcp_nak.chaddr[3] = 0xc0;
    dhcp_nak.chaddr[4] = 0x00;
    dhcp_nak.chaddr[5] = 0x08;

    // Opción de tipo de mensaje DHCP: DHCPNAK
    dhcp_nak.options[0] = DHCP_OPTION_MESSAGE_TYPE;
    dhcp_nak.options[1] = 1; // Longitud de la opción
    dhcp_nak.options[2] = 6; // DHCPNAK
    dhcp_nak.options[3] = DHCP_OPTION_END; // Fin de opciones

    printf("==== Caso de Prueba 5: DHCP NAK ====\n");
    print_dhcp_strct(&dhcp_nak);
    printf("=========================================\n\n");

    // Caso de prueba 6: DHCP Release
    struct dhcp_packet dhcp_release;
    memset(&dhcp_release, 0, sizeof(struct dhcp_packet)); // Limpiar la estructura
    dhcp_release.op = 1;  // Request
    dhcp_release.htype = 1; // Ethernet
    dhcp_release.hlen = 6;  // Longitud de dirección MAC
    dhcp_release.xid = htonl(0x12345678); // ID de transacción
    dhcp_release.secs = htons(60);  // Segundos transcurridos
    dhcp_release.flags = htons(0x0000);  // No es broadcast
    // Dirección MAC ficticia
    dhcp_release.chaddr[0] = 0x00;
    dhcp_release.chaddr[1] = 0x16;
    dhcp_release.chaddr[2] = 0x36;
    dhcp_release.chaddr[3] = 0x12;
    dhcp_release.chaddr[4] = 0x78;
    dhcp_release.chaddr[5] = 0x1a;

    // Opción de tipo de mensaje DHCP: DHCPRELEASE
    dhcp_release.options[0] = DHCP_OPTION_MESSAGE_TYPE;
    dhcp_release.options[1] = 1; // Longitud de la opción
    dhcp_release.options[2] = 7; // DHCPRELEASE
    dhcp_release.options[3] = DHCP_OPTION_END; // Fin de opciones

    printf("==== Caso de Prueba 6: DHCP Release ====\n");
    print_dhcp_strct(&dhcp_release);
    printf("=========================================\n\n");

    // Caso de prueba 7: DHCP Inform
    struct dhcp_packet dhcp_inform;
    memset(&dhcp_inform, 0, sizeof(struct dhcp_packet)); // Limpiar la estructura
    dhcp_inform.op = 1;  // Request
    dhcp_inform.htype = 1; // Ethernet
    dhcp_inform.hlen = 6;  // Longitud de dirección MAC
    dhcp_inform.xid = htonl(0xdeadbeef); // ID de transacción
    dhcp_inform.secs = htons(70);  // Segundos transcurridos
    dhcp_inform.flags = htons(0x0000);  // No es broadcast
    // Dirección MAC ficticia
    dhcp_inform.chaddr[0] = 0x00;
    dhcp_inform.chaddr[1] = 0x50;
    dhcp_inform.chaddr[2] = 0x56;
    dhcp_inform.chaddr[3] = 0xc0;
    dhcp_inform.chaddr[4] = 0x00;
    dhcp_inform.chaddr[5] = 0x08;

    // Opción de tipo de mensaje DHCP: DHCPINFORM
    dhcp_inform.options[0] = DHCP_OPTION_MESSAGE_TYPE;
    dhcp_inform.options[1] = 1; // Longitud de la opción
    dhcp_inform.options[2] = 8; // DHCPINFORM
    dhcp_inform.options[3] = DHCP_OPTION_END; // Fin de opciones

    printf("==== Caso de Prueba 7: DHCP Inform ====\n");
    print_dhcp_strct(&dhcp_inform);
    printf("=========================================\n\n");
}