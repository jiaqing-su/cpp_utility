
#include "log.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <stdarg.h>
#include <io.h>

#ifndef WIN32
#define FOREGROUND_RED    0x0001
#define FOREGROUND_GREEN  0x0002
#define FOREGROUND_BLUE   0x0004
#endif

#ifdef _WIN32
#include <Windows.h>
#include <process.h>
#include <ShlObj.h>
#pragma   comment(lib,   "shell32.lib") 
#define pthread_mutex_t               CRITICAL_SECTION
#define pthread_mutex_init(mtx, attr) InitializeCriticalSection(mtx)
#define pthread_mutex_destroy(mtx)    DeleteCriticalSection(mtx)
#define pthread_mutex_lock(mtx)       EnterCriticalSection(mtx)
#define pthread_mutex_unlock(mtx)     LeaveCriticalSection(mtx)
#define pthread_mutex_trylock(mtx)    (!TryEnterCriticalSection(mtx))
#define pthread_self                  GetCurrentThreadId
#define getpid                        _getpid
#define readlink(opt,buf,len)         GetModuleFileNameA(NULL, buf, len)
#define HLOCK                         HANDLE
#else
#include <unistd.h>
#include <pthread.h>
#define HLOCK                         int
#define _access                       access
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

//#define LOG_SAVE_PATH_AS_EXE 1
#ifndef LOG_SAVE_PATH_AS_EXE
#define LOG_SAVE_PATH_AS_EXE 0
#endif

#if defined(_DEBUG)||defined(DEBUG)
static int opt_mask = LOG_OPT_LEVEL|LOG_OPT_TIME|LOG_OPT_FUNCTION/*|LOG_OPT_FILELINE|LOG_OPT_PID_TID*/;
#else
static int opt_mask = LOG_OPT_LEVEL|LOG_OPT_TIME;
#endif
static int lvl_mask = LOG_LEVEL_TRACE;
static int out_mask = 0xFFFF;
static const char lv_name[][16] = {"[FATAL] ","[ERROR] ","[WARN ] ","[INFO ] ","[DEBUG] ","[TRACE] "};
static char log_file_path[MAX_PATH] = "";
static char log_file_name[MAX_PATH] = "";
static char log_tag[32] = "";

static int pid = 0;
static HLOCK hLock = NULL;

static void uninit_lock(void)
{
    if(hLock)
    {
#ifdef WIN32
        CloseHandle(hLock);
#endif
        hLock = 0;
    }
}

static int init_lock()
{

    if(hLock == NULL)
    {
		char* pName = NULL; 
		char szExeFileName[MAX_PATH] = {0};
		memset(szExeFileName, 0, MAX_PATH);
        readlink( "/proc/self/exe", szExeFileName,  MAX_PATH);
        pid = getpid();
        sprintf_s(szExeFileName+strlen(szExeFileName), sizeof(szExeFileName)-strlen(szExeFileName), "_%d_%d", pid, pid);
		pName = strrchr(szExeFileName, '\\')+1;

#ifdef WIN32
        hLock = CreateMutexA(NULL, FALSE, pName);
#endif
        assert(hLock);

        atexit(uninit_lock);

		*pName = 0;
		strcpy(pName, "log.ini");
		if(_access(szExeFileName, 0) == 0)
		{
			int mask = LOG_OPT_LEVEL|LOG_OPT_TIME;
			int log_lv = GetPrivateProfileIntA("LOG", "level", LOG_LEVEL_INFO, szExeFileName);
			char tzLogCfg[128] = {0};
			GetPrivateProfileStringA("LOG", "mask", "LEVEL|TIME", tzLogCfg, 128, szExeFileName);
			_strupr_s(tzLogCfg, sizeof(tzLogCfg));			
			if(strstr(tzLogCfg, "FILELINE"))
				mask |= LOG_OPT_FILELINE;
			if(strstr(tzLogCfg, "FUNCTION"))
				mask |= LOG_OPT_FUNCTION;

			set_log_level(log_lv);
			set_log_opt_mask(mask);
		}
    }

    return 0;
}

static void lock()
{
#ifdef WIN32
    if(hLock)
        WaitForSingleObject(hLock, INFINITE);
#endif
}

static void unlock()
{
#ifdef WIN32
    if(hLock)
        ReleaseMutex(hLock);
#endif
}

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
	if(path != NULL)
		strcpy_s(log_file_path, sizeof(log_file_path), path);
}

void set_log_file_name(const char* name)
{
	if(name != NULL)
		strcpy_s(log_file_name, sizeof(log_file_name), name);
}

void set_log_tag(const char* tag)
{
    if(tag != NULL)
        strcpy_s(log_tag, sizeof(log_tag), tag);
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
		highlight = 1;
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
	static char last_log_file[MAX_PATH] = {0};
	static struct tm last_tm;
	FILE* fp = NULL;

	if(strlen(log_file_path) == 0 || strlen(log_file_name) == 0)
	{
		int i, len = 0;
		readlink( "/proc/self/exe", last_log_file,  MAX_PATH);

		len = strlen(last_log_file);
		for (i = len -1; i >= 0; i--)
		{
			if(last_log_file[i] == '\\'||last_log_file[i] == '/')
				break;
		}

		if(strlen(log_file_path) == 0)
		{
#if LOG_SAVE_PATH_AS_EXE
			strncpy_s(log_file_path, last_log_file, i);
#else
	#if defined(WIN32)||defined(_WIN32)
			SHGetSpecialFolderPathA(NULL, log_file_path, CSIDL_APPDATA, FALSE);
	#else
			strcpy_s(log_file_path, "/tmp");
	#endif
#endif
		}

		if(strlen(log_file_name) == 0)
		{
			strcpy_s(log_file_name, sizeof(log_file_name), last_log_file+i+1);
		}
	}

	if( last_tm.tm_year != tm_time->tm_year
		||last_tm.tm_mon != tm_time->tm_mon
		||last_tm.tm_mday != tm_time->tm_mday )
	{
		sprintf_s(last_log_file, sizeof(last_log_file), 
			"%s/log/%s_%04d%02d%02d.log", 
			log_file_path, 
			log_file_name, 
			tm_time->tm_year+1900, 
			tm_time->tm_mon+1, 
			tm_time->tm_mday);
		memcpy(&last_tm, tm_time, sizeof(struct tm));
	}

	fopen_s(&fp, last_log_file, "a");
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
	struct tm tmpTM;

    if(level > lvl_mask || level < LOG_LEVEL_FATAL || level > LOG_LEVEL_TRACE)
        return;

	ftime(&tb);
	localtime_s(&tmpTM, &tb.time);

    if(opt_mask&LOG_OPT_LEVEL)
    {
        strcat_s(log_buf, sizeof(log_buf), lv_name[level]);
    }

    if(opt_mask&LOG_OPT_DATE)
    {
        sprintf_s(log_buf + strlen(log_buf), sizeof(log_buf) - strlen(log_buf),
			"%04d-%02d-%02d ",
           tmpTM.tm_year+1900,
           tmpTM.tm_mon+1,
           tmpTM.tm_mday);
    }
	
    if(opt_mask&LOG_OPT_TIME)
    {
        sprintf_s(log_buf + strlen(log_buf), sizeof(log_buf) - strlen(log_buf), 
			"%02d:%02d:%02d:%03d ",
            tmpTM.tm_hour,
            tmpTM.tm_min,
            tmpTM.tm_sec,
			tb.millitm);
    }

    if(strlen(log_tag) > 0)
    {
        strcat_s(log_buf, sizeof(log_buf), "(");
        strcat_s(log_buf, sizeof(log_buf), log_tag);
        strcat_s(log_buf, sizeof(log_buf), ")");
    }

	if(opt_mask&LOG_OPT_PID_TID)
	{
		sprintf_s(log_buf + strlen(log_buf), sizeof(log_buf) - strlen(log_buf), "pid=%d,tid=%d ", getpid(), pthread_self());
	}
	
    if(opt_mask&LOG_OPT_FILELINE)
    {
        sprintf_s(log_buf + strlen(log_buf), sizeof(log_buf) - strlen(log_buf), "%s[%d]|", file, line);
    }

    if(opt_mask&LOG_OPT_FUNCTION)
    {
		strcat_s(log_buf, sizeof(log_buf), func);
		strcat_s(log_buf, sizeof(log_buf), "|");
    }

    /*user msg*/
    {
		char* user_msg = log_buf + strlen(log_buf);
        va_list args;
        va_start( args, format );
        vsprintf_s(user_msg, sizeof(log_buf) - strlen(log_buf), format, args);
        va_end(args);
        strcat_s(log_buf, sizeof(log_buf), "\n");
    }

	if(out_mask&LOG_OUT_VS_DEBUG)
	{
#ifdef _MSC_VER
		OutputDebugStringA(log_buf);
#endif
	}

    init_lock();
	
    if(out_mask&LOG_OUT_CONSOLE)
    {
		lock();
        log_out_to_console(log_buf, level);
        unlock();
    }

    if(out_mask&LOG_OUT_FILE)
    {
		lock();
        log_out_to_file(log_buf, &tmpTM);
		unlock();
    }	
}
