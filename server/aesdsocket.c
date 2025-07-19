#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <fcntl.h>

int client_sock;
int serv_sock;

void handle_sigchld(int sig) {
    // Wait for all dead child processes to avoid zombie processes
    // Check serv_sock is still open before closing
    (void)sig; // Unused parameter
    shutdown(serv_sock, SHUT_RDWR);
    shutdown(client_sock, SHUT_RDWR);

    syslog(LOG_DEBUG, "Caught signal, exiting");

    // remove the file if it exists
    remove("/var/tmp/aesdsocketdata");
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    openlog("aesdsocket", LOG_PID | LOG_CONS, LOG_USER);
    // Check the input arguments
    if (argc != 1) {
        // Check the option -d for daemon mode
        if (strcmp(argv[1], "-d") == 0) {
            // Daemon mode
            pid_t pid = fork();
            if (pid < 0) {
                syslog(LOG_ERR, "Fork failed");
                return -1;
            } else if (pid > 0) {
                // Parent process exits
                exit(EXIT_SUCCESS);
            }

            // Child process continues
            if (setsid() < 0) {
                syslog(LOG_ERR, "setsid failed");
                return -1; 
            }
            // Change the working directory to root
            if (chdir("/") < 0) {
                syslog(LOG_ERR, "chdir failed");
                return -1;
            }
            // Close standard input, output, and error
            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);
            // Redirect standard input, output, and error to /dev/null
            int null_fd = open("/dev/null", O_RDWR);
            if (null_fd < 0) {
                syslog(LOG_ERR, "Failed to open /dev/null");
                return -1;
            }
            dup2(null_fd, STDIN_FILENO);
            dup2(null_fd, STDOUT_FILENO);
            dup2(null_fd, STDERR_FILENO);
            close(null_fd);
        }
    }

    FILE *fp;


    serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(9000);
    // Bind with SO_REUSEADDR to allow reuse of the address
    int opt = 1;
    if (setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        syslog(LOG_ERR, "setsockopt failed");
        return -1;
    }
    if(bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        syslog(LOG_ERR, "Bind failed");
        return -1;
    }
    if(listen(serv_sock, 5) < 0) {
        close(serv_sock);
        return -1;
    }
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    // Register signal handler for SIGCHLD
    signal(SIGINT, handle_sigchld);
    signal(SIGTERM, handle_sigchld);

    while (1) {
        client_sock = accept(serv_sock, (struct sockaddr*)&client_addr, &client_len);
        if (client_sock < 0) {
            syslog(LOG_ERR, "Accept failed");
            continue;
        }

        syslog(LOG_DEBUG, "Accepted connection from %s",inet_ntoa(client_addr.sin_addr));

        // Read data from the client till \n
        // And Write it to file /var/tmp/aesdsocketdata
        fp = fopen("/var/tmp/aesdsocketdata", "a+");
        if (fp == NULL) {
            close(client_sock);
            continue;
        }

        char buffer[1024];
        ssize_t bytes_read;
        while ((bytes_read = recv(client_sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[bytes_read] = '\0'; // Null-terminate the string
            char *newline_ptr = strchr(buffer, '\n');
            if (newline_ptr != NULL) {
                *newline_ptr = '\0'; // Replace the newline with null terminator
                fprintf(fp, "%s\n", buffer);
                break;
            }
            fprintf(fp, "%s", buffer);
        }
        fclose(fp);

        // Send the contents of /var/tmp/aesdsocketdata back to the client
        fp = fopen("/var/tmp/aesdsocketdata", "r");
        if (fp == NULL) {
            syslog(LOG_ERR, "Failed to open /var/tmp/aesdsocketdata for reading");
            close(client_sock);
            continue;
        }
        // Read the file size and send it to the client each 1024 bytes
        // Allocate buffer to hold the file contents
        char *file_buffer = malloc(1024 + 1);

        size_t bytes_read_file;
        while ((bytes_read_file = fread(file_buffer, 1, 1024, fp)) > 0) {
            file_buffer[bytes_read_file] = '\0'; // Null-terminate the string
            ssize_t bytes_sent = send(client_sock, file_buffer, bytes_read_file, 0);
            if (bytes_sent < 0) {
                syslog(LOG_ERR, "Failed to send data to client");
            }
        }

        free(file_buffer);
        fclose(fp);

        close(client_sock);
        syslog(LOG_DEBUG, "Closed connection from %s", inet_ntoa(client_addr.sin_addr));
    }
    
    return 0;
}