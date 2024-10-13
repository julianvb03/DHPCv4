#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
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


#define ETH_ALEN 6   // Length of an Ethernet address (MAC address)
#define DHCP_OPTIONS_LEN 312

// =========================================
// Estructura de un paquete DHCP y funciones para convertir entre host y network byte order
// =========================================

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

void print_dhcp_struc(const char* buffer, int numbytes){
    for (int i = 0; i < numbytes; i++) {
            printf("%02x ", (unsigned char)buffer[i]);
            if ((i + 1) % 16 == 0) {
                printf("\n");
            }
        }
        printf("\n\n");
}

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

// =========================================
// =========================================


// =========================================
// Funcion para crear un mensaje DHCP Offer
// =========================================

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

void handle_discover(struct dhcp_packet* packet, const char *interface_name) {

    // Prepare the data for the DHCPDISCOVER message

    // Get the MAC address of the network interface and copy it to the chaddr field

    uint8_t chaddr[16] = {0};
    get_hwaddr(interface_name, chaddr);          // Copy the MAC address

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

void handle_offer(struct dhcp_packet* packet, u_int8_t* client_mac) {
    uint8_t chaddr[16] = {0};

    // NERVER USED options
    uint8_t sname[64] = {0};                                    // Empty server name
    uint8_t file[128] = {0};                                    // Empty boot file name
    uint8_t options[DHCP_OPTIONS_LEN] = {0};      // Copy the MAC address

    uint32_t magic_cookie = htonl(DHCP_MAGIC_COOKIE);           // Magic cookie distinguishing DHCP and BOOTP packets
    memcpy(options, &magic_cookie, sizeof(magic_cookie));
    options[4] = DHCP_OPTION_MESSAGE_TYPE;                      // Option: DHCP Message Type
    options[5] = 1;                                             // Length of the option data
    options[6] = DHCPOFFER;                                  // DHCP Discover message type
    options[7] = DHCP_OPTION_END;                               // End Option

    get_dhcp_struc_hton(
        packet, 
        BOOTREPLY, 
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
// =========================================
// =========================================


// =========================================
// Funciones para crear y enviar un mensaje DHCP Discover
// =========================================
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

void send_dhcp_discover(int sockfd, struct dhcp_packet* packet) {
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(68);
    dest_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    if(sendto(sockfd, packet, sizeof(struct dhcp_packet), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        perror("Error al enviar el mensaje");
        close(sockfd);
        exit(1);
    }
    printf("DHCPDISCOVER enviado\n");
}

// =========================================
// =========================================


// =========================================
// Funcion main
// =========================================

int main(int argc, char *argv[]) {
    // Crear un socket DGRAM (UDP)
    if(argc < 2) {
        printf("Uso: %s <nombre_interfaz>\n", argv[0]);
        return 1;
    }

    uint8_t client_mac[ETH_ALEN];
    if(get_hwaddr(argv[1], client_mac) < 0) {
        return -1;
    }
    int sockfd = create_dgram_socket(argv[1]);

    // Crear un mensaje DHCP Discover
    struct dhcp_packet packet;
    handle_offer(&packet, argv[1]);
    //handle_discover(&packet, argv[1]);

    // Enviar el mensaje DHCP Discover
    send_dhcp_discover(sockfd, &packet);

    // Cerrar el socket
    close(sockfd);

    return 0;
}