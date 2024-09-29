#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#define PORT 8080
#define False 0
#define True 1


int main() {
    struct sockaddr* destination_addres_cast = NULL;
    int fd;
    int status;
    int ipv4;
    

    #ifdef IPV4
        struct sockaddr_in destination_addres_ipv4;
        destination_addres_ipv4.sin_family = AF_INET;
        destination_addres_ipv4.sin_port = htons(PORT);
        status = inet_pton(AF_INET, "127.0.0.1", &destination_addres_ipv4.sin_addr);
        destination_addres_cast = (struct sockaddr*) &destination_addres_ipv4;
        ipv4 = True;
        puts("IPv4");
    #else
        struct sockaddr_in6 destination_addres_ipv6;
        destination_addres_ipv6.sin6_family = AF_INET6;
        destination_addres_ipv6.sin6_port = htons(PORT);
        status = inet_pton(AF_INET6, "::1", &destination_addres_ipv6.sin6_addr);
        destination_addres_cast = (struct sockaddr*) &destination_addres_ipv6;
        ipv4 = False;
        puts("IPv6");
    #endif

    fd = socket(ipv4 ? AF_INET : AF_INET6, SOCK_STREAM, 0);
    if(fd == -1){
        perror("Error in socket");
        return -1;
    }

    status = connect(fd, destination_addres_cast, ipv4 ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6));
    if(status == -1){
        perror("Error in connect");
        return -1;
    }

    char message[] = "Te hablo desde la prisicion";

    send(fd, message, strlen(message), 0);



}