#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define SERVER_PORT 8067

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[1025];
    socklen_t addr_len = sizeof(client_addr);
    
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error al crear el socket");
        exit(1);
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &(int){1}, sizeof(int)) < 0) {
        perror("Error al configurar SO_BROADCAST");
        close(sockfd);
        exit(1);
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) == -1) {
        close(sockfd);
        perror("Error al configurar SO_REUSEADDR");
        return 3;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // Escuchar en cualquier IP

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error al enlazar el socket");
        close(sockfd);
        exit(1);
    }

    printf("Servidor DHCP escuchando en el puerto %d...\n", SERVER_PORT);

    while (1) {
        memset(buffer, 0, sizeof(buffer));

        int numbytes = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                                (struct sockaddr *)&client_addr, &addr_len);
        if (numbytes < 0) {
            perror("Error al recibir el mensaje");
            continue;
        }

        printf("Mensaje recibido de %s:%d\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        printf("Datos (%d bytes):\n", numbytes);

        // Imprimir los bytes recibidos en formato hexadecimal
        for (int i = 0; i < numbytes; i++) {
            printf("%02x ", (unsigned char)buffer[i]);
            if ((i + 1) % 16 == 0) {
                printf("\n");
            }
        }
        printf("\n\n");
    }

    close(sockfd);
    return 0;
}
