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

int test_with_simple_port_number()
{
    printf("\nTest with simple port number:\n");
    struct addrinfo **addr = create_addr();
    mlns_addrparse(addr, "80");

    printf("mlns_addrparse(addr, \"80\");\n");
    struct sockaddr *sa = (*addr)->ai_addr;

    printf("port == %hu \n",
           htons(((struct sockaddr_in *) (*addr)->ai_addr)->sin_port));
    assert(80 == htons(((struct sockaddr_in *) (*addr)->ai_addr)->sin_port));
    printf("address == %s \n",
           inet_ntoa(((struct sockaddr_in *) (*addr)->ai_addr)->sin_addr));
    assert(0 == strcmp
           ("0.0.0.0",
            inet_ntoa(((struct sockaddr_in *) (*addr)->ai_addr)->sin_addr))
        );
}

int test_with_hostname_and_port()
{
    printf("\nTest with hostname and port:\n");
    struct addrinfo **addr = create_addr();
    mlns_addrparse(addr, "localhost:8080");

    printf("mlns_addrparse(addr, \"localhost:8080\");\n");
    struct sockaddr *sa = (*addr)->ai_addr;

    printf("port == %hu \n",
           htons(((struct sockaddr_in *) (*addr)->ai_addr)->sin_port));
    assert(8080 == htons(((struct sockaddr_in *) (*addr)->ai_addr)->sin_port));
    printf("address == %s \n",
           inet_ntoa(((struct sockaddr_in *) (*addr)->ai_addr)->sin_addr));
    assert(0 == strcmp
           ("127.0.0.1",
            inet_ntoa(((struct sockaddr_in *) (*addr)->ai_addr)->sin_addr))
        );
}

int main(int argc, char **argv)
{
    test_with_simple_port_number();
    test_with_hostname_and_port();
}
