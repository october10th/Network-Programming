#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <algorithm>
#define MAXLINE 4096
#define SERV_PORT 7122
using namespace std;

typedef struct sockaddr SA;

void str_cli(FILE *fp, int sockfd) {
	int maxfdp1, stdineof, n;
	fd_set  rset;
	char sendline[MAXLINE], recvline[MAXLINE];
	stdineof = 0; /*use for test readable*/
	FD_ZERO(&rset);
	for ( ; ;) {
		if (stdineof == 0){
			FD_SET(fileno(fp), &rset);
			FD_SET(sockfd, &rset);
			maxfdp1 = max(fileno(fp), sockfd) + 1;
			select(maxfdp1, &rset, NULL, NULL, NULL);
			if (FD_ISSET(sockfd, &rset)) { /* socket is readable */
				if ((n=read(sockfd, recvline, MAXLINE))== 0) {
					if (stdineof == 1)
						return; /* normal termination */
					else
						printf("str_cli: server terminated prematurely");
				}
				recvline[n]='\0';
				printf("from server:\n%s\n", recvline);
				fputs(recvline, stdout);
				//
				// stdineof = 1;
				// shutdown(sockfd, SHUT_WR);//send FIN
				// // FD_CLR(fileno(fp), &rset);
					
			}
			if (FD_ISSET(fileno(fp), &rset)) { /* input is readable */
				if (fgets(sendline, MAXLINE, fp) == NULL) { //EOF
					stdineof = 1;
					shutdown(sockfd, SHUT_WR);//send FIN
					FD_CLR(fileno(fp), &rset);
					continue;
				}	
				
				
				write(sockfd, sendline, strlen(sendline));
				shutdown(sockfd, SHUT_WR);//send FIN OAOAOAOAOAAO
				
			}
		}
	}
}
int main(int argc, char **argv){
	int sockfd;
	struct sockaddr_in servaddr;
	if (argc!=2)puts("usage: tcpcli <IPaddress>");
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
	inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
	connect(sockfd, (SA *) &servaddr, sizeof(servaddr));
	str_cli(stdin, sockfd); 
	exit(0);
	/* do it all */
}
