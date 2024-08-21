/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

int main(int argc, char **argv) {
    // 1. 초기화
    int listenfd, connfd; // 리스닝 소켓의 listenfd와 연결소켓 connfd을 초기화 함
    char hostname[MAXLINE], port[MAXLINE]; //리스닝 포트를 위한 호스트이름과 포트
    socklen_t clientlen; // 받을 클라이언트의 주소 구조체
    struct sockaddr_storage clientaddr; // 받을 클라이언트의 주소 구조체

    // 2. 시작시 받을 인자 값의 개수가 2개인지 체크 (hostname 과 port)
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    // 3. Listen socket 열기 
    listenfd = Open_listenfd(argv[1]);
    // 4. 무한 루프를 돌며 클라이언트의 연결 요청을 받고 Accept를 통해 connfd로 연결 만들기
    while (1) { // 무한루프로 계속 요청 받음
        clientlen = sizeof(clientaddr); // 클라이언트의 주소 정보를 저장할 clientaddr 구조체의 크기를 설정
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);  // 클라이언트의 연결 요청 승낙후 연결 소켓의 파일 디스크립터를 반환
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0); // clientaddr에 저장된 클라이언트의 IP 주소와 포트 번호를 hostname과 port에 저장합니다. 이 정보를 사용하여 클라이언트가 누구인지 식별할 수 있습니다.
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        doit(connfd);   // line:netp:tiny:doit
        Close(connfd);  // line:netp:tiny:close
    }
}


/*************** doit: 클라이언트로부터 들어온 HTTP 요청을 처리 ******************/

void doit(int fd) {
    // 1. 변수 선언 및 초기화
    int is_static; // 요청된 리소스가 static or dynamic를 저장하는 변수
    struct stat sbuf; // 파일의 메타데이터를 저장하는 stat 구조체
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE]; //클라이언트의 요청 라인을 저장하는 문자열 버퍼들
    char filename[MAXLINE], cgiargs[MAXLINE]; // 요청된 파일 이름과 CGI 프로그램에 전달할 인자를 저장하는 버퍼
    rio_t rio; // rio_t 타입의 구조체

    // 2. 요청 라인과 헤더 읽기
    Rio_readinitb(&rio, fd); // rio를 초기화
    Rio_readlineb(&rio, buf, MAXLINE); // 요청의 첫 번째 줄(요청 라인)을 읽어 buf에 저장

    printf("Request headers:\n"); 
    printf("%s", buf);

    sscanf(buf, "%s %s %s", method, uri, version);// 요청 라인에서 HTTP 메서드(GET), URI, 버전 정보를 추출

    if (strcasecmp(method, "GET")) {
        clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
        return; 
    }
    // 3. 요청 헤더 처리:
    read_requesthdrs(&rio);// 함수는 요청 헤더를 읽어들이다

    // 4. 요청이 스태틱인지 동적 컨텐츠인지
    is_static = parse_uri(uri, filename, cgiargs);

    // 5. 만약 찾는 리소스가 없을시에 예외 처리
    if (stat(filename, &sbuf) < 0) {
        clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
        return; 
    }

    // 6. 요청이 스태틱 컨텐츠일때와 아닐때
    if (is_static) { // 스태틱일경우
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) { // (요청된 파일이 일반 파일이거나, 읽기 권한이 있는 파일일경우)가 아닌경우에 에러 출력
            clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
            return;
        }
        // S_ISREG: regular file인지 체크
        // S_IRUSR: 읽기 권한이 있는 지 체크
        serve_static(fd, filename, sbuf.st_size);// 요청된 파일이 일반 파일이거나, 읽기 권한이 있는 파일일경우
    }
    else { /* Serve dynamic content */
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) { // (일반 파일이거나 실행권한 체크)가 아닐때 에러표현
            clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
            return;
        }
        // S_IXUSR는 권한이 있는지 없는지 체크
        serve_dynamic(fd, filename, cgiargs);
    }
}
/*************** clienterror:HTTP 서버에서 클라이언트에게 오류 메시지를 전송 ******************/
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
    char buf[MAXLINE], body[MAXBUF];
    // 1. HTTP 응답 본문 구성: 클라이언트에게 보여질 에러 페이지 HTML 페이지를 만들기
    sprintf(body, "<html><title>Tiny Error</title>"); // HTML 문서의 제목은 "Tiny Error"로 설정
    sprintf(body, "%s<body bgcolor=\"ffffff\">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);
    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}
/*************** read_requesthdrs: HTTP 요청 헤더를 읽고 출력하는 역할 ******************/
void read_requesthdrs(rio_t *rp) {
    // 1. 초기화
    char buf[MAXLINE]; // 클라이언트로부터 읽어들인 헤더의 한 줄을 저장할 버프 초기화

    // 2. rp로부터 클라이언트의 데이터를 읽어 들인다. MAXLINE 만큼 읽어서 buf에 저장
    Rio_readlineb(rp, buf, MAXLINE);

    // 3. strcmp 함수는 buf와 \r\n (빈 줄)을 비교. 헤더 끝에는 빈 줄이 포함되므로, 빈 줄이 나올 때까지 반복
    while (strcmp(buf, "\r\n")) {
        Rio_readlineb(rp, buf, MAXLINE); // 계속해서 한줄씩 읽기
        printf("%s", buf); // 읽은것을 출력
    }
    return; 
}
/*************** parse_uri:HTTP 서버에서 클라이언트에게 오류 메시지를 전송 ******************/
int parse_uri(char *uri, char *filename, char *cgiargs) {
    // 인자들: 
        // uri: The URI from the HTTP request. URL의 기본 주소 다음에 들어오는 모든 것.
        // filename: A buffer to store the path of the file or CGI script.
        // cgiargs: A buffer to store CGI arguments (if any).
    char *ptr;
    // 1. static: uri에 cgi-bin가 있지 않으면 스태틱
    if (!strstr(uri, "cgi-bin")) {  /* Static content */
        strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcat(filename, uri);  
        if (uri[strlen(uri)-1] == '/') {
            strcat(filename, "home.html");
        }
        return 1;    
    }
    // 2. Dynamic
    else {  /* Dynamic content */
        ptr = index(uri, '?');
        if (ptr) {
            strcpy(cgiargs, ptr+1);
            *ptr = '\0'; 
        }
        else {
            strcpy(cgiargs, "");
        }
        strcpy(filename, ".");
        strcat(filename, uri);
        return 0;
    }
}

/*************** serve_static: 클라이언트에게 정적 콘텐츠를 제공하는 역할 ******************/
void serve_static(int fd, char *filename, int filesize) {
    // filename = 클라이언트가 요청한 파일의 이름, filesize = 파일의 크기
    // 1. 변수 선정 초기화
    int srcfd; // 소스 파일을 읽기 위해 열 파일 디스크립터
    char *srcp, filetype[MAXLINE], buf[MAXBUF];
    // srcp: 메모리 매핑된 파일의 내용을 가리키는 포인터.
    // filetype: 파일의 MIME 타입을 저장할 버퍼.
    // buf: HTTP 응답 헤더를 저장할 버퍼

    // 2. 응답 헤더를 만들고 클라이언트로 전송한다
    get_filetype(filename, filetype);// 파일 타입 결정: filename을 기반으로 filetype에 MIME 타입을 설정
        // Ex) .html 파일은 text/html로 설정됨
    sprintf(buf, "HTTP/1.0 200 OK\r\n"); // HTTP/1.0 200 OK를 buf에 작성하여 성공적인 요청
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf); //서버 정보 헤더 추가
    sprintf(buf, "%sConnection: close\r\n", buf); // 연결 종료 헤더 추가
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize); // 컨텐츠 길이 헤더 추가
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);// 컨텐츠 타입 헤더 추가
    Rio_writen(fd, buf, strlen(buf)); // 응답 헤더 전송

    // 3. 응답 바디를 클라이언트로 전송 
    srcfd = Open(filename, O_RDONLY, 0); // 파일 열기: 요청된 파일을 읽기 전용으로 열기
    srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); // 파일 메모리 매핑
        // srcp는 메모리 매핑된 파일의 시작 주소를 가리킴 
        // PROT_READ는 읽기 전용
        // MAP_PRIVATE는 파일을 비공유 모드로 매핑
    Close(srcfd); // 파일 닫기
    Rio_writen(fd, srcp, filesize); // 파일 내용 전송: 
    Munmap(srcp, filesize); // 메모리 매핑 해제
}

/*************** get_filetype: 함수는 파일 이름에 기반하여 해당 파일의 MIME 타입을 결정 ******************/
void get_filetype(char *filename, char *filetype) {
    // filetype: 결정된 MIME 타입을 저장할 버퍼
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if (strstr(filename, ".png"))
        strcpy(filetype, "image/png");
    else if (strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpeg");
    else
        strcpy(filetype, "text/plain");
}
/*************** serve_dynamic: 동적 콘텐츠를 처리하기 위해 CGI 프로그램을 실행 ******************/
void serve_dynamic(int fd, char *filename, char *cgiargs) {
    // fd: 클라이언트와의 연결을 나타내는 파일 디스크립터
    char buf[MAXLINE], *emptylist[] = { NULL }; // 버퍼와 빈 인자 리스트 선언

    //1. 응답 헤더 작성과 전송
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));
    // 2. 자식 프로세스 생성
    if (Fork() == 0) { /* Child */
        /* Real server would set all CGI vars here */
        setenv("QUERY_STRING", cgiargs, 1);
        Dup2(fd, STDOUT_FILENO);         /* Redirect stdout to client */
        Execve(filename, emptylist, environ); /* Run CGI program */
    }
    Wait(NULL); /* Parent waits for and reaps child */
}
