
#ifndef _MY_UTILITY_H_
#define _MY_UTILITY_H_

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#ifdef WIN32
#include <io.h>
#include <direct.h>

#elif defined(__linux__)
#define _access   access
#define _mkdir(x) mkdir(x, 0)

#else
#error unsupported platform
#endif

#ifdef WIN32
#define get_sys_error() (int)GetLastError()
#else
#define get_sys_error() errno
#endif

#ifdef WIN32
#define perror(x) {\
	char buf[512] = {0};\
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,NULL,GetLastError(),\
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),\
                    (LPTSTR) &buf,0,NULL);\
    fprintf(stderr, "%s: %s", x, buf);\
    fflush(stderr);\
}
#endif

int64_t get_system_time(void){
#ifdef WIN32
	return GetTickCount64();
#else
	struct timeb loTimeb;
	ftime(&loTimeb);
	return ((int64_t)loTimeb.time * 1000) + loTimeb.millitm;
#endif
}

int is_dir(const char* path)
{
    if(_access(path, 0) == 0)
    {
        struct stat fd_buf;
        if(stat( path, &fd_buf ) == 0)
        {
            if(S_IFDIR & fd_buf.st_mode)
            {
                return 0;
            }
        }
    }
    return -1;
}

void mkdirs(const char* path)
{
    int i = 0;
    char new_dir[1024];
    for (; (path+i != NULL) && (path[i] != 0); i++)
    {
        if(path[i] == '\\' || path[i] == '/')
        {
            new_dir[i] = 0;
            _mkdir(new_dir);
        }
        new_dir[i] = path[i];
    }
    _mkdir(path);
}

#endif