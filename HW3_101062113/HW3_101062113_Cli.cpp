#include "header.h"

int SERV_PORT=7122, CLI_PORT;
// struct timeval timeout;

Account me;
// int isCurrentAuthor;// current author

// int retVal;
// int n, maxfdp1;
// int totalbytes, totallines, recvbytes, sendbytes;
fd_set rset;
vector<string>tok;
vector<string> recvBuff, sendBuff;
void showMsg(){

	if(me.state==Init){
		puts("************* Welcome *****************");
		puts("[R]egister [L]ogin");
		puts("----------------------------------------");
		puts("usage: [R]egister [User ID] [Password]");
		puts("usage: [L]ogin [User ID] [Password]");
	}
	else if(me.state==Normal){
		printf("************* Hello %s *****************\n", me.ID.c_str());
		puts("[SU]Show User [SF]Show Filelist [D]ownload [U]pload");
		puts("[C]hat [L]ogout");
		puts("[Del]eteAccount");// change account?
		puts("----------------------------------------");
		puts("[C]hat [User ID]");
		puts("[D]ownload [filename]");
		puts("[U]pload [User ID] [filename]");
	}
	else if(me.state==Chat){
		// show nothing
		// until EOF
	}
	
	
}
void* listenClient(void *arg){

	return NULL;
}
void dg_cli(FILE *fp, int connfd, const struct sockaddr *servaddr, socklen_t servlen) {
	char sendline[MAXLINE], recvline[MAXLINE + 1], buf[MAXLINE];
	int n;
	string recv;
	vector<string>tok;
	showMsg();
	while(fgets(sendline, MAXLINE, stdin)!=NULL){
		sendline[strlen(sendline)-1]='\0';// remove '\n'
		puts("sendline:");
		puts(sendline);
		write(connfd, sendline, strlen(sendline));
		tok.clear();
		tok=parse(sendline);

		// recv
		n=read(connfd, recvline, MAXLINE);
		recvline[n]=0;
		puts("recvline:");
		puts(recvline);
		system("clear");
		recv=recvline;
		if(me.state==Init){
			if(strcmp(recvline, SUCCESS)==0){// register or login success
				me.state=Normal;
				me.ID=tok[1],me.pw=tok[2];
				string str="mkdir Client_"+tok[1];
				system(str.c_str());
				str="Client_"+tok[1];
				chdir(str.c_str());
			}
			// puts(recvline);
		}
		else if(me.state==Normal){
			if(tok[0]=="Del"){// delete account
				if(strcmp(recvline, SUCCESS)==0){
					me.state=Init;
					puts("delete success");
				}
			}
			else if(tok[0]=="L"){// logout
				if(strcmp(recvline, SUCCESS)==0){
					me.state=Init;
					puts("logout success");
				}
			}
		}
		else if(me.state==Chat){

		}
		else{

		}
		showMsg();

	}
}
int main(int argc, char **argv) {
	int sockfd;
	struct sockaddr_in servaddr;
	
	if (argc < 2){
		puts("usage: cli <IPaddress> <port>");
	}
	else{
		SERV_PORT=atoi(argv[2]);
	}
	// from hw1 TCP
	if((sockfd=socket(AF_INET, SOCK_STREAM,0)) < 0) puts("socket error");
	//ceate an Internet(AF_INET) stream(SOCK_STREAM) socket
	bzero(&servaddr,sizeof(servaddr)); 
	//reset address to zero
	servaddr.sin_family=AF_INET; //IPv4
	servaddr.sin_port=htons(SERV_PORT);
	if(inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)printf("inet_ption error for %s\n", argv[1]);
	printf("connect to %s %d\n", argv[1], SERV_PORT);
	if(connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))<0)puts("connect error");

	
	// init
	me.state=Init;

	 
	dg_cli(stdin, sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
	exit(0);

	return 0;
}