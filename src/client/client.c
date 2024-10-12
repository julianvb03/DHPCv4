#include "dhcpv4_client.h"

int main() {
    int sockfd;
    char buffer[4096];
    struct ifreq ifr;
    struct sockaddr_ll socket_address;
    char interface[] = "lo"; // Cambiar a la interfaz adecuada

    // Crear un socket raw de nivel Ethernet para capturar todos los paquetes IP
    if ((sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP))) < 0) {
        perror("Error creating raw socket");
        exit(EXIT_FAILURE);
    }

    // Vincular el socket a la interfaz específica
    memset(&ifr, 0, sizeof(ifr));
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), interface);
    if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0) {
        perror("Error binding raw socket to interface");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Obtener el índice de la interfaz de red
    if (ioctl(sockfd, SIOCGIFINDEX, &ifr) < 0) {
        perror("Error getting interface index");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Configurar la estructura sockaddr_ll para enviar el paquete
    memset(&socket_address, 0, sizeof(socket_address));
    socket_address.sll_family = AF_PACKET;
    socket_address.sll_ifindex = ifr.ifr_ifindex;
    socket_address.sll_protocol = htons(ETH_P_IP);
    socket_address.sll_halen = ETH_ALEN;

    // Aquí debes rellenar `socket_address.sll_addr` con la dirección MAC de destino
    // Por ejemplo, para enviar un broadcast:
    memset(socket_address.sll_addr, 0xff, ETH_ALEN); // Dirección de broadcast

    // Preparar el paquete DHCP DISCOVER
    struct dhcp_packet packet;
    handle_discover(&packet, &ifr);

    // Imprimir el paquete antes de enviar para depuración
    // print_dhcp_struc((const char*)&packet, sizeof(packet));

    // Enviar el paquete DHCP DISCOVER usando `sendto`
    if (sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr*)&socket_address, sizeof(socket_address)) < 0) {
        perror("Error sending DHCP DISCOVER");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("DHCP DISCOVER enviado\n");

    // Esperar y recibir respuestas DHCP
        ssize_t len = recv(sockfd, buffer, sizeof(buffer), 0);
        if (len < 0) {
            perror("Error receiving data");
        }
        printf("Received a packet of length %zd\n", len);

    close(sockfd);
    return EXIT_SUCCESS;
}