
#include "log.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/timeb.h>
#include <stdarg.h>

#ifndef WIN32
#define FOREGROUND_RED    0x0001
#define FOREGROUND_GREEN  0x0002
#define FOREGROUND_BLUE   0x0004
#endif

#ifdef _WIN32
#include <Windows.h>
#define pthread_mutex_t               CRITICAL_SECTION
#define pthread_mutex_init(mtx, attr) InitializeCriticalSection(mtx)
#define pthread_mutex_destroy(mtx)    DeleteCriticalSection(mtx)
#define pthread_mutex_lock(mtx)       EnterCriticalSection(mtx)
#define pthread_mutex_unlock(mtx)     LeaveCriticalSection(mtx)
#define pthread_mutex_trylock(mtx)    (!TryEnterCriticalSection(mtx))
#define pthread_self                  GetCurrentThreadId
#define getpid                        GetCurrentProcessId
#else
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#endif

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

static unsigned int opt_mask = LOG_OPT_LEVEL|LOG_OPT_TIME/*|LOG_OPT_FUNCTION|LOG_OPT_PID_TID*/;
static unsigned int lvl_mask = LOG_LEVEL_TRACE;
static unsigned int out_mask = 0xFFFF;
static const char lv_name[][16] = {"[FATAL] ","[ERROR] ","[WARN ] ","[INFO ] ","[DEBUG] ","[TRACE] "};
static char log_file_path[MAX_PATH] = "";
static char log_file_name[MAX_PATH] = "";

static int mutex_init = 0;
static pthread_mutex_t mtx;

void set_log_opt_mask(unsigned int mask)
{
    opt_mask = mask;
}

void set_log_level(int lv)
{
    lvl_mask = min(lv, LOG_LEVEL_TRACE);
}

void set_log_out_mask(unsigned int mask)
{
    out_mask = mask;
}

void set_log_file_path(const char* path)
{
	if(path != NULL && strlen(log_file_path) == 0)
		strcpy(log_file_path, path);
}

void set_log_file_name(const char* name)
{
	if(name != NULL && strlen(log_file_name) == 0)
		strcpy(log_file_name, name);
}

static void log_out_to_console(const char* log_buf, int level)
{
    int highlight = 0;
    unsigned short wColor = FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE;
    switch(level)
    {
    case LOG_LEVEL_FATAL:
        wColor = FOREGROUND_RED;
        highlight = 1;
        break;
    case LOG_LEVEL_ERROR:
        wColor = FOREGROUND_RED;
        break;
    case LOG_LEVEL_WARNING:
        wColor = FOREGROUND_RED|FOREGROUND_GREEN;
        break;
    case LOG_LEVEL_INFO:
        break;
    case LOG_LEVEL_DEBUG:
        wColor = FOREGROUND_RED|FOREGROUND_BLUE;
        break;
    case LOG_LEVEL_TRACE:
        wColor = FOREGROUND_BLUE;
        break;
    default:
        break;
    }

#ifdef WIN32
    if(highlight)
        wColor |= FOREGROUND_INTENSITY;
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), wColor);
    printf("%s", log_buf);
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE);
#else
    wColor += 30;
    printf("\x1b[%d;%dm%s\x1b[%dm", 40, wColor, log_buf, highlight);
    printf("\x1b[%dm", 0);
#endif
}

static void log_out_to_file(const char* log_buf, struct tm* tm_time)
{
	static char last_log_file[MAX_PATH];
	static struct tm last_tm;
	FILE* fp = NULL;

	if(strlen(log_file_path) == 0 || strlen(log_file_name) == 0)
	{
		int i, len = 0;
#ifdef WIN32
		GetModuleFileNameA(NULL, last_log_file, MAX_PATH);
#else
		readlink( "/proc/self/exe", last_log_file,  MAX_PATH);
#endif
		len = strlen(last_log_file);
		for (i = len -1; i >= 0; i--)
		{
			if(last_log_file[i] == '\\'||last_log_file[i] == '/')
				break;
		}

		if(strlen(log_file_path) == 0)
		{
			strncpy(log_file_path, last_log_file, i);
		}

		if(strlen(log_file_name) == 0)
		{
			strcpy(log_file_name, last_log_file+i+1);
		}
	}

	if(last_tm.tm_year != tm_time->tm_year||
		last_tm.tm_mon != tm_time->tm_mon||
		last_tm.tm_mday != tm_time->tm_mday)
	{
		sprintf(last_log_file, "%s/log/%s_%04d%02d%02d.log", 
			log_file_path, 
			log_file_name, 
			tm_time->tm_year+1900, 
			tm_time->tm_mon+1, 
			tm_time->tm_mday);
		memcpy(&last_tm, tm_time, sizeof(struct tm));
	}

	fp = fopen(last_log_file, "a");
	if(fp)
	{
		fwrite(log_buf, strlen(log_buf), 1, fp);
		fclose(fp);
	}
}

void log_print(int level, const char* file, int line, const char* func, const char* format, ...)
{
	char log_buf[2048] = {0};
	struct timeb tb;
	struct tm* ptm = NULL;

    if(level > lvl_mask || level < LOG_LEVEL_FATAL || level > LOG_LEVEL_TRACE)
        return;
	
	ftime(&tb);
    ptm = localtime(&tb.time);

    if(opt_mask&LOG_OPT_LEVEL)
    {
        strcat(log_buf, lv_name[level]);
    }

    if(opt_mask&LOG_OPT_DATE)
    {
		char* time_buf = log_buf + strlen(log_buf);
        sprintf(time_buf, "%04d-%02d-%02d ",
                ptm->tm_year+1900,
                ptm->tm_mon+1,
                ptm->tm_mday);
    }
	
    if(opt_mask&LOG_OPT_TIME)
    {
		char* time_buf = log_buf + strlen(log_buf);
        sprintf(time_buf, "%02d:%02d:%02d:%03d ",
                ptm->tm_hour,
                ptm->tm_min,
                ptm->tm_sec,
				tb.millitm);
    }

	if(opt_mask&LOG_OPT_PID_TID)
	{
		char* pid_tid = log_buf + strlen(log_buf);
		sprintf(pid_tid, "pid=%d,tid=%d ", getpid(), pthread_self());
	}
	
    if(opt_mask&LOG_OPT_FILELINE)
    {
        char* line_buf = log_buf + strlen(log_buf);
        const char* p1 = strrchr(file, '\\');
        const char* p2 = strrchr(file, '/');
        const char* p  = max(p1, p2) + 1;
		if(p == NULL)
			p = file;
        sprintf(line_buf, "%s[%d]|", p, line);
    }

    if(opt_mask&LOG_OPT_FUNCTION)
    {
        char* func_buf = log_buf + strlen(log_buf);
        sprintf(func_buf, "%s|", func);
    }

    /*user msg*/
    {
		char* user_msg = log_buf + strlen(log_buf);
        va_list args;
        va_start( args, format );
        vsprintf(user_msg, format, args);
        va_end(args);
        strcat(log_buf, "\n");
    }

	if(out_mask&LOG_OUT_VS_DEBUG)
	{
#ifdef _MSC_VER
		OutputDebugStringA(log_buf);
#endif
	}

	if (!mutex_init)
		pthread_mutex_init(&mtx, NULL);

	pthread_mutex_lock(&mtx);
    if(out_mask&LOG_OUT_CONSOLE)
    {
        log_out_to_console(log_buf, level);
    }

    if(out_mask&LOG_OUT_FILE)
    {
        log_out_to_file(log_buf, ptm);
    }
	pthread_mutex_unlock(&mtx);
}

