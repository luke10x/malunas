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
#include <ctype.h>

int mlns_addrparse(struct addrinfo **res, char *str)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int n;

    for (n = (int) strlen(str) - 1; isdigit(str[n]) && n > 0; n--);

    char *host_part;
    if (n != 0) {
        host_part = strncpy(malloc(n * sizeof(char)), str, n);
        host_part[n] = 0;
        n++;
    } else {
        host_part = "0.0.0.0";
    }

    int rc = getaddrinfo(host_part, str + n, NULL, res);
    if (rc != 0) {
        fprintf(stderr, "getaddrinfo for %s: %s\n", str, gai_strerror(rc));
    }
    return 1;
}
