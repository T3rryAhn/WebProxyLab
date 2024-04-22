#include <stdio.h>

#include "csapp.h"  // 기본적인 시스템 호출 및 소켓 관련 함수들

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

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
    while (1) {                                                           // 무한 루프를 시작합니다g. 서버는 계속해서 클라이언트의 연결 요청을 기다립니다.
        clientlen = sizeof(struct sockaddr_storage);                      // 클라이언트 주소 구조체의 크기를 clientlen에 저장합니다.
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);         // 클라이언트의 연결 요청을 수락합니다. 수락된 연결의 소켓 디스크립터를 connfd에 저장합니다.
        Getnameinfo((SA *)&clientaddr, clientlen,                         // 클라이언트의 주소 정보를 사용하여,
                    client_hostname, MAXLINE,                             // 호스트 이름과
                    client_port, MAXLINE, 0);                             // 포트 번호를 문자열로 변환합니다.
        printf("Connected to (%s, %s)\n", client_hostname, client_port);  // 클라이언트의 호스트 이름과 포트 번호를 출력합니다.
        Close(connfd);                                                    // 연결 소켓을 닫습니다. 실제 프록시 서버에서는 여기서 추가 작업(데이터 전송 등)이 이루어진 후 연결을 종료합니다.
    }
    return 0;  // 메인 함수 종료

    printf("%s", user_agent_hdr);
    return 0;
}
