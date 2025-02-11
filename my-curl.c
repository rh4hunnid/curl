#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PROTOCOL_HTTP "http://"
#define PROTOCOL_HTTPS "https://"
#define MAX_REQ 1024
#define MAX_REC 4096

struct request {
    char protocol[8];
    char host[256];
    int port;
    char path[1024];
};

int parse_request(struct request *req, const char *url) {
    memset(req, 0, sizeof(*req));
    if (strncmp(url, PROTOCOL_HTTP, 7) == 0) {
        strncpy(req->protocol, url, 7);
        req->protocol[4] = '\0';
        url += 7;
    } else if (strncmp(url, PROTOCOL_HTTPS, 8) == 0) {
        strncpy(req->protocol, url, 8);
        req->protocol[5] = '\0';
        url += 8;
    } else {
        printf("Invalid Protocol\n");
        return 1;
    }

    const char *slash = strchr(url, '/');
    if (slash) {
        size_t host_len = slash - url;
        if (host_len < sizeof(req->host)) {
            strncpy(req->host, url, host_len);
            req->host[host_len] = '\0';
        } else {
            printf("Host is too long\n");
            return 1;
        }
        strncpy(req->path, slash, sizeof(req->path) - 1);
        req->path[sizeof(req->path) - 1] = '\0';
    } else {
        strncpy(req->host, url, sizeof(req->host) - 1);
        req->host[sizeof(req->host) - 1] = '\0';
    }


    char *colon = strchr(req->host, ':');
    if (colon) {
        char *port_str = colon + 1;
        if (strlen(port_str) < 6) {
            char temp_port[6];
            strncpy(temp_port, port_str, sizeof(temp_port));
            temp_port[sizeof(temp_port) - 1] = '\0';
            sscanf(temp_port, "%d", &req->port);
            *colon = '\0';
        } else {
            printf("Port number too long\n");
            return 1;
        }
    }
    return 0;
}


int handle_request(struct request *req) {
    int sockfd;
    struct addrinfo hints, *res;
    char request[MAX_REQ], response[MAX_REC];
    ssize_t bytes_received;

    memset(&hints,0,sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    char port_str[6];
    snprintf(port_str,sizeof(port_str),"%d",req->port);


    getaddrinfo(req->host,port_str,&hints,&res);

    sockfd = socket(res->ai_family,res->ai_socktype,res->ai_protocol);
    if (sockfd == -1) {
        printf("ERROR: Failed to create socket");
        freeaddrinfo(res);
        return 1;
    }

    if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("connect");
        close(sockfd);
        freeaddrinfo(res);
        return 1;
    }

    snprintf(request, sizeof(request),
             "GET /%s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n\r\n",
             req->path, req->host);

    send(sockfd, request, strlen(request), 0);



    while ((bytes_received = recv(sockfd, response, sizeof(response) - 1, 0)) > 0) {
        response[bytes_received] = '\0';
        printf("%s", response);
    }

    freeaddrinfo(res);


    return 1;
}



int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <url>\n", argv[0]);
        return 1;
    }
    struct request req;
    if (parse_request(&req, argv[1]) == 0) {
        printf("Protocol: %s\n", req.protocol);
        printf("Host: %s\n", req.host);
        printf("Port: %d\n", req.port);
        printf("Path: %s\n", req.path);
    }

    handle_request(&req);
    return 0;
}
