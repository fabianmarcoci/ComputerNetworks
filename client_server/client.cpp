#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
const char* FIFO_PATH_WRITING = "./connection/client_to_server_fifo";
const char* FIFO_PATH_READING = "./connection/server_to_client_fifo";
int main() {
    int fifo_fd_writing = open(FIFO_PATH_WRITING, O_WRONLY);
    int fifo_fd_reading = open(FIFO_PATH_READING, O_RDONLY);
    
    if (fifo_fd_writing == -1 || fifo_fd_reading == -1) {
        perror("open");
        return 1;
    }

    char message[31];
    char buffer[251];
    while (true) {
        memset(message, 0, sizeof(message));
        printf("Enter command: ");
        fgets(message, sizeof(message), stdin);

        size_t length = strlen(message);
        if (length > 0 && message[length - 1] == '\n') {
            message[length - 1] = '\0';
        }

        if (write(fifo_fd_writing, message, strlen(message)) == -1) {
            perror("Some error occured for sending the message, please try again.");
            continue;
        }

        memset(buffer, 0, sizeof(buffer));
        ssize_t bytesRead = read(fifo_fd_reading, buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';  
            printf("%s\n", buffer);
        }

        if (strcmp(message, "quit") == 0) {
            break;
        }

    }
    close(fifo_fd_writing);
    close(fifo_fd_reading);
}