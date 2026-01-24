#include "StdAfx.h"
#include "Command.h"
#include <Windows.h>

Command::Command(void)
{
}

Command::~Command(void)
{
}

bool Command::exec(const std::wstring& cmd_str, const wchar_t* cmd_path)
{
    exit_code = -1;
    str_std_err.clear();
    str_std_out.clear();

	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	HANDLE hStdOutRead,hStdOutWrite;
	if (!CreatePipe(&hStdOutRead,&hStdOutWrite,&sa, 1024*1024*100))
	{
		return false;
	}

	HANDLE hStdErrRead,hStdErrWrite;
	if (!CreatePipe(&hStdErrRead,&hStdErrWrite,&sa,0))
	{
		CloseHandle(hStdOutRead);
		CloseHandle(hStdOutWrite);
		return false;
	}

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	si.cb = sizeof(STARTUPINFO);
	GetStartupInfo(&si);
	//si.hStdInput = hStdOutWrite;
	si.hStdError = hStdErrWrite;
	si.hStdOutput = hStdOutWrite;
	si.wShowWindow = SW_HIDE;
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    if (!CreateProcess(cmd_path, (LPWSTR)(cmd_str.c_str()), NULL, NULL, TRUE, NULL, NULL, NULL, &si, &pi))
	{
		CloseHandle(hStdOutRead);
		CloseHandle(hStdErrRead);
		CloseHandle(hStdOutWrite);
		CloseHandle(hStdErrWrite);
		DWORD err = GetLastError();
		return false;
	}

	::WaitForSingleObject(pi.hProcess, 30*1000);
	CloseHandle(hStdOutWrite);
	CloseHandle(hStdErrWrite);

	char buffer[4096] = {0};
	while (true)
	{
		BOOL bOver = FALSE;
		DWORD bytesRead = 0;
		if (ReadFile(hStdOutRead, buffer, 4095, &bytesRead, NULL))
		{
			if(bytesRead > 0)
			{
				buffer[bytesRead] = 0;
				//str_std_out += buffer;		
				str_std_out = buffer;
				bOver = TRUE;
			}
		}

		bytesRead = 0;
		if (ReadFile(hStdErrRead, buffer, 4095, &bytesRead, NULL))
		{
			if(bytesRead > 0)
			{
				buffer[bytesRead] = 0;
				//str_std_err += buffer;		
				str_std_err = buffer;
				bOver = TRUE;
			}
		}

		if(!bOver)
			break;
	}

	
	GetExitCodeProcess(pi.hProcess, (LPDWORD)&exit_code);

	CloseHandle(hStdOutRead);
	CloseHandle(hStdErrRead);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return true; 
}
