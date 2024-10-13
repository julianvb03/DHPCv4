#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68
#define DHCP_MAGIC_COOKIE 0x63825363

typedef u_int32_t ip4_t;
typedef u_int8_t mac_t[6];

#define DHCP_CHADDR_LEN 16
#define DHCP_SNAME_LEN  64
#define DHCP_OPTIONS_LEN 312


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

// Crear el mensaje DHCP Offer
void create_dhcp_offer(struct dhcp_packet *dhcp_offer, mac_t client_mac) {
    memset(dhcp_offer, 0, sizeof(struct dhcp_packet));

    dhcp_offer->op = 2; // Boot Reply
    dhcp_offer->htype = 1;  // Tipo de hardware Ethernet
    dhcp_offer->hlen = 6;   // Longitud de la dirección de hardware
    dhcp_offer->xid = htonl(0x12345678); // ID de la transacción (dummy)
    dhcp_offer->yiaddr = inet_addr("192.168.1.100"); // Dirección IP ofrecida
    dhcp_offer->siaddr = inet_addr("192.168.1.1"); // Dirección IP del servidor DHCP
    memcpy(dhcp_offer->chaddr, client_mac, 6); // MAC del cliente

    // Magic Cookie DHCP
    dhcp_offer->options[0] = htonl(DHCP_MAGIC_COOKIE);

    // Opciones DHCP (mensaje DHCP Offer)
    int opt_idx = 0;
    dhcp_offer->options[opt_idx++] = 53; // Código de la opción: DHCP Message Type
    dhcp_offer->options[opt_idx++] = 1;  // Longitud
    dhcp_offer->options[opt_idx++] = 2;  // DHCP Offer

    dhcp_offer->options[opt_idx++] = 54; // Código de la opción: Server Identifier
    dhcp_offer->options[opt_idx++] = 4;  // Longitud
    dhcp_offer->options[opt_idx++] = 192; // 192.168.1.1 (Servidor DHCP)
    dhcp_offer->options[opt_idx++] = 168;
    dhcp_offer->options[opt_idx++] = 1;
    dhcp_offer->options[opt_idx++] = 1;

    dhcp_offer->options[opt_idx++] = 1; // Código de la opción: Subnet Mask
    dhcp_offer->options[opt_idx++] = 4; // Longitud
    dhcp_offer->options[opt_idx++] = 255; // 255.255.255.0
    dhcp_offer->options[opt_idx++] = 255;
    dhcp_offer->options[opt_idx++] = 255;
    dhcp_offer->options[opt_idx++] = 0;

    dhcp_offer->options[opt_idx++] = 255; // End Option
}

int main() {
    int sockfd;
    struct sockaddr_in dest_addr;
    struct dhcp_packet dhcp_offer;
    mac_t client_mac = {0x00, 0x0c, 0x29, 0x3e, 0x4d, 0x5f}; // MAC del cliente (dummy)

    // Crear un socket DGRAM (UDP)
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }

    // Habilitar el broadcast
    int broadcast_enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)) < 0) {
        perror("Error al habilitar broadcast");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Configurar la dirección de destino (broadcast)
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(DHCP_CLIENT_PORT);
    dest_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    // Crear el mensaje DHCP Offer
    create_dhcp_offer(&dhcp_offer, client_mac);

    // Enviar el paquete DHCP Offer
    if (sendto(sockfd, &dhcp_offer, sizeof(struct dhcp_packet), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        perror("Error al enviar el paquete DHCP Offer");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Paquete DHCP Offer enviado\n");

    // Cerrar el socket
    close(sockfd);

    return 0;
}
