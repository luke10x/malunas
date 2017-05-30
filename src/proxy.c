#define _XOPEN_SOURCE 700
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <string.h>
#include <poll.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <getopt.h>

#include "addrparse.h"

static struct option const longopts[] = {
    {"tty", required_argument, NULL, 't'},
};

static struct option const proxy_longopts[] = {
    {"tty", no_argument, NULL, 't'},
    {NULL, 0, NULL, 0}
};


void proxy_usage(int status)
{
    fputs("\
HELLO\n\
\n", stdout);
    exit(1);
}

extern int pass_traffic(int front_read, int front_write, int back_read, int back_write);

void mlns_proxy_handle(int server_fd, int logfd, int argc, char *argv[])
{

    int client_fd;
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Error : Could not create socket \n");
        return;
    }

    struct addrinfo **target_addr;
    target_addr = malloc(sizeof(target_addr));
    int rc = mlns_addrparse(target_addr, argv[1]);
    if (!rc) {
        printf("\n mlns_addrparse error occured\n");
        return;
    }

    rc = connect(client_fd, (struct sockaddr *) ((*target_addr)->ai_addr),
                 (*target_addr)->ai_addrlen);
    if (rc < 0) {
        printf("Error : Connect to '%s' failed \n", argv[1]);

        struct sockaddr *res = (struct sockaddr *) ((*target_addr)->ai_addr);
        char *s = NULL;
        switch (res->sa_family) {
        case AF_INET:{
                struct sockaddr_in *addr_in = (struct sockaddr_in *) res;
                s = malloc(INET_ADDRSTRLEN);
                inet_ntop(AF_INET, &(addr_in->sin_addr), s, INET_ADDRSTRLEN);
                break;
            }
        case AF_INET6:{
                struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *) res;
                s = malloc(INET6_ADDRSTRLEN);
                inet_ntop(AF_INET6, &(addr_in6->sin6_addr), s,
                          INET6_ADDRSTRLEN);
                break;
            }
        default:
            break;
        }
        printf("IP address: %s\n", s);
        free(s);

        return;
    }

    /* Read-end of backend is a write-end of the process */
    pass_traffic(server_fd, server_fd, client_fd, client_fd);

    close(client_fd);
    close(server_fd);
}
