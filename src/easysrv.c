#define _XOPEN_SOURCE 600
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#define __USE_BSD
#include <termios.h>
#include <sys/ioctl.h>
#include <string.h>
#include <ctype.h>
#include <poll.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>

int exec_cmd(int ac, char *av[])
{
    char **cmd;
    int i;
    cmd = (char **) malloc(ac * sizeof(char *));
    for (i = 1; i < ac; i++) {
        cmd[i - 1] = strdup(av[i]);
    }
    cmd[i - 1] = NULL;
    execvp(cmd[0], cmd);
    fflush(stdout);
    perror(cmd[0]);
    exit(-1);
}

pid_t popen_pipes(int ac, char *av[], int *infd, int *outfd, int *errfd)
{
    int in_pipe[2] = { };
    int out_pipe[2] = { };
    int err_pipe[2] = { };

    pipe(in_pipe);
    pipe(out_pipe);
    pipe(err_pipe);

    pid_t pid;
    if ((pid = fork()) == -1) {
        perror("fork");
        return -1;

    } else if (pid == 0) {
        close(in_pipe[1]);
        dup2(in_pipe[0], 0);
        close(in_pipe[0]);

        close(out_pipe[0]);
        dup2(out_pipe[1], 1);
        close(out_pipe[1]);

        close(err_pipe[0]);
        dup2(err_pipe[1], 2);
        close(err_pipe[1]);

        exec_cmd(ac, av);
    }

    close(in_pipe[0]);
    close(out_pipe[1]);
    close(err_pipe[1]);

    *infd = in_pipe[1];
    *outfd = out_pipe[0];
    *errfd = err_pipe[0];

    return pid;
}

pid_t popen_tty(int ac, char *av[], int *fd)
{
    int fdm, fds;
    int rc;
    fdm = posix_openpt(O_RDWR);
    if (fdm < 0) {
        fprintf(stderr, "Error %d on posix_openpt()\n", errno);
        return 1;
    }

    rc = grantpt(fdm);
    if (rc != 0) {
        fprintf(stderr, "Error %d on grantpt()\n", errno);
        return 1;
    }

    rc = unlockpt(fdm);
    if (rc != 0) {
        fprintf(stderr, "Error %d on unlockpt()\n", errno);
        return 1;
    }

    fds = open(ptsname(fdm), O_RDWR);

    pid_t pid;
    if ((pid = fork()) == -1) {
        perror("Error while fork");
        exit(-1);
    } else if (pid == 0) {
        struct termios slave_orig_term_settings;
        struct termios new_term_settings;

        close(fdm);

        rc = tcgetattr(fds, &slave_orig_term_settings);
        new_term_settings = slave_orig_term_settings;
        tcsetattr(fds, TCSANOW, &new_term_settings);

        close(0);
        close(1);
        close(2);

        dup2(fds, 0);
        dup2(fds, 1);
        dup2(fds, 2);

        close(fds);

        setsid();
        ioctl(0, TIOCSCTTY, 1);

        exec_cmd(ac, av);
    }

    close(fds);

    *fd = fdm;
    return pid;
}

void sigchld_handler(int s)
{
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

void process_req(int conn_fd, char *worker_name, int ac, char *av[])
{

    int rc;
    char input[150];

    int writefd, readfd, errfd;
    pid_t pid;
    if (1) {
        int fdm;
        pid = popen_tty(ac, av, (int *) &fdm);
        writefd = readfd = fdm;
    } else {
        pid = popen_pipes(ac, av,
                          (int *) &writefd, (int *) &readfd, (int *) &errfd);
    }

    struct pollfd poll_fds[2];
    poll_fds[0].fd = readfd;
    poll_fds[0].events = POLLIN;

    poll_fds[1].fd = conn_fd;
    poll_fds[1].events = POLLIN | POLLPRI;

    do {
        int ret = poll((struct pollfd *) &poll_fds, 2, 1000);

        if (ret == -1) {
            perror("poll");
            break;
        } else if (ret == 0) {
            // Poll timeout
            /*fprintf(stderr, "Time out\n"); */
        } else {
            /*fprintf(stderr, "Polled:\n"); */
            /*fprintf(stderr, "  poll_fds[0].revents=%d\n", poll_fds[0].revents); */
            /*fprintf(stderr, "  poll_fds[1].revents=%d\n", poll_fds[1].revents); */

            if (poll_fds[1].revents & POLLIN) {
                poll_fds[1].revents -= POLLIN;
                int n;
                char buf[0x10] = { 0 };
                if ((n = read(poll_fds[1].fd, buf, 0x10)) >= 0) {

                    write(writefd, buf, n);


                    buf[n] = 0;
                    for (int i = 0; i < strlen(buf); i++) {
                        buf[i] = toupper(buf[i]);
                    }
                    fprintf(stderr, "%s> %s", worker_name, buf);
                    continue;
                }
            }
            if (poll_fds[0].revents & POLLIN) {
                poll_fds[0].revents -= POLLIN;
                int n;
                char buf[0x10] = { 0 };
                if ((n = read(poll_fds[0].fd, buf, 0x10)) >= 0) {
                    send(conn_fd, buf, n, 0);

                    buf[n] = 0;
                    for (int i = 0; i < strlen(buf); i++) {
                        buf[i] = toupper(buf[i]);
                    }
                    fprintf(stderr, "%s< %s", worker_name, buf);
                    continue;
                }
            }

            break;
        }
    } while (1);
}

int main(int argc, char *argv[])
{
    char *port_to;
    int rc;
    struct addrinfo hints;
    struct addrinfo *res;
    int sockfd;
    int conn_fd;
    struct sockaddr_storage their_addr;
    struct sigaction sa;
    char s[INET6_ADDRSTRLEN];

    if (argc <= 3) {
        fprintf(stderr, "Usage: %s port program_name [parameters]\n", argv[0]);
        exit(1);
    }

    port_to = argv[1];

    int ac = argc - 1;
    char *av[ac];
    int i;
    av[0] = argv[0];
    for (i = 1; i < ac; i++)
        av[i] = argv[i + 1];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    rc = getaddrinfo(NULL, port_to, &hints, &res);
    if (rc != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
        exit(EXIT_FAILURE);
    }

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (!sockfd) {
        perror("socket");
        exit(1);
    }
    rc = bind(sockfd, res->ai_addr, res->ai_addrlen);
    if (rc != 0) {
        close(sockfd);
        perror("bind");
        exit(1);
    }

    rc = listen(sockfd, 5);
    if (rc != 0) {
        close(sockfd);
        perror("listen");
        exit(1);
    }

    freeaddrinfo(res);

    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    for (i = 0; i < 2; i++) {
        pid_t pid;
        if ((pid = fork()) == -1) {
            exit(1);
        } else if (pid == 0) {
            char worker_name[10];
            sprintf(worker_name, "WRK-%d", i);

            // Accept loop
            do {
                socklen_t addr_size;
                addr_size = sizeof their_addr;

                printf("%s is waiting...\n", worker_name);
                conn_fd =
                    accept(sockfd, (struct sockaddr *) &their_addr, &addr_size);

                inet_ntop(their_addr.ss_family,
                          get_in_addr((struct sockaddr *) &their_addr), s,
                          sizeof s);
                printf("%s accepted a connection from %s %d \n", worker_name, s,
                       conn_fd);

                process_req(conn_fd, worker_name, ac, av);

                close(conn_fd);
            } while (1);
        }
    }
    while (wait(NULL) > 0);
}
