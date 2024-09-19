#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main() {
   struct sockaddr_in sa;
   inet_pton(AF_INET, "10.12.110.57", &(sa.sin_addr));
   printf("The ip is: %d\n", sa.sin_addr.s_addr);
}
