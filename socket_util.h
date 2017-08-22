#ifndef _SOCKET_UTIL_H_
#define _SOCKET_UTIL_H_

#ifdef WIN32
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

#else
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>/*inet_ntoa和printf的使用问题*/ 
#include <netinet/in.h>
#include <sys/socket.h>

#define closesocket close
typedef int SOCKET;
#endif

inline int make_socket_non_block(SOCKET fd){
#ifdef WIN32
	unsigned long ulNonBlock = 1;
	return ioctlsocket(fd, FIONBIO, (unsigned long *)&ulNonBlock);
#else
	return fcntl(fd, F_SETFL, O_NONBLOCK);
#endif
}

inline int set_socket_reuse_addr(SOCKET fd, int flag){
	return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&flag, sizeof(int));
}

inline int set_socket_recv_buf(SOCKET fd, int buf_size){
	return setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (const char*)&buf_size, sizeof(int));
}

#endif/*_SOCKET_UTIL_H_*/