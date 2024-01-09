#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>

#define PORT 2728

char *conv_addr(struct sockaddr_in address)
{
    static char str[25];
    char port[7];

    strcpy(str, inet_ntoa(address.sin_addr));
    bzero(port, 7);
    sprintf(port, ":%d", ntohs(address.sin_port));
    strcat(str, port);
    return (str);
}

int main()
{
    struct sockaddr_in server; /* structurile pentru server si clienti */
    struct sockaddr_in from;
    fd_set readfds;    /* multimea descriptorilor de citire */
    fd_set actfds;     /* multimea descriptorilor activi */
    struct timeval tv; /* structura de timp pentru select() */
    int sd, client;    /* descriptori de socket */
    int optval = 1;    /* optiune folosita pentru setsockopt()*/
    int fd;            /* descriptor folosit pentru
                      parcurgerea listelor de descriptori */
    int nfds;          /* numarul maxim de descriptori */
    socklen_t len;           /* lungimea structurii sockaddr_in */

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[server] Error occured while creating the socket.\n");
        return errno;
    }

    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    bzero(&server, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT);

    if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[server] Binding error.\n");
        return errno;
    }

    if (listen(sd, 5) == -1)
    {
        perror("[server] Error occured while trying to listen.\n");
        return errno;
    }

    FD_ZERO(&actfds);    /* initial, multimea este vida */
    FD_SET(sd, &actfds); /* includem in multime socketul creat */

    tv.tv_sec = 1;
    tv.tv_usec = 0;

    /* valoarea maxima a descriptorilor folositi */
    nfds = sd;

    printf("[server] Waiting at port %d...\n", PORT);
    fflush(stdout);

    while (true)
    {
        bcopy((char *)&actfds, (char *)&readfds, sizeof(readfds));

        if (select(nfds + 1, &readfds, NULL, NULL, &tv) < 0)
        {
            perror("[server] Error occured while trying to select.\n");
            return errno;
        }

        if (FD_ISSET(sd, &readfds))
        {
            len = sizeof(from);
            bzero(&from, sizeof(from));

            /* a venit un client, acceptam conexiunea */
            client = accept(sd, (struct sockaddr *)&from, &len);

            /* eroare la acceptarea conexiunii de la un client */
            if (client < 0)
            {
                perror("[server] Error occured while trying to accept.\n");
                continue;
            }

            if (nfds < client)
                nfds = client;

           
            FD_SET(client, &actfds);

            printf("[server] Client with the descriptor %d just connected, from address %s.\n", client, conv_addr(from));
            fflush(stdout);
        }
    
        for (fd = 0; fd <= nfds; fd++)
        {
            if (fd != sd && FD_ISSET(fd, &readfds))
            {
                if (sayHello(fd))
                {
                    printf("[server] Client with the descriptor %d disconnected.\n", fd);
                    fflush(stdout);
                    close(fd);   
                    FD_CLR(fd, &actfds);
                }
            }
        } 
    }     
} 

int sayHello(int fd)
{  
    int bytes;   
    char msg[100];         
    char msgrasp[100] = " "; 

    bytes = read(fd, msg, sizeof(msg));
    if (bytes < 0)
    {
        perror("[server] Error occured while trying read from client.\n");
        return 0;
    }
    printf("[server] Recieved the message: %s\n", msg);

    bzero(msgrasp, 100);
    strcat(msgrasp, "Hello ");
    strcat(msgrasp, msg);

    printf("[server] Sending back the message...%s\n", msgrasp);

    if (bytes && write(fd, msgrasp, bytes) < 0)
    {
        perror("[server] Error occured while writing to the client.\n");
        return 0;
    }

    return bytes;
}
