#ifndef LINUX_STU_EPOLL_CLIENT_H
#define LINUX_STU_EPOLL_CLIENT_H

#include <sys/epoll>

class Epoll_Client {
public:
    Epoll_Client();
    ~Epoll_Client();

    int Get();
private:

}

#endif //LINUX_STU_EPOLL_CLIENT_H