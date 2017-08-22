
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>

#ifndef WIN32
#include <unistd.h>
#include <arpa/inet.h>/*inet_ntoa和printf的使用问题*/ 
#include <netinet/in.h>
#include <sys/socket.h>
#define closesocket close
typedef int SOCKET;
#else
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
typedef int socklen_t;
#endif

typedef int BOOL;
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef SOCKET_ERROR
#define SOCKET_ERROR (-1)
#endif

#define BUFFER_SIZE  1024
#define DEFAULT_PORT 60000

typedef enum {
	SOCK_ERROR_OK,
	SOCK_ERROR_INVALID_ARG,
	SOCK_ERROR_SOCKET,
	SOCK_ERROR_SET_OPT,
	SOCK_ERROR_BIND,
	SOCK_ERROR_LISTEN,
	SOCK_ERROR_ACCEPT,
	SOCK_ERROR_CONNECT,
	SOCK_ERROR_SEND,
	SOCK_ERROR_RECV,
}SOCK_ERROR_CODE;

static void sock_error(const char* msg)
{
#ifdef WIN32
	LPVOID lpMsgBuf;
	printf(msg);
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR) &lpMsgBuf,
		0, NULL );
	printf("%s", lpMsgBuf);
	LocalFree(lpMsgBuf);
#else
	perror(msg);
#endif
}

void usage(){
	printf("Usage:\n");
	printf("\targv[1] = ip\n");
	printf("\targv[2] = port\n");
	printf("\targv[3] = ts|tc|us|uc, t=tcp,u=udp,s=server,c=client\n");
	printf("\targv[4] = remote ip, only uc\n");
	printf("\targv[5] = remote port, only uc\n");
}


int main(int argc, char* argv[])
{
	int err = SOCK_ERROR_OK;
	SOCKET clt_fd = 0;
	SOCKET svr_fd = 0;
	BOOL is_tcp = TRUE;
	BOOL is_svr = TRUE;
	char buf[BUFFER_SIZE] = "";
	int len = 0;
	struct sockaddr_in addr;
	socklen_t addr_len = sizeof(struct sockaddr_in);

	addr.sin_family = AF_INET;
	if(argc > 2){
		addr.sin_addr.s_addr = inet_addr(argv[1]);//INADDR_ANY;
		addr.sin_port = htons((u_short)atoi(argv[2]));
	}else{
		addr.sin_addr.s_addr = INADDR_ANY;
		addr.sin_port = htons(DEFAULT_PORT);
	}

	if(argc > 3){
		if(strcmp(argv[3], "ts") == 0){
			is_tcp = TRUE;
			is_svr = TRUE;
		}else if(strcmp(argv[3], "tc") == 0){
			is_tcp = TRUE;
			is_svr = FALSE;
		}else if(strcmp(argv[3], "us") == 0){
			is_tcp = FALSE;
			is_svr = TRUE;
		}else if(strcmp(argv[3], "uc") == 0){
			is_tcp = FALSE;
			is_svr = FALSE;
			if(argc < 6){
				usage();
				return SOCK_ERROR_INVALID_ARG;
			}
		}else{
			usage();
			return SOCK_ERROR_INVALID_ARG;
		}
	}

	printf("working as %s %s %s:%d\n",is_tcp?"tcp":"udp", is_svr?"server":"client", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
#ifdef WIN32
	{WSADATA wsaData;
	WSAStartup(WINSOCK_VERSION, &wsaData);}
#endif
	svr_fd = socket(AF_INET, is_tcp?SOCK_STREAM:SOCK_DGRAM, is_tcp?IPPROTO_TCP:IPPROTO_UDP);
	if(svr_fd == -1){
		sock_error("socket");
		err = SOCK_ERROR_SOCKET;
		goto END;
	}

	if(is_svr || !is_tcp){
		int opt = 1;
		if(setsockopt(svr_fd, SOL_SOCKET, SO_REUSEADDR, (const char *) &opt, sizeof(opt))){
			sock_error("setsockopt");
			err = SOCK_ERROR_SET_OPT;
			goto END;
		}

		if(bind(svr_fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in))){
			sock_error("bind");
			err = SOCK_ERROR_BIND;
			goto END;
		}
	}

	if(is_tcp){
		if(is_svr){
			if(listen(svr_fd, 3)){
				sock_error("listen");
				err = SOCK_ERROR_LISTEN;
				goto END;
			}
ACCEPT:
			clt_fd = accept(svr_fd, (struct sockaddr*)&addr, &addr_len);
			if(clt_fd == SOCKET_ERROR){
				sock_error("accept");
				err = SOCK_ERROR_ACCEPT;
			}else{
				printf("accept %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
			}
		}else{
			if(connect(svr_fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in))){
				sock_error("connect");
			}else{
				clt_fd = svr_fd;
				svr_fd = 0;
				printf("connect %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
			}
		}
	}else{
		clt_fd = svr_fd;
		svr_fd = 0;
		if(!is_svr){
			addr.sin_addr.s_addr = inet_addr(argv[4]);
			addr.sin_port = htons((u_short)atoi(argv[5]));
		}
	}

	while(clt_fd > 0){
		if(!is_svr){
			printf("\ninput:");
			fgets(buf, BUFFER_SIZE, stdin);
			if(strcmp(buf, "q") == 0)
				break;
SEND:
			len = strlen(buf);
			if(is_tcp){
				len = send(clt_fd, buf, len, 0);
			}else{
				len = sendto(clt_fd, buf, len , 0,  (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
			}

			if(len < 0){
				sock_error(is_tcp?"send":"sendto");
				err = SOCK_ERROR_SEND;
				break;
			}
		}

		if(is_tcp){
			len = recv(clt_fd, buf, BUFFER_SIZE, 0);
		}else{
			len = recvfrom(clt_fd, buf, BUFFER_SIZE, 0, (struct sockaddr*)&addr, &addr_len);
		}

		if(len <= 0){
			if(len == 0 && is_tcp){
				printf("disconnect\n");
			}else{
				sock_error(is_tcp?"recv":"recvfrom");
				err = SOCK_ERROR_RECV;
			}
			break;
		}

		buf[len] = 0;
		printf("recv:%s\n", buf);

		if(is_svr){
			goto SEND;
		}
	}

	if(is_svr && is_tcp){
		closesocket(clt_fd);
		clt_fd = 0;
		goto ACCEPT;
	}

END:
	if(svr_fd <= 0)
		closesocket(svr_fd);
	if(clt_fd <= 0)
		closesocket(clt_fd);
#ifdef WIN32
	WSACleanup();
#endif
	return err;	
}


