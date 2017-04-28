#ifndef exec_h__

#define exec_h__

void mlns_exec_handle(int conn_fd, int logfd, char *worker_name, int ac, char *av[], int tty,
                 int verbose);

#endif

