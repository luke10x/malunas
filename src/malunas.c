#define _XOPEN_SOURCE 700
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <getopt.h>
#include <poll.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/un.h>
/*#include <openssl/applink.c>*/
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "exec.h"
#include "proxy.h"
#include "event.h"

#define program_name "malunas"

static struct option const longopts[] = {
    {"workers", required_argument, NULL, 'w'},
    {NULL, 0, NULL, 0}
};

typedef struct {
    char *name;
    void (*getends_func) (int, char **, int *, int *);
} t_modulecfg;

t_modulecfg modules[] = {
    {"exec", mlns_exec_getends},
    {"proxy", mlns_proxy_getends}
};

void usage(int status)
{
    printf
        ("usage: %s [-w|--workers] <port> [<handler> [<args>]]\n\n",
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
\n", stdout);


    fputs("\
HANDLERS:\n\
  exec     maps to input and output of a locally executed command\n\
  proxy    forward socket data to a new TCP connection\n\
\n", stdout);

    printf("Run '%s HANDLER --help' for more information on a handler.\n",
           program_name);

    exit(status);
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

/* Will be set for each process while forking */
int worker_id;

/* IPC */
int msqid;

/* Will be increased for each request */
long unsigned int request_id;

/*
 * Logs will be printed in thes format:
 *
 * Status --------------------------------------------+
 * Bytes out -----------------------------------+     |
 * Bytes in -------------------------------+    |     |
 * Port --------------------------------+  |    |     |
 * IP -------------------------------+  |  |    |     |
 * Request number ---------------+   |  |  |    |     |
 * Process name -------------+   |   |  |  |    |     |
 * Icon -----------------+   |   |   |  |  |    |     |
 *                       |   |   |   |  |  |    |     |   */
const char *LOG_FMT = "[%c] %s #%lu %s:%d %luB/%luB [%s]\n";

int debugfd = 0;

char debugpath[108];

void dbg_signal_handler(int sig)
{
    unsigned int debuglistenfd, s2;
    struct sockaddr_un serveraddr, clientaddr;
    int len;

    debuglistenfd = socket(AF_UNIX, SOCK_STREAM, 0);

    serveraddr.sun_family = AF_UNIX;
    sprintf(serveraddr.sun_path, "dbg-%d.sock", getpid());
    unlink(serveraddr.sun_path);
    len = strlen(serveraddr.sun_path) + sizeof(serveraddr.sun_family);
    bind(debuglistenfd, (struct sockaddr *) &serveraddr, len);
    listen(debuglistenfd, 1);

    len = sizeof(struct sockaddr_un);

    struct evt_base evt;
    evt.mtype = 1;
    evt.etype = EVT_DEBUG_OPENED;
    evt.edata.debug_opened.worker_id = worker_id;
    evt.edata.debug_opened.request_id = request_id;
    strcpy(evt.edata.debug_opened.path, (char *) &serveraddr.sun_path);
    int evt_size =
        sizeof evt.mtype + sizeof evt.etype + sizeof evt.edata.debug_opened;
    if (msgsnd(msqid, &evt, evt_size, 0) == -1) {
        perror("msgsnd");
    }

    debugfd = accept(debuglistenfd, (struct sockaddr *) &clientaddr, &len);
}

/**
 * Bridges backend and frontend
 *
 * It is supposed to be used from within handling function of the modules
 * Poll event constants are defined here:
 * include/uapi/asm-generic/poll.h
 */
int pass_traffic(int front_read, int front_write, int back_read, int back_write)
{
    struct pollfd poll_fds[2];
    struct pollfd *fr_pollfd = &poll_fds[0];
    struct pollfd *br_pollfd = &poll_fds[1];

    fr_pollfd->fd = front_read;
    fr_pollfd->events = POLLIN;

    br_pollfd->fd = back_read;
    br_pollfd->events = POLLIN;

    do {
        int ret = poll((struct pollfd *) &poll_fds, 2, 4000);
        if (ret == -1) {
            if (debugfd != 0) {
                continue;
            }
            perror("poll");
            break;
            /* SIGUSR1 causes poll() fail with -1.
             * On real life load it is likely that signal is handled
             * during another system call and that should be addressed.
             */
        } else if (ret == 0) {
            /*Poll timeout */
        } else {
            if (br_pollfd->revents & POLLIN) {
                char buf[0x10 + 1] = { 0 };
                int n = read(br_pollfd->fd, buf, 0x10);

                if (n > 0) {
                    buf[n] = 0;
                    send(front_write, buf, n, 0);

                    struct evt_base evt;
                    evt.mtype = 1;
                    evt.etype = EVT_RESPONSE_SENT;
                    evt.edata.response_sent.worker_id = worker_id;
                    evt.edata.response_sent.request_id = request_id;
                    evt.edata.response_sent.bytes = n;
                    int evt_size =
                        sizeof evt.mtype + sizeof evt.etype +
                        sizeof evt.edata.response_sent;
                    if (msgsnd(msqid, &evt, evt_size, 0) == -1) {
                        perror("msgsnd");
                    }
                } else
                    break;
            }

            if (fr_pollfd->revents & POLLIN) {
                char buf[0x10 + 1] = { 0 };
                int n = read(fr_pollfd->fd, buf, 0x10);

                if (n > 0) {
                    buf[n] = 0;
                    write(back_write, buf, n);

                    struct evt_base evt;
                    evt.mtype = 1;
                    evt.etype = EVT_REQUEST_READ;
                    evt.edata.request_read.worker_id = worker_id;
                    evt.edata.request_read.request_id = request_id;
                    evt.edata.request_read.bytes = n;
                    int evt_size =
                        sizeof evt.mtype + sizeof evt.etype +
                        sizeof evt.edata.request_read;
                    if (msgsnd(msqid, &evt, evt_size, 0) == -1) {
                        perror("msgsnd");
                    }
                } else
                    break;
            }

            if (br_pollfd->revents & POLLHUP) {
                break;
            }
            if (fr_pollfd->revents & POLLHUP) {
                break;
            }
        }
    } while (1);

    struct evt_base evt;
    evt.mtype = 1;
    evt.etype = EVT_REQUEST_ENDED;
    evt.edata.request_ended.worker_id = worker_id;
    evt.edata.request_ended.request_id = request_id;
    int evt_size =
        sizeof evt.mtype + sizeof evt.etype + sizeof evt.edata.request_ended;
    if (msgsnd(msqid, &evt, evt_size, 0) == -1) {
        perror("msgsnd");
    }
}

void InitializeSSL()
{
    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();
}

void DestroySSL()
{
    ERR_free_strings();
    EVP_cleanup();
}

/*void ShutdownSSL()*/
/*{*/
    /*SSL_shutdown(ssl); */
    /*SSL_free(ssl); */
/*}*/

int accept_wrapper(int sockfd, struct sockaddr *addr, socklen_t * addrlen)
{
    return accept(sockfd, addr, addrlen);

#if 0
    /* From: https://stackoverflow.com/questions/7698488/turn-a-simple-socket-into-an-ssl-socket
     */
    int newsockfd;
    SSL_CTX *sslctx;
    SSL *cSSL;

    InitializeSSL();

    struct sockaddr_in saiServerAddress;
    bzero((char *) &saiServerAddress, sizeof(saiServerAddress));
    saiServerAddress.sin_family = AF_INET;
    saiServerAddress.sin_addr.s_addr = addr;
    saiServerAddress.sin_port = htons(443);

    newsockfd = accept(sockfd, addr, addrlen);

    sslctx = SSL_CTX_new(SSLv23_server_method());
    SSL_CTX_set_options(sslctx, SSL_OP_SINGLE_DH_USE);
    int use_cert =
        SSL_CTX_use_certificate_file(sslctx, "/serverCertificate.pem",
                                     SSL_FILETYPE_PEM);

    int use_prv =
        SSL_CTX_use_PrivateKey_file(sslctx, "/serverCertificate.pem",
                                    SSL_FILETYPE_PEM);

    cSSL = SSL_new(sslctx);
    SSL_set_fd(cSSL, newsockfd);
    //Here is the SSL Accept portion.  Now all reads and writes must use SSL
    int ssl_err = SSL_accept(cSSL);
    if (ssl_err <= 0) {
        //Error occurred, log and close down ssl
        ShutdownSSL();
    }
#endif
}

/**
 * Accepts and handles one request
 */
int handle_request(int log, int listen_fd, t_modulecfg * module, int ac,
                   char **av)
{
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    addr_size = sizeof their_addr;

    dprintf(log, "waiting for a connection...");

    struct evt_base evt;
    evt.mtype = 1;
    evt.etype = EVT_WORKER_READY;
    evt.edata.worker_ready.worker_id = worker_id;
    size_t evt_size =
        sizeof evt.mtype + sizeof evt.etype + sizeof evt.edata.worker_ready;
    if (msgsnd(msqid, &evt, evt_size, 0) == -1) {
        perror("msgsnd");
    }

    int conn_fd;
    conn_fd =
        accept_wrapper(listen_fd, (struct sockaddr *) &their_addr, &addr_size);

    char s[INET6_ADDRSTRLEN];
    inet_ntop(their_addr.ss_family,
              get_in_addr((struct sockaddr *) &their_addr), s, sizeof s);

    dprintf(log, "accepted connection from %s (socket FD: %d)", s, conn_fd);

    evt.mtype = 1;
    evt.etype = EVT_CONN_ACCEPTED;
    evt.edata.conn_accepted.worker_id = worker_id;
    evt.edata.conn_accepted.request_id = request_id;
    evt.edata.conn_accepted.sockaddr = *(struct sockaddr *) &their_addr;
    evt.edata.conn_accepted.fd = conn_fd;
    evt_size =
        sizeof evt.mtype + sizeof evt.etype + sizeof evt.edata.conn_accepted;
    if (msgsnd(msqid, &evt, evt_size, 0) == -1) {
        perror("msgsnd");
    }

    int writefd;
    int readfd;
    module->getends_func(ac, av, &readfd, &writefd);
    pass_traffic(conn_fd, conn_fd, readfd, writefd);

    close(conn_fd);
}

struct request_state {
    unsigned long in;
    unsigned long out;
    int fd;
    struct sockaddr client_addr;
    int status;
    unsigned long request_id;
    int debugfd;
    int debugflag;
    char debugpath[108];
};

int del_reqstates(int reqstates_printed)
{
    int i;
    for (i = 0; i < reqstates_printed; i++) {
        dprintf(2, "\033[A");
        dprintf(2, "\033[2K");
    }
}

int print_reqstates(struct request_state *reqstates, int worker_count,
                    char **worker_names)
{
    int printed = 0;
    int i;
    for (i = 0; i < worker_count; i++) {

        if (reqstates[i].status == 1) {
            struct sockaddr client_addr = reqstates[i].client_addr;
            char s[INET6_ADDRSTRLEN];
            inet_ntop(client_addr.sa_family, get_in_addr(&client_addr), s,
                      sizeof s);
            char status[128];
            if (reqstates[i].debugflag == 0) {
                strcpy(status, "CONNECTED...");
            } else {
                /*sprintf(worker_name, "PID:%d", pid); */
                sprintf((char *) &status, "DEBUG WAITING ON %s",
                        (char *) &(reqstates[i].debugpath));
                /*strcpy(status, reqstates[i].debugpath); */
            }

            dprintf(2, LOG_FMT, '+',
                    worker_names[i],
                    reqstates[i].request_id, s,
                    ntohs(*get_in_port(&client_addr)),
                    reqstates[i].in, reqstates[i].out, status);
            printed++;
        }
    }
    return printed;
}

int main(int argc, char *argv[])
{
    char *port_to;
    int rc;
    struct addrinfo hints;
    struct addrinfo *res;
    int sockfd;
    struct sigaction sa;
    int c;
    int workers;
    int tty;

    tty = 0;
    workers = 2;
    while ((c = getopt_long(argc, argv, "+w:", longopts, NULL)) != -1) {
        switch (c) {
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

    char *handler = av[0];

    t_modulecfg *module = &modules[0];
    int len = sizeof(modules) / sizeof(modules[0]);

    while (strcmp(handler, module->name) != 0) {
        // is it last module?
        if (module == &modules[len - 1]) {
            fprintf(stderr, "Module '%s' is not a valid module\n", handler);
            exit(1);
        }
        module++;
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
    char s[INET6_ADDRSTRLEN];
    inet_ntop(sa1->sa_family, get_in_addr(sa1), s, sizeof s);
    fprintf(stderr, "%s: Listening on %s:%d\n",
            program_name, s, ntohs(*get_in_port(sa1)));

    int logs_in[workers][2];
    char *worker_names[workers];

    struct pollfd poll_fds_in[workers];

    key_t key;

    if ((key = ftok("malunas", 'a')) == -1) {
        perror("ftok");
        exit(1);
    }

    if ((msqid = msgget(key, 0664 | IPC_CREAT)) == -1) {
        perror("msgget");
        exit(1);
    }

    for (i = 0; i < workers; i++) {

        pid_t pid;
        pipe(logs_in[i]);

        poll_fds_in[i].fd = logs_in[i][0];
        poll_fds_in[i].events = POLLIN;

        /* Set global before forking */
        worker_id = i;
        if ((pid = fork()) == -1) {
            exit(1);
        } else if (pid == 0) {

            close(logs_in[i][0]);
            int log = logs_in[i][1];

            /* each worker will count from beginning */
            request_id = 0;

            sa.sa_handler = dbg_signal_handler;
            sigemptyset(&sa.sa_mask);
            sigaddset(&sa.sa_mask, SIGUSR1);
            sa.sa_flags = SA_RESTART;
            if (sigaction(SIGUSR1, &sa, NULL) == -1) {
                perror("sigaction sigusr1");
                exit(1);
            }
            // Accept loop
            while (1) {
                request_id++;
                handle_request(log, sockfd, module, ac, av);
            }
        }

        close(logs_in[i][1]);

        char *worker_name = malloc(10);
        sprintf(worker_name, "PID:%d", pid);
        worker_names[i] = worker_name;
        worker_names[i][9] = 0;
    }

    if ((msqid = msgget(key, 0644)) == -1) {
        perror("msgget");
        exit(1);
    }

    struct request_state reqstates[workers];
    int active_rqs = 0;

    int msgsize;
    int reqstates_printed = 0;
    for (;;) {
        struct evt_base msg;
        if ((msgsize = msgrcv(msqid, &msg, 512, 0, 0)) == -1) {
            perror("msgrcv");
            exit(1);
        }

        switch (msg.etype) {
        case EVT_WORKER_READY:
            reqstates[msg.edata.worker_ready.worker_id].in = 0;
            reqstates[msg.edata.worker_ready.worker_id].out = 0;
            reqstates[msg.edata.worker_ready.worker_id].debugflag = 0;
            break;
        case EVT_CONN_ACCEPTED:
            reqstates[msg.edata.conn_accepted.worker_id].status = 1;
            reqstates[msg.edata.conn_accepted.worker_id].request_id =
                msg.edata.conn_accepted.request_id;
            reqstates[msg.edata.conn_accepted.worker_id].client_addr =
                msg.edata.conn_accepted.sockaddr;
            reqstates[msg.edata.conn_accepted.worker_id].fd =
                msg.edata.conn_accepted.fd;
            break;

        case EVT_REQUEST_READ:
            reqstates[msg.edata.request_read.worker_id].in +=
                msg.edata.request_read.bytes;

            del_reqstates(reqstates_printed);
            reqstates_printed =
                print_reqstates((struct request_state *) &reqstates, workers,
                                (char **) &worker_names);
            break;
        case EVT_RESPONSE_SENT:
            reqstates[msg.edata.response_sent.worker_id].out +=
                msg.edata.response_sent.bytes;

            del_reqstates(reqstates_printed);
            reqstates_printed =
                print_reqstates((struct request_state *) &reqstates, workers,
                                (char **) &worker_names);
            break;
        case EVT_REQUEST_ENDED:
            reqstates[msg.edata.conn_accepted.worker_id].status = 0;

            struct sockaddr client_addr =
                reqstates[msg.edata.request_ended.worker_id].client_addr;
            inet_ntop(client_addr.sa_family, get_in_addr(&client_addr), s,
                      sizeof s);

            del_reqstates(reqstates_printed);
            dprintf(2, LOG_FMT, '-',
                    worker_names[msg.edata.request_ended.worker_id],
                    msg.edata.request_ended.request_id, s,
                    ntohs(*get_in_port(&client_addr)),
                    reqstates[msg.edata.request_ended.worker_id].in,
                    reqstates[msg.edata.request_ended.worker_id].out, "DONE");

            reqstates_printed =
                print_reqstates((struct request_state *) &reqstates, workers,
                                (char **) &worker_names);
            break;
        case EVT_DEBUG_OPENED:
            strcpy(reqstates[msg.edata.debug_opened.worker_id].debugpath,
                   msg.edata.debug_opened.path);
            reqstates[msg.edata.debug_opened.worker_id].debugflag = 1;

            del_reqstates(reqstates_printed);
            reqstates_printed =
                print_reqstates((struct request_state *) &reqstates, workers,
                                (char **) &worker_names);
            break;
        default:
            printf("Unknown event!!!\n");
        }
    }

    do {
        int ret = poll((struct pollfd *) &poll_fds_in, workers, 1000);
        if (ret == -1) {
            perror("poll");
            break;
        } else if (ret == 0) {
            // Poll timeout
        } else {

            for (i = 0; i < workers; i++) {
                if (poll_fds_in[i].revents & POLLIN) {
                    poll_fds_in[i].revents -= POLLIN;

                    int n;
                    char buf[0x100 + 1] = { 0 };
                    n = read(poll_fds_in[i].fd, buf, 0x100);
                    buf[n] = 0;
                    dprintf(2, "%s >>> %s\n", worker_names[i], buf);
                }
            }
        }
    } while (1);

    while (wait(NULL) > 0);
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
