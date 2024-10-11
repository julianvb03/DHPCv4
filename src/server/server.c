#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include "dhcpv4_utils.h"

#define SERVER_PORT 8067

struct dhcp_thread_data {
    char buffer[1024];
    int numbytes;
    struct sockaddr_in client_addr;
};

void *handle_dhcp_generic(void *arg) {
    struct dhcp_thread_data *data = (struct dhcp_thread_data *)arg;

    struct dhcp_packet local_net_packet;
    struct dhcp_packet local_host_packet;

    memcpy(&local_net_packet, data->buffer, sizeof(struct dhcp_packet));
    get_dhcp_struc_ntoh(&local_net_packet, &local_host_packet);

    printf("Paquete DHCP recibido y procesado en el hilo %ld:\n", (unsigned long int)pthread_self());
    print_dhcp_struc((const char*)&local_host_packet, sizeof(local_host_packet));
    usleep(1000000);
    free(data);
    return NULL;
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

        struct dhcp_thread_data *data = malloc(sizeof(struct dhcp_thread_data));
        if (data == NULL) {
            perror("Error al asignar memoria para los datos del hilo");
            continue;
        }

        memcpy(data->buffer, buffer, numbytes);
        data->numbytes = numbytes;
        data->client_addr = client_addr;

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_dhcp_generic, data) != 0) {
            perror("Error al crear el hilo");
            free(data);
        } else {
            pthread_detach(thread_id);
        }
        usleep(100000);
    }

    close(sockfd);
    return 0;
}
