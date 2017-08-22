
#ifndef _NUMA_H_
#define _NUMA_H_

#include <Windows.h>

int numa_set_node(unsigned char numa_node_id)
{
	SYSTEM_INFO sysInfo;
	ULONG ulHighestNodeNumber = 0UL;
	HANDLE hProcess = NULL;
	DWORD dwProcessAffinityMask = 0UL;
	UCHAR ucProcessIndex = 0;

	if(FALSE == ::GetNumaHighestNodeNumber(&ulHighestNodeNumber))
		return -1;

	if(0 == ulHighestNodeNumber)
		return -1;

	::GetSystemInfo(&sysInfo);
	for (; ucProcessIndex < sysInfo.dwNumberOfProcessors; ucProcessIndex++)
	{
		UCHAR ucNumaNode = 0;
		if (::GetNumaProcessorNode (ucProcessIndex, &ucNumaNode))
		{
			if(ucNumaNode == numa_node_id)
			{
				dwProcessAffinityMask |= (1 << ucProcessIndex);
			}
		}
	}

	if(dwProcessAffinityMask)
	{
		hProcess = ::GetCurrentProcess();
		if(TRUE == ::SetProcessAffinityMask(hProcess,dwProcessAffinityMask))
		{
			return 0;
		}
	}

	return -1;
}

//linux
//numactl --membind 1 --cpunodebind 1 --localalloc myapplication

#endif/*_NUMA_H_*/