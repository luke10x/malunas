#ifndef event_h__

#define event_h__

#define EVT_WORKER_READY 1
#define EVT_CONN_ACCEPTED 2
#define EVT_REQUEST_READ 3
#define EVT_RESPONSE_SENT 4
#define EVT_REQUEST_ENDED 5
#define EVT_DEBUG_OPENED 6

struct evt_worker_ready {
    int worker_id;
};

struct evt_conn_accepted {
    int worker_id;
    long request_id;
    int fd;
    struct sockaddr sockaddr;
};

struct evt_request_read {
    int worker_id;
    long request_id;
    int bytes;
};

struct evt_response_sent {
    int worker_id;
    long request_id;
    int bytes;
};

struct evt_request_ended {
    int worker_id;
    long request_id;
};

struct evt_debug_opened {
    int worker_id;
    long request_id;
    int fd;
    char path[108];
};

struct evt_base {
    long mtype;
    int etype;
    union {
        struct evt_worker_ready worker_ready; 
        struct evt_conn_accepted conn_accepted;
        struct evt_request_read request_read;
        struct evt_response_sent response_sent;
        struct evt_request_ended request_ended;
        struct evt_debug_opened debug_opened;
    } edata;
};

#endif
