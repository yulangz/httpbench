//
// Created by yulan on 2021/5/5.
//
#include "utils.h"

void unix_error(char *msg) {
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(1);
}


pid_t Fork(void) {
    pid_t pid;

    if ((pid = fork()) < 0)
        unix_error("Fork error");
    return pid;
}


handler_t Signal(int signum, handler_t handler) {
    struct sigaction action, old_action;

    action.sa_handler = handler;
    sigemptyset(&action.sa_mask); /* 只阻塞待处理信号量 */
    action.sa_flags = SA_RESTART; /* 如果可能，允许系统调用重启 */

    if (sigaction(signum, &action, &old_action) < 0)
        unix_error("Signal error");
    return (old_action.sa_handler);
}


ssize_t Read(int fd, void *buf, size_t count) {
    ssize_t rc;

    if ((rc = read(fd, buf, count)) < 0)
        unix_error("Read error");
    return rc;
}


ssize_t Write(int fd, const void *buf, size_t count) {
    ssize_t rc;

    if ((rc = write(fd, buf, count)) < 0)
        unix_error("Write error");
    return rc;
}


void Close(int fd) {
    if (close(fd) < 0)
        unix_error("Close error");
}


void Pipe(int pipefd[2]) {
    if (pipe(pipefd) < 0) {
        unix_error("Create pipe error.");
    }
}


int test_and_get_addrinfo(const char *hostname, const char *port, struct addrinfo *addr) {
    int clientfd, rc;
    struct addrinfo hints, *listp, *p;

    /* Get a list of potential server addresses */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;        /* 指定 IPv4 */
    hints.ai_socktype = SOCK_STREAM;  /* Open a connection */
    hints.ai_flags = AI_NUMERICSERV;  /* using a numeric port arg. */
    hints.ai_flags |= AI_ADDRCONFIG;  /* Recommended for connections */
    if ((rc = getaddrinfo(hostname, port, &hints, &listp)) != 0) {
        fprintf(stderr, "getaddrinfo failed (%s:%s): %s\n", hostname, port, gai_strerror(rc));
        freeaddrinfo(listp);    // 清理掉listp
        return -1;
    }

    /* Walk the list for one that we can successfully connect to */
    for (p = listp; p; p = p->ai_next) {
        /* Create a socket descriptor */
        if ((clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue; /* Socket failed, try the next */

        /* Connect to the server */
        if (connect(clientfd, p->ai_addr, p->ai_addrlen) != -1) {
            memcpy(addr, p, sizeof(struct addrinfo));
            break; /* Success */
        }

        if (close(clientfd) < 0) { /* Connect failed, try another */
            fprintf(stderr, "open_clientfd: close failed: %s\n", strerror(errno));
            freeaddrinfo(listp);    // 清理掉listp
            return -1;
        }
    }

    /* Clean up */
    freeaddrinfo(listp);
    if (!p) /* All connects failed */
        return -1;
    else { /* The last connect succeeded */
        Close(clientfd);
        return 0;
    }
}

void Test_and_get_addrinfo(const char *hostname, const char *port, struct addrinfo *addr) {
    if (test_and_get_addrinfo(hostname, port, addr) < 0) {
        char msg[128];
        sprintf(msg, "Can not connect to %s:%s.", hostname, port);
        unix_error(msg);
    }
}


int open_clientfd_non_blocking_addr(const struct addrinfo *addr) {
    int clientfd;
    if ((clientfd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol)) < 0) {
        unix_error("socket error");
        return -1;
    }

    if (set_non_blocking(clientfd) < 0) {
        close(clientfd);
        return -1;
    }

    if (connect(clientfd, addr->ai_addr, addr->ai_addrlen) != -1 || errno == EINPROGRESS)
        return clientfd;
    else {
        close(clientfd);
        return -1;
    }

}


int set_non_blocking(int sock) {
    int opts;
    opts = fcntl(sock, F_GETFL);
    if (opts < 0) {
        return -1;
    }
    opts = opts | O_NONBLOCK;
    if (fcntl(sock, F_SETFL, opts) < 0) {
        return -1;
    }
    return 0;
}


int Epoll_create() {
    int fd;
    if ((fd = epoll_create(1)) < 0) {
        unix_error("epoll create failed");
        exit(1);
    }
    return fd;
}


void Epoll_ctl(int epfd, int op, int fd, struct epoll_event *event) {
    if (epoll_ctl(epfd, op, fd, event) < 0) {
        unix_error("epoll ctl error");
    }
}


void *Malloc(size_t size) {
    void *p;

    if ((p = malloc(size)) == NULL)
        unix_error("Malloc error");
    return p;
}


void Free(void *ptr) {
    free(ptr);
}