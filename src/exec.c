#define _XOPEN_SOURCE 700
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <getopt.h>

static struct option const longopts[] = {
    {"tty", required_argument, NULL, 't'},
};

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

void mlns_exec_handle(int conn_fd, int logfd, char *worker_name, int ac,
                      char *av[], int tty, int verbose)
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

                    dprintf(logfd, "%s received %d bytes:\n", worker_name, n);

                    if (verbose) {
                        trim_log(buf, n);
                        dprintf(logfd, "%s\n", buf);
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

                    dprintf(logfd, "%s sent %d bytes:\n", worker_name, n);

                    if (verbose) {
                        trim_log(buf, n);
                        dprintf(logfd, "%s\n", buf);
                    }
                    continue;
                }
            }

            break;
        }
    } while (1);

    kill(pid, SIGKILL);

    dprintf(logfd, "%s finished processing request\n", worker_name);
}
