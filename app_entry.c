
#include "app_entry.h"
#include <signal.h>
#include <stdio.h>

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

#include <DbgHelp.h>
#pragma comment(lib, "DbgHelp.lib")

#include <process.h>
#define getpid _getpid

#ifdef _DEBUG
#define CRTDBG_MAP_ALLOC    
#include <stdlib.h>    
#include <crtdbg.h> 
#endif

#define sleep(x) Sleep(1000*x)

#endif/*_WIN32*/

#define MAX_QUIT_WAIT_TIME 3/*sec*/ 

static int stop = 0;
static struct APP_ENV env;
const  struct APP_ENV* app = &env;

extern int  _main(int argc, char** argv);

void signal_proc(int)
{
	int wait_time = 0;
	
	/*notify user app need to quit*/
	env.state = 0;
	
	/*wait _main return*/
	for(; !stop&&wait_time<MAX_QUIT_WAIT_TIME; wait_time++)
	{
		sleep(1);
	}
	
	if(wait_time == MAX_QUIT_WAIT_TIME*1000)
	{
		env.state = -1;
		exit(1);
	}
}

void on_exit()
{
	if(env.state == -1)
	{
		fprintf(stderr, "app didn't quit in time, sys force killed it\n");
	}
}

#ifdef _WIN32
LONG WINAPI MyUnhandledExceptionFilter(struct _EXCEPTION_POINTERS * apExceptionInfo)
{
	char szDumpFileName[1024] = { 0 };
	HANDLE lhDumpFile = NULL;
	MINIDUMP_TYPE lemMdT;
	MINIDUMP_EXCEPTION_INFORMATION loExceptionInfo;

	{
		SYSTEMTIME sys_time;
		GetLocalTime(&sys_time);
		sprintf(szDumpFileName, "%s_%04d_%02d_%02d_%02d_%02d_%02d.dmp",
			env.argv[0], sys_time.wYear, sys_time.wMonth, sys_time.wDay, 
			sys_time.wHour, sys_time.wMinute, sys_time.wSecond);
	}
	
	//释放本进行重新启动信息
	lhDumpFile = ::CreateFileA(szDumpFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(lhDumpFile == NULL)
	{
		return EXCEPTION_EXECUTE_HANDLER;
	}

	lemMdT = (MINIDUMP_TYPE)(//MiniDumpWithFullMemory | 
		MiniDumpWithFullMemoryInfo | 
		MiniDumpWithHandleData | 
		MiniDumpWithThreadInfo | 
		MiniDumpWithUnloadedModules);

	loExceptionInfo.ExceptionPointers = apExceptionInfo;
	loExceptionInfo.ThreadId = ::GetCurrentThreadId();
	loExceptionInfo.ClientPointers = TRUE;
	::MiniDumpWriteDump(::GetCurrentProcess(), ::GetCurrentProcessId(), lhDumpFile, lemMdT, &loExceptionInfo, NULL, NULL);
	::CloseHandle(lhDumpFile);
	::ExitProcess(0);
	return EXCEPTION_EXECUTE_HANDLER;
}

BOOL WINAPI WindowsConsoleControlHandler(DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		{
			signal_proc(dwCtrlType);
			break;
		}
	default:
		break;
	}

	return FALSE;
}

#endif


void show_version()
{
	printf("%s\n", env.argv[0]);

#ifdef _DEBUG
	printf("build at %s %s\n",__TIME__,__DATE__);
#endif

	if(env.ver == NULL)
	{
	    printf("ver %d.%d.%d\n", _VERSION_MAJOR, _VERSION_MINOR, _REVISION);
	}
	else
	{
		printf(env.ver);
	}
}

int main(int argc, char** argv)
{
	int ret = 0;

	env.argc = argc;
	env.argv = argv;
	env.pid  = getpid();
	env.state = 1;

	signal(SIGINT, signal_proc);/*ctrl+c*/
	signal(SIGBREAK, signal_proc);
	signal(SIGTERM, signal_proc);
	signal(SIGABRT, signal_proc);

	atexit(on_exit);

	int stdio_deal_type = 0;
	if(stdio_deal_type == 1)
	{
		fclose(stdout);
		fclose(stderr);
	}
	else if(stdio_deal_type == 2)
	{
		freopen("stdout.txt", "a+", stdout);
		freopen("stderr.txt", "a+", stderr);
	}

	/*set log level*/

	show_version();

#ifdef _WIN32
	{
		WSADATA wsaData;
		WSAStartup(0x0202, &wsaData);
		::SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);
		::SetConsoleCtrlHandler(WindowsConsoleControlHandler, TRUE);
	}
#endif	

	ret = _main(argc, argv);
	stop = 1;
	env.state = 0;
	
#ifdef _WIN32
	WSACleanup();
	::SetConsoleCtrlHandler(WindowsConsoleControlHandler, FALSE);
#ifdef _DEBUG
	_CrtDumpMemoryLeaks();
#endif
#endif/*_WIN32*/	
	
	return ret;
}

