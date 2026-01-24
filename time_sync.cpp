#include "stdafx.h"
#ifdef WIN32
#include <WinSock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <netdb.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define  closesocket(s) close(s)
#endif
#include <time.h>
#include <stdio.h>
#include <sys/timeb.h>
#include "time_sync.h"

#define UTC_NTP 2208988800U /* 1970 - 1900 */

typedef struct
{
	unsigned int Control_Word;
	unsigned int root_delay;
	unsigned int root_dispersion;
	unsigned int reference_identifier;
	unsigned int reference_timestamp_seconds;
	unsigned int reference_timestamp_fractions;
	unsigned int originate_timestamp_seconds;
	unsigned int originate_timestamp_fractions;
	unsigned int receive_timestamp_seconds;
	unsigned int receive_timestamp_fractions;
	unsigned int transmit_timestamp_seconds;
	unsigned int transmit_timestamp_fractions;
} NTP;

static int init_socket()
{
#ifdef WIN32
	WORD wVersionRequested;
	WSADATA wsaData;

	wVersionRequested = MAKEWORD(1, 1);
	if (WSAStartup(wVersionRequested, &wsaData))
	{
		return -1;
	}
	if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1)
	{
		WSACleanup();
		return -2;
	}
#endif // WIN32
	return 0;
}

static void uninit_socket()
{
#ifdef WIN32
	WSACleanup();
#endif // WIN32
}

static SOCKET open_connect(const char *server)
{
	SOCKET s = -1;
	struct addrinfo *saddr = NULL;

	do
	{
		if (getaddrinfo(server, "123", NULL, &saddr))
		{
			perror("Server address not correct.");
			break;
		}

		s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (s == -1)
		{
			perror("Can not create socket.");
			break;
		}

		if (connect(s, saddr->ai_addr, (int)saddr->ai_addrlen))
		{
			perror("Connect error");
			closesocket(s);
			s = -1;
			break;
		}
	} while (0);

	if (saddr)
		freeaddrinfo(saddr);

	return s;
}

static int ntp_send_and_recv(NTP *NTP_Send, NTP *NTP_Recv, const char *time_svr, long timeout_ms)
{
	int err = -1;
	int len = sizeof(NTP), n = 0;
	fd_set fdread;
	struct timeval tv = {30, timeout_ms * 1000};
	SOCKET fd = 0;

	init_socket();

	do
	{
		fd = open_connect(time_svr);
		if (fd <= 0)
			break;

		n = send(fd, (const char *)NTP_Send, len, 0);
		if (n != len)
		{
			perror("send error");
			break;
		}

		FD_ZERO(&fdread);
		FD_SET(fd, &fdread);
		n = select(0, &fdread, NULL, NULL, &tv);
		if (n == 0)
		{
			perror("recv.time out");
			break;
		}

		n = recv(fd, (char *)NTP_Recv, len, 0);
		if (n != len)
		{
			perror("didn't recv enough bytes(48)");
			break;
		}

		err = 0;

	} while (0);

	if (fd > 0)
	{
		closesocket(fd);
	}

	uninit_socket();

	return err;
}

int64_t get_utc_time(const char *time_svr, long timeout_ms)
{
	struct timeb tb;
	double t4, t1, t2, t3, delay;
	NTP ntp;
	unsigned char *p;
	int64_t millisecs = 0;

	ntp.Control_Word = 0;
	ntp.root_delay = 0;
	ntp.root_dispersion = 0;
	ntp.reference_identifier = 0;
	ntp.reference_timestamp_seconds = 0;
	ntp.reference_timestamp_fractions = 0;
	ntp.originate_timestamp_seconds = 0;
	ntp.originate_timestamp_fractions = 0;
	ntp.receive_timestamp_seconds = 0;
	ntp.receive_timestamp_fractions = 0;
	ntp.transmit_timestamp_seconds = 0;
	ntp.transmit_timestamp_fractions = 0;

	p = (unsigned char *)&ntp;
	p[0] = 0x23;

	ftime(&tb);
	t1 = (double)tb.time * 1000.0f + tb.millitm;
	ntp.transmit_timestamp_seconds = (unsigned int)htonl((unsigned long)tb.time + UTC_NTP);
	ntp.transmit_timestamp_fractions = 0;

	if (ntp_send_and_recv(&ntp, &ntp, time_svr, timeout_ms))
		return -1;

	ftime(&tb);
	t4 = (double)tb.time * 1000.0f + tb.millitm;

	t2 = (double)ntohl(ntp.receive_timestamp_seconds) * 1000.0f +
		 (double)ntohl(ntp.receive_timestamp_fractions) * 0.000000000200f * 1000.0f;

	t3 = (double)ntohl(ntp.transmit_timestamp_seconds) * 1000.0f +
		 (double)ntohl(ntp.transmit_timestamp_fractions) * 0.000000000200f * 1000.0f;

	delay = ((t4 - t1) - (t3 - t2)) / 2;

	millisecs = ntohl(ntp.transmit_timestamp_seconds) - UTC_NTP;
	millisecs *= 1000;
	millisecs += (int64_t)(delay);

	return millisecs;
}

int set_local_time(int64_t millisecs)
{
	int err = -1;
#ifdef WIN32
	struct tm ptm;
	SYSTEMTIME newtime;
	time_t tt = millisecs / 1000;

	localtime_s(&ptm, &tt);

	newtime.wYear = ptm.tm_year + 1900;
	newtime.wMonth = ptm.tm_mon + 1;
	newtime.wDayOfWeek = ptm.tm_wday;
	newtime.wDay = ptm.tm_mday;
	newtime.wHour = ptm.tm_hour;
	newtime.wMinute = ptm.tm_min;
	newtime.wSecond = ptm.tm_sec;
	newtime.wMilliseconds = (WORD)(millisecs - tt * 1000);
	XLOG_DEBUG(L"time_sync %04d-%02d-%02d %02d:%02d:%02d", newtime.wYear, newtime.wMonth, newtime.wDay, newtime.wHour, newtime.wMinute, newtime.wSecond);
	if (TRUE == SetLocalTime(&newtime)) {
		err = 0;
	}
	else {
		XLOG_ERROR(L"time_sync failed")
	}
#else
	struct timeval tv;
	tv.tv_sec = millisecs / 1000;
	tv.tv_usec = (millisecs - tv.tv_sec * 1000) * 1000;
	err = settimeofday(&tv, NULL);
#endif // WIN32

	//printf_s("%04d-%02d-%02d %02d:%02d:%02d:%03d\n",
	//    newtime.wYear, newtime.wMonth, newtime.wDay,
	//    newtime.wHour, newtime.wMinute, newtime.wSecond, newtime.wMilliseconds);

	return err;
}