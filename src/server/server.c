#include "dhcpv4_server.h"

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

    struct ip_in_poll ips[300];
    memset(ips, 0, sizeof(ips));
    if(create_poll(ips, 202, "192.168.1.1", "255.255.255.0") < 0) {
        close(sockfd);
        exit(1);
    }


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
