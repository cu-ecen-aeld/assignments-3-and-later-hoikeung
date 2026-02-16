#include <syslog.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

int main(int argc, char  *argv[])
{
    openlog("writer.c log", 0, LOG_USER);

    // printf("Test12345");

    if (argc != 3)
    {
        syslog(LOG_ERR, "Invalid Number of arguments: %d", argc);
        closelog();
        return 1;
    }

    const char *writefile = argv[1];
    const char *writestr = argv[2];

    FILE *file = fopen(writefile, "w");

    if (file == NULL)
    {
        syslog(LOG_ERR, "Cannot open file, Error: %s", strerror(errno));
        fclose(file);
        closelog();
        return 1;
    }
    else
    {
        ssize_t n = fwrite(writestr, strlen(writestr), 1, file);
        if (n < 0)
        {
            syslog(LOG_ERR, "Cannot write to file, Error: %s", strerror(errno));
            fclose(file);
            closelog();
            return 1;
        }
        else
        {
            syslog(LOG_DEBUG, "Writing %s to %s", writestr, writefile);
            fclose(file);
            closelog();
            return 0;
        }
    }
}
