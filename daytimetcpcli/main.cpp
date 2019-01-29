#include <iostream>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAXLINE 1024
#define SA const struct sockaddr

int main(int argc, char **argv)
{
   // 1. 套接字的初始化
   int sockfd;
   int n;
   char recvline[MAXLINE + 1];
   struct sockaddr_in servaddr;

   sockfd = socket(AF_INET, SOCK_STREAM, 0);
   if (sockfd < 0)
   {
      return -1;
   }
   bzero(&servaddr, sizeof(servaddr));

   // 2. 构造套接字的连接信息
   servaddr.sin_family = AF_INET;
   servaddr.sin_port = htons(13);

   // cn.pool.ntp.org
   if (inet_pton(AF_INET, "193.228.143.14", &servaddr.sin_addr) <= 0)
   {
      return -1;
   }

   // 3. 连接
   if (connect(sockfd, (SA *)&servaddr, sizeof(servaddr)) < 0)
   {
      return -1;
   }

   // 4. 从套接字中获取信息
   while ((n = read(sockfd, recvline, MAXLINE)) > 0)
   {
      recvline[n] = '\0';
      if (fputs(recvline, stdout) == EOF)
      {
         return -1;
      }
   }

   if (n < 0)
   {
      return -1;
   }

   return 0;
}