#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <net/if.h>

int main() {
    int sockfd;
    struct sockaddr_in their_addr, my_addr;
    struct ifreq ifr;
    char interface[] = "enp0s3"; // Nombre de la interfaz de red
    unsigned char buffer[] = {0x01, 0x23, 0x45, 0x67, 0x89}; // Mensaje en formato hexadecimal
    char recv_buffer[1024]; // Búfer para almacenar el mensaje recibido.
    socklen_t addr_len = sizeof(their_addr);

    // Crear el socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("Error creando el socket");
        exit(EXIT_FAILURE);
    }

    // Configurar el socket para usar la interfaz de red especificada
    memset(&ifr, 0, sizeof(ifr));
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), interface);
    if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0) {
        perror("Error al vincular el socket a la interfaz");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Configuración de la dirección my_addr para recibir
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(8068); // Puerto donde se escucharán los mensajes.
    my_addr.sin_addr.s_addr = INADDR_ANY; // Escuchar en cualquier IP disponible.
    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
        perror("Error al vincular el socket");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Habilitar la opción de difusión (broadcast)
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &(int){1}, sizeof(int)) < 0) {
        perror("Error al habilitar la difusión");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Configuración de la dirección de destino para el broadcast
    memset(&their_addr, 0, sizeof(their_addr));
    their_addr.sin_family = AF_INET;
    their_addr.sin_port = htons(8067); // Cambia SERVER_PORT por el puerto que quieras usar.
    their_addr.sin_addr.s_addr = INADDR_BROADCAST; // Configura para enviar a una dirección de broadcast.

    // Enviar el mensaje de broadcast
    if (sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&their_addr, addr_len) < 0) {
        perror("Error al enviar el mensaje de broadcast");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Mensaje de broadcast enviado exitosamente desde la interfaz %s, puerto %d.\n", interface, 8067);

    // Escuchar un único mensaje de broadcast después de enviar
    memset(recv_buffer, 0, sizeof(recv_buffer));
    ssize_t num_bytes = recvfrom(sockfd, recv_buffer, sizeof(recv_buffer) - 1, 0, (struct sockaddr *)&their_addr, &addr_len);
    if (num_bytes < 0) {
        perror("Error al recibir datos");
    } else {
        recv_buffer[num_bytes] = '\0'; // Asegurar que el mensaje termine en '\0'
        printf("Mensaje recibido de %s:%d - %s\n",
               inet_ntoa(their_addr.sin_addr),
               ntohs(their_addr.sin_port),
               recv_buffer);
    }

    close(sockfd);
    return EXIT_SUCCESS;
}

