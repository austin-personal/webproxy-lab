/*

/week07-web-proxy/webproxy-lab/hostinfo# gcc -o hostinfo hostinfo.c -g -Wall
/week07-web-proxy/webproxy-lab/hostinfo# ./hostinfo naver.com

223.130.192.248
223.130.200.236
223.130.192.247
223.130.200.219

*/
#include "../csapp.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

int main(int argc, char **argv)
{
    struct addrinfo *p, *listp, hints;
    char buf[MAXLINE];
    int rc, flags;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <domain name>\n", argv[0]);
        exit(0);
    }

    /* Get a list of addrinfo records */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; /* IPv4 only */
    hints.ai_socktype = SOCK_STREAM; /* Connections only */
    if ((rc = getaddrinfo(argv[1], NULL, &hints, &listp)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(rc));
        exit(1);
    }

    /* Walk the list and display each IP address */
    flags = NI_NUMERICHOST; /* Display address string instead of domain name */
    for (p = listp; p; p = p->ai_next) {
        getnameinfo(p->ai_addr, p->ai_addrlen, buf, MAXLINE, NULL, 0, flags);
        printf("%s\n", buf);
    }

    /* Clean up */
    freeaddrinfo(listp);
    exit(0);
}