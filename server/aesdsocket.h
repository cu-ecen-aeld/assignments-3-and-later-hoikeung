#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <signal.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>
#include <arpa/inet.h>

#define MAX_BUFFER_LEN  50000
#define PORT            "9000"
#define FILE_NAME       "/var/tmp/aesdsocketdata"
#define TIMESTAMP_INTERVAL 10

//Colour code for debug
#define RESET   "\033[0m"
#define RED     "\033[31m"

struct thread_entry 
{
    pthread_t            thread_id;
    int                  connection_fd;
    char                 ip[INET_ADDRSTRLEN];
    volatile int         done;
    SLIST_ENTRY(thread_entry) entries;
};
