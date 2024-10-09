#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT "3490"
#define SERVER_IP "127.0.0.1"

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

    status = getaddrinfo(SERVER_IP, PORT, &hints, &res);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1) {
        perror("socket");
        return 2;
    }

    const char *message = "Hello, server!";
    status = sendto(sockfd, message, strlen(message), 0, res->ai_addr, res->ai_addrlen);
    if (status == -1) {
        perror("sendto");
        close(sockfd);
        return 3;
    }

    printf("client: sent packet to %s\n", SERVER_IP);

    addr_len = sizeof their_addr;
    status = recvfrom(sockfd, buffer, sizeof buffer, 0, (struct sockaddr *)&their_addr, &addr_len);
    if (status == -1) {
        perror("recvfrom");
        close(sockfd);
        return 4;
    }

    buffer[status] = '\0';
    inet_ntop(their_addr.ss_family, &((struct sockaddr_in *)&their_addr)->sin_addr, ip_str, sizeof ip_str);
    printf("client: received packet from %s\n", ip_str);
    printf("client: packet is %d bytes long\n", status);
    printf("client: packet contains \"%s\"\n", buffer);

    close(sockfd);
    return 0;
}