//
// Created by yulan on 2021/5/5.
//

#ifndef HTTPBENCH_UTILS_H
#define HTTPBENCH_UTILS_H

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <sys/epoll.h>
#include <fcntl.h>

#define MAX_HOST_LEN        64
#define MAX_PATH_LEN        1500
#define MAX_URL_LEN         1564
#define MAX_REQUEST_LEN     8192

#define METHOD_GET          0
#define METHOD_HEAD         1
#define METHOD_OPTIONS      2
#define METHOD_TRACE        3

#define HTTP_VERSION10      10
#define HTTP_VERSION11      11
#define HTTP_VERSION20      20

#define MAX_HERDER_PARAM_LEN 1024
#define MAX_HEADER_PARAM_NUM 20

#define STAT_NOT_CONNECTED      0
#define STAT_SOCK_CREATED       1
#define STAT_DATA_SENT          2
struct ConnectionArgument {
    int fd;
    int status;
    ssize_t read_bytes;
};

void unix_error(char *msg);

pid_t Fork(void);

typedef void (*handler_t)(int);

handler_t Signal(int signum, handler_t handler);

ssize_t Read(int fd, void *buf, size_t count);

ssize_t Write(int fd, const void *buf, size_t count);

void Close(int fd);

void Pipe(int pipefd[2]);

int set_non_blocking(int sock);

/**
 * Test whether the server can connect, and return the server addrinfo struct. If the server
 * can not connect, the process will exit and print the error message.
 * @param hostname  hostname of the server.
 * @param port      port number, must be str.
 * @param addr      return the server addrinfo struct if server can connect.
 */
void Test_and_get_addrinfo(const char *hostname, const char *port, struct addrinfo *addr);

/**
 * Open a client fd by the server addrinfo struct.
 * @param addr  addrinfo struct of the server.
 * @return      0 on success, -1 on failed.
 */
int open_clientfd_non_blocking_addr(const struct addrinfo *addr);

int Epoll_create();

void Epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);

void *Malloc(size_t size);

void Free(void *ptr);

/**
 * parse url
 * @param url   input param,thr url need to be parse
 * @param host  output param,the host
 * @param port  output param,the port
 * @param path  output param,the request path
 * @return  0 on success, -1 on failed and print the error to the stderr
 */
int parse_url(const char *url, char *host, int *port, char *path);

/**
 * build request header
 * @param request_header    output param，return the request header
 * @param host
 * @param port
 * @param path
 * @param method
 * @param http_version
 * @param params            additional request header parameters, must be end with an empty string
 * @return  0 on success, -1 on failed and print the error to the stderr
 */
int bulid_header(char *request_header, const char *host, int port,
                 const char *path, int method, int http_version,
                 char params[][MAX_HERDER_PARAM_LEN]);


/**
 * circular queue struct
 */
typedef struct ConnectionArgument *QUEUE_ELEMENT_TYPE;
struct CircularQueue {
    QUEUE_ELEMENT_TYPE *queue;
    size_t front;
    size_t back;
    size_t size;
};

int CQ_in_queue(struct CircularQueue *q, QUEUE_ELEMENT_TYPE v);

int CQ_pop_queue(struct CircularQueue *q, QUEUE_ELEMENT_TYPE *out);

int CQ_empty(struct CircularQueue *q);

size_t CQ_get_size(struct CircularQueue *q);

int CQ_init(struct CircularQueue *q, size_t size);

void CQ_destroy(struct CircularQueue *q);

#endif //HTTPBENCH_UTILS_H
