//Example code: A simple server side code, which echos back the received message. 
//Handle multiple socket connections with select and fd_set on Linux
#include <stdio.h>  
#include <string.h>   //strlen  
#include <stdlib.h>
#include <stdbool.h>  
#include <errno.h>  
#include <unistd.h>   //close  
#include <arpa/inet.h>    //close  
#include <sys/types.h>
#include <sys/stat.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros  
#include <signal.h>

#define NO_SOCKET -1     
#define PORT 8888  

#define MAX_CLIENTS 10
int msg;
int sock;
int client_socket[MAX_CLIENTS];
struct sockaddr_in address;

void pwm_open( void )
{
    FILE *fp;
    bool isdir;
    char path[100] = "";
    strcpy( path, "/sys/class/pwm/pwmchip0/pwm0" );
    struct stat st = {0};
    if ( !stat(path, &st) ) {
        isdir = S_ISDIR( st.st_mode );
    }
    if ( !isdir ) {
        fp = fopen( "/sys/class/pwm/pwmchip0/export", "w" );
        fprintf( fp, "%d", 0 );
        fclose( fp );
    }
}

void pwm_polarity( void )
{
    FILE *fp;
    fp = fopen( "/sys/class/pwm/pwmchip0/pwm0/polarity", "w" );
    fputs( "normal", fp );
    fclose( fp );
}

void pwm_enable( void )
{
    FILE *fp;
    fp = fopen( "/sys/class/pwm/pwmchip0/pwm0/enable", "w" );
    fprintf( fp, "%d", 1 );
    fclose( fp );
}

void pwm_stop( void )
{
    FILE *fp;
    fp = fopen( "/sys/class/pwm/pwmchip0/pwm0/enable", "w" );
    fprintf( fp, "%d", 0 );
    fclose( fp );
}

void pwm_close( void )
{
    FILE *fp;
    fp = fopen( "/sys/class/pwm/pwmchip0/unexport", "w" );
    fprintf( fp, "%d", 0 );
    fclose( fp );
}

int pwm_freq( int freq )
{
    FILE *fp;
    int pwm_period;
    pwm_period = 1000000000.0 / freq;
    fp = fopen( "/sys/class/pwm/pwmchip0/pwm0/period", "w" );
    fprintf( fp, "%d", pwm_period );
    fclose( fp );
    return ( pwm_period );
}

void pwm_duty( float duty, int pwm_period )
{
    FILE *fp;
    int dutycycle;
    dutycycle = duty * pwm_period;
    fp = fopen( "/sys/class/pwm/pwmchip0/pwm0/duty_cycle", "w" );
    fprintf( fp, "%d", dutycycle );
    fclose( fp );
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
    int pwm_period;
    addrlen = sizeof(address);
    char message2[40];
    char buffer[1025];
    fd_set readfds;   
    fd_set writefds;     

    signal(SIGINT, signal_callback_handler);
 
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
            pwm_open();
            pwm_period = pwm_freq( 50 );
            pwm_duty( 0.05, pwm_period );
            pwm_polarity();
            pwm_enable();
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
                    pwm_stop();
                    pwm_close();  
                }   
                     
                //Echo back the message that came in
                else 
                {
                    buffer[valread] = '\0';
                    if (strcmp(buffer, "middle") == 0) {
                        pwm_duty( 0.1, pwm_period );
                        strcpy( message2, "Middle " );
                        msg = 1;
                    } else if (strcmp(buffer, "right") == 0) {
                        pwm_duty( 0.05, pwm_period );
                        strcpy( message2, "Right " );
                        msg = 1;
                    } else if (strcmp(buffer, "left") == 0) {
                        pwm_duty( 0.15, pwm_period );
                        strcpy( message2, "Left " );
                        msg = 1;
                    } else if (strcmp(buffer, "stop") == 0) {
                        pwm_stop();
                        strcpy( message2, "Stop " );
                        msg = 1;
                    } else {
                        strcpy( message2, "Error " );
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
