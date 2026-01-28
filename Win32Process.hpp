#ifndef _PROCESS_HPP_
#define _PROCESS_HPP_

#include <Windows.h>
#include <tchar.h>
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

namespace sjq {
	class Process
	{
		HANDLE m_hProcess = NULL;
		HANDLE m_hPipeToChildRead = NULL;//子进程读->子进程stdin
		HANDLE m_hPipeToChildWrite = NULL;//父进程写
		HANDLE m_hPipeToParentRead = NULL;//父进程读
		HANDLE m_hPipeToParentWrite = NULL;//子进程 stdout/stderr 写
	public:
		Process() {}
		~Process() {
			Detach();
			if (m_hPipeToChildRead)CloseHandle(m_hPipeToChildRead);
			if (m_hPipeToChildWrite)CloseHandle(m_hPipeToChildWrite);
			if (m_hPipeToParentRead)CloseHandle(m_hPipeToParentRead);
			if (m_hPipeToParentWrite)CloseHandle(m_hPipeToParentWrite);
		}

		Process(const Process&) = delete;
		Process& operator=(const Process&) = delete;

		void Detach() {
			if (m_hProcess) {
				CloseHandle(m_hProcess);
				m_hProcess = NULL;
			}
		}

		uint32_t GetPid() {
			if (m_hProcess) {
				return GetProcessId(m_hProcess);
			}
			return 0;
		}

		static uint32_t GetThisPid() {
			return GetCurrentProcessId();
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

		uint32_t PipeRead(char* buf, uint32_t buf_len) {
			DWORD bytesRead = 0;
			if (ReadFile(m_hPipeToParentRead, buf, buf_len, &bytesRead, NULL)) {
			}
			return bytesRead;
		}
		uint32_t PipeWrite(const char* buf, uint32_t buf_len) {
			DWORD bytesWrite = 0;
			if (WriteFile(m_hPipeToChildWrite, buf, buf_len, &bytesWrite, NULL)) {
				WriteFile(m_hPipeToChildWrite, "\n", 1, NULL, NULL);
			}
			return bytesWrite;
		}

		static std::shared_ptr<Process> Create(
			const std::string& cmdLine, bool showWindow = false,
			const std::string& workDir = "", bool withPipe = false) {
			wchar_t wstrCmd[1024] = { 0 };
			MultiByteToWideChar(CP_ACP, 0, cmdLine.c_str(), -1, wstrCmd, 1024);
			wchar_t wstrWorkDir[1024] = { 0 };
			MultiByteToWideChar(CP_ACP, 0, workDir.c_str(), -1, wstrWorkDir, 1024);
			return Process::Create(wstrCmd, showWindow, wstrWorkDir, withPipe);
		}

		static std::shared_ptr<Process> Create(
			const std::wstring& cmdLine, bool showWindow = false,
			const std::wstring& workDir = L"", bool withPipe = false)
		{
			//std::shared_ptr<Process> p;

			STARTUPINFOW si = { sizeof(STARTUPINFOW) };
			PROCESS_INFORMATION pi = { 0 };
			
			std::vector<wchar_t> cmd(cmdLine.begin(), cmdLine.end());
			cmd.push_back(L'\0');

			auto curWorkDir = workDir.empty() ? NULL : workDir.c_str();

			si.dwFlags = STARTF_USESHOWWINDOW;
			si.wShowWindow = showWindow ? SW_SHOW : SW_HIDE;
			DWORD dwCreationFlags = showWindow ? 0 : CREATE_NO_WINDOW;
			BOOL bInheritHandles = FALSE;

			HANDLE hPipeToChildRead = NULL;//子进程读->子进程stdin
			HANDLE hPipeToChildWrite = NULL;//父进程写
			HANDLE hPipeToParentRead = NULL;//父进程读
			HANDLE hPipeToParentWrite = NULL;//子进程 stdout/stderr 写

			do {
				if (withPipe) {
					SECURITY_ATTRIBUTES sa;
					sa.nLength = sizeof(SECURITY_ATTRIBUTES);
					sa.lpSecurityDescriptor = NULL;
					sa.bInheritHandle = TRUE;

					if (!CreatePipe(&hPipeToChildRead, &hPipeToChildWrite, &sa, 0)) {
						break;
					}

					if (!CreatePipe(&hPipeToParentRead, &hPipeToParentWrite, &sa, 0)) {
						break;
					}

					si.hStdInput = hPipeToChildRead;
					si.hStdError = hPipeToParentWrite;
					si.hStdOutput = hPipeToParentWrite;
					si.dwFlags |= STARTF_USESTDHANDLES;
					bInheritHandles = TRUE;
				}

				BOOL bSuccess = CreateProcessW(
					NULL,
					cmd.data(),
					NULL,
					NULL,
					bInheritHandles,
					dwCreationFlags,
					NULL,
					curWorkDir,
					&si,
					&pi);
				if (!bSuccess) {
					const auto dwErr = GetLastError();
					auto wstrErr = L"failed to create process [" + cmdLine + L"], last error = " + std::to_wstring(dwErr) + L"\n";
					OutputDebugStringW(wstrErr.c_str());
					break;
				}

				CloseHandle(pi.hThread);
				auto p = std::make_shared<Process>();
				p->m_hProcess = pi.hProcess;
				p->m_hPipeToChildRead = hPipeToChildRead;
				p->m_hPipeToChildWrite = hPipeToChildWrite;
				p->m_hPipeToParentRead = hPipeToParentRead;
				p->m_hPipeToParentWrite = hPipeToParentWrite;
				return p;

			} while (0);

			if (hPipeToChildRead)CloseHandle(hPipeToChildRead);
			if (hPipeToChildWrite)CloseHandle(hPipeToChildWrite);
			if (hPipeToParentRead)CloseHandle(hPipeToParentRead);
			if (hPipeToParentWrite)CloseHandle(hPipeToParentWrite);

			return nullptr;
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

		static std::shared_ptr<Process> Open(const std::string& name, unsigned int additionalAccess = 0) {
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
		
		static unsigned int GetProcessIdByName(const std::string& name) {
			wchar_t wstrName[MAX_PATH] = { 0 };
			MultiByteToWideChar(CP_ACP, 0, name.c_str(), -1, wstrName, MAX_PATH);
			return GetProcessIdByName(wstrName);
		}
	};
}

#endif