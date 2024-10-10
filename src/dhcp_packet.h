#ifndef DHCPv4
#define DHCPv4

#include <stdint.h>

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

struct dhcp_packet {
    uint8_t op;        // Message op code / message type
    uint8_t htype;     // Hardware address type
    uint8_t hlen;      // Hardware address length
    uint8_t hops;      // Hops
    uint32_t xid;      // Transaction ID
    uint16_t secs;     // Seconds elapsed
    uint16_t flags;    // Flags
    uint32_t ciaddr;   // Client IP address
    uint32_t yiaddr;   // 'Your' IP address
    uint32_t siaddr;   // Next server IP address
    uint32_t giaddr;   // Relay agent IP address
    uint8_t chaddr[16]; // Client hardware address
    uint8_t sname[64];  // Optional server host name
    uint8_t file[128];  // Boot file name
    uint8_t options[DHCP_OPTIONS_LEN]; // Optional parameters field
} __attribute__((packed));  // For avoid padding

void print_dhcp_struc(const char* buffer, int numbytes);

void get_dhcp_struc_hton(struct dhcp_packet *packet, uint8_t op, uint8_t htype, uint8_t hlen, uint8_t hops, uint32_t xid, uint16_t secs, uint16_t flags, uint32_t ciaddr, uint32_t yiaddr, uint32_t siaddr, uint32_t giaddr, uint8_t chaddr[16], uint8_t sname[64], uint8_t file[128], uint8_t options[312]);

void get_dhcp_struc_ntoh(struct dhcp_packet *net_packet, struct dhcp_packet *host_packet);

#endif