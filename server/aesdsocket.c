#include "aesdsocket.h"

static int socket_fd;
int status;
struct addrinfo hints;
struct addrinfo *servinfo;

static pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
static SLIST_HEAD(thread_list_head, thread_entry) thread_list;

static void exit_prog(int e)
{
    freeaddrinfo(servinfo);
    close(socket_fd);
    closelog();
    exit(e);
}

static void signal_handler(int signal) 
{
    if (signal == SIGINT || signal == SIGTERM) 
    {
        syslog(LOG_NOTICE, "Caught signal, exiting.");
        remove(FILE_NAME);
        // int removed = remove(FILE_NAME);
        // if (removed == 0)
        // {
        //     printf("Removed success\n");
        // }
        // else
        // {
        //     printf("Removed fail\n");
        // }
        shutdown(socket_fd, SHUT_RDWR);
    }
    // printf("Exit\n");
    exit_prog(0);
}

ssize_t write_file(const char* file_name, const char* str)
{
    ssize_t n = 0;
    FILE *file = fopen(file_name, "a+");

    if (file == NULL)
    {
        fprintf(stderr, "Cannot open write file, Error: %s\n", strerror(errno));
        fprintf(stderr, RED "Cannot open write file, Error: %s\n" RESET, gai_strerror(status));
        fclose(file);
        exit_prog(1);
    }
    else
    {
        n = fwrite(str, strlen(str), 1, file);
        if (n < 0)
        {
            fprintf(stderr, RED "Cannot write to file, Error: %s\n" RESET, gai_strerror(status));
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

static void *timer_thread(void *arg)
{
    struct timespec req;
    req.tv_sec = TIMESTAMP_INTERVAL;
    req.tv_nsec = 0;

    while (1)
    {
        nanosleep(&req, NULL);

        time_t now = time(NULL);
        struct tm *tm_p = localtime(&now);
        char buf[128] = {0};
        strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S %z", tm_p);
        buf[strlen(buf)] = '\n';

        char timestmp[256] = "timestamp:";
        strcat(timestmp, buf);

        pthread_mutex_lock(&file_mutex);
        write_file(FILE_NAME, timestmp);
        pthread_mutex_unlock(&file_mutex);
    }

    return NULL;
}

static void *connection_thread(void *arg)
{
    struct thread_entry *entry = (struct thread_entry *) arg;

    syslog(LOG_NOTICE, "Accepted connection from %s", entry->ip);

    char recv_buf[MAX_BUFFER_LEN] = {0};
    ssize_t buf_len = recv(entry->connection_fd, recv_buf, MAX_BUFFER_LEN, MSG_DONTWAIT); 
    if (buf_len < 0)
    {
        fprintf(stderr, RED "Receive error: %s\n" RESET, gai_strerror(status));
        exit_prog(1);
    }
    else if (recv_buf[buf_len-1] == '\n')
    {
        pthread_mutex_lock(&file_mutex);
        write_file(FILE_NAME, recv_buf);
        char send_buf[MAX_BUFFER_LEN] = {0};
        ssize_t read_length = read_file(FILE_NAME, send_buf, MAX_BUFFER_LEN);
        send(entry->connection_fd, send_buf, read_length, MSG_DONTWAIT);
        pthread_mutex_unlock(&file_mutex);
        syslog(LOG_NOTICE, "Closed connection from from %s", entry->ip);
        entry->done = 1;
    }

    return NULL;
}

int main(int argc, char  *argv[])
{
    openlog("aesdsocket.c log", 0, LOG_USER);

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    SLIST_INIT(&thread_list);

    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    status = getaddrinfo(NULL, PORT, &hints, &servinfo);
    if (status != 0)
    {
        fprintf(stderr, RED "getaddrinfo error: %s\n" RESET, gai_strerror(status));
        exit_prog(1);
    }

    socket_fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (socket_fd == -1)
    {
        
        exit_prog(1);
    }

    if (bind(socket_fd, servinfo->ai_addr, servinfo->ai_addrlen) != 0)
    {
        fprintf(stderr, RED "bind error: %s\n" RESET, gai_strerror(status));
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
                fprintf(stderr, RED "Failed to run in daemon mode.\n" RESET);
                exit_prog(1);
            }
        }
    }

    if (listen(socket_fd, 10) != 0)
    {
        fprintf(stderr, RED "listen error: %s\n" RESET, gai_strerror(status));
        exit_prog(1);
    }

    // Timestamp
    // pthread_t timer_tid;
    // if (pthread_create(&timer_tid, NULL, timer_thread, NULL) != 0) 
    // {
    //     syslog(LOG_ERR, "timer_thread create error: %s", strerror(errno));
    //     fprintf(stderr, RED "timer_thread create error: %s\n" RESET, gai_strerror(status));
    //     exit_prog(1);
    // }

    while (1)
    {
        int connection_fd = -1;
        struct sockaddr_in connection; 
        socklen_t len = -1; 
        len = sizeof(connection);
        connection_fd = accept(socket_fd, (struct sockaddr *)&connection, &len);
        if (connection_fd == -1)
        {
            fprintf(stderr, RED "accept error: %s\n" RESET, gai_strerror(status));
            exit_prog(1);
        }
        else
        {
            // syslog(LOG_NOTICE, "Accepted connection from %s, fd = %d", inet_ntoa(connection.sin_addr), connection_fd);

            struct thread_entry *entry = malloc(sizeof(struct thread_entry));
            entry->connection_fd = connection_fd;
            entry->done = 0;
            inet_ntop(AF_INET, &connection.sin_addr, entry->ip, sizeof(entry->ip));

            if (pthread_create(&entry->thread_id, NULL, connection_thread, entry) != 0) 
            {
                syslog(LOG_ERR, "connection_thread create error: %s", strerror(errno));
                fprintf(stderr, RED "Fconnection_thread create error: %s\n" RESET, gai_strerror(status));
                close(connection_fd);
                free(entry);
                exit_prog(1);
            }
            SLIST_INSERT_HEAD(&thread_list, entry, entries);

            struct thread_entry *e;
            SLIST_FOREACH(e, &thread_list, entries)
            {
                if (e->done)
                {
                    pthread_join(e->thread_id, NULL);
                    SLIST_REMOVE(&thread_list, e, thread_entry, entries);
                    free(e);
                }
            }
            
        }
    }

    fprintf(stderr, RED "Final: %s\n" RESET, gai_strerror(status));
    exit_prog(1);
}
