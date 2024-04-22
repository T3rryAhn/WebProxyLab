#include <stdio.h>

#include "csapp.h"  // 기본적인 시스템 호출 및 소켓 관련 함수들

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

// function prototype
int doit(int fd);
void forward_request(int clientfd, int serverfd);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
int parse_uri(char *uri, char *hostname, char *port, char *path);

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

int main(int argc, char **argv) {
    int listenfd, connfd;                                 // listenfd는 리스팅 소켓의 fd, connfd는 연결 소켓의 fd
    socklen_t clientlen;                                  // clientlen은 클라이언트 주소 구조체의 크기를 저장. Accept 함수에서 사용
    struct sockaddr_storage clientaddr;                   // client 주소 정보를 저장하기 위한 큰 구조체. ipv4 와 ipv6 모두 지원
    char client_hostname[MAXLINE], client_port[MAXLINE];  // 클라이언트의 호스트 이름과 포트 번호를 저장하는 배열

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    listenfd = Open_listenfd(argv[1]);                                    // 지정된 포트로 리스닝 소켓을 열고, 해당 소켓의 fd를 listenfd에 저장
    while (1) {                                                           // 무한 루프를 시작합니다. 서버는 계속해서 클라이언트의 연결 요청을 기다립니다.
        clientlen = sizeof(struct sockaddr_storage);                      // 클라이언트 주소 구조체의 크기를 clientlen에 저장합니다.
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);         // 클라이언트의 연결 요청을 수락합니다. 수락된 연결의 소켓 디스크립터를 connfd에 저장합니다.
        Getnameinfo((SA *)&clientaddr, clientlen,                         // 클라이언트의 주소 정보를 사용하여,
                    client_hostname, MAXLINE,                             // 호스트 이름과
                    client_port, MAXLINE, 0);                             // 포트 번호를 문자열로 변환합니다.
        printf("Connected to (%s, %s)\n", client_hostname, client_port);  // 클라이언트의 호스트 이름과 포트 번호를 출력합니다.
        printf("%s\n", user_agent_hdr);
        doit(connfd);
        Close(connfd);  // 연결 소켓을 닫습니다. 실제 프록시 서버에서는 여기서 추가 작업(데이터 전송 등)이 이루어진 후 연결을 종료합니다.
    }
    return 0;  // 메인 함수 종료
}

int doit(int fd) {
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char hostname[MAXLINE], path[MAXLINE], port[MAXLINE];
    rio_t rio, server_rio;
    int serverfd;

    // Read request line and headers
    Rio_readinitb(&rio, fd);
    if (!Rio_readlineb(&rio, buf, MAXLINE))  // Read the request line
        return;

    sscanf(buf, "%s %s %s", method, uri, version);  // Parse request line
    if (strcasecmp(method, "GET") && strcasecmp(method, "HEAD")) {
        clienterror(fd, method, "501", "Not Implemented", "Proxy does not implement this method");
        return;
    }

    // Parse URI from GET request
    if (!parse_uri(uri, hostname, port, path)) {
        clienterror(fd, uri, "400", "Bad Request", "Proxy received a malformed request");
        return;
    }

    /* Check if the request is for favicon.ico and ignore it */
    if (strstr(uri, "favicon.ico")) {
        printf("Ignoring favicon.ico request\n");
        return;  // Just return without sending any response
    }

    // Connect to the target server
    printf("Connect to {hostname : %s, port : %s}\n", hostname, port);
    serverfd = Open_clientfd(hostname, port);

    if (serverfd < 0) {
        fprintf(stderr, "Connection to %s on port %s failed.\n", hostname, port);
        clienterror(fd, "Connection Failed", "5-3", "Service Unavailable", "The proxy server could not retrieve the resource.");
        return;
    }

    // Write the HTTP headers to the server
    sprintf(buf, "GET %s HTTP/1.0\r\n", path);
    sprintf(buf, "%sHost: %s\r\n", buf, hostname);
    sprintf(buf, "%s%s\r\n", buf, user_agent_hdr);
    sprintf(buf, "%sConnection: close\r\n", buf);
    Rio_writen(serverfd, buf, strlen(buf));

    // Transfer the response back to the client
    Rio_readinitb(&server_rio, serverfd);
    forward_request(fd, serverfd);

    Close(serverfd);
    return 0;
}

// Forward the HTTP response from server to client
void forward_request(int clientfd, int serverfd) {
    char buf[MAXLINE];
    ssize_t n;
    ssize_t received_size = 0;
    while ((n = Rio_readn(serverfd, buf, MAXLINE)) > 0) {
        // printf("Proxy received %zd bytes, now sending...\n", n);
        received_size += n;
        Rio_writen(clientfd, buf, n);
    }
    printf("Proxy received %zd bytes, now sending...\n", received_size);
}

// parse uri
int parse_uri(char *uri, char *hostname, char *port, char *pathname) {
    char *pos;
    char *start;

    if (uri == NULL) return 0;

    // skip protocol (http:// or https://)
    start = strstr(uri, "://");
    if (start != NULL) {
      start += 3;
    } else {
      start = uri;
    }

    // find port num pos
    pos = strchr(start, ':');
    if (pos != NULL) {
      *pos = '\0';
      sscanf(start, "%s", hostname);
      sscanf(pos + 1, "%[^/]", port);
      start = pos + strlen(port) + 1;
    } else {
      // 포트가 명시되지 않았다면 기본 포트를 사용.
        pos = strchr(start, '/');
        if (pos != NULL) {
            *pos = '\0';
            sscanf(start, "%s", hostname);
            *pos = '/';
            strcpy(port, "8080");
        } else {
            sscanf(start, "%s", hostname);
            strcpy(port, "8080");
            strcpy(pathname, "/");
            return 1;
        }
        start = pos;
    }

    // 경로 추출
    if (*start == '/') {
      sscanf(start, "%s", pathname);
    } else {
      strcpy(pathname, "/");
    }

    return 1;
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Proxy Error</title>");
    sprintf(body,
            "%s<body bgcolor="
            "ffffff"
            ">\r\n",
            body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Proxy server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}