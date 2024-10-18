#include "dhcpv4_utils.h"

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
    // el place holder anterior de todos antes era s, tener en cuenta por si causa algun error
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

void get_dhcp_struc_hton(struct dhcp_packet *packet, uint8_t op, uint8_t htype, uint8_t hlen, uint8_t hops, uint32_t xid, uint16_t secs, uint16_t flags, uint32_t ciaddr, uint32_t yiaddr, uint32_t siaddr, uint32_t giaddr, uint8_t chaddr[16], uint8_t sname[64], uint8_t file[128], uint8_t options[312]){
     // Set single-byte fields directly
    packet->op = op;
    packet->htype = htype;
    packet->hlen = hlen;
    packet->hops = hops;

    // Convert multi-byte fields to network byte order
    packet->xid = htonl(xid);      // Transaction ID
    packet->secs = htons(secs);    // Seconds elapsed
    packet->flags = htons(flags);  // Flags
    packet->ciaddr = htonl(ciaddr); // Client IP address
    packet->yiaddr = htonl(yiaddr); // 'Your' (client) IP address
    packet->siaddr = htonl(siaddr); // Next server IP address
    packet->giaddr = htonl(giaddr); // Relay agent IP address

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

