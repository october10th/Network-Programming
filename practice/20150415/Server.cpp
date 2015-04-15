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
#define LISTENQ 1024
#define MAXLINE 4096
#define SERV_PORT 7122
using namespace std;
typedef struct sockaddr SA;

int main(int argc, char **argv) {
	int i, maxi, maxfd, listenfd, connfd, sockfd;
	int nready, client[FD_SETSIZE];
	ssize_t n;
	char hell[50]="hello\r\n";
	fd_set rset, allset;
	char line[MAXLINE];
	socklen_t clilen;
	struct sockaddr_in cliaddr, servaddr;	
	
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);

	bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
	listen(listenfd, LISTENQ);
	maxfd = listenfd; /* initialize */
	maxi = -1; /* index into client[] array */ for (i = 0; i < FD_SETSIZE; i++)
	client[i] = -1; /* -1 indicates available entry */
	FD_ZERO(&allset); FD_SET(listenfd, &allset);
	for ( ; ; ) {
		rset = allset; /* structure assignment */
		nready = select(maxfd+1, &rset, NULL, NULL, NULL);
		if (FD_ISSET(listenfd, &rset)) { /* new client connection */
			clilen = sizeof(cliaddr);
			connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);
			for (i = 0; i < FD_SETSIZE; i++)
				if (client[i] < 0) {
					client[i] = connfd; /* save descriptor */
					break;
				}
			if (i == FD_SETSIZE)
				printf("too many clients");

			FD_SET(connfd, &allset);
			if (connfd > maxfd)
				maxfd = connfd;
			if (i > maxi)
				maxi = i;
			if (--nready <= 0)
				continue;
		}

		for (i = 0; i <= maxi; i++) {
			if ( (sockfd = client[i]) < 0)
				continue;
			if (FD_ISSET(sockfd, &rset)) {
				if ( (n = read(sockfd, line, MAXLINE)) == 0) {

					puts("XD");
					/* connection closed by client */
					for(int i=0;i<10;i++){
						// puts("send Hello");
						printf("send %s\n", hell);
						write(sockfd, hell, strlen(hell));
						sleep(1);
					}
					close(sockfd);
					FD_CLR(sockfd, &allset);
					client[i] = -1;
				}
				else{
					puts("from client");
					line[n]='\0';
					n=strlen(line);
					puts(line);
					write(sockfd, line, n);
				}
				if (--nready <= 0)
					break;
				/* no more readable descriptors */
			}
		}

	}
}
