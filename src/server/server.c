#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include "dhcpv4_utils.h"

#define SERVER_PORT 67
#define CLIENT_PORT 68

struct ip_in_poll {
    char ip_str[INET_ADDRSTRLEN];
    u_int8_t state;
    char mac_str[18];
    u_int64_t arr_time;             // Time on seconds
};

struct dhcp_thread_data {
    char buffer[1024];
    int numbytes;
    struct sockaddr_in client_addr;
};

char* assign_ip(struct ip_in_poll* ips, char* mac_str, u_int32_t time_tp, int num_ips) {
    time_t current_time = time(NULL);
    for (int i = 0; i < num_ips; i++) {
        if(ips[i].state == 1 && current_time >= ips[i].arr_time) {
            ips[i].state = 0;
            memset(ips[i].mac_str, 0, sizeof(ips[i].mac_str));
        }

        if (ips[i].state == 0) {
            ips[i].state = 1;
            strcpy(ips[i].mac_str, mac_str);
            ips[i].arr_time = current_time + time_tp;           // Time on seconds of arrendation
            return ips[i].ip_str;
        }
    }
    return NULL;
}


int create_poll(struct ip_in_poll* ips, int num_ips, const char* ip_base, const char* ip_mask) {
    struct in_addr base_addr, mask_addr;

    // Convertir la IP base y la máscara de red de string a formato binario
    if (inet_pton(AF_INET, ip_base, &base_addr) != 1) {
        perror("Error converting IP base");
        return -1;
    }
    if (inet_pton(AF_INET, ip_mask, &mask_addr) != 1) {
        perror("Error converting IP mask");
        return -1;
    }

    // Calcular la dirección de red (AND entre IP base y máscara)
    uint32_t network = ntohl(base_addr.s_addr) & ntohl(mask_addr.s_addr);
    uint32_t mask = ntohl(mask_addr.s_addr);

    // Calcular el número máximo de hosts en la subred
    uint32_t max_hosts = ~mask - 1; // Restamos 1 porque la dirección de broadcast no es asignable

    // Verificar si el número de IPs solicitado no supera el número de hosts disponibles menos las primeras 50
    if (num_ips > max_hosts - 50) {
        printf("Number of IPs requested (%d) exceeds available range (%u) after reserving 50 static addresses.\n", num_ips, max_hosts - 50);
        return -1;
    }

    // Generar las direcciones IP para el pool, empezando desde network + 51 para omitir las primeras 50
    for (int i = 0; i < num_ips; i++) {
        // Incrementar la parte del host en el rango de la red, comenzando desde la IP 51
        uint32_t ip_host = network + 51 + i;

        // Convertir la dirección IP a formato de red
        struct in_addr ip_addr;
        ip_addr.s_addr = htonl(ip_host);

        // Convertir la dirección IP a formato string y guardarla en la estructura
        inet_ntop(AF_INET, &ip_addr, ips[i].ip_str, INET_ADDRSTRLEN);

        // Inicializar los otros campos de la estructura
        ips[i].state = 0;  // Inicialmente desocupada (o en el estado deseado)
        ips[i].arr_time = 0;   // Tiempo inicial

        // Dejar el campo de MAC vacío por ahora, ya que no hay una MAC asociada inicialmente
        memset(ips[i].mac_str, 0, sizeof(ips[i].mac_str));

        printf("Assigned IP: %s\n", ips[i].ip_str); // Para ver el proceso (puedes quitarlo)
    }

    return 0;
}


void handle_discover(struct dhcp_packet* packet, struct sockaddr_in* client_addr) {
    // Prepare the data for the DHCPOFFER message

    // Get the MAC address of the network interface and copy it to the chaddr field
    client_addr->sin_port = htons(CLIENT_PORT); // Puerto DHCP del cliente
    client_addr->sin_addr.s_addr = htonl(INADDR_BROADCAST);
    uint8_t chaddr[16] = {0};
    memcpy(chaddr, packet->chaddr, ETH_ALEN);          // Copy the MAC address

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
    options[6] = DHCPOFFER;                                     // DHCP Offer message type
    options[7] = DHCP_OPTION_END;                               // End Option

    // Prepare the DHCP packet
    struct dhcp_packet offer_packet;
    get_dhcp_struc_hton(&offer_packet, BOOTREPLY, HTYPE_ETHERNET, ETH_ALEN, 0, packet->xid, 0, 0, 0, 0, 0, 0, chaddr, sname, file, options);

    // Send the DHCPOFFER message
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error al crear el socket");
        exit(1);
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &(int){1}, sizeof(int)) < 0) {
        perror("Error al configurar SO_BROADCAST en el envío");
        close(sockfd);
        exit(1);
    }

    if (sendto(sockfd, &offer_packet, sizeof(offer_packet), 0, (struct sockaddr *)client_addr, sizeof(*client_addr)) < 0) {
        perror("Error al enviar el mensaje");
        close(sockfd);
        exit(1);
    }
    printf("DHCPOFFER enviado\n");

    close(sockfd);
}

void *handle_dhcp_generic(void *arg) {
    struct dhcp_thread_data *data = (struct dhcp_thread_data *)arg;

    struct dhcp_packet local_net_packet;
    struct dhcp_packet local_host_packet;

    memcpy(&local_net_packet, data->buffer, sizeof(struct dhcp_packet));
    get_dhcp_struc_ntoh(&local_net_packet, &local_host_packet);

    printf("Paquete DHCP recibido y procesado en el hilo %ld:\n", (unsigned long int)pthread_self());
    print_dhcp_struc((const char*)&local_host_packet, sizeof(local_host_packet));

    printf("Opciones del paquete DHCP:\n");

    int message_type;
    for (int iter = 0; iter < DHCP_OPTIONS_LEN && local_host_packet.options[iter] != DHCP_OPTION_END; iter++) {
        if(local_host_packet.options[iter] == DHCP_OPTION_MESSAGE_TYPE) {
            int len = local_host_packet.options[iter + 1];
            if (len != 1) {
                printf("Error: Invalid DHCP message type option length\n");
                break;
            }
            message_type = local_host_packet.options[iter + 2];
        }
    }
    
    if(message_type == DHCPDISCOVER) {
        printf("DHCPDISCOVER\n");
        handle_discover(&local_host_packet, &data->client_addr);
    } else if(message_type == DHCPREQUEST) {
        printf("DHCPREQUEST\n");
        //handle_request(&local_host_packet, data->client_addr);
    } else if(message_type == DHCPRELEASE) {
        printf("DHCPRELEASE\n");
        //handle_release(&local_host_packet, data->client_addr);
    } else {
        //printf("Unknown message type\n");
    }

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
