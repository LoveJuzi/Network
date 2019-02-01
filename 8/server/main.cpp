#include <iostream>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <syslog.h>

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
#define SERV_PORT 9877
#define LISTENQ 5
#define MAXFD 64

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

int daemon_init(const char *pname, int facility) {
    int i;
    pid_t pid;

    if ( (pid=fork()) < 0) {
        return -1;
    } else if (pid) {
        _exit(0);
    }

    /* child 1 continues... */

    if (setsid() < 0) {            // become session leader
        return (-1);
    }

    signal(SIGHUP, SIG_IGN);

    if ( (pid=fork()) < 0) {
        return -1;
    } else if (pid ) {
        _exit(0);
    }

    /* child 2 continues... */

    chdir("/");

    /* close off file descriptors */
    for (i=0; i<MAXFD; i++) {
        close(i);
    }

    /* redirect stdin, stdout, and stderr to /dev/null */
    open("/dev/null", O_RDONLY);
    open("/dev/null", O_WRONLY);
    open("/dev/null", O_RDWR);

    openlog(pname, LOG_PID, facility);
}

int main(int argc, char **argv)
{
    int sockfd;
    struct sockaddr_in servaddr;
    struct sockaddr_in cliaddr;
    socklen_t clilen;
    char buf[MAXLINE];

    // 创建 socket 
    sockfd = Socket(AF_INET, SOCK_DGRAM, 0);

    // 绑定端口
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    Bind(sockfd, (SA *)&servaddr, sizeof(servaddr));

    // 无限循环，等待输入
    for (;;) {
        clilen = sizeof(cliaddr);
        auto n = recvfrom(sockfd, buf, MAXLINE, 0, (SA *)&cliaddr, &clilen);

        sendto(sockfd, buf, n, 0, (SA *)&cliaddr, clilen);
    }
}