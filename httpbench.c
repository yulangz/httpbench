#include "utils.h"

#define PROGRAM_VERSION "HTTPBench v1.0"


static void usage() {
    printf("httpbench [option]... URL\n"
           "  -t|--time <sec>          Run benchmark for <sec> seconds. Default 30.\n"
           "  -p|--process <n>         Run benchmark with n process. Default 1."
           "  -c|--concurrency <n>     Run <n> concurrent connection in each process. Default 1.\n"
           "  -r|--reload              Send reload request -- Pragma:no-cache and Cache-Control:no-cache.\n"
           "  -P|--proxy <server:port>    Use proxy server for request.\n"
           "  --http10                 Use HTTP/1.0 protocol.\n"
           "  --http11                 Use HTTP/1.1 protocol. Default ust HTTP/1.1 .\n"
           "  --http20                 Use HTTP/2.0 protocol.\n"
           "  --get                    Use GET request method. By default, the method is GET.\n"
           "  --head                   Use HEAD request method.\n"
           "  --options                Use OPTIONS request method.\n"
           "  --trace                  Use TRACE request method.\n"
           "  -?|-h|--help             Print this message.\n"
           "  -V|--version             Print program version.\n");
}

static int method = METHOD_GET;
static int http_version = HTTP_VERSION11;
static const char *proxy_host = NULL;
static int proxy_port;
static int force_reload = 0;
static int concurrency = 1;
static int process = 1;
static int bench_time = 30;

static int succeed = 0;
static int failed = 0;
static int bytes = 0;

static int timeout = 0;

static void version() {
    printf(PROGRAM_VERSION "\n");
}

static void copy_right() {
    version();
    printf("Copyright (c) yulangz 2021, MIT License.\n");
}

static void sig_alarm_handler(int a) {
    timeout = 1;
}

static int bench(const char *host, int port, const char *request);

static void bench_core(const struct addrinfo *addr, const char *request);

// TODO 加入允许自定义头
static struct option options[] = {
        {"time",        required_argument, NULL,    't'},
        {"process",     required_argument, NULL,    'p'},
        {"concurrency", required_argument, NULL,    'c'},
        {"reload",      no_argument, &force_reload, 1},
        {"proxy",       required_argument, NULL,    'P'},
        {"http10",      no_argument, &http_version, HTTP_VERSION10},
        {"http11",      no_argument, &http_version, HTTP_VERSION11},
        {"http20",      no_argument, &http_version, HTTP_VERSION20},
        {"get",         no_argument, &method,       METHOD_GET},
        {"head",        no_argument, &method,       METHOD_HEAD},
        {"options",     no_argument, &method,       METHOD_OPTIONS},
        {"trace",       no_argument, &method,       METHOD_TRACE},
        {NULL, 0,                          NULL,    0}
};

int main(int argc, char **argv) {
    int opt;
    int options_index = 0;
    char *tmp = NULL;

    if (argc == 1) {
        usage();
        return 1;
    }

    while ((opt = getopt_long(argc, argv, "t:p:c:rP:Vh?", options, &options_index)) != EOF) {
        switch (opt) {
            case 0:
                break;
            case 't':
                bench_time = atoi(optarg);
                break;
            case 'p':
                process = atoi(optarg);
                break;
            case 'c':
                concurrency = atoi(optarg);
                break;
            case 'r':
                force_reload = 1;
                break;
            case 'P':
                /* 从server:port解析proxy host和proxy port */
                tmp = strrchr(optarg, ':');
                proxy_host = optarg;
                if (tmp == NULL) {
                    break;
                }
                if (tmp == optarg) {
                    fprintf(stderr, "Error in option --proxy %s: Missing hostname.\n", optarg);
                    return 1;
                }
                if (tmp == optarg + strlen(optarg) - 1) {
                    fprintf(stderr, "Error in option --proxy %s Port number is missing.\n", optarg);
                    return 1;
                }
                *tmp = '\0'; // 截断 proxy host 字符串
                proxy_port = atoi(tmp + 1);
                break;
            case 'V':
                version();
                break;
            case ':':
            case 'h':
            case '?':
            default:
                usage();
                return 1;
        }
    }

    if (optind == argc) {
        fprintf(stderr, "HTTPBench: Missing URL!\n");
        usage();
        return 1;
    }

    copy_right();

    char host[MAX_HOST_LEN];
    char path[MAX_PATH_LEN];
    char request[MAX_REQUEST_LEN];
    int port;
    if (parse_url(argv[optind], host, &port, path) < 0)
        return 1;
    char header_params[MAX_HEADER_PARAM_NUM + 1][MAX_HERDER_PARAM_LEN];
    int ind = 0;
    sprintf(header_params[ind++], "User-Agent: " PROGRAM_VERSION);
    sprintf(header_params[ind++], "Connection: close");
    if (force_reload) {
        sprintf(header_params[ind++], "Pragma:no-cache");
        sprintf(header_params[ind++], "Cache-Control:no-cache");
    }

    header_params[ind][0] = 0;
    if (bulid_header(request, host, port, path, method, http_version, header_params) < 0)
        return 1;
    if (proxy_host == NULL)
        return bench(host, port, request);
    else
        return bench(proxy_host, proxy_port, request);
}

static int bench(const char *host, int port, const char *request) {

    printf("\nBenchmark start, the request header is:\n%s", request);
    printf("The request will be sent to: %s:%d\n", host, port);
    printf("Use ");
    if (process == 1)
        printf("1 process");
    else
        printf("%d processes", process);

    if (concurrency == 1)
        printf(", 1 connection in each process");
    else
        printf(", %d concurrent connections in each process", concurrency);
    printf(", running %d sec", bench_time);
    printf(".\n");

    char port_str[8];
    sprintf(port_str, "%d", port);

    // 测试连通性，并获得addrinfo
    struct addrinfo addr;
    Test_and_get_addrinfo(host, port_str, &addr);

    // 创建管道
    int pipefd[2];
    Pipe(pipefd);

    // 创建子进程
    pid_t pid = -1;
    for (int i = 0; i < process; ++i) {
        pid = Fork();
        if (pid == 0) {
            // 子进程
            sleep(1);   // 让父进程先行fork
            break;
        }
    }

    if (pid == 0) {
        // 子进程
        Close(pipefd[0]);   // 关闭读端口

        bench_core(&addr, request);

        char tmp[512];
        sprintf(tmp, "%d %d %d\n", succeed, failed, bytes);
        Write(pipefd[1], tmp, strlen(tmp));   // 原子性有管道保证
        Close(pipefd[1]);
        return 0;
    }
    else {
        // 父进程
        Close(pipefd[1]);   //  关闭写端口
        char buf[512];
        while (process--) {
            // 收到一次信息就代表一个子进程结束
            // 为了减轻负担，把儿子丢了，让 init 进程去回收
            Read(pipefd[0], buf, 512);
            int suc, fai, byt;
            sscanf(buf, "%d %d %d", &suc, &fai, &byt);
            succeed += suc;
            failed += fai;
            bytes += byt;
        }
        printf("\n"
               "Requests: %d succeed, %d failed, totally received %d Bytes.\n"
               "Speed: %.2f query/sec succeed, %.2f query/sec in total, %.2f Bytes/sec received.\n",
               succeed, failed, bytes,
               ((double) succeed / bench_time), (((double) succeed + failed) / bench_time),
               ((double) bytes / bench_time)
        );

    }

    return 0;
}

// TODO 有bug没找到，在单个进程中并发到3000时，会报double free的错误
static void bench_core(const struct addrinfo *addr, const char *request) {
    int req_len = strlen(request);
    char buff[40960];
    int i;
    struct epoll_event tmp_event, list_event[concurrency];
    int err;
    socklen_t err_len = sizeof(err);

    // 建立epoll和参数表
    int ep_fd = Epoll_create();
    struct ConnectionArgument *connection_list =
            (struct ConnectionArgument *) Malloc(concurrency * sizeof(struct ConnectionArgument));

    // 初始化用于存放未建立连接的connection_list 索引
    struct CircularQueue queue;
    CQ_init(&queue, concurrency);

    Signal(SIGALRM, sig_alarm_handler);
    alarm(bench_time);

    // 先创建concurrency 个连接
    for (i = 0; i < concurrency; ++i) {
        int client_fd;

        client_fd = open_clientfd_non_blocking_addr(addr);

        if (client_fd < 0) {
            // 建立连接失败
            ++failed;
            connection_list[i].status = STAT_NOT_CONNECTED;
            CQ_in_queue(&queue, connection_list + i);   // 放入队列等待重新建立连接
        }
        else {
            // 套接字申请成功
            connection_list[i].fd = client_fd;
            connection_list[i].status = STAT_SOCK_CREATED;
            connection_list[i].read_bytes = 0;
            tmp_event.data.ptr = connection_list + i;
            tmp_event.events = EPOLLOUT | EPOLLET;
            Epoll_ctl(ep_fd, EPOLL_CTL_ADD, client_fd, &tmp_event);
        }
    }

    while (!timeout) {
        struct ConnectionArgument *ind;
        while (CQ_pop_queue(&queue, &ind) == 0) {
            // 有未建立连接的位置，此时并发数小于 concurrency
            int client_fd;

            client_fd = open_clientfd_non_blocking_addr(addr);

            if (client_fd < 0) {
                // 建立连接失败
                ++failed;
                ind->status = STAT_NOT_CONNECTED;
                CQ_in_queue(&queue, ind);
            }
            else {
                // 套接字申请成功
                ind->fd = client_fd;
                ind->status = STAT_SOCK_CREATED;
                ind->read_bytes = 0;
                tmp_event.data.ptr = ind;
                tmp_event.events = EPOLLOUT | EPOLLET;
                Epoll_ctl(ep_fd, EPOLL_CTL_ADD, client_fd, &tmp_event);
            }
        }

        int num_events;
        if ((num_events = epoll_wait(ep_fd, list_event, concurrency, -1)) < 0) {
            if (errno == EINTR) {
                // epoll_wait 被alarm 中断
                break;
            }
            else {
                unix_error("epoll wait error.");
            }
        }

        for (i = 0; i < num_events; ++i) {
            struct ConnectionArgument *conn_arg =
                    (struct ConnectionArgument *) list_event[i].data.ptr;

            if (conn_arg->status == STAT_SOCK_CREATED) {
                // 新创建的连接

                if (getsockopt(conn_arg->fd, SOL_SOCKET, SO_ERROR, &err, &err_len) < 0) {
                    unix_error("Inner logical error.");
                }

                if (err == 0) {
                    // 连接建立成功
                    // 写数据，改状态
                    int code = write(conn_arg->fd, request, req_len);
                    if (code != req_len) {
                        // 写失败
                        ++failed;
                        close(conn_arg->fd);
                        // Epoll_ctl(ep_fd, EPOLL_CTL_DEL, conn_arg->fd, NULL);
                        // close 时会自动删除
                        conn_arg->status = STAT_NOT_CONNECTED;
                        CQ_in_queue(&queue, conn_arg);
                    }
                    else {
                        // 写成功
                        conn_arg->status = STAT_DATA_SENT;
                        tmp_event.events = EPOLLIN | EPOLLET;
                        tmp_event.data.ptr = conn_arg;
                        Epoll_ctl(ep_fd, EPOLL_CTL_MOD, conn_arg->fd, &tmp_event);
                    }
                }
                else {
                    // 连接建立失败
                    // ++failed，创建新的连接
                    ++failed;
                    close(conn_arg->fd);
                    // Epoll_ctl(ep_fd, EPOLL_CTL_DEL, conn_arg->fd, NULL);
                    conn_arg->status = STAT_NOT_CONNECTED;
                    CQ_in_queue(&queue, conn_arg);
                }
            }

            else {
                // 已经建立好的连接

                if (list_event[i].events & EPOLLERR) {
                    // 出错
                    // ++failed，创建新的连接
                    ++failed;
                    conn_arg->status = STAT_NOT_CONNECTED;
                    close(conn_arg->fd);
                    // Epoll_ctl(ep_fd, EPOLL_CTL_DEL, conn_arg->fd, NULL);
                    CQ_in_queue(&queue, conn_arg);
                }

                else {
                    if (list_event[i].events & EPOLLIN) {
                        // 可读
                        // 读数据，++bytes
                        ssize_t revc_bytes;
                        while ((revc_bytes = read(conn_arg->fd, buff, sizeof(buff))) > 0) {
                            // 读到数据
                            conn_arg->read_bytes += revc_bytes;
                        }
                        if (revc_bytes == 0) {
                            // 对方关闭连接，读取成功
                            bytes += conn_arg->read_bytes;

                            // Epoll_ctl(ep_fd, EPOLL_CTL_DEL, conn_arg->fd, NULL);
                            close(conn_arg->fd);
                            conn_arg->status = STAT_NOT_CONNECTED;
                            CQ_in_queue(&queue, conn_arg);
                            ++succeed;
                        }
                        else if (revc_bytes < 0) {
                            // 出错，进行判断
                            // 测试机器（ubuntu18）上有    #define	EWOULDBLOCK	EAGAIN
                            if (errno == EAGAIN || errno == EWOULDBLOCK ||
                                    (errno == (EAGAIN | EWOULDBLOCK))) {
                                // 正常错误，数据未全部发送完
                                continue;
                            }
                            else {
                                // 读出错
                                ++failed;
                                conn_arg->status = STAT_NOT_CONNECTED;
                                close(conn_arg->fd);
                                // Epoll_ctl(ep_fd, EPOLL_CTL_DEL, conn_arg->fd, NULL);
                                CQ_in_queue(&queue, conn_arg);
                            }
                        }
                    }
                }

            }
        }

    }

    // 释放资源
    for (i = 0; i < concurrency; ++i) {
        if (connection_list[i].status != STAT_NOT_CONNECTED) {
            close(connection_list[i].fd);
        }
    }
    close(ep_fd);
    Free(connection_list);
    CQ_destroy(&queue);
}