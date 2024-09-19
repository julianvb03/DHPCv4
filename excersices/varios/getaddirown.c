#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<stdio.h>
#include<string.h>

int main() {
    struct addrinfo hints, *response;
    memset(&hints, 0, sizeof hints);
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    int result = getaddrinfo(NULL, "80", &hints, &response);

    for(struct addrinfo *p = response; p != NULL; p = p->ai_next) {
        char host[NI_MAXHOST];
        char serv[NI_MAXSERV];
        getnameinfo(p->ai_addr, p->ai_addrlen, host, NI_MAXHOST, serv, NI_MAXSERV, NI_NUMERICHOST);
        printf("Host: %s, Servicio: %s\n", host, serv);
    }

    if(result != 0) {
        fprintf(stderr, "Error en getaddrinfo: %s\n", gai_strerror(result));
        return 1;
    }

    freeaddrinfo(response);
}