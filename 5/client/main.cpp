#include <iostream>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAXLINE 1024
#define SA struct sockaddr
#define SERV_PORT 9877
#define LISTENQ 5

int Socket(int __domain, int __type, int __protocol)
{
   int fd;
   fd = socket(__domain, __type, __protocol);
   if (fd <= 0)
   {
      exit(-1);
   }

   return fd;
}

int Bind(int __fd, __CONST_SOCKADDR_ARG __addr, socklen_t __len)
{
   if (bind(__fd, __addr, __len) < 0)
   {
      exit(-1);
   }

   return 0;
}

int Listen(int __fd, int __n)
{
   if (listen(__fd, __n) < 0)
   {
      exit(-1);
   }

   return 0;
}

int Accept(int __fd, __SOCKADDR_ARG __addr, socklen_t *__restrict __addr_len)
{
   int fd;

   fd = accept(__fd, __addr, __addr_len);

   if (fd <= 0)
   {
      exit(-1);
   }

   return fd;
}

int Close(int __fd)
{
   if (close(__fd) < 0)
   {
      exit(-1);
   }

   return 0;
}

int Connect(int __fd, __CONST_SOCKADDR_ARG __addr, socklen_t __len)
{
   if (connect(__fd, __addr, __len) < 0)
   {
      exit(-1);
   }

   return 0;
}

ssize_t readn(int fd, void *vptr, size_t n)
{
   size_t nleft;
   ssize_t nread;
   char *ptr;

   ptr = (char *)vptr;
   nleft = n;

   while (nleft > 0)
   {
      if ((nread = read(fd, ptr, nleft)) < 0)
      {
         if (errno == EINTR)
         {
            nread = 0;
         }
         else
         {
            return -1;
         }
      }
      else if (nread == 0)
      {
         break;
      }

      nleft -= nread;
      ptr += nread;
   }

   return (n - nleft);
}

ssize_t Readn(int fd, void *vptr, size_t n)
{
   if (readn(fd, vptr, n) < 0)
   {
      exit(-1);
   }

   return n;
}

ssize_t writen(int fd, const void *vptr, size_t n)
{
   size_t nleft;
   ssize_t nwritten;

   const char *ptr;

   ptr = (const char *)vptr;
   nleft = n;
   while (nleft > 0)
   {
      if ((nwritten = write(fd, ptr, nleft)) <= 0)
      {
         if (nwritten < 0 && errno == EINTR)
         {
            nwritten = 0;
         }
         else
         {
            return -1;
         }
      }
      nleft -= nwritten;
      ptr += nwritten;
   }

   return n;
}

ssize_t Writen(int fd, const void *vptr, size_t n)
{
   if (writen(fd, vptr, n) < 0)
   {
      exit(-1);
   }

   return n;
}

void str_echo(int sockfd)
{
   ssize_t n;
   char buf[MAXLINE];

again:
   while ((n = read(sockfd, buf, MAXLINE)) < 0)
   {
      Writen(sockfd, buf, n);
   }

   if (n < 0 && errno == EINTR)
   {
      goto again;
   }
   else if (n < 0)
   {
      exit(-1);
   }
}

void str_cli(int sockfd)
{
   char sendline[MAXLINE];
   char recvline[MAXLINE];

   while (fgets(sendline, MAXLINE, stdin) != NULL)
   {
      Writen(sockfd, sendline, strlen(sendline));

      Readn(sockfd, recvline, MAXLINE);

      fputs(recvline, stdout);
   }
}

int main(int argc, char **argv)
{
   int sockfd;
   struct sockaddr_in servaddr;

   sockfd = Socket(AF_INET, SOCK_STREAM, 0);

   bzero(&servaddr, sizeof(servaddr));
   servaddr.sin_family = AF_INET;
   servaddr.sin_port = htons(SERV_PORT);
   inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);

   Connect(sockfd, (SA *)&servaddr, sizeof(servaddr));

   str_cli(sockfd);

   return 0;
}