//
// Created by yulan on 2021/5/5.
//
#include <string.h>
#include <stdio.h>
#include "utils.h"

int bulid_header(char *request_header, const char *host, int port,
                 const char *path, int method, int http_version,
                 char params[][MAX_HERDER_PARAM_LEN]) {
    request_header[0] = 0;
    switch (method) {
        case METHOD_GET:
            strcat(request_header, "GET ");
            break;
        case METHOD_HEAD:
            strcat(request_header, "HEAD ");
            break;
        case METHOD_OPTIONS:
            strcat(request_header, "OPTIONS ");
            break;
        case METHOD_TRACE:
            strcat(request_header, "TRACE ");
            break;
        default:
            fprintf(stderr, "\nmethod error.\n");
            break;
    }

    strcat(request_header, path);

    switch (http_version) {
        case HTTP_VERSION10:
            strcat(request_header, " HTTP/1.0\r\n");
            break;
        case HTTP_VERSION11:
            strcat(request_header, " HTTP/1.1\r\n");
            break;
        case HTTP_VERSION20:
            strcat(request_header, " HTTP/2.0\r\n");
            break;
        default:
            fprintf(stderr, "\nhttp version error.\n");
            break;
    }


    char tmp[1024];
    if (port == 80)
        sprintf(tmp, "Host: %s\r\n", host);
    else
        sprintf(tmp, "Host: %s:%d\r\n", host, port);
    strcat(request_header, tmp);


    if (params) {
        int i = 0;
        while (params[i][0] != 0) {
            if (strlen(request_header) + strlen(params[i]) + 4 > MAX_REQUEST_LEN) {
                fprintf(stderr, "Build request header failed: request header too len");
                return -1;
            }
            strcat(request_header, params[i]);
            strcat(request_header, "\r\n");
            ++i;
        }
    }

    strcat(request_header, "\r\n");
    return 0;
}
