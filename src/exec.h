#ifndef exec_h__

#define exec_h__

void process_req(int conn_fd, char *worker_name, int ac, char *av[], int tty,
                 int verbose);

#endif

