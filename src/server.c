#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include "dhcp_packet.h"

#define SERVER_PORT 8067

int handle_dhcp_generic(const char *buffer, int numbytes, struct sockaddr *client_addr) {
    char local_buffer[1024] = {0};
    struct dhcp_packet local_net_packet;
    struct dhcp_packet local_host_packet;

    if (numbytes > sizeof(local_buffer)) {
        fprintf(stderr, "Error: numbytes es mayor que el tamaño del buffer local.\n");
        return -1;
    }

    memcpy(local_buffer, buffer, numbytes);
    memcpy(&local_net_packet, local_buffer, sizeof(struct dhcp_packet));
    get_dhcp_struc_ntoh(&local_net_packet, &local_host_packet);

    print_dhcp_struc((const char*)&local_host_packet, sizeof(local_host_packet));

    return 0;
}

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
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error al enlazar el socket");
        close(sockfd);
        exit(1);
    }

    printf("Servidor DHCP escuchando en el puerto %d...\n", SERVER_PORT);

    while (1) {
        memset(buffer, 0, sizeof(buffer));

        int numbytes = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                                (struct sockaddr *)&client_addr, &addr_len);
        if (numbytes < 0) {
            perror("Error al recibir el mensaje");
            continue;
        }

        handle_dhcp_generic(buffer, numbytes, (struct sockaddr *)&client_addr);

    }

    close(sockfd);
    return 0;
}
