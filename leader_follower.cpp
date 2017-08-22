
#include <sys/types.h>
#include <stdio.h>

#include <socket_util.h>
#include <posix_thread_api.h>
#include <my_utilitys.h>

#include <stdint.h>

#define MAX_FD_NUM 64

volatile int64_t s_SendPackCount = 0;
volatile int64_t s_RecvPackCount = 0;

enum fd_stat
{
	NOTHING,
	ACTIVE,
	PROCESSING,
};

struct fd_item
{
	SOCKET fd;
	enum fd_stat stat;
};

struct fd_item_set
{
	fd_item fds[MAX_FD_NUM];
	int active_index[MAX_FD_NUM];
	int active_count;
	pthread_mutex_t mtx;
	char stop;
};

void wait_to_be_leader(struct fd_item_set* items)
{
	pthread_mutex_lock(&items->mtx);
	//printf("%d is leader\n", gettid());
}

fd_item* get_active_item(struct fd_item_set* items)
{
	int i = items->active_count - 1;
	if(items->active_count <= 0)
		return NULL;

	i = items->active_index[i];
	items->fds[i].stat = PROCESSING;
	items->active_count--;
	return &items->fds[i];
}

void wait_io_event(struct fd_item_set* items)
{
	fd_set fd_rd;
	int cnt = 0;

	while(!items->stop && items->active_count == 0)
	{	
		int i = 0;
		int max_fd = 0;

		FD_ZERO(&fd_rd);

		for (; i < MAX_FD_NUM; i++)
		{
			if(items->fds[i].stat == NOTHING && items->fds[i].fd > 0)
			{
				FD_SET(items->fds[i].fd, &fd_rd);
				if(items->fds[i].fd > (unsigned)max_fd)
					max_fd = items->fds[i].fd;
			}
		}

		if(max_fd == 0)
		{
			sleep(1);
			continue;
		}

		cnt = select(max_fd+1, &fd_rd, NULL, NULL, NULL);
		if(cnt <= 0)
		{
			int dwErr = get_system_error();
			printf("select err = %d\n");
			sleep(1);
			continue;
		}

		for (i = 0; i < MAX_FD_NUM; i++)
		{
			if(items->fds[i].fd <= 0 || items->fds[i].stat != NOTHING)
				continue;

			if(FD_ISSET(items->fds[i].fd, &fd_rd))
			{
				items->fds[i].stat = ACTIVE;
				FD_CLR(items->fds[i].fd, &fd_rd);
				items->active_index[items->active_count++] = i;
			}
		}
	}
}

void discard_and_select_new_leader(struct fd_item_set* items)
{
	pthread_mutex_unlock(&items->mtx);
}

void process_item(struct fd_item* active_item, struct fd_item_set* items)
{
	if(active_item == items->fds)
	{
		int i = 1;
		SOCKET fd = accept(active_item->fd, NULL, NULL);
		for (;i < MAX_FD_NUM; i++)
		{
			if(items->fds[i].fd == 0)
			{
				make_socket_non_block(fd);
				set_socket_recv_buf(fd, 1024*1024*4);

				items->fds[i].fd = fd;
				items->fds[i].stat = NOTHING;
				break;
			}
		}

		if(i >= MAX_FD_NUM)
		{
			printf("no enough fd buf\n");
			closesocket(fd);
		}
	}
	else
	{
		static char buf[1024];
		int len;

		len = recv(active_item->fd, buf, 1024, 0);
		if(len <= 0)
		{
			closesocket(active_item->fd);
			active_item->fd = 0;
		}
		else
		{
			ATOMIC_INC(&s_RecvPackCount);
			int snd = send(active_item->fd, buf, len, 0);
			if(snd > 0)
				ATOMIC_INC(&s_SendPackCount);
		}
	}
}

void become_follower(struct fd_item_set* items)
{
	//printf("%d is follower\n", gettid());
}

void* thread_proc(void* p)
{
	fd_item_set* items = (fd_item_set*)p;

	while(!items->stop)
	{
		wait_to_be_leader(items);

		fd_item* active_item = get_active_item(items);
		if (active_item == 0)
		{
			wait_io_event(items);
			active_item = get_active_item(items);
		}

		active_item->stat = PROCESSING;
		discard_and_select_new_leader(items);

		process_item(active_item, items);

		active_item->stat = NOTHING;
		become_follower(items);
	}

	return 0;
}

void dump()
{
	static int64_t i64LastDumpTime = get_system_time();
	static int64_t i64LastSendPackCount = 0;
	static int64_t i64LastRecvPackCount = 0;

	int64_t i64CurSendPackCount = s_SendPackCount;
	int64_t i64CurRecvPackCount = s_RecvPackCount;
	int64_t i64CurTime = get_system_time();

	double fSendSpd = 1000.f*(i64CurSendPackCount - i64LastSendPackCount)/(i64CurTime - i64LastDumpTime);
	double fRecvSpd = 1000.f*(i64CurRecvPackCount - i64LastRecvPackCount)/(i64CurTime - i64LastDumpTime);

	i64LastDumpTime = i64CurTime;
	i64LastSendPackCount = i64CurSendPackCount;
	i64LastRecvPackCount = i64CurRecvPackCount;

	printf("发包总数:%I64d , 收包总数:%I64d\n", i64CurSendPackCount, i64CurRecvPackCount);
	printf("发包速度:%f 收包速度:%f\n", fSendSpd, fRecvSpd);
	printf("每秒丢包:%f 丢包率：%f%%\n", fSendSpd - fRecvSpd, 100.0f*(fSendSpd-fRecvSpd)/fSendSpd);
}

void start_work()
{
	fd_item_set* items = new fd_item_set;
	int ncpus = 4;
	int i = 0;
	struct sockaddr_in addr;
	pthread_t thds[4];

	for (i = 0; i < MAX_FD_NUM; i++)
	{
		items->fds[i].fd = 0;
		items->fds[i].stat = NOTHING;
		items->active_index[i] = 0;
	}

	items->active_count = 0;

	items->fds[0].fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	set_socket_reuse_addr(items->fds[0].fd, 1);

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(60000);
	if(::bind(items->fds[0].fd, (const sockaddr*)&addr, (int)sizeof(addr)))
	{
		return;
	}

	if(::listen(items->fds[0].fd, SOMAXCONN))
	{
		return;
	}	

	pthread_mutex_init(&items->mtx, NULL);

	items->stop = 0;

	for (i = 0; i < ncpus; ++i)
	{
		pthread_create(thds+i, NULL, thread_proc, items);
	}

	while(1)
	{
		sleep(3);
		//dump();
	}

	for (i = 0; i < ncpus; ++i)
	{
		pthread_join(thds[i], NULL);
	}

	//pthread_mutex_destroy(&items.mtx);

	//getchar();
}