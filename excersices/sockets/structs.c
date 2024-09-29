#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>	// For Internet Domain Sockets (sockaddr_in & sockadd_in6)
#include <string.h>
/*
struct addrinfo {
    int ai_flags; 		// AI_PASSIVE, AI_CANONNAME, etc.
    int ai_family; 		// AF_INET, AF_INET6, AF_UNSPEC
    int ai_socktype; 		// SOCK_STREAM, SOCK_DGRAM
    int ai_protocol; 		// use 0 for "any"
    size_t ai_addrlen; 		// size of ai_addr in bytes
    struct sockaddr *ai_addr; 	// struct sockaddr_in or _in6
    char *ai_canonname;		// full canonical hostname
    struct addrinfo *ai_next;	// linked list, next node
};
*/


/*
struct sockaddr {
    sa_family_t sa_family;	// Address family
    char sa_data[];		// Socket address
};

struct sockaddr_storage {
    sa_family_t ss_family;	// Address family
};
*/

/*
struct sockaddr_in {
    sa_family_t sin_family;     // AF_INET 
    in_port_t sin_port;       	// Port number
    struct in_addr sin_addr;	// IPv4 address
};

struct sockaddr_in6 {
    sa_family_t sin6_family;    // AF_INET6
    in_port_t sin6_port;      	// Port number
    uint32_t sin6_flowinfo;  	// IPv6 flow inf
    struct in6_addr sin6_addr;  // IPv6 address
    uint32_t sin6_scope_id;  	// Set of interfaces for a scope
};

struct in_addr {
    in_addr_t s_addr;
};

struct in6_addr {
    uint8_t   s6_addr[16];
};

typedef uint32_t in_addr_t;
typedef uint16_t in_port_t;
*/

int main(int argc, char* argv[]) {
    struct addrinfo socket_information, *response;
    int status;
    memset(&socket_information, 0, sizeof(socket_information));

    socket_information.ai_family = AF_UNSPEC;
    socket_information.ai_protocol = 0;
    socket_information.ai_flags = AI_PASSIVE;

    status = getaddrinfo(NULL, "80", &socket_information, &response);
    if(status != 0) {
	puts("Error on getaddrinfo");
	return -1;
    }

    for(struct addrinfo *p = response; p != NULL; p = p->ai_next) {
    
	char ip_type[5]; 
    int ip_size = 0;
    if(p->ai_family == AF_INET) {
        strcpy(ip_type, "IPv4");
        ip_size

    } else if(p->ai_family == AF_INET6) {
        strcpy(ip_type, "IPv6");
    } else {
        strcpy(ip_type, "NN");
    }
    int port_size = 0;
    char ip_address[INET6_ADDRSTRLEN];
	printf("Tipo de la ip: %s\n", ip_type);
    }

    return 0;
}
