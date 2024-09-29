#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>	// For Internet Domain Sockets (sockaddr_in & sockadd_in6)
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#define False 0
#define True 1
#define MAX_BUFFER 10000
#define PORT 8080

int main() {
    char buffer[MAX_BUFFER] = {0};

    int addrinfo_size = sizeof (struct addrinfo);

    int sockaddr_size = sizeof (struct sockaddr);
    int sockaddr_storage_size = sizeof (struct sockaddr_storage);
    int sockaddr_in_size = sizeof (struct sockaddr_in);
    int sockaddr_in6_size = sizeof (struct sockaddr_in6); 
    int in_addr_size = sizeof (struct in_addr);
    int in6_addr_size = sizeof (struct in6_addr);

    printf("Size of struct addrinfo: %d\n\n", addrinfo_size);
    printf("Size of struct sockaddr: %d\n", sockaddr_size);
    printf("Size of struct sockaddr_storage: %d\n", sockaddr_storage_size);
    printf("Size of struct sockaddr_in: %d\n", sockaddr_in_size);
    printf("Size of struct sockaddr_in6: %d\n", sockaddr_in6_size);
    printf("Size of struct in_addr: %d\n", in_addr_size);
    printf("Size of struct in6_addr: %d\n", in6_addr_size);

    int status = 0;
    int ipv4;
    int len;

    struct sockaddr* my_addres_cast = NULL;
    

    #ifdef IPV4
        struct sockaddr_in my_adress_ipv4;
        my_adress_ipv4.sin_family = AF_INET;
        my_adress_ipv4.sin_port = htons(PORT);
        status = inet_pton(AF_INET, "127.0.0.1", &my_adress_ipv4.sin_addr);
        my_addres_cast = (struct sockaddr*) &my_adress_ipv4;
        ipv4 = True;
        puts("IPv4");
    #else
        struct sockaddr_in6 my_adress_ipv6;
        my_adress_ipv6.sin6_family = AF_INET6;
        my_adress_ipv6.sin6_port = htons(PORT);
        status = inet_pton(AF_INET6, "::1", &my_adress_ipv6.sin6_addr);
        my_addres_cast = (struct sockaddr*) &my_adress_ipv6;
        ipv4 = False;
        puts("IPv6");
    #endif

    if(status != 1){
        perror("Error in inet_pton");
        return -1;
    }

    int fd;
    fd = socket(my_addres_cast->sa_family, SOCK_STREAM, 0);
    if(fd == -1){
        perror("Error in socket");
        return -1;
    }

    status = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    if(status == -1){
        perror("Error in setsockopt");
        return -1;
    }

    status = bind(fd, my_addres_cast, ipv4 ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6));
    if(status == -1){
        perror("Error in bind");
        return -1;
    }

    status = listen(fd, 10);
    if(status == -1){
        perror("Error in listen");
        return -1;
    }

    struct sockaddr_storage client_addr;
    socklen_t client_addr_size = sizeof(client_addr);
    int client_fd = accept(fd, (struct sockaddr*) &client_addr, &client_addr_size);
    if(client_fd == -1){
        perror("Error in accept");
        return -1;
    }

    char host[NI_MAXHOST];
    char service[NI_MAXSERV];

    status = getnameinfo((struct sockaddr*) &client_addr, client_addr_size, host, NI_MAXHOST, service, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);
    if(status != 0){
        perror("Error in getnameinfo");
        return -1;
    }

    len = recv(client_fd, buffer, MAX_BUFFER, 0);
    if(len == -1){
        perror("Error in recv");
        return -1;
    } else buffer[len] = '\0';

    printf("Connection from %s:%s\n", host, service);
    printf("Message: %s\n", buffer);


    close(client_fd);
    close(fd);
}


