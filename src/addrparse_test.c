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
#include <assert.h>

#include "addrparse.h"

struct addrinfo **create_addr()
{
    struct addrinfo **addr;
    addr = malloc(sizeof(addr));
    return addr;
}

int main(int argc, char **argv)
{
    struct addrinfo **addr = create_addr();
    mlns_addrparse(addr, "80");

    printf("mlns_addrparse(addr, \"80\");\n");
    struct sockaddr *sa = (*addr)->ai_addr;

    printf("nu ir ka %p\n", sa);
    printf("port == %hu \n",
           htons(((struct sockaddr_in *) (*addr)->ai_addr)->sin_port));
    assert(80 == htons(((struct sockaddr_in *) (*addr)->ai_addr)->sin_port));
}
