#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
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


int is_from_self(struct ethhdr *eth_header, uint8_t *local_mac) {
    // Compara la dirección MAC de origen del paquete con la MAC local
    return memcmp(eth_header->h_source, local_mac, ETH_ALEN) == 0;
}

void print_hex(const char *buffer, ssize_t length) {
    for (ssize_t i = 0; i < length; i++) {
        // Imprimir cada byte en formato hexadecimal
        printf("%02x ", (unsigned char)buffer[i]);

        // Insertar una nueva línea cada 16 bytes para que se vea ordenado
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }

    // Asegurar que haya un salto de línea al final si no se completó la última fila
    if (length % 16 != 0) {
        printf("\n");
    }
}

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
    uint8_t local_mac[ETH_ALEN];
    char interface[] = "lo"; // Cambiar a la interfaz adecuada

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
    memcpy(local_mac, ifr.ifr_hwaddr.sa_data, ETH_ALEN);

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

	struct sock_fprog bpf;
    struct sock_filter code[] = {
        // Cargar el campo de protocolo de la cabecera Ethernet (offset 12)
        { 0x30, 0, 0, 0x00000009 },  // ldb [9]: cargar el protocolo de la cabecera IP (offset 9)
        { 0x15, 0, 6, IPPROTO_UDP }, // jeq #0x11: si es UDP (0x11), continuar, de lo contrario saltar al final

        // Cargar la longitud de la cabecera IP
        { 0x28, 0, 0, 0x00000014 },  // ldh [14]: cargar la longitud de la cabecera IP
        { 0x45, 4, 0, 0x00001fff },  // jset #0x1fff: verificar que no esté fragmentado
        { 0xb1, 0, 0, 0x0000000e },  // ldxb 4*([14]&0x0f): calcular la longitud de la cabecera IP

        // Cargar el puerto de destino de UDP
        { 0x48, 0, 0, 0x00000010 },  // ldh [x + 16]: cargar el puerto de destino de UDP
        { 0x15, 0, 1, CLIENT_PORT }, // jeq #68: si el puerto de destino es 68, aceptar el paquete
        { 0x6, 0, 0, 0x00040000 },   // ret #0x40000: aceptar el paquete (65535 bytes)
        { 0x6, 0, 0, 0x00000000 }    // ret #0: descartar el paquete
    };
	int si = 1234; 

	bpf.len = sizeof(code) / sizeof(struct sock_filter);
    bpf.filter = code;

    if (setsockopt(sockfd_raw, SOL_SOCKET, SO_ATTACH_FILTER, &bpf, sizeof(bpf)) < 0) {
        perror("Error al aplicar el filtro BPF");
        close(sockfd_raw);
	close(sockfd_dgram);
        exit(EXIT_FAILURE);
    }
    
    int retries = 0;
    while (retries < MAX_RETRIES) {
        ssize_t num_bytes = recvfrom(sockfd_raw, buffer, sizeof(buffer), 0, NULL, NULL);

        struct ethhdr *eth_header = (struct ethhdr *)buffer;
        if (is_from_self(eth_header, local_mac)) {
                printf("Paquete recibido desde la propia máquina, ignorando.\n");
                continue; // Ignorar este paquete y continuar
            }

        if (num_bytes > 0) {
            printf("Paquete capturado, tamaño: %zd bytes\n", num_bytes);
            // Aquí puedes analizar o procesar el paquete capturado
	print_hex(buffer, num_bytes);		
            // Si se ha recibido un paquete, salir del bucle
            break;
        } else if (num_bytes < 0 && errno == EWOULDBLOCK) {
            // Si no se recibe nada y el tiempo de espera expira
            printf("Timeout esperando el paquete DHCP OFFER, reintentando...\n");
            retries++;
        } else {
            // Si ocurrió otro error
            perror("Error receiving data");
            close(sockfd_raw);
            close(sockfd_dgram);
            exit(EXIT_FAILURE);
        }
    }

    if (retries == MAX_RETRIES) {
        printf("No se recibió ninguna respuesta después de %d intentos, terminando.\n", MAX_RETRIES);
    }
    
    close(sockfd_raw);
    close(sockfd_dgram);

    return EXIT_SUCCESS; 
}