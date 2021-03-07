//Example code: A simple server side code, which echos back the received message. 
//Handle multiple socket connections with select and fd_set on Linux
#include <stdio.h>  
#include <string.h>   //strlen  
#include <stdlib.h>  
#include <errno.h>  
#include <unistd.h>   //close  
#include <arpa/inet.h>    //close  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros  
#include <signal.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#define NO_SOCKET -1     
#define PORT 8888  

#define MAX_CLIENTS 10
int msg;
int sock;
int client_socket[MAX_CLIENTS];
struct sockaddr_in address;

void wiringPiI2CWriteReg8( int fd, int addr, int reg, int data )
{
    unsigned char buf[2] = { reg, data };

    ioctl( fd, I2C_SLAVE, addr );
    write( fd, buf, 2 );
}

void set_backlight( int fd, int r, int g, int b )
{
    wiringPiI2CWriteReg8( fd, 0x62, 0, 0 );
    wiringPiI2CWriteReg8( fd, 0x62, 1, 0 );
    wiringPiI2CWriteReg8( fd, 0x62, 8, 0xaa );
    wiringPiI2CWriteReg8( fd, 0x62, 4, r );
    wiringPiI2CWriteReg8( fd, 0x62, 3, g );
    wiringPiI2CWriteReg8( fd, 0x62, 2, b );
}

void textCommand(int fd, int cmd)
{
    wiringPiI2CWriteReg8( fd, 0x3e, 0x80, cmd );
}

void setText(int fd, char * text)
{
    int i;

    textCommand( fd, 0x01 );        // clear display
    usleep( 5000);
    textCommand( fd, 0x08 | 0x04 ); // display on, no cursor
    textCommand( fd, 0x28 );        // 2 lines
    usleep( 5000 );
    for (i=0; text[i] != '\0'; i++) {
        if (text[i] == '\n') {
            textCommand( fd, 0xc0 );
        } else {
            wiringPiI2CWriteReg8( fd, 0x3e, 0x40, text[i] );
        }
    }
}

void signal_callback_handler(int signum) {
   printf("SIGINT or CTRL-C detected. Exiting gracefully\n");
   // Terminate program
   exit(signum);
}

int make_socket( uint16_t port )
{
    int opt = 1;

    // Creating socket file descriptor
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                                                  &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( port );

    // Forcefully attaching socket to the port 8080
    if (bind(sock, (struct sockaddr *)&address,
                                 sizeof(address))<0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    printf("Listener on port %d \n", port);

    //try to specify maximum of 3 pending connections for the master socket
    if (listen(sock, 3) < 0)
    {  
        perror("listen");
        exit(EXIT_FAILURE);
    }

    return 0;
}

int build_fd_sets(fd_set *readfds, fd_set *writefds)
{
    int i;

    //clear the socket set
    FD_ZERO(readfds);
    FD_ZERO(writefds);
    FD_SET(sock, readfds);

    for (i = 0; i < MAX_CLIENTS; ++i)
      if (client_socket[i] != NO_SOCKET)
        FD_SET(client_socket[i], readfds);

    for (i = 0; i < MAX_CLIENTS; ++i)
      if (client_socket[i] != NO_SOCKET && (msg > 0))
        FD_SET(client_socket[i], writefds);

    return 0;
}

int handle_new_connection()
{
    int i;
    int addrlen;
    addrlen = sizeof(address);
    int new_socket;
    //const char *message = "ECHO Daemon v1.0\n";

    if ((new_socket = accept(sock,
            (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    //inform user of socket number - used in send and receive commands
    printf("New connection , socket fd is %d , ip is : %s , port : %d\n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
/*
    //send new connection greeting message
    if( send(new_socket, message, strlen(message), 0) != strlen(message) )
    {  
        perror("send");
    }
*/
    for (i = 0; i < MAX_CLIENTS; ++i) {
        if (client_socket[i] == NO_SOCKET) {
            client_socket[i] = new_socket;
            return 0;
        }
    }
}

int main(int argc , char *argv[])   
{   
    int activity, i, valread, sd, max_sd;
    int addrlen;
    int fd;
    addrlen = sizeof(address);
    char message2[40];
    char buffer[1025];
    fd_set readfds;   
    fd_set writefds;     

    signal(SIGINT, signal_callback_handler);
    fd = open( "/dev/i2c-1", O_RDWR );

    textCommand( fd, 0x01 );        // clear display
    usleep( 5000 );
    textCommand( fd, 0x08 | 0x04 ); // display on, no cursor
    textCommand( fd, 0x28 );        // 2 lines
    usleep( 5000 );

    //initialise all client_socket[] to 0 so not checked
    for (i = 0; i < MAX_CLIENTS; i++)   
    {   
        client_socket[i] = NO_SOCKET;   
    }   
        
    make_socket( PORT );
 
    //accept the incoming connection
    puts("Waiting for connections ...");   
 
    while(1)   
    {
        build_fd_sets(&readfds, &writefds);
  
        // calculate the max fd 
        max_sd = sock; 
        for (i = 0; i < MAX_CLIENTS; ++i) {
            if (client_socket[i] > max_sd)
                max_sd = client_socket[i];
        }     

        //wait for an activity on one of the sockets , timeout is NULL ,
        //so wait indefinitely
        activity = select( max_sd + 1, &readfds, &writefds, NULL , NULL);

        if ((activity < 0) && (errno!=EINTR))
        {
            printf("select error");
        }
     
        //If something happened on the master socket ,  
        //then its an incoming connection
        if (FD_ISSET(sock, &readfds))   
        {
            handle_new_connection();
            //msg = 1;    // for every new connection, just send one msg
        }

        for (i = 0; i < MAX_CLIENTS; ++i) {
            if (client_socket[i] != NO_SOCKET && FD_ISSET(client_socket[i], &readfds)) {
                if ((valread = read(client_socket[i], buffer, 1024)) == 0)   
                {   
                    //Somebody disconnected , get his details and print
                    getpeername(sd , (struct sockaddr*)&address , \
                        (socklen_t*)&addrlen);   
                    printf("Host disconnected , ip %s , port %d \n" ,  
                          inet_ntoa(address.sin_addr) , ntohs(address.sin_port));   
                         
                    //Close the socket and mark as 0 in list for reuse
                    close(client_socket[i]);
                    FD_CLR(client_socket[i], &readfds);
                    client_socket[i] = NO_SOCKET;   
                }   
                     
                //Echo back the message that came in
                else 
                {
                    buffer[valread] = '\0';
                    if (strcmp(buffer, "red") == 0) {
                        set_backlight( fd, 255, 0, 0 );
                        setText( fd, "\n\n" );
                        strcpy( message2, "Red" );
                        msg = 1;
                    } else if (strcmp(buffer, "green") == 0) {
                        set_backlight( fd, 0, 255, 0 );
                        setText( fd, "\n\n" );
                        strcpy( message2, "Green" );
                        msg = 1;
                    } else if (strcmp(buffer, "blue") == 0) {
                        set_backlight( fd, 0, 0, 255 );
                        setText( fd, "\n\n" );
                        strcpy( message2, "Blue" );
                        msg = 1;
                    } else if (strcmp(buffer, "yellow") == 0) {
                        set_backlight( fd, 255, 255, 0 );
                        setText( fd, "\n\n" );
                        strcpy( message2, "Yellow" );
                        msg = 1;
                    } else if (strcmp(buffer, "cyan") == 0) {
                        set_backlight( fd, 0, 255, 255 );
                        setText( fd, "\n\n" );
                        strcpy( message2, "Cyan" );
                        msg = 1;
                    } else if (strcmp(buffer, "purple") == 0) {
                        set_backlight( fd, 255, 0, 255 );
                        setText( fd, "\n\n" );
                        strcpy( message2, "Purple" );
                        msg = 1;
                    } else if (strcmp(buffer, "black") == 0) {
                        set_backlight( fd, 0, 0, 0 );
                        setText( fd, "\n\n" );
                        strcpy( message2, "Black" );
                        msg = 1;
                    } else if (strcmp(buffer, "white") == 0) {
                        set_backlight( fd, 255, 255, 255 );
                        setText( fd, "\n\n" );
                        strcpy( message2, "White" );
                        msg = 1;
                    } else {
                        set_backlight( fd, 128, 255, 0 );
                        setText( fd, buffer );
                        //close( fd );
                        strcpy( message2, "OK" );
                        msg = 1;
                    }  
                    //printf("%s\n", buffer);
                    FD_CLR(client_socket[i], &readfds);
                }   
            }
  
            if (client_socket[i] != NO_SOCKET && FD_ISSET(client_socket[i], &writefds)) {
                msg = 0;
                send(client_socket[i], message2, strlen(message2), 0);
                FD_CLR(client_socket[i], &writefds);
            }
        } // for
    }  // while 
         
    return 0;   
}
