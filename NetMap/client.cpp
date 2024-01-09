#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{
    int sd;
    struct sockaddr_in server;
    char message[100];

    if (argc != 3)
    {
        printf("[client] Syntax: %s <server_address> <port>\n", argv[0]);
        return -1;
    }

    int port = atoi(argv[2]);

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[client] Error occured for creating the socket.\n");
        return errno;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_port = htons(port);

    if (server.sin_addr.s_addr == INADDR_NONE) {
        printf("[client] The IP address entered is not valid.\n");
        return -1;
    }


    if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[client] Error occured while trying to connect to server.\n");
        return errno;
    }

    char buffer[100];
    size_t bytesRead;
    size_t messageLength;

    while (true)
    {
        bzero(message, 100);
        printf("[client] Write a command: ");
        fflush(stdout);
        read(0, message, 100);

        messageLength = strlen(message);
        if (messageLength > 0 && message[messageLength - 1] == '\n')
        {
            message[messageLength - 1] = '\0';
        }
        if (write(sd, message, 100) <= 0)
        {
            perror("[client] Error occured while trying to write to server.\n");
            close(sd);
            return errno;
        }

        bzero(buffer, 100);

        if ((bytesRead = read(sd, buffer, sizeof(buffer) - 1)) < 0)
        {
            perror("[client] Error occure while trying to read from server.\n");
            close(sd);
            return errno;
        }
        if (bytesRead > 0)
        {
            buffer[bytesRead] = '\0';
        }

        printf("[client] The received message is: %s\n", buffer);

        if (strcmp(message, "quit") == 0)
        {
            break;
        }
    }
    close(sd);
}
