#ifndef _SHARED_MEMORY_HPP_
#define _SHARED_MEMORY_HPP_

#include <Windows.h>
#include <string>
#include <functional>

namespace sjq {
	class SharedMemory
	{
		HANDLE m_hMapping = NULL;
		LPVOID m_pData = NULL;

		SharedMemory(const SharedMemory&) = delete;
		SharedMemory& operator = (const SharedMemory&) = delete;

	public:
		SharedMemory(void) = default;
		~SharedMemory(void) {
			Close();
		}

		void PrintLog(const std::string& log) {
			std::string error = log + ",GetLastError()==" + std::to_string(GetLastError()) + "\n";
			OutputDebugStringA(error.c_str());
		}

		bool Create(const std::string& name, uint32_t size, const std::function<void(void*, uint32_t)>& initCb = nullptr) {
			bool bCrt = false;

			std::string mutexName = name + "_mutex";
			HANDLE hMutex = CreateMutexA(NULL, TRUE, mutexName.c_str());
			if (hMutex == NULL) {
				PrintLog("error: CreateMutexA = NULL");
			}

			do {
				m_hMapping = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, size, name.c_str());
				if (m_hMapping == NULL) {
					PrintLog("error: CreateFileMappingA = NULL");
					break;
				}

				bool bAlreadyExists = (GetLastError() == ERROR_ALREADY_EXISTS);
				m_pData = MapViewOfFile(m_hMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
				if (m_pData == NULL) {
					PrintLog("error: MapViewOfFile = NULL");
					break;
				}

				if (!bAlreadyExists) {
					memset(m_pData, 0, size);
					if (initCb) {
						initCb(m_pData, size);
					}
				}
				bCrt = true;
			} while (0);

			if (!bCrt) {
				Close();
			}

			if (hMutex) {
				ReleaseMutex(hMutex);
				CloseHandle(hMutex);
			}

			return bCrt;
		}
		bool Open(const std::string& name) {
			do {
				m_hMapping = OpenFileMappingA(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, name.c_str());
				if (m_hMapping == NULL) {
					PrintLog("error: OpenFileMappingA = NULL");
					break;
				}

				m_pData = MapViewOfFile(m_hMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
				if (m_pData == NULL) {
					PrintLog("error: MapViewOfFile = NULL");
					break;
				}
				return true;
			} while (0);

			Close();
			return false;
		}

		void Close(void) {
			if (m_pData != NULL) {
				UnmapViewOfFile(m_pData);
				m_pData = NULL;
			}
			if (m_hMapping != NULL) {
				CloseHandle(m_hMapping);
				m_hMapping = NULL;
			}
		}

		void* GetPtr() const {
			return m_pData;
		}
	};
}

#endif//_SHARED_MEMORY_H_