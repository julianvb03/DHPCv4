#include <sys/types.h>
#include <sys/socket.h> // socket(), connect()
#include <netdb.h>      // addrinfo, getaddrinfo()
#include <stdio.h>
#include <string.h>

int main() {
    const char *direccion = "www.google.com";
    printf("Dirección: %s\n", direccion);

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo(direccion, "80", &hints, &res);
    for(struct addrinfo *p = res; p != NULL; p = (*p).ai_next) {
        char host[NI_MAXHOST];
        char serv[NI_MAXSERV];
        int addrlen;
        if(p->ai_family == AF_INET)
            addrlen = sizeof (struct sockaddr_in);
        else
            addrlen = sizeof (struct sockaddr_in6);
        getnameinfo((*p).ai_addr, addrlen, host, NI_MAXHOST, serv, NI_MAXSERV, NI_NUMERICHOST);
        printf("Host: %s, Servicio: %s\n", host, serv);

    }

    freeaddrinfo(res);
    return 0;
}
