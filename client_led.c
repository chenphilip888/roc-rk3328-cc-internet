// Client side C/C++ program to demonstrate Socket programming
#include <stdio.h> 
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <netinet/in.h> 
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <string.h> 
#include <stdlib.h>

#define HOST "192.168.86.202"
#define PORT 8888

int make_socket( uint16_t port )
{
    int sock;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons( port );

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, HOST, &serv_addr.sin_addr)<=0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }
    return sock;
}
   
int main(int argc, char const *argv[]) 
{ 
    int sock, valread, i; 
    const char *led_on = "1";
    const char *led_off  = "0"; 
    char buffer[1024] = {0}; 
     
    sock = make_socket( PORT );

    for ( i = 0; i < 5; i++ ) {
        send(sock , led_on, strlen(led_on) , 0 ); 
        valread = read( sock, buffer, 1024 ); 
        buffer[valread] = '\0';
        printf( "%s\n", buffer );
        sleep( 1 );
        send(sock , led_off, strlen(led_off) , 0 );
        valread = read( sock, buffer, 1024 );
        buffer[valread] = '\0';
        printf( "%s\n", buffer );
        sleep( 1 );
    } 
    return 0; 
} 
