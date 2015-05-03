#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#define MAXLINE 2048
#define PN 5
typedef struct sockaddr SA;
int PORT_NUM;
void showMsg(){puts("[C]hange dir, [S]how file, [D]ownload file, [U]pload file, [E]xit");}
void Read(char recvline[], int sockfd, int &n){

	if((n=read(sockfd, recvline, MAXLINE))>0) {
		recvline[n]=0; /* null terminate */
		
		if (n<0){
			puts("read error");
			exit(0);
		}
	}
}
void download(char *filename, int sockfd){
	
	FILE *fout=fopen(filename, "wb");
	char byte, buf[MAXLINE+2], recvline[MAXLINE+2];
	int n;
	Read(recvline, sockfd, n);
	int totalbytes=atoi(recvline), currentbyte=0;
	printf("start download (%d bytes)\n", totalbytes);
	if(totalbytes==0){
		puts("NO file");
		return ;
	}

	while(currentbyte<totalbytes){
		snprintf(buf, sizeof(buf), "%d", currentbyte);
		write(sockfd, buf, strlen(buf));
		if((n=read(sockfd, recvline, MAXLINE))>0) {
			recvline[n]=0;
			fwrite(recvline, sizeof(char), n, fout);
			printf(".download %d (%d) bytes\n", n, totalbytes);
			currentbyte+=n;
			
		}
	}
	
	
	usleep(1000);
	fclose(fout);
	printf("download finished remain %d\n", totalbytes);

}
void upload(char *filename, int connfd){
	struct stat filestat;
	char byte, buf[MAXLINE], recvline[MAXLINE+2];
	FILE *fin=fopen(filename, "rb");

	if(!fin){
		snprintf(buf, sizeof(buf), "0");
		write(connfd, buf, strlen(buf));
		return ;
	}
	
	int i=-1, n;
	lstat(filename, &filestat);
	printf("start sending (%lld bytes)\n", filestat.st_size);
	snprintf(buf, sizeof(buf), "%lld", filestat.st_size);
	write(connfd, buf, strlen(buf));
	usleep(100);
	int sendbytes, totalbytes=0, nextbyte=0;
	while (!feof(fin)) {
		Read(recvline, connfd, n);
        nextbyte=atoi(recvline);
        sendbytes=fread(buf, sizeof(char), sizeof(buf), fin);
        printf(". sendbytes %d (%d)\n", sendbytes, totalbytes);
        write(connfd, buf, sendbytes);
        totalbytes+=sendbytes;
    }
    usleep(1000);
	fclose(fin);
    printf("finished: sended %d bytes\n", totalbytes);
} 
int main (int argc, char **argv) {
	chdir("clientFold");
	system("pwd");
	system("mkdir Download");
	chdir("Download");
	system("pwd");
	int sockfd, n;
	char sendline[MAXLINE], buf[MAXLINE], cmd[30], str[MAXLINE], path[MAXLINE];
	char recvline[MAXLINE]; /*MAXLINE=4096: max text line length */
	struct sockaddr_in servaddr; /*declare server address*/
	if(argc<=2) puts("usage: ./HW1_101062113_Cli <IP> <port>");
	if((sockfd=socket(AF_INET, SOCK_STREAM,0)) < 0) puts("socket error");
	//ceate an Internet(AF_INET) stream(SOCK_STREAM) socket
	bzero(&servaddr,sizeof(servaddr)); 
	//reset address to zero
	servaddr.sin_family=AF_INET; //IPv4
	PORT_NUM=argc>2?atoi(argv[2]):9877;
	servaddr.sin_port=htons(PORT_NUM); //Port: 13
	if(inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)printf("inet_ption error for %s\n", argv[1]);
	printf("connect to %s %d\n", argv[1], PORT_NUM);
	if(connect(sockfd, (SA *) &servaddr, sizeof(servaddr))<0)puts("connect error");
	
	showMsg();
	while(fgets(sendline, MAXLINE-2, stdin)!=NULL){
		sscanf(sendline, "%s%s", cmd, path);
		snprintf(buf, sizeof(buf), "%s", sendline);
		write(sockfd, buf, strlen(buf));
		// printf("type:%s",sendline);
		if(sendline[0]=='C' || sendline[0]=='S'){
			Read(recvline, sockfd, n);
			if(fputs(recvline, stdout)==EOF) printf("fputs error");
		}
		else if(sendline[0]=='D'){
			download(path, sockfd);

		}
		else if(sendline[0]=='U'){
			upload(path, sockfd);
		}
		else if(sendline[0]=='E'){
			break;
		}
		else{
			Read(recvline, sockfd, n);
			if(fputs(recvline, stdout)==EOF) printf("fputs error");
		}

		showMsg();
	}
	puts("client terminated");
	return 0;
}