#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#define LISTENQ 1024
#define MAXLINE 4096
typedef struct sockaddr SA;
int main(int argc, char **argv) {
	int listenid, connfd;
	struct sockaddr_in servaddr, cliaddr;
	char buf[MAXLINE];
	char clientName[MAXLINE];
	time_t ticks;
	listenid=socket(AF_INET, SOCK_STREAM,0);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	servaddr.sin_port=htons(8888);
	bind(listenid, (SA *)&servaddr, sizeof(servaddr));
	listen(listenid, LISTENQ); /*LISTENQ=1024*/
	for(;;) {
		socklen_t clilen = sizeof(cliaddr);
		connfd=accept(listenid, (SA*)&cliaddr, &clilen);
		ticks=time(NULL);
		snprintf(buf, sizeof(buf), "%.24s\r\n", ctime(&ticks));
		if(inet_ntop(AF_INET,&cliaddr.sin_addr.s_addr, clientName, sizeof(clientName))!=NULL){
			printf("connect from: %s, port %d\n", clientName, ntohs(cliaddr.sin_port));
		}
		write(connfd, buf, strlen(buf));
		pid_t pid;
		pid=fork();
		if(pid){
			puts("Start netstat");
			system("netstat");
			puts("end netstat~");
			exit(0);

		}
		close(connfd);
	}
}