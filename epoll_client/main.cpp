//
// Created by czllo on 2017/3/12.
//
#include "epoll_client.h"
#include <iostream>

using namespace std;

int main(int argc, char *argv[])
{
    EpollClient cli;

    cout << argv[0] << endl;

    if (cli.Init("127.0.0.1", 9877) < 0) {
        cout << cli.GetErrMsg() << endl;
        return -1;
    }

    cli.set_rw_time_out(0);

	int cnt = 0;
	for (;;) {
		cout << "===" << cnt << "===" << endl;
		char buf[1024] = "abcd";
		snprintf(buf, sizeof(buf), "abcd%d", cnt++);
		if (cli.Send(buf, strlen(buf)) != 0) {
            cout << cli.GetErrMsg() << endl;
        }

		char frbuf[1024] = {0};
		unsigned int len = sizeof(frbuf);
		if (cli.Recv(frbuf, len) == 0) {
			cout << frbuf << ":";
			cout << len << endl;
            //do_echo
		} else {
            cout << cli.GetErrMsg() << endl;
        }
		sleep(1);
	}
    return 0;
}
