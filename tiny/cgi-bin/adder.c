/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(void) {
  // 초기화
  char *buf, *p;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE]; // 
  int n1=0, n2=0;
  /* Extract the two arguments */
  // CGI 프로그램이 실행될 때 설정된 QUERY_STRING 환경 변수를 가져온다.
  if ((buf = getenv("QUERY_STRING")) != NULL) { // QUERY_STRING은 문자열은 GET 요청에서 ? 뒤에 오는 인수 부분
    p = strchr(buf, '&'); // buf에서 '&' 문자의 위치
    *p = '\0'; // '&'를 NULL 문자로 대체하여 첫 번째 인수와 두 번째 인수를 분리
    strcpy(arg1, buf); // strcpy(arg1, buf)는 첫 번째 인수를 arg1에 복사
    strcpy(arg2, p+1); // strcpy(arg2, p + 1)는 두 번째 인수를 arg2에 복사
    n1 = atoi(arg1);
    n2 = atoi(arg2);
  }
  /* Make the response body */
  sprintf(content, "QUERY_STRING=%s", buf);
  sprintf(content, "Welcome to add.com: ");
  sprintf(content, "%sTHE Internet addition portal.\r\n<p>", content);
  sprintf(content, "%sThe answer is: %d + %d = %d\r\n<p>", content, n1, n2, n1 + n2);
  sprintf(content, "%sThanks for visiting!\r\n", content);
  /* Generate the HTTP response */
  printf("Connection: close\r\n");
  printf("Content-length: %d\r\n", (int)strlen(content));
  printf("Content-type: text/html\r\n\r\n");
  printf("%s", content);
  fflush(stdout);
  exit(0);
}
/* $end adder */
