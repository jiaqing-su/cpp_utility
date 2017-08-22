#ifndef _POSIX_THREAD_API_H_
#define _POSIX_THREAD_API_H_

#ifndef __cplusplus
#ifndef inline
#define inline __inline
#endif
#endif

#ifdef WIN32
#include <Windows.h>
#include <process.h>

typedef struct timespec{
	time_t tv_sec;       
	long int tv_nsec;      
}timespec;

typedef struct pthread_t{
	DWORD tid;
	HANDLE hThread;
	void*(*start_routine)(void*);
	void* arg;
	char detach;
}pthread_t;

typedef struct pthread_attr_t{
	int detach;
}pthread_attr_t;

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine) (void *), void *arg);
int pthread_join(pthread_t thread, void **retval);
#define pthread_exit(ret)             _endthreadex((unsigned)ret)
#define pthread_self()                (pthread_t{GetCurrentThreadId})

#define sleep(x)                      Sleep(x*1000)
#define usleep(x)                     Sleep(x/1000)
typedef DWORD pid_t;
#define getpid                        GetCurrentProcessId

/*Thread Local Storage*/
#define __thread __declspec(thread)
typedef DWORD pthread_key_t;
#define pthread_key_create(key,destructor) ((int)(*key=TlsAlloc()))
#define pthread_key_delete(key)  (TlsFree(key)==TRUE?0:(int)GetLastError())
#define pthread_getspecific(key) TlsGetValue(key)
#define pthread_setspecific(key,voidp_val) (TlsSetValue(key,(LPVOID)voidp_val)==TRUE?0:(int)GetLastError())

/*互斥锁*/
#define pthread_mutex_t               CRITICAL_SECTION
#define pthread_mutex_init(mtx, attr) InitializeCriticalSection(mtx)
#define pthread_mutex_destroy(mtx)    DeleteCriticalSection(mtx)
#define pthread_mutex_lock(mtx)       EnterCriticalSection(mtx)
#define pthread_mutex_unlock(mtx)     LeaveCriticalSection(mtx)
#define pthread_mutex_trylock(mtx)    (!TryEnterCriticalSection(mtx))

/*条件变量*/
#define pthread_cond_t                CONDITION_VARIABLE
#define pthread_cond_init(cond,attr)  InitializeConditionVariable(cond)
#define pthread_cond_destroy
#define pthread_cond_wait(cond, mtx)  SleepConditionVariableCS(cond, mtx, INFINITE)
#define pthread_cond_timedwait(cond,mtx, ts)\
                                      SleepConditionVariableCS(cond, mtx, (ts->tv_sec*1000+ts->tv_nsec/1000))
#define pthread_cond_signal(cond)     WakeConditionVariable(cond)
#define pthread_cond_broadcast(cond)  WakeAllConditionVariable(cond)

/*读写锁*/
#define pthread_rwlock_t              SRWLOCK
#define pthread_rwlock_init(rwl)      InitializeSRWLock(rwl)
#define pthread_rwlock_destroy(rwl)       
#define pthread_rwlock_rdlock(rwl)    AcquireSRWLockShared(rwl)
#define pthread_rwlock_wrlock(rwl)    AcquireSRWLockExclusive(rwl)
#define pthread_rwlock_unlock(rwl)    (TryAcquireSRWLockShared(rwl)?ReleaseSRWLockShared(rwl),ReleaseSRWLockShared(rwl):ReleaseSRWLockExclusive(rwl))

/*自旋锁*/
#define pthread_spinlock_t            CRITICAL_SECTION
#define pthread_spin_init(lock, shared) InitializeCriticalSectionAndSpinCount(lock, shared)
#define pthread_spin_destroy(lock)    DeleteCriticalSection(lock)
#define pthread_spin_lock(lock)       EnterCriticalSection(lock)
#define pthread_spin_trylock(lock)    (!TryEnterCriticalSection(lock))
#define pthread_spin_unlock(lock)     LeaveCriticalSection(lock)

#define ATOMIC_INC(x)                 InterlockedIncrement64(x)
#define ATOMIC_DEC(x)                 InterlockedDecrement64(x)

/*信号量*/
#define sem_t                         HANDLE
#ifndef SEM_VALUE_MAX
#define SEM_VALUE_MAX                 (2147483647)
#endif
#define sem_init(sem, shared, init)   !((*sem)=CreateSemaphore(NULL, init, SEM_VALUE_MAX, NULL))
#define sem_destroy(sem)              CloseHandle(*sem)
#define sem_wait(sem)                 WaitForSingleObject(*sem, INFINITE)
#define sem_trywait(sem)              WaitForSingleObject(*sem, 0)
#define sem_timedwait(sem,ts)         WaitForSingleObject(*sem, (ts->tv_sec*1000+ts->tv_nsec/1000))
#define sem_post(sem)                 ReleaseSemaphore(*sem, 1, NULL)

inline unsigned __stdcall win32_thread_proc(void* p)
{
	pthread_t* pth = (pthread_t*)p;
	void* ret = pth->start_routine(pth->arg);
	return (unsigned)ret;
}

inline int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine) (void *), void *arg)
{
	thread->start_routine = start_routine;
	thread->arg = arg;
	thread->hThread = (HANDLE)_beginthreadex(NULL,0,win32_thread_proc, thread,0, (unsigned int*)&thread->tid);
	if(thread->hThread == NULL)
		return GetLastError();
	return 0;
}

inline int pthread_join(pthread_t thread, void **retval)
{
	if(thread.hThread == NULL)
		return 0;

    if(WAIT_OBJECT_0 == WaitForSingleObject(thread.hThread,INFINITE)){
        if(retval != NULL){
			DWORD dwExitCode = 0;
			GetExitCodeThread(thread.hThread, &dwExitCode);
			*((void**)retval) = (void*)dwExitCode;
		}
	}
	
	CloseHandle(thread.hThread);
	thread.hThread = NULL;
	return 0;
}

#else
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <semaphore.h>

#define ATOMIC_INC(x)                 __sync_fetch_and_add(x, 1)
#define ATOMIC_DEC(x)                 __sync_fetch_and_sub(x, 1)
#endif

#endif/*_POSIX_THREAD_API_H_*/
