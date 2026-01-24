#ifndef _PROCESS_HPP_
#define _PROCESS_HPP_

#include <Windows.h>
#include <memory>
#include <string>
#include <vector>

//#define USE_PSAPI

#if defined(USE_PSAPI)
#include <psapi.h>
#pragma comment(lib,"psapi.lib")
#else
#include <tlhelp32.h>
#endif


namespace sj {
	class Process
	{
		HANDLE m_hProcess = NULL;

	public:
		Process() {}
		~Process() {
			Detach();
		}

		Process(const Process&) = delete;
		Process& operator=(const Process&) = delete;

		void Detach() {
			if (m_hProcess) {
				CloseHandle(m_hProcess);
				m_hProcess = NULL;
			}
		}

		unsigned int GetPid() {
			if (m_hProcess) {
				return GetProcessId(m_hProcess);
			}
			return 0;
		}

		bool GetExitCode(unsigned int* dwExitCode) {
			if (m_hProcess) {
				if (GetExitCodeProcess(m_hProcess, (DWORD*)dwExitCode)) {
					return true;
				}
			}
			return false;
		}

		bool WaitExit(unsigned int dwMs = INFINITE) {
			if (m_hProcess) {
				if (WAIT_OBJECT_0 == WaitForSingleObject(m_hProcess, dwMs)) {
					return true;
				}
			}
			return false;
		}

		bool Terminate(unsigned int exitCode = 0) {
			if (m_hProcess) {
				if (TerminateProcess(m_hProcess, exitCode)) {
					CloseHandle(m_hProcess);
					m_hProcess = NULL;
					return true;
				}
			}
			return false;
		}

		bool IsRunning() const {
			bool isRuning = false;
			if (m_hProcess) {
				DWORD waitResult = WaitForSingleObject(m_hProcess, 0);
				isRuning = (waitResult == WAIT_TIMEOUT);
			}
			return isRuning;
		}

		static std::shared_ptr<Process> Create(
			const std::wstring& cmdLine, bool showWindow = false,
			const std::wstring& workDir = L"")
		{
			std::shared_ptr<Process> p;
			std::vector<wchar_t> cmd(cmdLine.begin(), cmdLine.end());
			cmd.push_back(L'\0');

			STARTUPINFO si = { sizeof(STARTUPINFO) };
			PROCESS_INFORMATION pi = { 0 };

			si.dwFlags = STARTF_USESHOWWINDOW;
			si.wShowWindow = showWindow ? SW_SHOW : SW_HIDE;
			DWORD dwCreationFlags = showWindow ? 0 : CREATE_NO_WINDOW;

			auto curWorkDir = workDir.empty() ? NULL : workDir.c_str();

			BOOL bSuccess = CreateProcess(
				NULL,
				cmd.data(),
				NULL,
				NULL,
				NULL,
				dwCreationFlags,
				NULL,
				curWorkDir,
				&si,
				&pi);

			if (bSuccess) {
				CloseHandle(pi.hThread);
				p = std::make_shared<Process>();
				p->m_hProcess = pi.hProcess;
			}
			else {
				const auto dwErr = GetLastError();
				auto wstrErr = L"failed to create process [" + cmdLine + L"], last error = " + std::to_wstring(dwErr) + L"\n";
				OutputDebugStringW(wstrErr.c_str());
			}

			return p;
		}

		static std::shared_ptr<Process> Open(unsigned int dwProcessId, unsigned int additionalAccess = 0) {
			DWORD dwDesiredAccess = PROCESS_TERMINATE | SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION;
			dwDesiredAccess |= additionalAccess;
			HANDLE hProcess = OpenProcess(dwDesiredAccess, FALSE, dwProcessId);
			std::shared_ptr<Process> p;
			if (hProcess) {
				p = std::make_shared<Process>();
				p->m_hProcess = hProcess;
			}
			return p;
		}
		static std::shared_ptr<Process> Open(const std::wstring& name, unsigned int additionalAccess = 0) {
			auto pid = GetProcessIdByName(name);
			return Open(pid, additionalAccess);
		}

		static unsigned int GetCurrentPid() noexcept {
			return GetCurrentProcessId();
		}

		static unsigned int GetProcessIdByName(const std::wstring& name) {
			DWORD dwPid = 0;
			auto targetFileName = name;
			if (targetFileName.rfind(L".exe") == std::wstring::npos) {
				targetFileName += L".exe";
			}

#ifdef USE_PSAPI
			std::vector<DWORD> pids(1024);			
			while (1) {
				DWORD bytesNeeded = 0;
				if (EnumProcesses(pids.data(), static_cast<DWORD>(pids.size() * sizeof(DWORD)), &bytesNeeded)) {
					size_t dwPids = static_cast<size_t>(bytesNeeded / sizeof(DWORD));
					if (dwPids > pids.size()) {
						pids.resize(pids.size() * 2);//数组容量不够
					}
					else {
						pids.resize(dwPids);
						break;
					}
				}
				else {
					DWORD dwErr = GetLastError();
					auto wstrErr = L"failed to call EnumProcesses, last error = " + std::to_wstring(dwErr) + L"\n";
					OutputDebugStringW(wstrErr.c_str());
					break;
				}
			}

			for (auto pid : pids) {
				HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
				if (hProcess) {
					TCHAR tzPath[MAX_PATH] = { 0 };
					DWORD dwLen = GetProcessImageFileName(hProcess, tzPath, MAX_PATH);
					CloseHandle(hProcess);
					if (dwLen > 0) {
						tzPath[dwLen] = 0;
						auto fileName = _tcsrchr(tzPath, _T('\\')) + 1;
						OutputDebugString(fileName);
						OutputDebugString(_T("\n"));
						if (_tcsicmp(fileName, targetFileName.c_str()) == 0) {
							dwPid = pid;
							break;
						}
					}
				}
				else {
					OutputDebugString(L"OpenProcess error\n");
				}
			}

#else
			//CreateToolhelp32Snapshot (Toolhelp)
			HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			if (hSnapshot != INVALID_HANDLE_VALUE) {
				PROCESSENTRY32W pe = { sizeof(PROCESSENTRY32W) };
				if (Process32First(hSnapshot, &pe)) {
					do {
						auto fileName = _tcsrchr(pe.szExeFile, _T('\\'));
						if (fileName) {
							fileName += 1;
						}
						else {
							fileName = pe.szExeFile;
						}
						if (_tcsicmp(fileName, targetFileName.c_str()) == 0) {
							dwPid = pe.th32ProcessID;
							break;
						}

					} while (Process32Next(hSnapshot, &pe));
				}
				CloseHandle(hSnapshot);
			}
#endif

			return dwPid;
		}

	};
}

#endif