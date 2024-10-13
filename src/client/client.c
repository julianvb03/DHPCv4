#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <sys/ioctl.h>  // Para obtener la dirección MAC
#include "dhcpv4_utils.h"

// Definir el tamaño del mensaje DHCP
#define DHCP_MSG_SIZE 312 //548

// Función para imprimir los datos en formato hexadecimal
void print_hex(const uint8_t *data, int len) {
    for (int i = 0; i < len; i++) {
        // Imprimir el dato en formato hexadecimal
        printf("%02x ", data[i]);

        // Formato: 16 bytes por línea
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    // Añadir un salto de línea si no se completaron 16 bytes en la última línea
    if (len % 16 != 0) {
        printf("\n");
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

void print_dhcp_strct(struct dhcp_packet *packet) {
    // Imprimir los campos de la estructura dhcp_packet
    printf("op: %d\n", packet->op);
    printf("htype: %d\n", packet->htype);
    printf("hlen: %d\n", packet->hlen);
    printf("hops: %d\n", packet->hops);
    printf("xid: %u\n", ntohl(packet->xid));
    printf("secs: %d\n", ntohs(packet->secs));
    printf("flags: %d\n", ntohs(packet->flags));
    printf("ciaddr: %u\n", ntohl(packet->ciaddr));
    printf("yiaddr: %u\n", ntohl(packet->yiaddr));
    printf("siaddr: %u\n", ntohl(packet->siaddr));
    printf("giaddr: %u\n", ntohl(packet->giaddr));
    printf("chaddr: ");
    printf("Dirección MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", 
           packet->chaddr[0], packet->chaddr[1], packet->chaddr[2], packet->chaddr[3], packet->chaddr[4], packet->chaddr[5]);
    printf("sname: ");
    print_hex(packet->sname, 64);
    printf("file: ");
    print_hex(packet->file, 128);
    printf("options: ");
    for (int i = 0; i < DHCP_OPTIONS_LEN; i++) {

        unsigned char option = packet->options[i];
        // Verificar si es la opción de fin (0xFF)
        if (option == DHCP_OPTION_END) {
            printf("Fin de las opciones DHCP.\n");
            break;
        }

        // Verificar si es una opción de relleno (0x00)
        if (option == DHCP_OPTION_PAD) {
            continue;  // Saltar las opciones de relleno
        }

        // Obtener la longitud de la opción
        unsigned char length = packet->options[++i];

        // Si encontramos la opción DHCP_OPTION_MESSAGE_TYPE (53)
        if (option == DHCP_OPTION_MESSAGE_TYPE) {
            unsigned char message_type = packet->options[i + 1];  // El valor de la opción (tipo de mensaje)
            printf("Tipo de mensaje DHCP encontrado: ");
            print_dhcp_message_type(message_type);  // Llamar a la función para imprimir el tipo de mensaje
        }

        // Avanzar en el array tantas posiciones como longitud de la opción
        i += length;
    }

}

int open_raw_socket(const char* ifname, uint8_t hwaddr[ETH_ALEN], int if_index) {
    // Crear un socket RAW para capturar paquetes (usamos SOCK_RAW y ETH_P_ALL)
    int s = socket(PF_PACKET, SOCK_RAW | SOCK_CLOEXEC, htons(ETH_P_ALL));
    if (s < 0) {
        perror("Error al crear socket RAW");
        return -1;
    }

    // Asociar el socket a la interfaz de red
    struct sockaddr_ll bindaddr = {
        .sll_family = AF_PACKET,
        .sll_protocol = htons(ETH_P_ALL),  // Captura todo tipo de tráfico
        .sll_ifindex = if_index,
        .sll_halen = ETH_ALEN,
    };
    memcpy(bindaddr.sll_addr, hwaddr, ETH_ALEN);

    // Enlazar el socket a la interfaz de red especificada
    if (bind(s, (struct sockaddr *)&bindaddr, sizeof(bindaddr)) < 0) {
        perror("No se pudo enlazar el socket a la interfaz de red");
        close(s);
        return -1;
    }

    return s;  // Retorna el descriptor del socket RAW
}

// Estructura simplificada para un mensaje DHCP
// struct dhcp_msg {
//     uint8_t op;
//     uint8_t htype;
//     uint8_t hlen;
//     uint8_t hops;
//     uint32_t xid;
//     uint16_t secs;
//     uint16_t flags;
//     uint32_t ciaddr;
//     uint32_t yiaddr;
//     uint32_t siaddr;
//     uint32_t giaddr;
//     uint8_t chaddr[16];
//     uint8_t sname[64];
//     uint8_t file[128];
//     uint8_t options[DHCP_MSG_SIZE];
// };
int get_dhcp_message_type(uint8_t *options, int options_len) {
    int i = 0;
    while (i < options_len) {
        uint8_t option = options[i];
        
        // Fin de opciones
        if (option == 255) break;
        
        // Si es la opción 53, el siguiente byte contiene el tipo de mensaje DHCP
        if (option == DHCP_OPTION_MESSAGE_TYPE) {
            return options[i + 2];  // i+2 contiene el valor del tipo de mensaje
        }
        
        // Saltar al siguiente campo de opción (i+1 es la longitud de la opción)
        i += 2 + options[i + 1];
    }
    return -1;  // No se encontró la opción 53
}

int check_dhcp_magic_cookie(uint8_t *buffer) {
    // La Magic Cookie empieza en el byte 236 (luego de la estructura fija de 236 bytes)
    uint32_t *cookie_location = (uint32_t *)(buffer + 236); // Desplazamiento de 236 bytes
    uint32_t magic_cookie = ntohl(*cookie_location); // Convertir de big-endian a host-endian

    // Imprimir las dos Magic Cookies en formato hexadecimal
    printf("Magic Cookie recibida: 0x%08x\n", magic_cookie);
    printf("Magic Cookie esperada: 0x%08x\n", DHCP_MAGIC_COOKIE);
	
    // Imprimir las primeras 256 posiciones del buffer en hexadecimal
    printf("Contenido del buffer hasta la posición 256:\n");
    for (int i = 0; i < 256; i++) {
        printf("%02x ", buffer[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    // Compara la Magic Cookie con la versión en formato de red
    return magic_cookie == DHCP_MAGIC_COOKIE;
} 

int find_dhcp_magic_cookie(uint8_t *buffer, int buffer_len) {
    // Buscar la Magic Cookie (4 bytes) en el buffer
    for (int i = 0; i <= buffer_len - 4; i++) {
        if (buffer[i] == 0x63 && buffer[i + 1] == 0x82 &&
            buffer[i + 2] == 0x53 && buffer[i + 3] == 0x63) {
            return i;  // Devolver el offset donde se encuentra la Magic Cookie
        }
    }
    return -1;  // No se encontró la Magic Cookie
}

int receive_broadcast_message(int sockfd, struct dhcp_packet *local) {
    uint8_t buffer[2048];  // Buffer para almacenar el paquete recibido
    int nread;

    // Leer un paquete del socket
    nread = read(sockfd, buffer, sizeof(buffer));
    if (nread < 0) {
        perror("Error al leer el paquete");
        return -1;
    }

    // Buscar la Magic Cookie en el paquete recibido
    int magic_cookie_offset = find_dhcp_magic_cookie(buffer, nread);
    puts("Magic Cookie encontrada");
    if (magic_cookie_offset == -1) {
        printf("Magic Cookie no encontrada. No es un paquete DHCP\n");
        return -1;
    }

    // Calcular el inicio del mensaje DHCP en el buffer
    int dhcp_start_offset = magic_cookie_offset - 236;  // La Magic Cookie se encuentra en el byte 236 de un mensaje DHCP típico
    if (dhcp_start_offset < 0) {
        printf("Paquete DHCP incompleto o mal formado\n");
        return -1;
    }

    // Copiar la parte del buffer que contiene el mensaje DHCP a la estructura dhcp_packet
    memcpy(local, buffer + dhcp_start_offset, sizeof(struct dhcp_packet));

    // Imprimir el paquete DHCP en formato hexadecimal
    print_hex(buffer + dhcp_start_offset, sizeof(struct dhcp_packet));

    // Comprobar el tipo de mensaje DHCP
    int dhcp_message_type = get_dhcp_message_type(local->options, DHCP_MSG_SIZE);
    if (dhcp_message_type == DHCPOFFER) {
        printf("DHCP Offer recibido\n");
    } else if (dhcp_message_type == DHCPACK) {
        printf("DHCP ACK recibido\n");
    } else if (dhcp_message_type == DHCPOFFER) {
        printf("DHCP OFFER recibido\n");
    } else if (dhcp_message_type == DHCPREQUEST) {
        printf("DHCP REQUEST recibido\n");
    } else {
        printf("Otro tipo de mensaje DHCP recibido o no se pudo determinar\n");
        return -1;
    }
    print_dhcp_strct(local);

    return 0;
}

int main() {
    // Datos de ejemplo para obtener el índice de la interfaz y la dirección MAC
    const char* ifname = "eth0";  // Cambia a la interfaz de red que estás usando
    uint8_t hwaddr[ETH_ALEN];

    // Obtener la dirección MAC de la interfaz
    if (get_hwaddr(ifname, hwaddr) < 0) {
        return -1;
    }

    // Imprimir la dirección MAC obtenida
    printf("Dirección MAC de %s: %02x:%02x:%02x:%02x:%02x:%02x\n", ifname,
           hwaddr[0], hwaddr[1], hwaddr[2], hwaddr[3], hwaddr[4], hwaddr[5]);

    // Obtener el índice de la interfaz
    int if_index = if_nametoindex(ifname);
    if (if_index == 0) {
        perror("No se pudo obtener el índice de la interfaz");
        return -1;
    }

    // Abrir un socket RAW
    int sockfd = open_raw_socket(ifname, hwaddr, if_index);
    if (sockfd < 0) {
        perror("Error al abrir el socket RAW");
        return -1;
    }

    struct dhcp_packet local_net_packet;

    // Esperar y recibir un mensaje broadcast
    while (1) {
        receive_broadcast_message(sockfd, &local_net_packet);
    }

    close(sockfd);
    return 0;
}
