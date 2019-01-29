#include <iostream>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAXLINE 1024
#define SA struct sockaddr
#define SERV_PORT 8080
#define LISTENQ 5

int Socket(int __domain, int __type, int __protocol) {
   int fd;
   fd = socket(__domain, __type, __protocol);
   if (fd <= 0) {
      exit(-1);
   }

   return fd;
}

int Bind(int __fd, __CONST_SOCKADDR_ARG __addr, socklen_t __len) {
   if (bind(__fd, __addr, __len) < 0) {
      exit(-1);
   }

   return 0;
}

int Listen (int __fd, int __n) {
   if (listen(__fd, __n) < 0) {
      exit(-1);
   }

   return 0;
}

int Accept(int __fd, __SOCKADDR_ARG __addr, socklen_t *__restrict __addr_len) {
   int fd;
   
   fd = accept(__fd, __addr, __addr_len);

   if ( fd <= 0) {
      exit(-1);
   }

   return fd;
}

int Close(int __fd) {
   if (close(__fd) < 0) {
      exit(-1);
   }

   return 0;
}

int main(int argc, char **argv)
{
   int listenfd, connfd;
   pid_t childpid;
   socklen_t clilen;
   struct sockaddr_in cliaddr, servaddr;

   // 构造listen sockert
   listenfd = Socket(AF_INET, SOCK_STREAM, 0);

   // 初始化 servaddr 相关信息
   bzero(&servaddr, sizeof(servaddr));
   servaddr.sin_family =  AF_INET;
   servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
   servaddr.sin_port = htons(SERV_PORT);

   // 绑定服务到指定的内核
   Bind(listenfd, (SA *)&servaddr, sizeof(servaddr));

   // 监听服务
   Listen(listenfd, LISTENQ);

   // 并发处理业务
   for (;;) {
      clilen = sizeof(cliaddr);
      connfd = Accept(listenfd, (SA *)&cliaddr, &clilen);

      if ( (childpid = fork()) == 0) {
         Close(listenfd);
         exit(0);
      }
      Close(connfd);
   }

   return 0;
}