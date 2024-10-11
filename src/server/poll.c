#include "dhcpv4_server.h"


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