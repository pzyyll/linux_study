/*
 * @file    epoll_client.cpp
 * @auth    zhilicai
 * @E-mail  pzyyll@gmail.com
 * @brief   tcp client by epoll im
 *
 */

#include "epoll_client.h"

EpollClient::EpollClient()
        : check_conn_(false),
          fd(0),
          rw_time_out_(kRwTimeOut),
          connect_time_out_(kConnTimeOut)
{
    memset(errmsg_, 0, sizeof(errmsg_));
}

EpollClient::~EpollClient() {
    close(fd);
}

int EpollClient::Init(TYPE_IPADDR af, const char *ip, unsigned int port)
{
    int ret = 0;

    //创建套接字
    fd = socket(af, SOCK_STREAM, 0);

    if (fd < 0)
    {
        ret = -1;
        snprintf(errmsg_, sizeof(errmsg_), "get socket fd fail.");
        return ret;
    }

    //先支持IPv4先
    bzero(&svraddr_, sizeof(svraddr_));
    if (af == IPV4) {
        struct sockaddr_in *addrv4 = reinterpret_cast<struct sockaddr_in *>(&svraddr_);

        addrv4->sin_family = af;
        addrv4->sin_port = htons(port);

        //该函数已废弃
        //svraddr_.sin_addr.s_addr = inet_addr(ip);

        //仅适用ipv4
        //inet_aton(ip, &svraddr_.sin_addr);

        if (inet_pton(af, ip, &addrv4->sin_addr) < 0) {
            ret = -2;
            snprintf(errmsg_, sizeof(errmsg_), "IP invalid.");
            return ret;
        }
    }

    if (this->connect(fd, &svraddr_, sizeof(svraddr_)) < 0)
    {
        ret = -3;
        snprintf(errmsg_, sizeof(errmsg_), "connect to svr fail");
        return ret;
    }

    check_conn_ = true;
    return ret;
}

int EpollClient::Send(const char *buf, unsigned int bsize)
{
    if (!CheckConn())
        return -1;
    //todo
    writen(buf, bsize);

    return bsize;
}

int EpollClient::Recv(char *buf, unsigned int &bsize, unsigned int excp_len)
{
    if (!CheckConn())
        return -1;

    //todo
    bsize = readn(buf, bsize);

    return bsize;
}

unsigned int EpollClient::writen(const void *vptr, unsigned int n)
{
    unsigned int nleft = n;
    unsigned int nwriten = 0;
    const char *ptr = static_cast<const char *>(vptr);

    while (nleft > 0) {
        if ( (nwriten = write(fd, ptr, nleft)) <= 0) {
            //系统调用被捕获的信息中断
            if (nwriten < 0 && errno == EINTR)
                nwriten = 0;
            else
                return -1;
        }
        nleft -= nwriten;
        ptr += nwriten;
    }
    return n;
}

unsigned int EpollClient::readn(void *vptr, unsigned int nbyes)
{
    unsigned int nleft = nbyes;
    unsigned int nread = 0;
    char *ptr = static_cast<char *>(vptr);

    while (nleft > 0) {
        if ( (nread = read(fd, ptr, nleft)) < 0) {
            if (errno == EINTR)
                nread = 0;
            else
                return -1;
        } else if (0 == nread )
            break;              // EOF
        nleft -= nread;
        ptr += nread;
    }
    return nbyes - nleft;
}

int EpollClient::ReconnSvr()
{
    return this->connect(fd, &svraddr_, sizeof(svraddr_));
}

int EpollClient::connect(int fd, const struct sockaddr *addr, unsigned int len)
{
    return ::connect(fd, addr, len);
}

int EpollClient::SetFlagBlock(int fd, EpollClient::FLAGS_BLOCK flag) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        snprintf(errmsg_, sizeof(errmsg_), "set flag block: get flasg fail.");
        return -1;
    }

    (BLOCK == flag) ? (flags &= ~O_NONBLOCK) : (flags |= O_NONBLOCK);

    if (fcntl(fd, F_SETFL, flags) < 0) {
        snprintf(errmsg_, sizeof(errmsg_), "set flag block fail.(%d)", flag);
        return -2;
    }

    return 0;
}

void EpollClient::setRw_time_out(unsigned int rw_time_out) {
    EpollClient::rw_time_out_ = rw_time_out;
}

void EpollClient::setConnect_time_out(unsigned int connect_time_out) {
    EpollClient::connect_time_out_ = connect_time_out;
}
