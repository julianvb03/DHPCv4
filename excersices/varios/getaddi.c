#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<stdio.h>

int main(){
    const char direcction[] = "www.google.com";
    printf("Direccion %s\n", direcction);
    
    struct addrinfo hints;
    memsent(&hists, 0, sizeof hints);  
    getaddrinfo(direcction, AF_INET, 
}

