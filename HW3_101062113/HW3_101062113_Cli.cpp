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
		puts("[T]ell [L]ogout");
		puts("[Del]eteAccount");// change account?
		puts("----------------------------------------");
		puts("[T]ell [User ID]");
		puts("[D]ownload [filename]");
		puts("[U]pload [User ID] [filename]");
	}
	else if(me.state==Tell){
		// show nothing
		// until EOF
	}
}
void showResult(int connfd){
	char recvline[MAXLINE];
	int n=recvWrite(connfd, recvline);
	

	int totalbytes=atoi(recvline);
	
	for(int i=0;i<totalbytes;i++){
		int totallines=10;
		n=recvWrite(connfd, recvline);
		totallines=atoi(recvline);
		
		for(int j=0;j<totallines;j++){
			n=recvWrite(connfd, recvline);
		
			if(j%2==0 && totallines>1)printf("%s:", recvline);
			else puts(recvline);
			
		}
		puts("");
	
	}
}
void* listenClient(void *arg){

	return NULL;
}

void sendFilelist(int& connfd){
	
	vector<string>filelist=getCurFilelist();
	string str=toString(filelist.size());
	writeRecv(connfd, str.c_str(), str.length());
	for(int i=0;i<filelist.size();i++){
		str=filelist[i];
		writeRecv(connfd, str.c_str(), str.length());
	}

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

		system("clear");
		
		// recv
		n=receive(connfd, recvline);
		puts("recvline:");
		puts(recvline);
		
		
		recv=recvline;
		if(me.state==Init){
			if(strcmp(recvline, SUCCESS)==0){// register or login success
				me.state=Normal;
				me.ID=tok[1],me.pw=tok[2];
				string str="mkdir Folder_"+tok[1];
				system(str.c_str());
				str="Folder_"+tok[1];
				chdir(str.c_str());
				// send filelist
				sendFilelist(connfd);
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
			//------------------------------------------------
			// show filelist/ show user
			else if(tok[0]=="SU"){// show user
				if(strcmp(recvline, SUCCESS)==0){
					puts("show users:");
					showResult(connfd);
					puts("----------------------------------------");		
				}
			}
			else if(tok[0]=="SF"){// show filelist
				if(strcmp(recvline, SUCCESS)==0){
					puts("show filelist:");
					showResult(connfd);
					puts("----------------------------------------");		
				}
			}
			//------------------------------------------------
			// upload download

			//------------------------------------------------
			
			else if(tok[0]=="T"){// tell
				
				// set TCP connection(write)

				// create a thread to read?
				// close?

			}

		}
		else if(me.state==Tell){

		}
		else{

		}
		showMsg();

	}
	close(connfd);
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
	TCPconnect(sockfd, servaddr, argv[1], SERV_PORT);

	
	// init
	me.state=Init;

	 
	dg_cli(stdin, sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
	exit(0);

	return 0;
}