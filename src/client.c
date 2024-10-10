#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>

#define SERVER_PORT 8067
#define CLIENT_PORT 8068

// Estructura del paquete DHCP (simplificada)
struct dhcp_packet {
    uint8_t op;        // Message op code / message type
    uint8_t htype;     // Hardware address type
    uint8_t hlen;      // Hardware address length
    uint8_t hops;      // Hops
    uint32_t xid;      // Transaction ID
    uint16_t secs;     // Seconds elapsed
    uint16_t flags;    // Flags
    uint32_t ciaddr;   // Client IP address
    uint32_t yiaddr;   // 'Your' IP address
    uint32_t siaddr;   // Next server IP address
    uint32_t giaddr;   // Relay agent IP address
    uint8_t chaddr[16]; // Client hardware address
    uint8_t sname[64];  // Optional server host name
    uint8_t file[128];  // Boot file name
    uint8_t options[312]; // Optional parameters field
} __attribute__((packed));

int main() {
    int sockfd;
    struct sockaddr_in their_addr, my_addr;
    struct dhcp_packet packet;
    struct ifreq ifr;
    char interface[] = "lo"; // Reemplaza con tu interfaz

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("Error al crear el socket");
        exit(1);
    }

    // Enlazar el socket a la interfaz de red específica
    memset(&ifr, 0, sizeof(ifr));
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), interface);
    if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0) {
        perror("Error al enlazar el socket a la interfaz");
        close(sockfd);
        exit(1);
    }

    // Enlazar el socket a INADDR_ANY y al puerto CLIENT_PORT (68)
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(CLIENT_PORT);
    my_addr.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0
    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
        perror("Error al enlazar el socket");
        close(sockfd);
        exit(1);
    }

    // Configurar la dirección del servidor (broadcast)
    memset(&their_addr, 0, sizeof(their_addr));
    their_addr.sin_family = AF_INET;
    their_addr.sin_port = htons(SERVER_PORT);
    their_addr.sin_addr.s_addr = INADDR_BROADCAST; // 255.255.255.255

    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &(int){1}, sizeof(int)) < 0) {
        perror("Error al configurar el broadcast");
        close(sockfd);
        exit(1);
    }

    // Obtener la dirección MAC de la interfaz
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("Error al abrir socket para obtener MAC");
        exit(1);
    }
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);
    if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
        perror("Error al obtener la dirección MAC");
        close(fd);
        exit(1);
    }
    close(fd);

    // Construir el mensaje DHCPDISCOVER
    memset(&packet, 0, sizeof(packet));
    packet.op = 0x01;        // Boot Request
    packet.htype = 0x01;     // Ethernet
    packet.hlen = 0x06;      // Longitud de la dirección MAC
    packet.hops = 0x00;
    packet.xid = htonl(0x12345678); // ID de transacción (puedes usar un valor aleatorio)
    packet.secs = htons(0x0000);
    packet.flags = htons(0x8000);   // Solicitar respuesta por broadcast
    memcpy(packet.chaddr, ifr.ifr_hwaddr.sa_data, 6);

    // Opciones DHCP
    packet.options[0] = 99;
    packet.options[1] = 130;
    packet.options[2] = 83;
    packet.options[3] = 99;  // Magic Cookie

    packet.options[4] = 53;  // Opción: DHCP Message Type
    packet.options[5] = 1;   // Longitud
    packet.options[6] = 1;   // DHCPDISCOVER

    packet.options[7] = 255; // End Option

    // Enviar el mensaje DHCPDISCOVER
    if (sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&their_addr, sizeof(their_addr)) < 0) {
        perror("Error al enviar el mensaje DHCPDISCOVER");
        close(sockfd);
        exit(1);
    }

    printf("Mensaje DHCPDISCOVER enviado.\n");

    close(sockfd);
    return 0;
}