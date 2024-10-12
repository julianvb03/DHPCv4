#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/filter.h>
#include "dhcpv4_utils.h"

#define SERVER_PORT 67
#define CLIENT_PORT 68
#define TIMEOUT_SECONDS 4
#define MAX_RETRIES 2

int send_with_retries(int sockfd, struct dhcp_packet *packet, size_t packet_len, struct sockaddr *dest_addr, socklen_t addr_len, void *buffer, size_t buffer_len, int max_retries) {
    int retries = 0;

    while (retries < max_retries) {
        // Enviar el paquete DHCP
        if (sendto(sockfd, packet, packet_len, 0, dest_addr, addr_len) < 0) {
            perror("Error sending DHCP DISCOVER");
            return -1;
        }

        printf("DHCP DISCOVER sent, waiting for DHCP OFFER (attempt %d/%d)...\n", retries + 1, max_retries);

        // Intentar recibir una respuesta
        int recv_len = recvfrom(sockfd, buffer, buffer_len, 0, NULL, NULL);
        if (recv_len >= 0) {
            // Verificar que el paquete es un DHCP OFFER válido
            struct ethhdr *eth_header = (struct ethhdr *)buffer;
            struct iphdr *ip_header = (struct iphdr *)(buffer + sizeof(struct ethhdr));
            struct udphdr *udp_header = (struct udphdr *)(buffer + sizeof(struct ethhdr) + sizeof(struct iphdr));

            // Asegurarse de que el paquete es UDP y que el puerto de destino es 68
            if (ip_header->protocol == IPPROTO_UDP && ntohs(udp_header->dest) == CLIENT_PORT) {
                // Verificar si el paquete es un DHCP OFFER
                struct dhcp_packet *dhcp_response = (struct dhcp_packet *)(buffer + sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct udphdr));

                if (dhcp_response->options[0] == DHCP_OPTION_MESSAGE_TYPE &&
                    dhcp_response->options[1] == 1 &&
                    dhcp_response->options[2] == DHCPOFFER) {
                    printf("Received DHCP OFFER from server.\n");
                    return recv_len;
                } else {
                    printf("Received a non-DHCP OFFER packet, ignoring.\n");
                }
            } else {
                printf("Received a packet that is not DHCP or not for the right port, ignoring.\n");
            }
        } else {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                printf("Timeout waiting for DHCP OFFER, retrying...\n");
            } else {
                perror("Error receiving DHCP OFFER");
                return -1;
            }
        }

        retries++;
    }

    printf("Unable to receive DHCP OFFER after %d attempts.\n", max_retries);
    return -1;
}

void handle_discover(struct dhcp_packet* packet, struct ifreq* ifr) {

    // Prepare the data for the DHCPDISCOVER message

    // Get the MAC address of the network interface and copy it to the chaddr field
    uint8_t chaddr[16] = {0};
    memcpy(chaddr, ifr->ifr_hwaddr.sa_data, ETH_ALEN);          // Copy the MAC address

    // NERVER USED options
    uint8_t sname[64] = {0};                                    // Empty server name
    uint8_t file[128] = {0};                                    // Empty boot file name
    
    // Initialize the options field with zeros
    uint8_t options[DHCP_OPTIONS_LEN] = {0};

    // Set the DHCP options, only one on this case
    uint32_t magic_cookie = htonl(DHCP_MAGIC_COOKIE);           // Magic cookie distinguishing DHCP and BOOTP packets
    memcpy(options, &magic_cookie, sizeof(magic_cookie));
    options[4] = DHCP_OPTION_MESSAGE_TYPE;                      // Option: DHCP Message Type
    options[5] = 1;                                             // Length of the option data
    options[6] = DHCPDISCOVER;                                  // DHCP Discover message type
    options[7] = DHCP_OPTION_END;                               // End Option

    // Use get_dhcp_struc_hton to fill the DHCP packet
    get_dhcp_struc_hton(
        packet, 
        BOOTREQUEST, 
        HTYPE_ETHERNET, 
        ETH_ALEN, 
        0, 
        0x12345678,  // Transaction ID (can be random)
        0, 
        0x8000,  // Flags: Broadcast
        0, 0, 0, 0,  // IP addresses (ciaddr, yiaddr, siaddr, giaddr)
        chaddr, 
        sname, 
        file, 
        options
    );

}

int main() {
    int sockfd_raw, sockfd_dgram;
    char buffer[4096];
    struct ifreq ifr;
    char interface[] = "enp0s3"; // Cambiar a la interfaz adecuada

    // Crear un socket raw de nivel Ethernet para capturar todos los paquetes
    if ((sockfd_raw = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
        perror("Error creating raw socket");
        exit(EXIT_FAILURE);
    }

    // Vincular el socket a la interfaz específica
    memset(&ifr, 0, sizeof(ifr));
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", interface);
    if (setsockopt(sockfd_raw, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0) {
        perror("Error binding raw socket to interface");
        close(sockfd_raw);
        close(sockfd_dgram);
        exit(EXIT_FAILURE);
    }

    // Configurar el tiempo de espera para recibir la respuesta
    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_SECONDS;
    timeout.tv_usec = 0;
    if (setsockopt(sockfd_raw, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Error setting timeout on raw socket");
        close(sockfd_raw);
        close(sockfd_dgram);
        exit(EXIT_FAILURE);
    }

    // Crear un socket DGRAM para enviar el mensaje DHCP DISCOVER
    if ((sockfd_dgram = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("Error creating UDP socket");
        close(sockfd_raw);
        exit(EXIT_FAILURE);
    }

    // Vincular el socket DGRAM a la interfaz específica
    if (setsockopt(sockfd_dgram, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0) {
        perror("Error binding UDP socket to interface");
        close(sockfd_raw);
        close(sockfd_dgram);
        exit(EXIT_FAILURE);
    }

    // Habilitar el envío de paquetes a la dirección de broadcast para el socket DGRAM
    int broadcast_enable = 1;
    if (setsockopt(sockfd_dgram, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)) < 0) {
        perror("Error enabling broadcast option");
        close(sockfd_raw);
        close(sockfd_dgram);
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

    // Enviar el paquete DHCP DISCOVER usando el socket DGRAM
    if (sendto(sockfd_dgram, &packet, sizeof(packet), 0, (struct sockaddr *)&broadcast_addr, sizeof(broadcast_addr)) < 0) {
        perror("Error sending DHCP DISCOVER");
        close(sockfd_raw);
        close(sockfd_dgram);
        exit(EXIT_FAILURE);
    }

    printf("DHCP DISCOVER sent, waiting for DHCP OFFER...\n");

    // Definir los offsets correctos considerando las cabeceras Ethernet e IP
    #define ETHERNET_HEADER_LEN 14
    #define MIN_IP_HEADER_LEN 20
    #define IP_PROTOCOL_OFFSET (ETHERNET_HEADER_LEN + 9)
    #define IP_TOTAL_LENGTH_OFFSET (ETHERNET_HEADER_LEN + 2)
    #define UDP_HEADER_OFFSET (ETHERNET_HEADER_LEN + MIN_IP_HEADER_LEN)
    #define UDP_SRC_PORT_OFFSET (UDP_HEADER_OFFSET)
    #define UDP_DST_PORT_OFFSET (UDP_HEADER_OFFSET + 2)

    // Convertir los puertos a orden de bytes de red (big-endian)
    uint16_t server_port_be = htons(SERVER_PORT);
    uint16_t client_port_be = htons(CLIENT_PORT);

    // Configurar el filtro BPF para capturar solo paquetes UDP del puerto 67 al puerto 68
    struct sock_filter bpf_code[] = {
        // Verificar que el paquete es lo suficientemente largo para contener la cabecera IP mínima
        // Instrucción 0: cargar la longitud del paquete
        BPF_STMT(BPF_LD + BPF_W + BPF_LEN, 0),
        // Instrucción 1: comprobar si el paquete es menor que la longitud mínima
        BPF_JUMP(BPF_JMP + BPF_JGE + BPF_K, ETHERNET_HEADER_LEN + MIN_IP_HEADER_LEN, 0, 10),

        // Cargar el byte del protocolo IP
        BPF_STMT(BPF_LD + BPF_B + BPF_ABS, IP_PROTOCOL_OFFSET),
        BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, IPPROTO_UDP, 0, 8), // Si no es UDP, saltar

        // Verificar que el paquete es lo suficientemente largo para contener los puertos UDP
        BPF_STMT(BPF_LD + BPF_W + BPF_LEN, 0),
        BPF_JUMP(BPF_JMP + BPF_JGE + BPF_K, UDP_DST_PORT_OFFSET + 2, 0, 6),

        // Cargar el puerto de origen
        BPF_STMT(BPF_LD + BPF_H + BPF_ABS, UDP_SRC_PORT_OFFSET),
        BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, server_port_be, 0, 4), // Si no es puerto 67, saltar

        // Cargar el puerto de destino
        BPF_STMT(BPF_LD + BPF_H + BPF_ABS, UDP_DST_PORT_OFFSET),
        BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, client_port_be, 0, 2), // Si no es puerto 68, saltar

        // Aceptar el paquete si coincide
        BPF_STMT(BPF_RET + BPF_K, 0xFFFF),

        // Rechazar el paquete si no coincide
        BPF_STMT(BPF_RET + BPF_K, 0),
    };

    struct sock_fprog bpf_prog = {
        .len = sizeof(bpf_code) / sizeof(bpf_code[0]),
        .filter = bpf_code
    };

    // Aplicar el filtro BPF al socket RAW
    if (setsockopt(sockfd_raw, SOL_SOCKET, SO_ATTACH_FILTER, &bpf_prog, sizeof(bpf_prog)) < 0) {
        perror("Error attaching BPF filter");
        close(sockfd_raw);
        close(sockfd_dgram);
        exit(EXIT_FAILURE);
    }

    printf("Waiting for DHCP OFFER...\n");
    int recv_len = recv(sockfd_raw, buffer, sizeof(buffer), 0);
    if (recv_len < 0) {
        perror("Error receiving DHCP OFFER");
    } else {
        printf("Received a packet of length %d bytes.\n", recv_len);

        // Procesar el paquete recibido si es necesario
        struct ethhdr *eth_header = (struct ethhdr *)buffer;
        struct iphdr *ip_header = (struct iphdr *)(buffer + ETHERNET_HEADER_LEN);
        int ip_header_len = ip_header->ihl * 4; // Longitud de la cabecera IP
        struct udphdr *udp_header = (struct udphdr *)(buffer + ETHERNET_HEADER_LEN + ip_header_len);

        // Verificar si es un paquete UDP con el puerto de destino CLIENT_PORT (68)
        if (ip_header->protocol == IPPROTO_UDP && ntohs(udp_header->dest) == CLIENT_PORT) {
            // Extraer el contenido del paquete DHCP
            struct dhcp_packet *dhcp_response = (struct dhcp_packet *)(buffer + ETHERNET_HEADER_LEN + ip_header_len + sizeof(struct udphdr));

            // Verificar si es un DHCP OFFER
            if (dhcp_response->options[0] == DHCP_OPTION_MESSAGE_TYPE &&
                dhcp_response->options[1] == 1 &&
                dhcp_response->options[2] == DHCPOFFER) {
                printf("Received a valid DHCP OFFER.\n");
            } else {
                printf("Received a packet, but it is not a DHCP OFFER, ignoring.\n");
            }
        } else {
            printf("Received a non-UDP packet or one not for the right port, ignoring.\n");
        }
    }

    // Cerrar los sockets antes de salir
    close(sockfd_raw);
    close(sockfd_dgram);

    return EXIT_SUCCESS; 
}
