//
// Created by czllo on 2017/3/12.
//
#include "epoll_client.h"

int main(int argc, char *argv[])
{
    EpollClient cli;

    cli.Init(EpollClient::IPV4, "127.0.0.1", 9877);

    char buf[1024] = "abcd";
    cli.Send(buf, strlen(buf));

    char frbuf[1024] = {0};
    unsigned int len = sizeof(frbuf);
    cli.Recv(frbuf, len);

    cout << frbuf << endl;

    return 0;
}
