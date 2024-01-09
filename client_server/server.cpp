#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <utmp.h>
#include <time.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/socket.h>
const char* FIFO_PATH_READING = "./connection/client_to_server_fifo";
const char* FIFO_PATH_WRITING = "./connection/server_to_client_fifo";
const char* USERS_PATH = "./data/users";

bool handleLogin(char buffer[], int fifo_fd_writing, char message[], size_t message_size) {
    char* username = buffer + 8;

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("Pipe creation failed");
        return false;
    }

    pid_t child_pid = fork();
    if (child_pid == -1) {
        perror("Fork failed");
        close(pipefd[0]);
        close(pipefd[1]);
        return false;
    }

    if (child_pid == 0) {
        close(pipefd[0]);

        FILE* usersFile = fopen(USERS_PATH, "r");
        if (!usersFile) {
            perror("Failed to open users file.");
            strcpy(message, "System problem occurred, please try again later.");
            write(pipefd[1], message, strlen(message));
            close(pipefd[1]);
            exit(0);
        }

        bool flag = false;
        char line[15];
        while (fgets(line, sizeof(line), usersFile)) {
            line[strcspn(line, "\n")] = '\0';
            if (strcmp(line, username) == 0) {
                strcpy(message, "Logged in successfully.");
                write(pipefd[1], message, strlen(message));
                flag = true;
                break;
            }
        }

        fclose(usersFile);

        if (!flag) {
            strcpy(message, "There is no account with this username.");
            write(pipefd[1], message, strlen(message));
        }

        close(pipefd[1]);
        exit(0);
    } else {
        close(pipefd[1]);

        char chunk[256];
        int bytesRead;
        while ((bytesRead = read(pipefd[0], chunk, sizeof(chunk)-1)) > 0) {
            chunk[bytesRead] = '\0';
            size_t space_left = message_size - strlen(message) - 1;
            strncat(message, chunk, space_left);
        }
        close(pipefd[0]);

        if (write(fifo_fd_writing, message, strlen(message)) == -1) {
            perror("Failed to send the process info to client.");
            wait(NULL);
            return false;
        }

        bool logged = (strncmp(message, "Logged in successfully.", 23) == 0);
        wait(NULL);
        return logged;
    }
}


bool handleGetLoggedUsers(int fifo_fd_writing, char message[], size_t message_size) {
    int sockets[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == -1) {
        perror("socketpair");
        return false;
    }

    pid_t child_pid = fork();
    if (child_pid == -1) {
        perror("fork");
        close(sockets[0]);
        close(sockets[1]);
        return false;
    }

    if (child_pid == 0) {
        close(sockets[0]);

        struct utmp *ut;
        setutent();

        while ((ut = getutent()) != NULL) {
            if (ut->ut_type == USER_PROCESS) {
                time_t login_time = (time_t) ut->ut_tv.tv_sec;
                snprintf(message, message_size, "User: %s, Host: %s, Time: %s", ut->ut_user, ut->ut_host, ctime(&login_time));
                    
                size_t length = strlen(message);
                if (length > 0 && message[length - 1] == '\n') {
                    message[length - 1] = '\0';
                }

                if (write(sockets[1], message, strlen(message)) == -1) {
                    perror("Failed to send user information to parent.");
                }
            }
        }
        
        close(sockets[1]);
        endutent();
        exit(0);
    } else {
        close(sockets[1]);

        char chunk[256];
        int bytesRead;
        while ((bytesRead = read(sockets[0], chunk, sizeof(chunk)-1)) > 0) {
            chunk[bytesRead] = '\0';
            size_t space_left = message_size - strlen(message) - 1;
            strncat(message, chunk, space_left);
        }
        close(sockets[0]);

        size_t length = strlen(message);
        if (length > 0 && message[length - 1] == '\n') {
            message[length - 1] = '\0';
        }

        if (write(fifo_fd_writing, message, strlen(message)) == -1) {
            perror("Failed to send the process info to client.");
            return false;
        }
        wait(NULL);
    }
    return true;
}

bool handleGetProcInfo(char buffer[], int fifo_fd_writing, char message[], size_t message_size) {
    int charsUntilPID = 16;
    char* pid = buffer + charsUntilPID;

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("Pipe creation failed");
        return false;
    }

    pid_t child_pid = fork();
    if (child_pid == -1) {
        perror("Fork failed");
        close(pipefd[0]);
        close(pipefd[1]);
        return false;
    }

    if (child_pid == 0) {
        close(pipefd[0]);

        char path[31];
        snprintf(path, sizeof(path), "/proc/%s/status", pid);

        FILE *file = fopen(path, "r");
        if (file) {
            char line[51];
            while (fgets(line, sizeof(line), file)) {
                if (strncmp(line, "Name:", 5) == 0 ||
                strncmp(line, "State:", 6) == 0 ||
                strncmp(line, "PPid:", 5) == 0 ||
                strncmp(line, "Uid:", 4) == 0 ||
                strncmp(line, "VmSize:", 7) == 0) {
                write(pipefd[1], line, strlen(line));
            }
            }
            fclose(file);
        } else {
            char errorMsg[256];
            snprintf(errorMsg, sizeof(errorMsg), "Failed to open /proc/%s/status", pid);
            write(pipefd[1], errorMsg, strlen(errorMsg));
        }

        close(pipefd[1]);
        exit(0); 
    } else { 
        close(pipefd[1]);

        char chunk[256];
        int bytesRead;
        while ((bytesRead = read(pipefd[0], chunk, sizeof(chunk)-1)) > 0) {
            chunk[bytesRead] = '\0';
            size_t space_left = message_size - strlen(message) - 1;
            strncat(message, chunk, space_left);
        }
        close(pipefd[0]);

        size_t length = strlen(message);
        if (length > 0 && message[length - 1] == '\n') {
            message[length - 1] = '\0';
        }

        if (write(fifo_fd_writing, message, strlen(message)) == -1) {
            perror("Failed to send the process info to client.");
            return false;
        }
        
        wait(NULL);
    }

    return true;
}


int main() {
    struct stat fileInfo;
    
    if (stat(FIFO_PATH_READING, &fileInfo) != 0) {
        if (mkfifo(FIFO_PATH_READING, 0666) == -1) {
            perror("mkfifo writing");
            return 1;
        }
    }    

    if (stat(FIFO_PATH_WRITING, &fileInfo) != 0) {
        if (mkfifo(FIFO_PATH_WRITING, 0666) == -1) {
            perror("mkfifo writing");
            return 1;
        }
    }

    int fifo_fd_reading = open(FIFO_PATH_READING, O_RDONLY);
    int fifo_fd_writing = open(FIFO_PATH_WRITING, O_WRONLY);
    
    if (fifo_fd_writing == -1 || fifo_fd_reading == -1) {
        perror("open");
        return 1;
    }

    char buffer[31];
    bool logged = false;
    char message[251];
    while(true) {
        memset(buffer, 0, sizeof(buffer));
        memset(message, 0, sizeof(message));
        ssize_t bytesRead = read(fifo_fd_reading, buffer, sizeof(buffer) - 1);

        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';  
            printf("Received: %s\n", buffer);

            if (strncmp(buffer, "login :", 7) == 0) {
                logged = handleLogin(buffer, fifo_fd_writing, message, sizeof(message));
            } else if (strcmp(buffer, "get-logged-users") == 0) {
                if (logged == false) {
                    strcpy(message, "You must be logged in to perform this action.");
                    if (write(fifo_fd_writing, message, strlen(message)) == -1) {
                        perror("Failed to send the no permission message.");
                    }
                } else {
                    handleGetLoggedUsers(fifo_fd_writing, message, sizeof(message));
                }
            } else if (strncmp(buffer, "get-proc-info :", 15) == 0) {
                if (logged == false) {
                    strcpy(message, "You must be logged in to perform this action.");
                    if (write(fifo_fd_writing, message, strlen(message)) == -1) {
                        perror("Failed to send the no permission message.");
                    }
                } else {
                    handleGetProcInfo(buffer, fifo_fd_writing, message, sizeof(message));
                }
            } else if (strcmp(buffer, "logout") == 0) {
                if (logged == true) {
                    logged = false;
                    strcpy(message, "You are not logged in anymore.");
                    if (write(fifo_fd_writing, message, strlen(message)) == -1) {
                        perror("Failed to send the log out message.");
                    }
                } else {
                    strcpy(message, "You are already logged out.");
                    if (write(fifo_fd_writing, message, strlen(message)) == -1) {
                        perror("Failed to send the log out message.");
                    }
                }
            } else if (strcmp(buffer, "quit") == 0) {
                strcpy(message, "Application closed.");
                if (write(fifo_fd_writing, message, strlen(message)) == -1) {
                   perror("Failed to send the quit message.");
                }
               break;
            } else {
                strcpy(message, "Invalid command.");
                if (write(fifo_fd_writing, message, strlen(message)) == -1) {
                    perror("Failed to send the wrong command message.");
                }
            }
        }
    }
    close(fifo_fd_reading);
    close(fifo_fd_writing);
}
