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
#include <netdb.h>

int mlns_addrparse(struct addrinfo **res, char *str)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int rc = getaddrinfo("0.0.0.0", str, NULL, res);
    if (rc != 0) {
        fprintf(stderr, "getaddrinfo for port %s: %s\n", str, gai_strerror(rc));
    }
    return 1;
}
