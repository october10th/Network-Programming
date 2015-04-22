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
#define MAXLINE 4096
#define PN 5
#define SERV_PORT 7122
typedef struct sockaddr SA;

void dg_cli(FILE *fp, int sockfd, const struct sockaddr *pservaddr, socklen_t servlen) {
	int n;
	char sendline[MAXLINE], recvline[MAXLINE + 1];
	while (fgets(sendline, MAXLINE, fp) != NULL) {
		sendto(sockfd, sendline, strlen(sendline), 0, pservaddr, servlen);
		n = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL);
		recvline[n] = 0; /* null terminate */
		fputs(recvline, stdout);
	}
}

int
main(int argc, char **argv) {
	int sockfd;
	struct sockaddr_in servaddr;
	if (argc != 2)
		puts("usage: udpcli <IPaddress>");
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
	inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	dg_cli(stdin, sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
	exit(0);

	return 0;
}
// int main (int argc, char **argv) {
// 	int sockfd[PN], n;
// 	char sendline[MAXLINE+1], buf[MAXLINE+1];
// 	char recvline[MAXLINE+1]; /*MAXLINE=4096: max text line length */
// 	struct sockaddr_in servaddr[PN]; /*declare server address*/
// 	if(argc!=2) /*deal with error message*/
// 	puts("usage: a.out <IPaddress>");
// 	int i=0;
// 	// for(i=0;i<PN;i++){
// 		if((sockfd[i]=socket(AF_INET, SOCK_STREAM,0)) < 0) puts("socket error");
// 		//ceate an Internet(AF_INET) stream(SOCK_STREAM) socket
// 		bzero(&servaddr[i],sizeof(servaddr[i])); 
// 		//reset address to zero
// 		servaddr[i].sin_family=AF_INET; //IPv4
// 		servaddr[i].sin_port=htons(9877); //Port: 13
// 		if(inet_pton(AF_INET, argv[1], &servaddr[i].sin_addr) <= 0) printf("inet_ption error for %s\n", argv[1]);
		
// 		if(connect(sockfd[i], (SA *) &servaddr[i], sizeof(servaddr[i])) < 0) puts("connect error");
// 	// }
// 	puts("enter sth:");
// 	while(scanf("%s", sendline)!=EOF){
// 		snprintf(buf, sizeof(buf), "%s\r\n", sendline);
// 		write(sockfd[0], buf, strlen(buf));
// 		printf("read %s\n", sendline);
// 		if((n=read(sockfd[0], recvline, MAXLINE)) >0) {
// 				puts("from server");
// 				recvline[n]=0; /* null terminate */
// 				if (fputs(recvline, stdout)==EOF) //
// 					printf("fputs error");
				
// 			if (n<0){
// 				puts("read error");
// 				exit(0);
// 			}
// 		}	
// 	}
	
// }