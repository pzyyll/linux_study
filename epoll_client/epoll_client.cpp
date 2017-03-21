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
          socket_(-1),
          socket_inited_(false),
          check_conn_(false),
          rw_time_out_(kRwTimeOut),
          connect_time_out_(kConnTimeOut),
          epoll_fd_(-1)
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
    rw_time_out_ = rw_time;
    connect_time_out_ = conn_time;
    port_ = port;
    snprintf(ip_, sizeof(ip_), "%s", ip);

    if (Connect() < 0) {
        return -1;
    }

    check_conn_ = true;
    return 0;
}

int EpollClient::Send(const char *buf, unsigned int bsize)
{
    if (NULL == buf) {
        SetErrMsg("send buffer is null.");
        return -1;
    }

    struct epoll_event ev;
    ev.data.fd = socket_;
    ev.events = EPOLLOUT | EPOLLET;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, socket_, &ev) < 0) {
        SetErrMsg("epoll ctl mod fial(%s).", strerror(errno));
        return -1;
    }

    for (;;) {
        errno = 0;
        switch(epoll_wait(epoll_fd_, &evs_, 1, rw_time_out_)) {
            case -1:
                if (EINTR != errno) {
                    SetErrMsg("other errno rw epoll wait(%s).", strerror(errno));
                    check_conn_ = false;
                    return -1;
                }
                continue;
            case 0:
                errno = ETIMEDOUT;
                SetErrMsg("rw epoll timeout.");
                return -1;
            default:
                if (evs_.events & EPOLLOUT) {
                    //do send
                    writen(buf, bsize);
                    return 0;
                } else {
                    //黑人问号??
                    SetErrMsg("unkown err for rw epoll_wait.");
                    check_conn_ = false;
                    return -1;
                }
        }
    }
}

int EpollClient::Recv(char *buf, unsigned int &bsize, unsigned int excp_len)
{
    if (NULL == buf) {
        SetErrMsg("recv buffer is null.");
        return -1;
    }

    struct epoll_event ev;
    ev.data.fd = socket_;
    ev.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, socket_, &ev) < 0) {
        SetErrMsg("epoll ctl mod fial(%s).", strerror(errno));
        return -1;
    }

    for (;;) {
        errno = 0;
        switch (epoll_wait(epoll_fd_, &evs_, 1, rw_time_out_)) {
            case -1:
                if (EINTR != errno) {
                    SetErrMsg("other errno rw epoll wait(%s).", strerror(errno));
                    check_conn_ = false;
                    return -1;
                }
                continue;
            case 0:
                errno = ETIMEDOUT;
                SetErrMsg("epoll wait timeout.");
                return -1;
            default:
                if (evs_.events & EPOLLIN) {
                    //do_recv
                    if (excp_len > 0) {
                        bsize = readn(buf, excp_len);
                    } else {
                        bsize = readn(buf, bsize);
                    }
                    return 0;
                } else {
                    //黑人问号??
                    SetErrMsg("unkown err for rw epoll_wait.");
                    check_conn_ = false;
                    return -1;
                }
                //break;
        }
    }

    //return 0;
}

int EpollClient::writen(const void *vptr, unsigned int n)
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

int EpollClient::readn(void *vptr, int nbyes)
{
    int nleft = nbyes;
    int nread = 0;
    char *ptr = static_cast<char *>(vptr);

    while (nleft > 0) {
        if ( (nread = read(socket_, ptr, nleft) ) < 0) {
            if (errno == EINTR)
                nread = 0;
            else
                break;
        } else if (0 == nread)
            break;              // EOF
        nleft -= nread;
        ptr += nread;
    }
    return (nbyes - nleft);
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
        SetErrMsg("ip to socket addr struct fail.");
        return -1;
    }

    bool check_conn = false;
    errno = 0;
    //尝试连接，若服务器与客户端在同一主机上，往往会立即连接完成
    if (connect(socket_, reinterpret_cast<struct sockaddr *>(&svraddr_), sizeof(svraddr_)) < 0) {
        //EINPROGRESS表示连接正在进行中，若errno!=EINPROGRESS
        if (errno != EINPROGRESS) {
            SetErrMsg("connect fail:%s", strerror(errno));
            return -1;
        } else {
            //检查连接是否成功
            check_conn = true;
        }
    }

    if (check_conn) {
        epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
        if (epoll_fd_ < 0) {
            SetErrMsg("create epoll_fd fail:%s.", strerror(errno));
            return -1;
        }

        struct epoll_event ev;
        ev.data.fd = socket_;
        ev.events = EPOLLOUT | EPOLLET;
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, socket_, &ev) < 0) {
            SetErrMsg("epoll event add fail:%s.", strerror(errno));
            return -1;
        }

        for (;;) {
            errno = 0;
            switch(epoll_wait(epoll_fd_, &evs_, 1, connect_time_out_)) {
                case -1:
                    //若被外部中断继续等待
                    if (errno != EINTR) {
                        SetErrMsg("other err:%s", strerror(errno));
                        return -1;
                    }
                    continue;
                case 0:
                    //timeout
                    errno = ETIMEDOUT;
                    SetErrMsg("connect time out");
                    return -1;
                default:
                    //这里只注册了socket_,发生事件也只能是它??
                    //当连接成功时，socket_变为可写，若发生错误，socket_也会变为可读可写，通过获取套接字选项查看是否发生错误
                    if (evs_.events & EPOLLOUT) {
                        int error = 0;
                        socklen_t len = sizeof(error);
                        if (getsockopt(socket_, SOL_SOCKET, SO_ERROR, (void *)(&error), &len) < 0) {
                            SetErrMsg("%s", strerror(errno)); //Solaris pending error
                            return -1;
                        }

                        if (0 != error) {
                            SetErrMsg("getsockopt SOL_SOCKET SO_ERROR, error=%d", error);
                            return -1;
                        }

                        return 0;
                    } else {
                        //黑人问号??
                        SetErrMsg("unkown err");
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
        SetErrMsg("socket fd not init.");
        return -1;
    }

    int flags = fcntl(socket_, F_GETFL, 0);
    if (flags < 0) {
        SetErrMsg("set flag block: get flasg fail.");
        return -1;
    }

    (BLOCK == flag) ? (flags &= ~O_NONBLOCK) : (flags |= O_NONBLOCK);

    if (fcntl(socket_, F_SETFL, flags) < 0) {
        SetErrMsg("set flag block fail.(%d)", flag);
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
            SetErrMsg("get socket socket_ fail.");
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
            SetErrMsg("IP invalid.");
            return -1;
        }
    } else {
        //IPv6 todo
        return -1;
    }

    return 0;
}

void EpollClient::CloseSocket()
{
    if (socket_inited_) {
        close(socket_);
        socket_inited_ = false;
    }

    if (epoll_fd_ >= 0) {
        close(epoll_fd_);
        epoll_fd_ = -1;
    }
}

void EpollClient::SetErrMsg(const char *s, ...)
{
    va_list args;
    va_start(args, s);
    vsnprintf(errmsg_, sizeof(errmsg_), s, args);
    va_end(args);
}
