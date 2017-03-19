//
// Created by czllo on 2017/3/12.
//
#include "epoll_client.h"
#include <iostream>

using namespace std;

int main(int argc, char *argv[])
{
    EpollClient cli;

    if (cli.Init("127.0.0.1", 9877) < 0) {
        cout << cli.GetErrMsg() << endl;
        return -1;
    }

	int cnt = 0;
	for (;;) {
		cout << "===" << cnt << "===" << endl;
		char buf[1024] = "abcd";
		snprintf(buf, sizeof(buf), "abcd%d", cnt++);
		cli.Send(buf, strlen(buf));

		char frbuf[1024] = {0};
		unsigned int len = sizeof(frbuf);
		cli.set_rw_time_out(0);
		if (cli.Recv(frbuf, len) == 0) {
			cout << frbuf << ":";
			cout << len << endl;
            //do_echo
		}
		sleep(1);
	}
    return 0;
}
