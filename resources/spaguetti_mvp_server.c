#include <stdint.h>
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

#define SERVER_PORT 67
#define CLIENT_PORT 68

// DHCP Operation Codes
#define BOOTREQUEST 1
#define BOOTREPLY 2

// DHCP Hardware Types
#define HTYPE_ETHERNET 1

// DHCP Options
#define DHCP_OPTION_MESSAGE_TYPE 53
#define DHCP_OPTION_END 255
#define DHCP_MAGIC_COOKIE 0x63825363

// DHCP Message Types
#define DHCPDISCOVER 1
#define DHCPOFFER 2
#define DHCPREQUEST 3
#define DHCPDECLINE 4
#define DHCPACK 5
#define DHCPNAK 6
#define DHCPRELEASE 7
#define DHCPINFORM 8

// Lengths
#define ETH_ALEN 6   // Length of an Ethernet address (MAC address)
#define DHCP_OPTIONS_LEN 312
#define DHCP_OPTION_PAD 0

//int send_dhcp_package(struct dhcp_packet *packet);

// =================================================
// Functions for handling the dhcp_packet structure
// =================================================

struct dhcp_packet {
    uint8_t op;        // Message op code / message type { 1 = START 2 = REPLY }
    uint8_t htype;     // Hardware address type
    uint8_t hlen;      // MAC address length
    uint8_t hops;      // Counter for the number of jumps on routers before reaching the server
    uint32_t xid;      // Transaction ID { Static on own case for debugging }
    uint16_t secs;     // Seconds elapsed after the client started the DHCP process
    uint16_t flags;    // Flags 1 = Broadcast 0 = Unicast
    uint32_t ciaddr;   // Client IP address { For renewing }
    uint32_t yiaddr;   // 'Your' IP address { IP offered to the client }
    uint32_t siaddr;   // Next server IP address { Optional and using only wen we work with auxiliary servers }             NERVER USED
    uint32_t giaddr;   // Relay agent IP address
    uint8_t chaddr[16]; // MAC address client
    uint8_t sname[64];  // Optional server host name                                                                        NEVER USED
    uint8_t file[128];  // Boot file name { Outburst loading files; using for aditionals files for the client or OS }       NEVER USED
    uint8_t options[DHCP_OPTIONS_LEN]; // Optional parameters field
} __attribute__((packed));  // For avoid padding

void print_hex(const uint8_t *data, int len) {
    for (int i = 0; i < len; i++) {
        printf("%02x ", data[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    if (len % 16 != 0) {
        printf("\n");
    }
}

void print_dhcp_strct(struct dhcp_packet *packet) {
    struct in_addr ip_addr;

    printf("===== DHCP Packet =====\n");
    
    // Imprimir los campos de la estructura dhcp_packet
    printf("op: %d (1=Request, 2=Reply)\n", packet->op);
    printf("htype: %d (Hardware Type)\n", packet->htype);
    printf("hlen: %d (Hardware Address Length)\n", packet->hlen);
    printf("hops: %d (Hops)\n", packet->hops);
    
    printf("xid: 0x%x (Transaction ID)\n", ntohl(packet->xid));
    printf("secs: %d (Seconds elapsed)\n", ntohs(packet->secs));
    printf("flags: 0x%x (Flags)\n", ntohs(packet->flags));

    // Imprimir direcciones IP en formato legible
    ip_addr.s_addr = packet->ciaddr;
    printf("ciaddr: %s (Client IP Address)\n", inet_ntoa(ip_addr));

    ip_addr.s_addr = packet->yiaddr;
    printf("yiaddr: %s (Your IP Address - IP offered to the client)\n", inet_ntoa(ip_addr));

    ip_addr.s_addr = packet->siaddr;
    printf("siaddr: %s (Next Server IP Address)\n", inet_ntoa(ip_addr));

    ip_addr.s_addr = packet->giaddr;
    printf("giaddr: %s (Relay Agent IP Address)\n", inet_ntoa(ip_addr));

    // Imprimir la dirección MAC (los primeros 6 bytes son generalmente relevantes)
    printf("MAC Address (chaddr): ");
    for (int i = 0; i < 6; i++) {
        printf("%02x", packet->chaddr[i]);
        if (i < 5) printf(":");
    }
    printf("\n");

    printf("sname: \n");
    print_hex(packet->sname, 64);

    printf("file: \n");
    print_hex(packet->file, 128);

    printf("options: \n");
    print_hex(packet->options, DHCP_OPTIONS_LEN);

    printf("=========================\n");
}

void get_dhcp_struc_hton(struct dhcp_packet *packet, uint8_t op, uint8_t htype, uint8_t hlen, uint8_t hops, uint32_t xid, uint16_t secs, uint16_t flags, uint32_t ciaddr, uint32_t yiaddr, uint32_t siaddr, uint32_t giaddr, uint8_t chaddr[16], uint8_t sname[64], uint8_t file[128], uint8_t options[312]){
     // Set single-byte fields directly
    packet->op = op;
    packet->htype = htype;
    packet->hlen = hlen;
    packet->hops = hops;

    // Convert multi-byte fields to network byte order
    packet->xid = htonl(xid);                       // Transaction ID
    packet->secs = htons(secs);                     // Seconds elapsed
    packet->flags = htons(flags);                   // Flags
    packet->ciaddr = htonl(ciaddr);                 // Client IP address
    packet->yiaddr = htonl(yiaddr);                 // 'Your' (client) IP address
    packet->siaddr = htonl(siaddr);                 // Next server IP address
    packet->giaddr = htonl(giaddr);                 // Relay agent IP address

    // Copy arrays directly (no byte order conversion needed)
    memcpy(packet->chaddr, chaddr, 16);
    memcpy(packet->sname, sname, 64);
    memcpy(packet->file, file, 128);
    memcpy(packet->options, options, 312);
}

void get_dhcp_struc_ntoh(struct dhcp_packet *net_packet, struct dhcp_packet *host_packet) {
    // Copy single-byte fields directly
    host_packet->op = net_packet->op;
    host_packet->htype = net_packet->htype;
    host_packet->hlen = net_packet->hlen;
    host_packet->hops = net_packet->hops;

    // Convert multi-byte fields from network to host byte order
    host_packet->xid = ntohl(net_packet->xid);
    host_packet->secs = ntohs(net_packet->secs);
    host_packet->flags = ntohs(net_packet->flags);
    host_packet->ciaddr = ntohl(net_packet->ciaddr);
    host_packet->yiaddr = ntohl(net_packet->yiaddr);
    host_packet->siaddr = ntohl(net_packet->siaddr);
    host_packet->giaddr = ntohl(net_packet->giaddr);

    // Copy arrays directly (no byte order conversion needed)
    memcpy(host_packet->chaddr, net_packet->chaddr, 16);
    memcpy(host_packet->sname, net_packet->sname, 64);
    memcpy(host_packet->file, net_packet->file, 128);
    memcpy(host_packet->options, net_packet->options, 312);
}

// =================================================


// =================================================
// Funciones for handling the Pool
// =================================================

/*
    Structure for the IP addresses pool
        @param ip_str: IP address
        @param state: State of the IP address (0 = Free, 1 = Used)
        @param mac_str: MAC address of the client
        @param arr_time: Time on seconds
*/
struct ip_in_poll {
    char ip_str[INET_ADDRSTRLEN];
    u_int8_t state;
    char mac_str[18];
    u_int32_t arr_time;
};

/*
    Structure for the configuration of the DHCP server
        @param ip_base: Base IP address
        @param ip_mask: Mask of the network
        @param num_ips: Number of IP addresses on the array
        @param time_tp: Time of arrendation
        @param dns_server: DNS server
*/

struct configuration {
    char ip_base[INET_ADDRSTRLEN];
    char ip_mask[INET_ADDRSTRLEN];
    int num_ips;
    u_int32_t time_tp;
    char dns_server[INET_ADDRSTRLEN];
};

/*
    Structure for the thread data (for avoid the use of global variables, and got race conditions)
        @param buffer: Buffer with the data
        @param numbytes: Number of bytes
        @param client_addr: Client address
*/
struct dhcp_thread_data {
    char buffer[1024];
    int numbytes;
    struct sockaddr_in client_addr;
    struct configuration *config;
    struct ip_in_poll *ips;
};

/*
    Function to assign an IP to a MAC address (0 = Free, 1 = Used)
        @param ips: Array of IP addresses
        @param mac_str: MAC address of the client
        @param time_tp: Time of arrendation
        @param num_ips: Number of IP addresses on the array
        
        @return: The IP address assigned or NULL if there is no IP available
*/
int assign_ip(struct ip_in_poll* ips, char* mac_str, u_int32_t time_tp, int num_ips, char *ip_str_offer) {
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
            strcpy(ip_str_offer, ips[i].ip_str);
            return 0;
        }
    }
    return .1;
}

/*
    Functiof for create the ips Pool with a base IP addresses
        @param ips: Array of IP addresses
        @param num_ips: Number of IP addresses on the array
        @param ip_base: Base IP address
        @param ip_mask: Mask of the network

        @return: 0 if the pool was created successfully, -1 if there was an error
*/

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


// =================================================
// Fill the dhcp_packet structure with the DHCP options
// =================================================

/*
    Function to fill the DHCP Discover message
        @param packet: DHCP packet structure
        @param clien_mac: MAC address of the client
        @param ip_offer: Offered IP address
        @param default_gateway: Default gateway
        @param dns_server: DNS server

        @return: void

    We ned implement wen the discovir is received with an Agent Relay and subnets
*/
int fill_dhcp_offer(struct dhcp_packet *packet, u_int8_t* clien_mac, const char* ip_offer, struct configuration* config) {
    // DHCP options
    uint8_t sname[64] = {0};                                    // Never used
    uint8_t file[128] = {0};                                    // Never used
    uint8_t options[DHCP_OPTIONS_LEN] = {0};

    uint32_t magic_cookie = htonl(DHCP_MAGIC_COOKIE);           // Magic cookie distinguishing DHCP and BOOTP packets
    memcpy(options, &magic_cookie, sizeof(magic_cookie));
    options[4] = DHCP_OPTION_MESSAGE_TYPE;                      // Option: DHCP Message Type
    options[5] = 1;                                             // Length of the option data
    options[6] = DHCPOFFER;                                     // DHCP Offer message type

    options[7] = 1;                                             // Option: Bubnet mask
    options[8] = 4;                                             // Length of the option data
    inet_pton(AF_INET, config->ip_mask, &options[9]);               // Server IP address

    options[13] = 3;                                            // Option: DG
    options[14] = 4;                                            // Length of the option data
    inet_pton(AF_INET, config->ip_base, &options[15]);          // Default gateway

    options[19] = 6;                                            // Option: DNS
    options[20] = 4;                                            // Length of the option data
    inet_pton(AF_INET, config->dns_server, &options[21]);               // DNS server

    //51 (Lease Time) : 58 (Renewal Time) : 59 (Rebinding Time)     Time is still pending to implement: config->time_tp

    options[25] = DHCP_OPTION_END;

    uint32_t yiaddr_net;                                        // Convert the offered IP address to network byte order
    if (inet_pton(AF_INET, ip_offer, &yiaddr_net) != 1) {
        printf("Error al convertir la dirección IP\n");
        return -1;
    }

    get_dhcp_struc_hton(
        packet, 
        BOOTREPLY, 
        HTYPE_ETHERNET, 
        ETH_ALEN, 
        0, 
        0x12345678,                                             // Transaction ID Fixef for trace
        0, 
        0x8000,                                                 // Flags: Broadcast
        0, yiaddr_net, 0, 0,                                    // IP addresses (ciaddr, yiaddr, siaddr, giaddr)
        clien_mac,                                              // Aquí usamos directamente la dirección MAC del cliente
        sname, 
        file, 
        options
    );

    return 0;
}


int fill_dhcp_ack(struct dhcp_packet *packet) {
    // Implement the logic for filling the DHCP ACK message we need to and binding time
    packet->op = BOOTREPLY;
    packet->options[6] = DHCPACK;
    return 0;
}

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

int handle_discover(struct dhcp_packet* packet, struct configuration* config, char* ip_offer) {
    // Implement the logic for each case of DISCOVER
    uint8_t chaddr[16] = {0};
    memcpy(chaddr, packet->chaddr, ETH_ALEN);                    // Copy the MAC address

    if (fill_dhcp_offer(packet, chaddr, ip_offer, config) < 0) {
        printf("Error al llenar el paquete DHCP Offer\n");
        return -1;
    }

    printf("Se va a ofertar la IP: %s\n", ip_offer);
    //print_dhcp_strct(packet);

    usleep(1000000);
    if (send_dhcp_package(packet) < 0) {
        printf("Error al enviar el paquete DHCP Offer\n");
        return -1;
    }

    printf("Se envió la oferta de IP despues de esperar\n");
    return 0;
}

int handle_ack(struct dhcp_packet* packet, struct sockaddr_in* client_addr) {
    // Implement the logic for each case of ACK
     if(fill_dhcp_ack(packet) < 0) {
        printf("Error al llenar el paquete DHCP ACK\n");
        return -1;
    }

    printf("Se confirmo el uso de la ip con un ACK: %s\n");
    //print_dhcp_strct(packet);

    struct dhcp_packet packet_host;
    get_dhcp_struc_ntoh(packet, &packet_host);

    usleep(1000000);
    if(send_dhcp_package(packet) < 0) {
        printf("Error al enviar el paquete DHCP ACK\n");
        return -1;
    }

    return 0;
}

void *handle_dhcp_generic(void *arg) {
    struct dhcp_thread_data *data = (struct dhcp_thread_data *)arg;

    struct dhcp_packet local_net_packet;
    struct dhcp_packet local_host_packet;
    struct configuration *config = data->config;

    memcpy(&local_net_packet, data->buffer, sizeof(struct dhcp_packet));
    get_dhcp_struc_ntoh(&local_net_packet, &local_host_packet);

    printf("Paquete DHCP recibido y procesado en el hilo %ld:\n", (unsigned long int)pthread_self());
    //print_dhcp_strct(&local_host_packet);

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
        // Posible race condition
        char ip_offer[INET_ADDRSTRLEN];
        if(assign_ip(data->ips, local_host_packet.chaddr, data->config->time_tp, data->config->num_ips, ip_offer) < 0) {
            strcpy(ip_offer, "0.0.0.0");
            //this is temporal the real case is to send a NAK
            //handle_nak(&local_host_packet, data->client_addr);
        }
        printf("DHCPDISCOVER\n");
        handle_discover(&local_host_packet, config, ip_offer);
    } else if(message_type == DHCPREQUEST) {
        printf("DHCPREQUEST\n");
        handle_ack(&local_host_packet, &data->client_addr);
    } else if(message_type == DHCPRELEASE) {
        printf("DHCPRELEASE\n");
        //handle_release(&local_host_packet, data->client_addr);
    } else {
        print_dhcp_strct(&local_host_packet);
        printf("Unknown message type\n");
    }

    // usleep(1000000);
    free(data);
    return NULL;
}

/*
    Function to get the DNS server
        @param packet: pointer to the dns_server string

        @return: void
*/

void get_dns(char* dns_server) {
    strcpy(dns_server, "8.8.8.8");
}

// =================================================
// Socket functions
// =================================================

/*
    Function to create a socket
        @return: The socket file descriptor
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

// /*
//     Function to send a DHCP package
//         @param sockfd: Socket file descriptor
//         @param packet: DHCP packet structure

//         @return: 0 if the package was sent successfully, -1 if there was an error
// */

// int send_dhcp_package(struct dhcp_packet *packet) {
//     int sockfd = create_socket_dgram();


//     struct sockaddr_in client_addr;
//     memset(&client_addr, 0, sizeof(client_addr));
//     client_addr.sin_family = AF_INET;
//     client_addr.sin_port = htons(CLIENT_PORT);  // Puerto DHCP del cliente
//     client_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

//     if (sendto(sockfd, packet, sizeof(*packet), 0, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
//         perror("Error al enviar el mensaje");
//         close(sockfd);
//         return -1;
//     }
//     printf("DHCPOFFER enviado\n");
//     close(sockfd);
//     return 0;
// }

// =================================================


// =================================================
// Main function
// =================================================

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