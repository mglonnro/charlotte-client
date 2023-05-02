#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include "epoch.h"

unsigned long
getMillisecondsSinceEpoch ()
{
    struct timeval tv;

    gettimeofday (&tv, NULL);

    unsigned long long millisecondsSinceEpoch =
	(unsigned long long) (tv.tv_sec) * 1000 +
	(unsigned long long) (tv.tv_usec) / 1000;

    return millisecondsSinceEpoch;
}

// Allocate exactly 31 bytes for buf
// Credits to: https://stackoverflow.com/questions/8304259/formatting-struct-timespec
int
getUTCTimestamp (char buf[])
{
    const int tmpsize = 21;
    struct timespec now;
    struct tm tm;
    int retval = clock_gettime (CLOCK_REALTIME, &now);
    gmtime_r (&now.tv_sec, &tm);
    strftime (buf, tmpsize, "%Y-%m-%dT%H:%M:%S.", &tm);
    sprintf (buf + tmpsize - 1, "%09luZ", now.tv_nsec);
    return retval;
}
