#include "dhcpv4_client.h"

int main(int argc, char *argv[]) {
    if(argc < 2) {
        printf("Uso: <nombre_interfaz>\n");
        return 1;
    }

    uint8_t client_mac[ETH_ALEN];
    if(get_hwaddr(argv[1], client_mac) < 0) {
        return -1;
    }

    printf("Dirección MAC de %s: %02x:%02x:%02x:%02x:%02x:%02x\n", argv[1],
           client_mac[0], client_mac[1], client_mac[2], client_mac[3], client_mac[4], client_mac[5]);

    int if_index = if_nametoindex(argv[1]);
    if (if_index == 0) {
        perror("No se pudo obtener el índice de la interfaz");
        return -1;
    }

    struct dhcp_packet dhcp_discover;
    handle_discover(&dhcp_discover, client_mac);
    //print_dhcp_strct(&dhcp_discover);
    
    int sockfd_dgram = create_dgram_socket(argv[1]);
    if(sockfd_dgram < 0) {
        return -1;
    }

    if(send_dhcp_package(sockfd_dgram, &dhcp_discover) < 0) {
        close(sockfd_dgram);
        return -1;
    }

    int sockfd_raw = open_raw_socket(argv[1], client_mac, if_index);
    if(sockfd_raw < 0) {
        return -1;
    }

    //struct dhcp_packet local_net_packet;

    unsigned char buffer[65536]; // Buffer grande para capturar paquetes
    struct sockaddr saddr;
    socklen_t saddr_len = sizeof(saddr);

    printf("Capturando paquetes DHCPv4...\n");

    // Bucle para capturar paquetes continuamente
    struct dhcp_packet dhcp_packet;
    while (1) {
        printf("Esperando paquete...\n");
        int status;
        int packet_len = recvfrom(sockfd_raw, buffer, sizeof(buffer), 0, &saddr, &saddr_len);
        if (packet_len < 0) {
            perror("Error al recibir paquete");
            continue;
        }
        // Validar mensaje tio Offer
        status = process_packet(buffer, packet_len, &dhcp_packet);
        if(status == DHCPOFFER) {
            break;
        }
        else {
            printf("Not a DHCP Offer\n");
            close(sockfd_raw);
            close(sockfd_dgram);
            return -1;
        }
    }

    //print_dhcp_strct(&dhcp_packet);
    struct network_config net_config;
    if(handle_offer(&dhcp_packet, &net_config) < 0) {
        close(sockfd_raw);
        close(sockfd_dgram);
        return -1;
    }

    if(offer_validation(&net_config) < 0) {
        close(sockfd_raw);
        close(sockfd_dgram);
        return -1;
    }

    handle_request(&dhcp_packet, &net_config);

    if(send_dhcp_package(sockfd_dgram, &dhcp_packet) < 0) {
        close(sockfd_raw);
        close(sockfd_dgram);
        return -1;
    }

    struct dhcp_packet dhcp_packet2;
    while (1) {
        int status;
        int packet_len = recvfrom(sockfd_raw, buffer, sizeof(buffer), 0, &saddr, &saddr_len);
        if (packet_len < 0) {
            perror("Error al recibir paquete");
            continue;
        }
        // Validar mensaje tio Offer
        status = process_packet(buffer, packet_len, &dhcp_packet2);
        if(status == DHCPACK) {
            break;
        }
    }

    printf("DHCP ACK recibido\n");
    //print_dhcp_strct(&dhcp_packet2);
    if(set_ip_address(argv[1], net_config.ip_addr, net_config.subnet_mask) < 0) {
        printf("Error al configurar la dirección IP\n");
        close(sockfd_raw);
        close(sockfd_dgram);
        return -1;
    }

    printf("Dirección IP configurada correctamente\n");

    close(sockfd_raw);
    close(sockfd_dgram);

    return 0;
}