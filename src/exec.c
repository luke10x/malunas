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


static struct option const exec_longopts[] = {
    {"tty", no_argument, NULL, 't'},
    {NULL, 0, NULL, 0}
};


void exec_usage(int status)
{
    fputs("\
HELLO\n\
\n", stdout);
    exit(1);
}

extern int pass_traffic(int front_read, int front_write, int back_read,
                        int back_write);

void mlns_exec_handle(int conn_fd, int logfd, int argc, char *argv[])
{
    int c;
    int tty = 0;

    opterr = 0;
    optind = 1;
    while ((c = getopt_long(argc, argv, "+t", exec_longopts, NULL)) != -1) {
        switch (c) {
        case 't':
            tty = 1;
            break;
        default:
            exec_usage(1);
        }
    }

    int i;
    int ac;

    char **av;
    av = argv + optind;
    ac = argc - optind;

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

    /* Read-end of backend is a write-end of the process */
    pass_traffic(conn_fd, conn_fd, readfd, writefd);

    close(conn_fd);

    close(writefd);
    close(readfd);
    close(errfd);

    kill(pid, SIGKILL);
}
