#include "dhcpv4_server.h"

/*
    Function to send a DHCP package
        @param sockfd: Socket file descriptor
        @param packet: DHCP packet structure

        @return: 0 if the package was sent successfully, -1 if there was an error
*/
int send_dhcp_package(struct dhcp_packet *packet) {
    int sockfd = create_socket_dgram();


    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(CLIENT_PORT);  // Puerto DHCP del cliente
    client_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    if (sendto(sockfd, packet, sizeof(*packet), 0, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
        perror("Error al enviar el mensaje");
        close(sockfd);
        return -1;
    }
    //printf(" enviado\n");
    close(sockfd);
    return 0;
}

/*
    Function to fill the DHCP ACK message
        @return: 0 if the process was successful, -1 if there was an error
*/
int create_socket_dgram() {
    int sockfd;
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error al crear el socket");
        exit(1);
    }

    if(setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &(int){1}, sizeof(int)) < 0) {
        perror("Error al configurar SO_BROADCAST en el envío");
        close(sockfd);
        exit(1);
    }

    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) == -1) {
        close(sockfd);
        perror("Error al configurar SO_REUSEADDR");
        return 3;
    }

    return sockfd;
}