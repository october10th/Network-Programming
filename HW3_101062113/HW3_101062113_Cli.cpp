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
void* TCPlistenClient(void *arg){
	// TCPlisten(listenid, connfd, servaddr, SERV_PORT);

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
// void *reader(void*arg){
// 	pthread_detach(pthread_self());
// 	int n;
// 	char recvline[MAXLINE];
// 	ClientSock clientSock=*(ClientSock*)arg;
// 	int connfd=clientSock.connfd;
// 	while((n=read(connfd, recvline, MAXLINE)) >0) {

// 		recvline[n]=0;
// 		puts(recvline);
// 	}
// 	return NULL;
// }

void *writer(void*arg){
	pthread_detach(pthread_self());
	ClientSock *clientSock=(ClientSock*)arg;
	ClientSock tmpClientSock=*clientSock;
	char sendline[MAXLINE];

	while(fgets(sendline, MAXLINE, stdin)!=NULL){
		sendline[strlen(sendline)-1]='\0';// remove '\n'
		// printf("(writer)send to IP %s port %d connfd %d\n", getIP(clientSock->addr), getPort(clientSock->addr), clientSock->connfd);
		if(me.state==Tell){
			if(strcmp(sendline, EXIT)==0){
				// shutdown(clientSock->connfd, SHUT_WR);
				// me.state=Normal;
				writeWithSleep(clientSock->connfd, EXIT, strlen(EXIT));// to client
				// close(clientSock->connfd);
				*clientSock=tmpClientSock;
				printf("back to %s %d\n", getIP(clientSock->addr), getPort(clientSock->addr));
				// writeWithSleep(clientSock->connfd, EXIT, strlen(EXIT));// to server
				// showMsg();
				continue;
			}
			string str=me.ID+":"+sendline;
			cout<<str<<endl;
			writeWithSleep(clientSock->connfd, str.c_str(), str.length());
		}
		else{
			puts("sendline:");
			puts(sendline);
			write(clientSock->connfd, sendline, strlen(sendline));	
		}
		
	}
	return NULL;

}
void str_cli(int connfd, sockaddr_in servaddr) {
	showMsg();
	char recvline[MAXLINE];
	int n;
	string recv;
	pthread_t tid;
	ClientSock * clientSock = (ClientSock*)malloc(sizeof(ClientSock));
	clientSock->connfd=connfd;
	clientSock->addr=servaddr;
	
	ClientSock tmpClientSock=*clientSock;
	pthread_create(&tid, NULL, &writer, (void *)clientSock);
	while((n=receive(clientSock->connfd, recvline))>0){
		
		vector<string>tok;
		tok.clear();
		if(me.state==Tell){
			if(strcmp(recvline, EXIT)==0){
				// close(clientSock->connfd);
				me.state=Normal;
				writeWithSleep(clientSock->connfd, EXIT, strlen(EXIT));// to client
				*clientSock=tmpClientSock;//connfd=clientSock->connfd;
				writeWithSleep(clientSock->connfd, EXIT, strlen(EXIT));// to server
				system("clear");
				showMsg();
				continue;
			}
			else{
				puts(recvline);
			}
			continue;
		}
		if(strncmp(recvline, TALK, strlen(TALK))==0){// user B (client)
			me.state=Tell;
			puts("TALK~~~!!!");
			// receive(connfd, recvline);// IP port
			puts(recvline);
			tok=parse(recvline);
			// pthread_t tid;
			printf("before IP %s port %d\n", getIP(clientSock->addr), getPort(clientSock->addr));
			TCPconnect(clientSock->connfd, clientSock->addr, tok[1].c_str(), atoi(tok[2].c_str()));
			printf("after IP %s port %d\n", getIP(clientSock->addr), getPort(clientSock->addr));
			system("clear");
			
			continue;
		}
		*clientSock=tmpClientSock;
		tok=parse(recvline);// == sendline ?

		puts("recvline:(sendline)");
		puts(recvline);
		write(clientSock->connfd, SUCCESS, strlen(SUCCESS));
		system("clear");

		// recv
		n=receive(clientSock->connfd, recvline);// success or fail
		puts("recvline:");
		puts(recvline);
		
		
		recv=recvline;
		// someone tells
		
		if(me.state==Init){
			if(strcmp(recvline, SUCCESS)==0){// register or login success
				me.state=Normal;
				me.ID=tok[1],me.pw=tok[2];
				string str="mkdir Folder_"+tok[1];
				system(str.c_str());
				str="Folder_"+tok[1];
				chdir(str.c_str());
				// send filelist
				sendFilelist(clientSock->connfd);
			}
			// puts(recvline);
			showMsg();
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
					write(clientSock->connfd, SUCCESS, strlen(SUCCESS));
					puts("show users:");
					showResult(clientSock->connfd);
					puts("----------------------------------------");		
				}
			}
			else if(tok[0]=="SF"){// show filelist
				if(strcmp(recvline, SUCCESS)==0){
					write(clientSock->connfd, SUCCESS, strlen(SUCCESS));
					puts("show filelist:");
					showResult(clientSock->connfd);
					puts("----------------------------------------");		
				}
			}
			//------------------------------------------------
			// upload download

			//------------------------------------------------
			else if(tok[0]=="T"){// tell
				if(strcmp(recvline, SUCCESS)==0){// user A (server)
					// write(connfd, SUCCESS, strlen(SUCCESS));
					me.state=Tell;
					// choose a port
					int port=getRandPort();
					printf("open port = %d\n", port);

					string str=toString(port);
					writeRecv(clientSock->connfd, str.c_str(), str.length());

					// set TCP connection(write)
					struct sockaddr_in servaddr;
					int listenid, sockfd;
					TCPlisten(listenid, sockfd, servaddr, port);

					puts("listening ~");
					socklen_t clilen = sizeof(clientSock->addr);
					clientSock->connfd = accept(listenid, (struct sockaddr*)&(clientSock->addr), &clilen);
					printf("TCP connect from IP %s (port %d)\n", getIP(clientSock->addr), getPort(clientSock->addr));
					system("clear");
					// create a thread to read?
					
					// close(clientSock->connfd);
					
				}

				
			}
			showMsg();

		}
		else{

		}
		

	}
	close(clientSock->connfd);
}

int main(int argc, char **argv) {
	int sockfd;
	struct sockaddr_in servaddr;
	srand (time(NULL));
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

	 
	str_cli(sockfd, servaddr);
	exit(0);

	return 0;
}