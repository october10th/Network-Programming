#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <map>
#define LISTENQ 1024
#define MAXLINE 2048
using namespace std;
int PORT_NUM=9877;
typedef struct sockaddr SA;
struct Client{
	int port;
	char IP[20];
	Client(){}
	Client(int _port, char _IP[]){port=_port;strcpy(IP, _IP);}
};

map<int, Client>clients;// pid -> port & IP 
void sig_chld(int signo){
	pid_t pid;
	int stat;
	while((pid = waitpid(-1, &stat, WNOHANG))>0){
		int port=clients[(int)pid].port;
		if(port)
			printf("child %d terminated, IP: %s, port: %d\n", (int)pid, clients[(int)pid].IP, port);
		clients.erase(clients.find((int)pid));
	}
	return;
}
void Read(char recvline[], int sockfd, int &n){

	if((n=read(sockfd, recvline, MAXLINE))>0) {
		recvline[n]=0; /* null terminate */
		
		if (n<0){
			puts("read error");
			exit(0);
		}
	}
}
void LS(int connfd){
	char buf[MAXLINE+1], input[MAXLINE+1]={0};
	char path[MAXLINE+1];
	strcpy(path, "ls -al ");
	strcat(path, getcwd(NULL, 0));
	FILE *fin=popen(path, "r");
	while((fgets(input, 256, fin)) != NULL) {
		strcat(buf, input);
	}
	strcat(buf, "\n");
	write(connfd, buf, strlen(buf));
	pclose(fin);

}
void writeLine(const char *str, int connfd){
	char buf[MAXLINE+1];
	strcpy(buf, str);
	buf[strlen(buf)]=0;//NULL
	write(connfd, buf, strlen(buf));
}


void sendFile(const char *filename, int connfd){
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
void recvFile(const char *filename, int sockfd){
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
int main(int argc, char **argv) {
	chdir("serverFold");
	system("pwd");
	system("mkdir Upload");
	chdir("Upload");
	system("pwd");
	int listenid, connfd, n;
	struct sockaddr_in servaddr, cliaddr;
	char buf[MAXLINE], cmd[30], str[MAXLINE], path[MAXLINE];
	char clientName[MAXLINE];
	time_t ticks;
	char recvline[MAXLINE+1];
	listenid=socket(AF_INET, SOCK_STREAM,0);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	PORT_NUM=argc>1?atoi(argv[1]):9877;
	servaddr.sin_port=htons(PORT_NUM);
	bind(listenid, (SA *)&servaddr, sizeof(servaddr));
	listen(listenid, LISTENQ); /*LISTENQ=1024*/
	
	clients.clear();
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
		clients[(int)pid]=Client(ntohs(cliaddr.sin_port), clientName);
		if(pid==0){
			while((n=read(connfd, recvline, MAXLINE)) >0) {

				recvline[n]=0; /* null terminate */
				puts("from client");
				if(fputs(recvline, stdout)==EOF)printf("fputs error");	
				sscanf(recvline, "%s%s", cmd, path);
				
				if(cmd[0]=='C'){//change dir
					puts(path);
					if(chdir(path)){
						puts("client CHANGE DIR ERROR");
						writeLine("CHANGE DIR ERROR", connfd);
					}
					else{
						snprintf(buf, sizeof(buf), "Success to change directory to:%s\n", getcwd(NULL, 0));
						write(connfd, buf, strlen(buf));
					}
					puts(getcwd(NULL, 0));
				}
				else if(cmd[0]=='S'){//show file
					//ls 
					LS(connfd);
				}
				else if(cmd[0]=='D'){//download
					//send file
					sendFile(path, connfd);
				}
				else if(cmd[0]=='U'){//upload
					//read
					recvFile(path, connfd);
				}
				else if(cmd[0]=='E'){
					exit(0);
				}
				else{
					snprintf(buf, sizeof(buf), "%s", recvline);
					write(connfd, buf, strlen(buf));
				}
			}
			exit(0);
		}
		close(connfd);
	}

	puts("server terminated");
	return 0;
}