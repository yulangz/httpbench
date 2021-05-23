//
// Created by yulan on 2021/5/5.
//
#include "utils.h"

int parse_url(const char *url, char *host, int *port, char *path) {
    host[0] = 0;
    path[0] = 0;

    if (strstr(url, "://") == NULL) {
        fprintf(stderr, "\n%s is not a valid URL.\n", url);
        return -1;
    }
    if (strlen(url) > MAX_URL_LEN) {
        fprintf(stderr, "URL is too long.\n");
        return -1;
    }

    if (0 != strncasecmp("http://", url, 7)) {
        fprintf(stderr, "\nhttpbench Only support HTTP protocol.\n");
        return -1;
    }

    const char *full_path = url + strlen("http://"); // full_path是去掉http://后的全路径
    const char *path_start, *host_start, *port_start;
    char tmp[10];

    host_start = full_path;

    if ((path_start = strchr(full_path, '/')) == NULL) {
        fprintf(stderr, "\nInvalid URL syntax : hostname don't ends with '/'.\n");
        return -1;
    }

    // 解析port与host
    port_start = strchr(full_path, ':');
    if (port_start != NULL && port_start < path_start) {
        // 指定了port
        port_start++;
        bzero(tmp, 10);
        strncpy(tmp, port_start, path_start - port_start);
        *port = atoi(tmp);
        if (*port == 0)
            *port = 80;

        // 复制host，不包含端口号
        strncpy(host, host_start, port_start - 1 - host_start);
        host[port_start - 1 - host_start] = 0;
    }
    else {
        // 没有指定port
        *port = 80;
        // 复制host
        strncpy(host, host_start, path_start - host_start);
        host[path_start - host_start] = 0;
    }

    // 复制path
    strcpy(path, path_start);
    return 0;
}