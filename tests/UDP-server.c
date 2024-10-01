#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT "3490"
#define TRUE 1
#define FALSE 0

int main() {
    char buffer[512];
    int sockfd, status;
    struct addrinfo hints, *res;
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
    char ip_str[INET_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;    // Usar mi propia IP

    status = getaddrinfo(NULL, PORT, &hints, &res);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1) {
        perror("socket");
        return 2;
    }

    status = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    if (status == -1) {
        close(sockfd);
        perror("setsockopt");
        return 3;
    }

    status = bind(sockfd, res->ai_addr, res->ai_addrlen);
    if (status == -1) {
        close(sockfd);
        perror("bind");
        return 3;
    }

    freeaddrinfo(res);

    printf("server: waiting for connections...\n");
    
    addr_len = sizeof their_addr;
    status = recvfrom(sockfd, buffer, sizeof buffer, 0, (struct sockaddr *)&their_addr, &addr_len);
    if (status == -1) {
        perror("recvfrom");
        return 4;
    }

    buffer[status] = '\0'; 

    inet_ntop(their_addr.ss_family, &((struct sockaddr_in *)&their_addr)->sin_addr, ip_str, sizeof ip_str);
    printf("server: got packet from %s\n", ip_str);
    printf("server: packet is %d bytes long\n", status);
    printf("server: packet contains \"%s\"\n", buffer);

    const char* message = "Hello, client!";
    status = sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)&their_addr, addr_len);
    if (status == -1) {
        perror("sendto");
        return 5;
    }
    
    printf("server: sent packet to %s\n", ip_str);

    close(sockfd);
    return 0;
}
