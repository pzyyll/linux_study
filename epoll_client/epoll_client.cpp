/*
 * @file    epoll_client.cpp
 * @auth    zhilicai
 * @E-mail  pzyyll@gmail.com
 * @brief   tcp client by epoll im
 *
 */

#include "epoll_client.h"

EpollClient::EpollClient()
        : type_addr_(IPV4),
          port_(0),
          socket_(0),
          socket_inited_(false),
          check_conn_(false),
          rw_time_out_(kRwTimeOut),
          connect_time_out_(kConnTimeOut)
{
    memset(errmsg_, 0, sizeof(errmsg_));
    memset(ip_, 0, sizeof(ip_));
}

EpollClient::~EpollClient()
{
    CloseSocket();
}

int EpollClient::Init(const char *ip, unsigned int port, TYPE_IPADDR af, const unsigned int rw_time, const unsigned conn_time)
{
    int ret = 0;

    rw_time_out_ = rw_time;
    connect_time_out_ = conn_time;
    port_ = port;
    snprintf(ip_, sizeof(ip_), "%s", ip);

    if (Connect() < 0) {
        ret = -1;
        //snprintf(errmsg_, sizeof(errmsg_), "connect to svr fail");
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
        if ( (nwriten = write(socket_, ptr, nleft)) <= 0) {
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
        if ( (nread = read(socket_, ptr, nleft)) < 0) {
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
    CloseSocket();
    return Connect();
}

//todo 负责逻辑太多，可以再拆分，毕竟超过40行了...
int EpollClient::Connect()
{
    //非阻塞连接
    if (InitSocket() < 0) {
        return -1;
    }
    if (SetFlagBlock(NON_BLOCK) < 0)
        return -1;

    if (ToFillSocketAddr() < 0) {
        snprintf(errmsg_, sizeof(errmsg_), "ip to socket addr struct fail.");
        return -1;
    }

    bool check_conn = false;
    errno = 0;
    //尝试连接，若服务器与客户端在同一主机上，往往会立即连接完成
    if (connect(socket_, &svraddr_, sizeof(svraddr_)) < 0) {
        //EINPROGRESS表示连接正在进行中，若errno!=EINPROGRESS
        if (errno != EINPROGRESS) {
            snprintf(errmsg_, sizeof(errmsg_), "connect fail:%s", strerror(errno));
            return -1;
        } else {
            //检查连接是否成功
            check_conn = true;
        }
    }

    if (check_conn) {
        int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
        if (epoll_fd < 0) {
            snprintf(errmsg_, sizeof(errmsg_), "create epoll_fd fail:%s.", strerror(errno));
            return -1;
        }

        struct epoll_event ev, evs;
        ev.data.fd = socket_;
        ev.events = EPOLLOUT | EPOLLET;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_, &ev) < 0) {
            snprintf(errmsg_, sizeof(errmsg_), "epoll event add fail:%s.", strerror(errno));
            return -1;
        }

        for (;;) {
            errno = 0;
            switch(epoll_wait(epoll_fd, &evs, 1, connect_time_out_)) {
                case -1:
                    //若被外部中断继续等待
                    if (errno != EINTR) {
                        snprintf(errmsg_, sizeof(errmsg_), "other err:%s", strerror(errno));
                        return -1;
                    }
                    continue;
                case 0:
                    //timeout
                    errno = ETIMEDOUT;
                    snprintf(errmsg_, sizeof(errmsg_), "connect time out");
                    return -1;
                default:
                    //这里只注册了socket_,发生事件也只能是它??
                    //当连接成功时，socket_变为可写，若发生错误，socket_也会变为可读可写，通过获取套接字选项查看是否发生错误
                    if (evs.events & EPOLLOUT) {
                        int error = 0;
                        socklen_t len = sizeof(error);
                        if (getsockopt(socket_, SOL_SOCKET, SO_ERROR, (void *)(&error), &len) < 0) {
                            snprintf(errmsg_, sizeof(errmsg_), "%s", strerror(errno)); //Solaris pending error
                            return -1;
                        }

                        if (0 != error) {
                            snprintf(errmsg_, sizeof(errmsg_), "getsockopt SOL_SOCKET SO_ERROR, error=%d", error);
                            return -1;
                        }

                        return 0;
                    } else {
                        //黑人问号??
                        snprintf(errmsg_, sizeof(errmsg_), "unkown err");
                        return -1;
                    }
            }
        }
    }
    return 0;
}

int EpollClient::SetFlagBlock(EpollClient::FLAGS_BLOCK flag)
{
    if (!socket_inited_) {
        snprintf(errmsg_, sizeof(errmsg_), "socket fd not init.");
        return -1;
    }

    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        snprintf(errmsg_, sizeof(errmsg_), "set flag block: get flasg fail.");
        return -1;
    }

    (BLOCK == flag) ? (flags &= ~O_NONBLOCK) : (flags |= O_NONBLOCK);

    if (fcntl(fd, F_SETFL, flags) < 0) {
        snprintf(errmsg_, sizeof(errmsg_), "set flag block fail.(%d)", flag);
        return -1;
    }

    return 0;
}

void EpollClient::set_rw_time_out(unsigned int rw_time_out)
{
    EpollClient::rw_time_out_ = rw_time_out;
}

void EpollClient::set_connect_time_out(unsigned int connect_time_out)
{
    EpollClient::connect_time_out_ = connect_time_out;
}

int EpollClient::InitSocket()
{
    if (!socket_inited_) {
        socket_ = socket(type_addr_, SOCK_STREAM, 0);
        if (socket_ < 0) {
            snprintf(errmsg_, sizeof(errmsg_), "get socket socket_ fail.");
            return -1;
        }
        socket_inited_ = true;
    }
    return 0;
}

int EpollClient::ToFillSocketAddr()
{
    //先支持IPv4先
    bzero(&svraddr_, sizeof(svraddr_));
    if (type_addr_ == IPV4) {
        struct sockaddr_in *addrv4 = reinterpret_cast<struct sockaddr_in *>(&svraddr_);

        addrv4->sin_family = type_addr_;
        addrv4->sin_port = htons(port_);

        //该函数已废弃
        //svraddr_.sin_addr.s_addr = inet_addr(ip);

        //仅适用ipv4
        //inet_aton(ip, &svraddr_.sin_addr);

        if (inet_pton(type_addr_, ip_, &addrv4->sin_addr) < 0) {
            snprintf(errmsg_, sizeof(errmsg_), "IP invalid.");
            return -1;
        }
    } else {
        //IPv6
        return -1;
    }

    return 0;
}

void EpollClient::CloseSocket()
{
    if (socket_inited_) {
        close(socket_inited_);
        socket_inited_ = false;
    }
}
