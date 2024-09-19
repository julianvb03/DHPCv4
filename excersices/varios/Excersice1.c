#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<stdio.h>
#include<string.h>
#include<arpa/inet.h>

#define TRUE 1
#define FALSE 0

int main(int argc, char *argv[]) {
    struct addrinfo hints, *response;
    memset(&hints, 0, sizeof hints);
    int status;


    //hints.ai_family = AF_UNSPEC;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if(argc != 2) {
        fprintf(stderr, "You need to pass only one domain");
        return 1;
    }

    status = getaddrinfo(argv[1], NULL, &hints, &response);
    if(status != 0) {
        fprintf(stderr, "Error on getaddrinfo: %s\n", gai_strerror(status));
        return 2;
    }

    for(struct addrinfo *p = response; p != NULL; p = (*p).ai_next) {
        if((*p).ai_family == AF_INET) {
            char hostname[NI_MAXHOST];
            char service[NI_MAXSERV];
            char ip[INET_ADDRSTRLEN];
            
            status = getnameinfo((*p).ai_addr, sizeof((*p).ai_addr), hostname, NI_MAXHOST, service, NI_MAXSERV, NI_NUMERICHOST);
            if(status != 0) {
                fprintf(stderr, "Error on getnameinfo: %s\n", gai_strerror(status));
                return 3;
            }
            inet_ntop(AF_INET, &((struct sockaddr_in *)((*p).ai_addr))->sin_addr, ip, INET_ADDRSTRLEN);
            if(ip == NULL) {
                fprintf(stderr, "Error on inet_ntop\n");
                return 4;
            }
            printf("Hosntame: %s, IP: %s, Port: %s\n", hostname, ip, service);

        } else if((*p).ai_family == AF_INET6) {
            char hostname[NI_MAXHOST];
            char service[NI_MAXSERV];
            char ip[INET6_ADDRSTRLEN];

            status = getnameinfo((*p).ai_addr, sizeof((*p).ai_addr), hostname, NI_MAXHOST, service, NI_MAXSERV, NI_NUMERICHOST);
            if(status != 0) {
                fprintf(stderr, "Error on getnameinfo: %s\n", gai_strerror(status));
                return 3;
            }
            inet_ntop(AF_INET6, &((struct sockaddr_in6 *)((*p).ai_addr))->sin6_addr, ip, INET6_ADDRSTRLEN);
            if(ip == NULL) {
                fprintf(stderr, "Error on inet_ntop\n");
                return 4;
            }
            printf("Hosntame: %s, IP: %s, Port: %s\n", hostname, ip, service);
        } else {
            fprintf(stderr, "Unknown family\n");
        }
    }

}