#ifndef _SHARED_MEMORY_H_
#define _SHARED_MEMORY_H_

#include <Windows.h>
#include <tchar.h>

class CSharedMemory
{
    HANDLE m_hMapping = NULL;
    LPVOID m_pData = NULL;
private:
    CSharedMemory(const CSharedMemory&) {}
    CSharedMemory& operator = (const CSharedMemory&) {}
public:
    CSharedMemory(void) = default;
    ~CSharedMemory(void)
    {
        Close();
    }

    // lpName can use prefix "Global\\" to create global object
	bool Open(LPCTSTR lpName, unsigned int dwSize)
	{
        bool bCrt = false;

        TCHAR tzMtxName[MAX_PATH] = _T("");        
        _tcscat_s(tzMtxName, lpName);
        _tcscat_s(tzMtxName, L"_MUTEX");
        HANDLE hMutex = ::CreateMutex(NULL, TRUE, tzMtxName);

        m_hMapping = ::CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, dwSize, lpName);
        if (m_hMapping != NULL)
        {
            BOOL bAlreadyExists = (GetLastError() == ERROR_ALREADY_EXISTS);
            m_pData = ::MapViewOfFile(m_hMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
            if (m_pData != NULL)
            {
                if (!bAlreadyExists)
                {
                    ZeroMemory(m_pData, dwSize);
                }
                bCrt = true;
            }
            else
            {
                ::CloseHandle(m_hMapping);
                m_hMapping = NULL;
            }
        }
        //DWORD dw = GetLastError();

        if (hMutex)
        {
            ::ReleaseMutex(hMutex);
            ::CloseHandle(hMutex);
        }

        return bCrt;
	}

	void Close(void)
	{
		if (m_pData != NULL)
		{
			::UnmapViewOfFile(m_pData);
			m_pData = NULL;
		}
		if (m_hMapping != NULL)
		{
			::CloseHandle(m_hMapping);
			m_hMapping = NULL;
		}
	}

	void* GetPtr() { return m_pData; }
};

#endif//_SHARED_MEMORY_H_