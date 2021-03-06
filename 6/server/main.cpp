#include <iostream>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/time.h>

#include <set>
#include <vector>
#include <algorithm>

#define MAXLINE 1024
#define SA struct sockaddr
#define SERV_PORT 9879
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
        printf("writen error\n");
        exit(-1);
    }

    return n;
}

void str_echo(int sockfd)
{
    ssize_t n;
    char buf[MAXLINE];

again:
    while ((n = read(sockfd, buf, MAXLINE)) > 0)
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

void sig_chld(int signo)
{
    pid_t pid;
    int stat;

    // pid = wait(&stat);
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
        ;

    return;
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
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);

    // 绑定服务到指定的内核
    Bind(listenfd, (SA *)&servaddr, sizeof(servaddr));

    // 监听服务
    Listen(listenfd, LISTENQ);

    // 所有的客户端申请的fd
    std::set<int> fdset;

    // 并发处理业务
    for (;;)
    {
        fd_set allset;

        FD_ZERO(&allset);

        auto maxfdp1 = listenfd + 1;

        FD_SET(listenfd, &allset);
        for (auto itr = fdset.begin(); itr != fdset.end(); ++itr)
        {
            FD_SET(*itr, &allset);
            maxfdp1 = std::max(maxfdp1, *itr + 1);
        }

        select(maxfdp1, &allset, NULL, NULL, NULL);

        if (FD_ISSET(listenfd, &allset))
        {
            clilen = sizeof(cliaddr);
            connfd = Accept(listenfd, (SA *)&cliaddr, &clilen);
            fdset.insert(connfd);
        }

        std::vector<int> rmfd;
        for (auto itr = fdset.begin(); itr != fdset.end(); ++itr)
        {
            if (FD_ISSET(*itr, &allset))
            {
                ssize_t n;
                char buf[MAXLINE];

                bzero(buf, sizeof(buf));
                n = read(*itr, buf, MAXLINE);
                if (n == 0)
                {
                    rmfd.push_back(*itr);
                    Close(*itr);
                }
                else
                {
                    Writen(*itr, buf, n);
                }
            }
        }

        for (auto itr = rmfd.begin(); itr != rmfd.end(); ++itr)
        {
            fdset.erase(*itr);
        }
    }

    exit(0);
}