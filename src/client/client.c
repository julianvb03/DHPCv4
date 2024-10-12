#include "dhcpv4_client.h"

int main() {
    int sockfd;
    char buffer[4096];
    struct ifreq ifr;
    struct sockaddr_ll socket_address;
    char interface[] = "lo"; // Cambiar a la interfaz adecuada

    // Crear un socket raw de nivel Ethernet para capturar todos los paquetes
    if ((sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
        perror("Error creating raw socket");
        exit(EXIT_FAILURE);
    }

    // Vincular el socket a la interfaz específica
    memset(&ifr, 0, sizeof(ifr));
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", interface);
    if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0) {
        perror("Error binding socket to interface");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Configurar el tiempo de espera para recibir la respuesta
    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_SECONDS;
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Error setting timeout");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Preparar el paquete DHCP DISCOVER
    struct dhcp_packet packet;
    handle_discover(&packet, &ifr);

    // Configurar la dirección de broadcast para el envío
    struct sockaddr_in broadcast_addr;
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(SERVER_PORT);  // Puerto del servidor DHCP
    broadcast_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    // Habilitar el envío de paquetes a la dirección de broadcast
    int broadcast_enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)) < 0) {
        perror("Error enabling broadcast option");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Enviar el paquete DHCP DISCOVER usando `send_with_retries`

    int result = send_with_retries(
        sockfd, 
        &packet, 
        sizeof(packet), 
        (struct sockaddr*)&broadcast_addr, 
        sizeof(broadcast_addr), 
        buffer, 
        sizeof(buffer), 
        TIMEOUT_SECONDS
    );

    if (result < 0) {
        printf("Failed to receive a DHCP OFFER after multiple attempts.\n");
        close(sockfd);
        exit(EXIT_FAILURE); 
    } 

    printf("Received a DHCP OFFER of length %d bytes.\n", result);

    close(sockfd);
    return EXIT_SUCCESS; 
}