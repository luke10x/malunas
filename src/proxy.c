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

void mlns_proxy_handle(int server_fd, int logfd, int argc, char *argv[])
{

    int client_fd;
    if((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return;
    } 

    struct sockaddr_in target_addr; 
    memset(&target_addr, '0', sizeof(target_addr)); 

    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(10080); 

    if (inet_pton(AF_INET, "0.0.0.0", &target_addr.sin_addr) <=0) {
        printf("\n inet_pton error occured\n");
        return;
    }

    if (connect(client_fd, (struct sockaddr *)&target_addr, sizeof(target_addr)) < 0) {
        printf("\n Error : Connect Failed \n");
        return;
    } 

    struct pollfd poll_fds[2];
    struct pollfd *server_pollfd = &poll_fds[0];
    struct pollfd *client_pollfd = &poll_fds[1];

    server_pollfd->fd = server_fd;
    server_pollfd->events = POLLIN;

    client_pollfd->fd = client_fd;
    client_pollfd->events = POLLIN;

    do {
        int ret = poll((struct pollfd *) &poll_fds, 2, 1000);

        if (ret == -1) {
            perror("poll");
            break;
        } else if (ret == 0) {
             /*Poll timeout*/
        } else {

            if (client_pollfd->revents & POLLIN ) {
                /*client_pollfd->revents -= POLLIN;*/

                char buf[0x10] = { 0 };

                int n = read(client_pollfd->fd, buf, 0x10);
                if (n  == 0) {
                    break;
                }

                if (n > 0) {
                    buf[0x10] = 0;
                    write(server_fd, buf, n);

                    dprintf(logfd, "received %d bytes: %s", n, buf);
                }
            }

            if (server_pollfd->revents & POLLIN ) {
                /*server_pollfd->revents -= POLLIN;*/

                char buf[0x10] = { 0 };

                int n = read(server_pollfd->fd, buf, 0x10);

                if (n  == 0) {
                    break;
                }

                if (n > 0) {
                    buf[0x10] = 0;
                    send(client_fd, buf, n, 0);

                    dprintf(logfd, "sent %d bytes: %s", n, buf);
                }
            }

        }
    } while (1);

    close(client_pollfd->fd);
    close(server_pollfd->fd);
}
