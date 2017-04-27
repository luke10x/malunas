#define _XOPEN_SOURCE 600
#include <stdlib.h>
#include <assert.h>
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
#include <getopt.h>

#define program_name "tcpexecd"

static struct option const longopts[] = {
    {"tty", required_argument, NULL, 't'},
    {"workers", required_argument, NULL, 'w'},
    {"verbose", required_argument, NULL, 'v'},
    {NULL, 0, NULL, 0}
};

void usage(int status)
{
    printf
        ("usage: %s [-v|--verbose] [-w|--workers] <port> [<handler> [<args>]]\n\n",
         program_name);

    fputs("\
Listens for incomming connections on a TCP port. All the data that is\n\
received and sent from an accepted connection socket is mapped to one of the\n\
supported handlers.\n\
\n", stdout);


    fputs("\
OPTIONS:\n\
Mandatory arguments to long options are mandatory for short options too.\n\
  -w, --workers=NUMBER adjust number of preforked workers that accept connections\n\
  -v, --verbose        print more output\n\
\n", stdout);


    fputs("\
HANDLERS:\n\
  exec     maps to input and output of a locally executed command\n\
  proxy    forward socket data to a new TCP connection\n\
\n", stdout);

    printf("Run '%s HANDLER --help' for more information on a handler.\n", program_name);

    exit(status);
}

int exec_cmd(int ac, char *av[])
{
    execvp(av[0], &av[0]);
    fflush(stdout);
    perror(av[0]);
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
        new_term_settings.c_lflag &= ~ECHO;
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

in_port_t *get_in_port(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *) sa)->sin_port);
    }
    return &(((struct sockaddr_in6 *) sa)->sin6_port);
}

void trim_log(char *buf, int n)
{
    int loglen;
    if (n <= 80) {
        loglen = n;
    } else {
        loglen = 80;
    }
    for (int i = 0; i < loglen; i++) {
        if (buf[i] < 32) {
            buf[i] = '.';
        }
    }
    if (n > 80) {
        buf[loglen - 1] = 133;
    }
    buf[loglen] = 0;
}

void process_req(int conn_fd, char *worker_name, int ac, char *av[], int tty,
                 int verbose)
{
    int writefd, readfd, errfd;
    pid_t pid;
    if (tty) {
        int fdm;
        pid = popen_tty(ac, av, (int *) &fdm);
        writefd = readfd = fdm;
    } else {
        pid = popen_pipes(ac, av,
                          (int *) &writefd, (int *) &readfd, (int *) &errfd);
    }

    struct pollfd poll_fds[2];
    struct pollfd *read_pollfd = &poll_fds[0];
    struct pollfd *conn_pollfd = &poll_fds[1];

    read_pollfd->fd = readfd;
    read_pollfd->events = POLLIN;

    conn_pollfd->fd = conn_fd;
    conn_pollfd->events = POLLIN | POLLPRI;

    do {
        int ret = poll((struct pollfd *) &poll_fds, 2, 1000);

        if (ret == -1) {
            perror("poll");
            break;
        } else if (ret == 0) {
            // Poll timeout
        } else {

            if (conn_pollfd->revents & POLLIN) {
                conn_pollfd->revents -= POLLIN;
                int n;
                char buf[0x100] = { 0 };
                int event_confirmed = 0;
                if ((n = read(conn_pollfd->fd, buf, 0x100)) >= 0) {
                    if (event_confirmed == 0 && n <= 0) {
                        // It is a false event, perhaps client closed connection
                        break;
                    }
                    event_confirmed = 1;    // because if there was data, we read it all

                    write(writefd, buf, n);

                    fprintf(stderr, "%s received %d bytes:\n", worker_name, n);

                    if (verbose) {
                        trim_log(buf, n);
                        fprintf(stderr, "%s\n", buf);
                    }
                    continue;
                }
            }
            if (read_pollfd->revents & POLLIN) {
                read_pollfd->revents -= POLLIN;
                int n;
                char buf[0x10] = { 0 };
                if ((n = read(read_pollfd->fd, buf, 0x100)) >= 0) {
                    send(conn_fd, buf, n, 0);

                    fprintf(stderr, "%s sent %d bytes:\n", worker_name, n);

                    if (verbose) {
                        trim_log(buf, n);
                        fprintf(stderr, "%s\n", buf);
                    }
                    continue;
                }
            }

            break;
        }
    } while (1);

    kill(pid, SIGKILL);

    fprintf(stderr, "%s finished processing request\n", worker_name);
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
    int c;
    int workers;
    int tty;
    int verbose;

    tty = 0;
    verbose = 0;
    workers = 2;
    while ((c = getopt_long(argc, argv, "tw:v", longopts, NULL)) != -1) {
        int opt_fileno;

        switch (c) {
        case 't':
            tty = 1;
            break;
        case 'v':
            verbose = 1;
            break;
        case 'w':
            workers = atoi(optarg);
            break;
        case '?':
            if (optopt == 'f') {
                fprintf(stderr, "Option -%c requires an arg.\n", optopt);
            }
        default:
            usage(1);
        }
    }

    int i;
    int ac;

    char **av;
    av = argv + optind;
    ac = argc - optind;

    port_to = av[0];
    av++;
    ac--;

    if (port_to == 0) {
        fprintf(stderr, "Port is not specified.\n");
        usage(-1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    rc = getaddrinfo(NULL, port_to, &hints, &res);
    if (rc != 0) {
        fprintf(stderr, "getaddrinfo for port %s: %s\n", port_to,
                gai_strerror(rc));
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

    rc = getsockname(sockfd, res->ai_addr, &res->ai_addrlen);
    if (rc != 0) {
        fprintf(stderr, "getsockname for port %s: %s\n", port_to,
                gai_strerror(rc));
        exit(EXIT_FAILURE);
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

    struct sockaddr *sa1 = res->ai_addr;
    inet_ntop(sa1->sa_family, get_in_addr(sa1), s, sizeof s);
    fprintf(stderr, "%s: Listening on %s:%d\n",
            program_name, s, ntohs(*get_in_port(sa1)));

    for (i = 0; i < workers; i++) {
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
                printf("%s accepted a connection from %s (socket FD: %d)\n",
                       worker_name, s, conn_fd);

                process_req(conn_fd, worker_name, ac, av, tty, verbose);

                close(conn_fd);
            } while (1);
        }
    }
    while (wait(NULL) > 0);
}
