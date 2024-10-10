#include <dhcp_data.h>

void print_dhcp_struc(const char* buffer, int numbytes){
    for (int i = 0; i < numbytes; i++) {
            printf("%02x ", (unsigned char)buffer[i]);
            if ((i + 1) % 16 == 0) {
                printf("\n");
            }
        }
        printf("\n\n");
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