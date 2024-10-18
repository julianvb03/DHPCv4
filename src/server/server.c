#include "dhcpv4_server.h"

int main() {

    // Server configuration
    char buffer[1025];

    struct configuration config;
    config.num_ips = 20;
    config.time_tp = 108000; // Time of arrendation
    get_dns(config.dns_server);
    strcpy(config.ip_base, "192.168.1.1");
    strcpy(config.ip_mask, "255.255.255.0");

    struct ip_in_poll ips[config.num_ips];
    memset(ips, 0, sizeof(ips));
    if (create_poll(ips, config.num_ips, config.ip_base, config.ip_mask) < 0) {
        printf("Error creating the IP pool\n");
        exit(1);
    }

    // Socket creation and binding

    int sockfd = create_socket_dgram();
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error al enlazar el socket");
        close(sockfd);
        exit(1);
    }

    printf("Servidor DHCP escuchando en el puerto %d...\n", SERVER_PORT);   

    // Main loop for receiving and processing DHCP packets
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int numbytes = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &addr_len);
        if (numbytes < 0) { perror("Error al recibir el mensaje"); continue; }

        struct dhcp_thread_data *data = malloc(sizeof(struct dhcp_thread_data));
        if (data == NULL) {
            perror("Error al asignar memoria para los datos del hilo");
            continue;
        }

        memset(data, 0, sizeof(buffer));
        memcpy(data->buffer, buffer, numbytes);

        data->numbytes = numbytes;
        data->client_addr = client_addr;
        data->config = &config;
        data->ips = ips;

        // Create a new thread to handle the DHCP packet
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_dhcp_generic, data) != 0) {
            perror("Error al crear el hilo");
            free(data);
        } else {
            pthread_detach(thread_id);
        }
    }
    
}