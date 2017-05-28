#ifndef event_h__

#define event_h__

struct my_msgbuf {
    long mtype;
    char mtext[15];
};

#define EVT_WORKER_READY 1
#define EVT_CONN_ACCEPTED 2

struct evt_worker_ready {
    int worker_id;
};

struct evt_conn_accepted {
    int worker_id;
    int fd;
    struct sockaddr sockaddr;
};

struct evt_base {
    long mtype;
    int etype;
    union {
        struct evt_worker_ready worker_ready; 
        struct evt_conn_accepted conn_accepted;
    } edata;
};

#endif

