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
#include <algorithm>
#include <errno.h>
#define LISTENQ 1024
#define MAXLINE 4096
#define SERV_PORT 7122
using namespace std;
typedef struct sockaddr SA;
int listenfd, connfd, udpfd, nready, maxfdp1;
char mesg[MAXLINE];
pid_t childpid;
fd_set rset;
ssize_t n;
socklen_t len;
const int on = 1;
struct sockaddr_in cliaddr, servaddr;
void sig_chld(int signo){
	pid_t pid;
	int stat;
	while((pid = waitpid(-1, &stat, WNOHANG))>0){
		printf("stat = %d\n", stat);
		printf("child %d terminated\n", (int)pid);
	}
	return;
}
void str_echo(int sockfd){
	ssize_t n;
	char buf[MAXLINE]; //MAXLINE is defined by user
	again:
	while ( (n = read(sockfd, buf, MAXLINE)) > 0){
		buf[n]=0;
		puts(buf);
		write(sockfd, buf, n);
	}
	if (n < 0 && errno == EINTR) /* interrupted by a signal before any data was read*/ 
		goto again; //ignore EINTR
	else if(n<0)
		printf("str_echo: read error");

}
int main(int argc, char **argv) {
	/* for create listening TCP socket */
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	bzero(&servaddr, sizeof(servaddr)); servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
	listen(listenfd, LISTENQ);
	/* for create UDP socket */
	udpfd = socket(AF_INET, SOCK_DGRAM, 0);
	bzero(&servaddr, sizeof(servaddr)); servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);
	bind(udpfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

	signal(SIGCHLD, sig_chld);/* must call waitpid() */
	FD_ZERO(&rset);
	maxfdp1 = max(listenfd, udpfd) + 1;
	for ( ; ; ) {
		FD_SET(listenfd, &rset);
		FD_SET(udpfd, &rset);
		if ( (nready = select(maxfdp1, &rset, NULL, NULL, NULL)) < 0)
		{
			if (errno == EINTR)
				continue; /* back to for() */
			else
			printf("select error");
		}
		if (FD_ISSET(listenfd, &rset)) {
			len = sizeof(cliaddr);
			connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &len);

			// printf("%s\n", inet_ntoa(((struct sockaddr_in *)cliaddr)->sin_addr));
			int port = ntohs(((struct sockaddr_in *)&cliaddr)->sin_port);
			printf("TCP connected from port = %d\n", port);
			if ( (childpid = fork()) == 0) { /* child process */
				close(listenfd);
				str_echo(connfd);
				exit(0);
				
			}
			close(connfd);
		}
		close(connfd);
		if (FD_ISSET(udpfd, &rset)) {
			len = sizeof(cliaddr);
			int port = ntohs(((struct sockaddr_in *)&cliaddr)->sin_port);
			printf("UDP connected from port = %d\n", port);
			n = recvfrom(udpfd, mesg, MAXLINE, 0, (struct sockaddr *) &cliaddr, &len);
			printf("from UDP:%s\n", mesg);
			sendto(udpfd, mesg, n, 0, (struct sockaddr *) &cliaddr, len);
		}
	}
}

