#include <syslog.h>

int main() {
    openlog("syslogmy", LOG_NDELAY|LOG_PID, LOG_USER|LOG_INFO|LOG_WARNING);
    syslog(LOG_INFO, "a log.");
    syslog(LOG_WARNING, "a warning log.");
    syslog(LOG_DEBUG, "DEBUG");
    closelog();

    return 1;
}
