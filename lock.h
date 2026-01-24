#ifndef _LOCK_H_
#define _LOCK_H_

#include <windows.h>
#include <assert.h>

class CLockBase
{
public:
	virtual ~CLockBase() {}
	virtual void Lock() = 0;
	virtual void Unlock() = 0;
};

class CLockGuard
{
	CLockBase &m_lock;

public:
	CLockGuard(CLockBase &lock) : m_lock(lock)
	{
		m_lock.Lock();
	}
	~CLockGuard()
	{
		m_lock.Unlock();
	}
};

class CLock : public CLockBase
{
	CRITICAL_SECTION m_cs;
	CLock(const CLock &);

public:
	CLock()
	{
		InitializeCriticalSection(&m_cs);
	}
	~CLock()
	{
		DeleteCriticalSection(&m_cs);
	}
	void Lock()
	{
		EnterCriticalSection(&m_cs);
	}
	void Unlock()
	{
		LeaveCriticalSection(&m_cs);
	}
};

class CMutexLock : public CLockBase
{
	HANDLE m_hMtx;
	CMutexLock(const CMutexLock &) :m_hMtx(NULL) {}
	CMutexLock & operator = (const CMutexLock &) { return *this; }

public:
	CMutexLock()
	{
		m_hMtx = CreateMutex(NULL, FALSE, NULL);
		if (m_hMtx == NULL) throw "CreateMutex";
	}
	CMutexLock(LPCTSTR lpName)
	{
		m_hMtx = CreateMutex(NULL, FALSE, lpName);
		if (m_hMtx == NULL) throw "CreateMutex";
	}
	~CMutexLock()
	{
		if (m_hMtx)
			CloseHandle(m_hMtx);
	}

	void Lock()
	{
		WaitForSingleObject(m_hMtx, INFINITE);
	}
	void Unlock()
	{
		ReleaseMutex(m_hMtx);
	}
};

class CRWLock
{
	friend class CReadLock;
	friend class CWriteLock;
	SRWLOCK m_srwLock;

public:
	CRWLock() :m_srwLock(SRWLOCK_INIT) {}
};

class CReadLock : public CLockBase
{
	SRWLOCK &m_srwLock;

public:
	CReadLock(CRWLock &rwl) : m_srwLock(rwl.m_srwLock) {}
	void Lock()
	{
		AcquireSRWLockShared(&m_srwLock);
	}
	void Unlock()
	{
		ReleaseSRWLockShared(&m_srwLock);
	}
};

class CWriteLock : public CLockBase
{
	SRWLOCK &m_srwLock;

public:
	CWriteLock(CRWLock &rwl) : m_srwLock(rwl.m_srwLock) {}
	void Lock()
	{
		AcquireSRWLockExclusive(&m_srwLock);
	}
	void Unlock()
	{
		ReleaseSRWLockExclusive(&m_srwLock);
	}
};

#endif