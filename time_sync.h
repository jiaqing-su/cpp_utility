#ifndef _TIME_SYNC_H_
#define _TIME_SYNC_H_
#include <stdint.h>

/*
@param  time_svr : time.windows.com | time.nist.gov | ntp.ntsc.ac.cn
@param  timeout (millisecs)
@return millisecs from 1970.1.1, <=0 is error
*/
int64_t get_utc_time(const char *time_svr, long timeout_ms);

/*
@param  millisecs from 1970.1.1
@return 0 is ok
*/
int set_local_time(int64_t millisecs);

#endif /*_TIME_SYNC_H_*/