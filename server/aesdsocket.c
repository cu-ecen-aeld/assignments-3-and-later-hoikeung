
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <signal.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX_BUFFER_LEN  102400
#define PORT            "9000"
#define FILE_NAME       "/var/tmp/aesdsocketdata"

static int socket_fd;
int status;
struct addrinfo hints;
struct addrinfo *servinfo;

void exit_prog(int e)
{
    freeaddrinfo(servinfo);
    closelog();
    exit(e);
}

void signal_handler(int signal) 
{
    if (signal == SIGINT || signal == SIGTERM) 
    {
        syslog(LOG_NOTICE, "Caught signal, exiting.");
        int removed = remove(FILE_NAME);
        if (removed == 0)
        {
            printf("Removed success\r\n");
        }
        else
        {
            printf("Removed fail\r\n");
        }
        shutdown(socket_fd, SHUT_RDWR);
    }
    printf("Exit\r\n");
    exit_prog(0);
}

ssize_t write_file(const char* file_name, const char* str)
{
    ssize_t n = 0;
    FILE *file = fopen(file_name, "a+");

    if (file == NULL)
    {
        fprintf(stderr, "Cannot open write file, Error: %s\r\n", strerror(errno));
        fclose(file);
        exit_prog(1);
    }
    else
    {
        n = fwrite(str, strlen(str), 1, file);
        if (n < 0)
        {
            fprintf(stderr, "Cannot write to file, Error: %s\r\n", strerror(errno));
            fclose(file);
            exit_prog(1);
        }
        else
        {
            fclose(file);
        }
    }
    return n;
}

ssize_t read_file(const char* file_name, char* buffer, int length)
{
    FILE *file = fopen(file_name, "r");
    ssize_t read_size = fread(buffer, 1, length, file);
    fclose(file);
    return read_size;
}

int main(int argc, char  *argv[])
{
    openlog("aesdsocket.c log", 0, LOG_USER);

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // int status;
    // struct addrinfo hints;
    // struct addrinfo *servinfo;

    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    status = getaddrinfo(NULL, PORT, &hints, &servinfo);
    if (status != 0)
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit_prog(1);
    }

    socket_fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (socket_fd == -1)
    {
        fprintf(stderr, "socket error: %s\n", gai_strerror(status));
        exit_prog(1);
    }

    if (bind(socket_fd, servinfo->ai_addr, servinfo->ai_addrlen) != 0)
    {
        fprintf(stderr, "bind error: %s\n", gai_strerror(status));
        exit_prog(1);
    }

    int daemon_mode = 0;
    int opt;
    while ((opt = getopt(argc, argv, "d")) != -1) 
    {
        if (opt == 'd') daemon_mode = 1;
    }
    if (daemon_mode) 
    {
        if (daemon_mode) 
        {
            if ((daemon(0, 0)) == -1) 
            {
                fprintf(stderr, "Failed to run in daemon mode.\n");
                exit_prog(1);
            }
        }
    }

    if (listen(socket_fd, 1) != 0)
    {
        fprintf(stderr, "listen error: %s\n", gai_strerror(status));
        exit_prog(1);
    }

    while (1)
    {
        int connection_fd = -1;
        struct sockaddr_in connection; 
        socklen_t len = -1; 
        len = sizeof(connection);
        connection_fd = accept(socket_fd, (struct sockaddr *)&connection, &len);
        if (connection_fd == -1)
        {
            fprintf(stderr, "accepy error: %s\n", gai_strerror(status));
            exit_prog(1);
        }
        else
        {
            syslog(LOG_NOTICE, "Accepted connection from %s, fd = %d", inet_ntoa(connection.sin_addr), connection_fd);

            char recv_buf[MAX_BUFFER_LEN] = {0};
            ssize_t buf_len = recv(connection_fd, recv_buf, MAX_BUFFER_LEN, MSG_DONTWAIT); 
            if (buf_len < 0)
            {
                fprintf(stderr, "receive error: %s\n", gai_strerror(status));
            }
            else if (recv_buf[buf_len-1] == '\n')
            {
                write_file(FILE_NAME, recv_buf);
                char send_buf[MAX_BUFFER_LEN] = {0};
                ssize_t read_length = read_file(FILE_NAME, send_buf, MAX_BUFFER_LEN);
                // printf("Read %d words\r\n%s", (int) read_length, send_buf);
                send(connection_fd, send_buf, read_length, MSG_DONTWAIT);
                syslog(LOG_NOTICE, "Closed connection from from %s, fd = %d", inet_ntoa(connection.sin_addr), connection_fd);
            }
        }
    }

    exit_prog(1);
}
