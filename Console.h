#ifndef _WIN32_CONSOLE_H_
#define _WIN32_CONSOLE_H_

#include <Windows.h>
#include <stdio.h>

namespace sj
{
class Console
{
	FILE *m_Stdout = NULL;
	FILE *m_Stderr = NULL;
	BOOL m_bAlloc = false;

  public:
	Console()
	{
		m_bAlloc = AllocConsole();
	}
	Console(DWORD dwProcessId)
	{
		m_bAlloc = AttachConsole(dwProcessId);
	}
	~Console()
	{
		if (m_bAlloc)
		{
			FreeConsole();
		}
	}

	FILE *RedirectStdOut(const char *newFile = "CONOUT$")
	{
		FILE* fp = nullptr;
		freopen_s(&fp, newFile, "w", stdout);
		return fp;
	}
	FILE *RedirectStdErr(const char *newFile = "CONOUT$")
	{
		FILE* fp = nullptr;
		freopen_s(&fp, newFile, "w", stderr);
		return fp;
	}
	FILE *RedirectStdIn(const char *newFile = "CONIN$")
	{
		FILE* fp = nullptr;
		freopen_s(&fp, newFile, "r", stdin);
		return fp;
	}
};
} // namespace sj

#endif //_WIN32_CONSOLE_H_