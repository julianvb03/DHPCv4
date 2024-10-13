#include "dhcpv4_client.h"

void print_dhcp_message_type(unsigned char type) {
    switch (type) {
        case 1: printf("DHCP Discover\n"); break;
        case 2: printf("DHCP Offer\n"); break;
        case 3: printf("DHCP Request\n"); break;
        case 4: printf("DHCP Decline\n"); break;
        case 5: printf("DHCP ACK\n"); break;
        case 6: printf("DHCP NAK\n"); break;
        case 7: printf("DHCP Release\n"); break;
        case 8: printf("DHCP Inform\n"); break;
        default: printf("Unknown DHCP message type\n");
    }
}

int get_hwaddr(const char *ifname, uint8_t *hwaddr) {
    struct ifreq ifr;
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    
    if (sockfd < 0) {
        perror("Error al abrir socket");
        return -1;
    }

    // Copiar el nombre de la interfaz a la estructura ifreq
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);

    // Obtener la dirección MAC de la interfaz
    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) == -1) {
        perror("Error al obtener la dirección MAC");
        close(sockfd);
        return -1;
    }

    // Copiar la dirección MAC obtenida en el buffer de hwaddr
    memcpy(hwaddr, ifr.ifr_hwaddr.sa_data, ETH_ALEN);

    close(sockfd);
    return 0;
}

int process_packet(unsigned char *buffer, int len, struct dhcp_packet *dhcp_packet_main) {
    struct ethhdr *eth = (struct ethhdr *)(buffer);

    // Verificar si es un paquete IPv4
    if (ntohs(eth->h_proto) != ETH_P_IP) {
        printf("No es un paquete IPv4\n");
        return -1;
    }

    struct iphdr *iph = (struct iphdr *)(buffer + sizeof(struct ethhdr));

    if (iph->version != 4) {
        printf("No es un paquete IPv4\n");
        return -1;
    }

    if(iph->protocol != IPPROTO_UDP) {
        printf("No es un paquete UDP\n");
        return -1;
    }

    if(iph->protocol != IPPROTO_UDP) {
        printf("No es un paquete UDP\n");
        return -1;
    }

    struct udphdr *udph = (struct udphdr *)(buffer + sizeof(struct ethhdr) + iph->ihl * 4);
    if(udph->dest != htons(CLIENT_PORT)) {
        printf("No es un paquete DHCP Reply\n");
        return -1;
    }

    int udp_header_len = sizeof(struct udphdr);
    int ip_header_len = iph->ihl * 4;
    int dhcp_data_len = ntohs(udph->len) - udp_header_len;  // Longitud de los datos DHCP
    unsigned char *dhcp_data = buffer + sizeof(struct ethhdr) + ip_header_len + udp_header_len;
    struct dhcp_packet *dhcp_packet = (struct dhcp_packet *)dhcp_data;
    struct dhcp_packet local_packet;
    memset(&local_packet, 0, sizeof(local_packet));
    get_dhcp_struc_ntoh(dhcp_packet, &local_packet);

    //print_dhcp_strct(dhcp_packet);
    if(dhcp_packet->op == BOOTREPLY) {
        printf("DHCP Reply\n");
        unsigned char *options = dhcp_packet->options;
        int offset = find_dhcp_magic_cookie(options, DHCP_OPTIONS_LEN);
        if(offset < 0) {
            printf("Magic cookie not found\n");
            return -1;
        }
        unsigned char *dhcp_options = options + offset + 4;  // Skip the magic cookie
        unsigned char dhcp_message_type = 0;
        while(dhcp_options[0] != DHCP_OPTION_END) {
            if(dhcp_options[0] == DHCP_OPTION_MESSAGE_TYPE) {
                dhcp_message_type = dhcp_options[2];
                break;
            }
            dhcp_options += dhcp_options[1] + 2;
        }
        print_dhcp_message_type(dhcp_message_type);
        memcpy(dhcp_packet_main, &local_packet, sizeof(struct dhcp_packet));
        printf("DHCP packet copied\n");

        return dhcp_message_type;
    } else {
        printf("Not a DHCP Reply\n");
        return -1;
    } 

}