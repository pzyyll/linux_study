/*
 * @file    epoll_client.h
 * @auth    zhilicai
 * @E-mail  pzyyll@gmail.com
 * @brief   tcp client by epoll im
 *
 */

#ifndef LINUX_STU_EPOLL_CLIENT_H
#define LINUX_STU_EPOLL_CLIENT_H

#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <cstring>
#include <cstdio>

static const unsigned int MAX_BUF_LEN = 2048;

class EpollClient {
public:
    enum TYPE_IPADDR {
        IPV6 = AF_INET6,
        IPV4 = AF_INET
    };

    enum FLAGS_BLOCK {
        NON_BLOCK,
        BLOCK
    };
private:
    const static unsigned int kRwTimeOut = 500;
    const static unsigned int kConnTimeOut = 300;

public:
    EpollClient();

    ~EpollClient();

    int Init(TYPE_IPADDR af, const char *ip, unsigned int port);

    //int Connect();
    int Send(const char *buf, unsigned int bsize);
    int Recv(char *buf, unsigned int &bsize, unsigned int excp_len = 0);

    int ReconnSvr();
    int Close();
    bool CheckConn() { return check_conn_; }

    int SetFlagBlock(int fd, FLAGS_BLOCK flag = BLOCK);
    char *GetErrMsg() { return errmsg_; }

    void setRw_time_out(unsigned int rw_time_out);
    void setConnect_time_out(unsigned int connect_time_out);
private:
    int connect(int fd, const struct sockaddr *addr, unsigned int len);

    unsigned int writen(const void *vptr, unsigned int n);
    unsigned int readn(void *vptr, unsigned int nbyes);

private:
    char errmsg_[1024];
    bool check_conn_;

    int fd;
    struct sockaddr svraddr_;

    unsigned int rw_time_out_;
    unsigned int connect_time_out_;

/*
    struct SBUF {
        SBUF() : eptr(0), bptr(0) { }
        ~SBUF() { }
        char buf[MAX_BUF_LEN];
        unsigned int eptr;
        unsigned int bptr;
    };
    SBUF buf_to;
    SBUF buf_fr;
*/
};

#endif //LINUX_STU_EPOLL_CLIENT_H