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
#include <sys/wait.h>
#include <signal.h>
#define LISTENQ 1024
#define MAXLINE 4096
typedef struct sockaddr SA;
void sig_chld(int signo){
	pid_t pid;
	int stat;
	pid = wait(&stat);
	printf("stat = %d\n", stat);
	printf("child %d terminated\n", (int)pid);
	
	return;
}
int main(int argc, char **argv) {
	int listenid, connfd, n;
	struct sockaddr_in servaddr, cliaddr;
	char buf[MAXLINE];
	char clientName[MAXLINE];
	time_t ticks;
	char recvline[MAXLINE+1];
	listenid=socket(AF_INET, SOCK_STREAM,0);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	servaddr.sin_port=htons(9877);
	bind(listenid, (SA *)&servaddr, sizeof(servaddr));
	listen(listenid, LISTENQ); /*LISTENQ=1024*/
	
	signal (SIGCHLD, sig_chld);	
	for(;;) {
		socklen_t clilen = sizeof(cliaddr);
		connfd=accept(listenid, (SA*)&cliaddr, &clilen);
		ticks=time(NULL);
		// snprintf(buf, sizeof(buf), "%.24s\r\n", ctime(&ticks));
		if(inet_ntop(AF_INET,&cliaddr.sin_addr.s_addr, clientName, sizeof(clientName))!=NULL){
			printf("connect from: %s, port %d\n", clientName, ntohs(cliaddr.sin_port));
		}

		pid_t pid=fork();
		
		if(pid==0){
			while ((n=read(connfd, recvline, MAXLINE)) >0) {
				recvline[n]=0; /* null terminate */
				puts("from client");
				if (fputs(recvline, stdout)==EOF) //
					printf("fputs error");	
				
				snprintf(buf, sizeof(buf), "%s\r\n", recvline);
				write(connfd, buf, strlen(buf));
			}
			exit(0);
		}
		else
			close(connfd);
	
		
	}
}