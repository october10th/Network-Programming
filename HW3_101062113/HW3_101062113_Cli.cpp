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


void sendFilelist(int connfd){
	
	vector<string>filelist=getCurFilelist();
	string str=toString(filelist.size());
	writeRecv(connfd, str.c_str(), str.length());
	for(int i=0;i<filelist.size();i++){
		str=filelist[i];
		writeRecv(connfd, str.c_str(), str.length());
	}

}


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
				// shutdown(clientSock->connfd, SHUT_WR);
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
void *sendFile(void *arg){
	pthread_detach(pthread_self());
	SendFileInfo sendFileInfo=*(SendFileInfo*)arg;
	int connfd=sendFileInfo.clientSock.connfd;
	string filename=sendFileInfo.path;
	FILE *fin=fopen(filename.c_str(), "rb");
	struct stat filestat;
	char sendline[MAXLINE];

	if(!fin){
		write(connfd, toString(0).c_str(), toString(0).length());
		return NULL;
	}
	
	lstat(filename.c_str(), &filestat);
	printf("start sending (%d bytes)\n", (int)filestat.st_size);
	string str=toString((int)filestat.st_size);
	writeRecv(connfd, str.c_str(), str.length());
	usleep(100);
	int sendbytes, totalbytes=0;
	while (!feof(fin)) {
		sendbytes=fread(sendline, sizeof(char), sizeof(sendline), fin);
        printf(". sendbytes %d (%d)\n", sendbytes, totalbytes);
        writeRecv(connfd, sendline, sendbytes);
        totalbytes+=sendbytes;
    }
    usleep(1000);
	fclose(fin);
	system("clear");
    printf("finished: sended %d bytes\n", totalbytes);
    showMsg();
	return NULL;
}
int getSendBytes(int idx, int total, int bytes){
	if(idx!=total-1){
		return bytes/total;
	}
	else{
		return bytes/total+bytes%total;
	}
}
int getStartBytes(int idx, int total, int bytes){
	return bytes/total*idx;
}
void *downloadFile(void *arg){
	pthread_detach(pthread_self());

	SendFileInfo sendFileInfo=*(SendFileInfo*)arg;
	int connfd=sendFileInfo.clientSock.connfd;
	FILE *fout=fopen(sendFileInfo.path.c_str(), "wb");
	printf("download file from IP %s port %d connfd %d\n", getIP(sendFileInfo.clientSock.addr), getPort(sendFileInfo.clientSock.addr), sendFileInfo.clientSock.connfd);
	char recvline[MAXLINE];

	if(!fout){
		receive(connfd, recvline);
		return NULL;
	}
	
	
	recvWrite(connfd,recvline);
	usleep(100);
	int sendbytes, totalbytes=atoi(recvline), currentbyte=0;
	while (currentbyte<totalbytes) {
		sendbytes=recvWrite(connfd, recvline);
		fwrite(recvline, sizeof(char), sendbytes, fout);
        printf(". sendbytes %d (%d/%d)\n", sendbytes, currentbyte, totalbytes);
        
        currentbyte+=sendbytes;

    }
    usleep(1000);
	fclose(fout);
	system("clear");
    printf("finished: downloaded %d bytes (%d)\n", currentbyte, totalbytes);
    showMsg();
	return NULL;
}

void *sendDivideFile(void *arg){
	pthread_detach(pthread_self());
	DivideFileInfo divideFileInfo=*(DivideFileInfo*)arg;;

	// 		divideFileInfoInfo.path=tok[3];
	// 		divideFileInfoInfo.clientSock=*uploadSock;
	// 		divideFileInfo.idx=atoi(tok[4].c_str());
	// 		divideFileInfo.totalParts=atoi(tok[5].c_str());
	// SendFileInfo sendFileInfo=*(SendFileInfo*)arg;
	
	int connfd=divideFileInfo.clientSock.connfd;
	string filename=divideFileInfo.path;
	FILE *fin=fopen(filename.c_str(), "rb");
	struct stat filestat;
	char sendline[MAXLINE];
	string str;
	printf("send divide file : %s\n", filename.c_str());

	if(!fin){
		write(connfd, toString(0).c_str(), toString(0).length());
		return NULL;
	}
	
	lstat(filename.c_str(), &filestat);
	printf("start sending (%d bytes)\n", (int)filestat.st_size);
	int totalbytes=(int)filestat.st_size;
	int start=getStartBytes(divideFileInfo.idx, divideFileInfo.totalParts, totalbytes);
	int end=start+getSendBytes(divideFileInfo.idx, divideFileInfo.totalParts, totalbytes);

	// idx / total parts / total bytes
	str=toString(divideFileInfo.idx)+" "+toString(divideFileInfo.totalParts)+" "+toString((int)filestat.st_size);
	cout<<"addr: "<<getIP(divideFileInfo.clientSock.addr)<<" port: "<<getPort(divideFileInfo.clientSock.addr)<<" confd: "<<divideFileInfo.clientSock.connfd<<endl;
	cout<<"writeRecv(idx/totalParts/totalbytes): "<<str<<endl;
	writeRecv(connfd, str.c_str(), str.length());

	usleep(100);
	int sendbytes, currentbyte=0;
	printf("start = %d end = %d end-start = %d\n", start, end, end-start);
	while (!feof(fin)) {

		sendbytes=fread(sendline, sizeof(char), sizeof(sendline), fin);
		
		// send
		if(currentbyte+sendbytes>=start && currentbyte<end){
			if(currentbyte<start && currentbyte+sendbytes>=end){
				printf("[%d] sendbytes %d (%d)\n", divideFileInfo.idx, end-start, currentbyte);
        		writeRecv(connfd, &sendline[start-currentbyte], end-start);
			}
			else if(currentbyte<start){
				printf("[%d] sendbytes %d (%d)\n", divideFileInfo.idx, currentbyte+sendbytes-start, currentbyte);
        		writeRecv(connfd, &sendline[start-currentbyte], currentbyte+sendbytes-start);	
			}
			else if(currentbyte+sendbytes<end){
				printf("[%d] sendbytes %d at (%d)\n", divideFileInfo.idx, sendbytes, currentbyte);
        		writeRecv(connfd, sendline, sendbytes);	
			}
			else{
				printf("[%d] sendbytes %d at (%d)\n", divideFileInfo.idx, end-currentbyte, currentbyte);
        		writeRecv(connfd, sendline, end-currentbyte);
			}
			
		}
		// discard
		else{

		}
        currentbyte+=sendbytes;
    }
    usleep(1000);
	fclose(fin);
	system("clear");
    printf("finished: sended %d bytes\n", totalbytes);
    showMsg();
	// idx 
	// shutdown(connfd, SHUT_WR);
	return NULL;
}

void *downloadDivideFile(void *arg){
	// pthread_detach(pthread_self());
	
	DivideFileInfo sendFileInfo=*(DivideFileInfo*)arg;
	int connfd=sendFileInfo.clientSock.connfd;

	printf("download from IP %s port %d connfd %d\n", getIP(sendFileInfo.clientSock.addr), getPort(sendFileInfo.clientSock.addr), sendFileInfo.clientSock.connfd);
	char recvline[MAXLINE];

	puts("start download Divide file:");
	// idx / total parts / total bytes
	recvWrite(connfd,recvline);
	puts(recvline);
	vector<string>tok=parse(recvline);
	usleep(100);
	int sendbytes, totalbytes=atoi(tok[2].c_str()), currentbyte=0, idx=atoi(tok[0].c_str()), totalParts=atoi(tok[1].c_str());
	int start=getStartBytes(idx, totalParts, totalbytes);
	int end=start+getSendBytes(idx, totalParts, totalbytes);
	int goingtosend=end-start;
	printf("start = %d end = %d goingtosend = %d\n", start, end, goingtosend);
	string filename="tmp/"+sendFileInfo.path+"_"+toString(idx);
	FILE *fout=fopen(filename.c_str(), "wb");
	if(!fout){
		// receive(connfd, recvline);
		return NULL;
	}
	while (currentbyte<goingtosend) {
		sendbytes=recvWrite(connfd, recvline);
		fwrite(recvline, sizeof(char), sendbytes, fout);
        printf("[%d] sendbytes %d (%d/%d)\n", idx, sendbytes, currentbyte, totalbytes);
        
        currentbyte+=sendbytes;

    }
    usleep(1000);
	fclose(fout);
	system("clear");
    printf("finished: downloaded %d bytes (%d)\n", currentbyte, totalbytes);
    showMsg();
	return NULL;
}
void mergeDivideFile(string filename, int totalParts){
	FILE *fin, *fout;
	char sendline[MAXLINE];

	fout=fopen(filename.c_str(), "wb");
	for(int i=0;i<totalParts;i++){
		string str="tmp/"+filename+"_"+toString(i);
		fin=fopen(str.c_str(), "rb");
		if(!fin){
			printf("no file %d\n", i);
			i--;continue;
		}
		int recvbytes=0;
		while (!feof(fin)) {
			int sendbytes=fread(sendline, sizeof(char), sizeof(sendline), fin);
			fwrite(sendline, sizeof(char), sendbytes, fout);
			// printf("[%d] write %d\n", i, sendbytes);
			recvbytes+=sendbytes;
		}
		printf("[%d] write %d\n", i, recvbytes);
		fclose(fin);
	}
	// system("rm -r tmp");
	fclose(fout);
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
	again:
	while((n=receive(clientSock->connfd, recvline))>0){

		
		vector<string>tok;
		tok.clear();
		// Upload (client)
		if(strncmp(recvline, UPLOAD, strlen(UPLOAD))==0){

			puts("upload~~~!!!");
			puts(recvline);
			tok=parse(recvline);
			pthread_t tid;
			ClientSock * uploadSock=(ClientSock*)malloc(sizeof(ClientSock));
			TCPconnect(uploadSock->connfd, uploadSock->addr, tok[1].c_str(), atoi(tok[2].c_str()));
			SendFileInfo sendFileInfo;
			sendFileInfo.path=tok[3];
			sendFileInfo.clientSock=*uploadSock;
			pthread_create(&tid, NULL, &downloadFile, (void *)&sendFileInfo);


			continue;
		}
		if(strncmp(recvline, DOWNLOAD, strlen(DOWNLOAD))==0){

			puts("download~~~!!!");
			puts(recvline);
			tok=parse(recvline);
			pthread_t tid;
			ClientSock * uploadSock=(ClientSock*)malloc(sizeof(ClientSock));
			printf("before IP %s port %d connfd %d\n", getIP(clientSock->addr), getPort(clientSock->addr), clientSock->connfd);
			
			TCPconnect(uploadSock->connfd, uploadSock->addr, tok[1].c_str(), atoi(tok[2].c_str()));
			printf("after IP %s port %d connfd %d\n", getIP(uploadSock->addr), getPort(uploadSock->addr), uploadSock->connfd);
			
			DivideFileInfo divideFileInfo;
			
			divideFileInfo.path=tok[3];
			divideFileInfo.clientSock=*uploadSock;
			divideFileInfo.idx=atoi(tok[4].c_str());
			divideFileInfo.totalParts=atoi(tok[5].c_str());
			cout<<"download: "<<tok[3]<<endl;
			
			pthread_create(&tid, NULL, &sendDivideFile, (void *)&divideFileInfo);

			puts("pthread_create");
			// free(uploadSock);
			continue;
		}
		// Tell
		if(me.state==Tell){
			if(strcmp(recvline, EXIT)==0){
				// close(clientSock->connfd);
				me.state=Normal;
				writeWithSleep(clientSock->connfd, EXIT, strlen(EXIT));// to client
				
				// shutdown(clientSock->connfd, SHUT_WR);
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
			puts("type [EXIT] when you finish");
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
		
		
		// recv=recvline;
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
					chdir("..");
				}
			}
			else if(tok[0]=="L"){// logout
				if(strcmp(recvline, SUCCESS)==0){
					me.state=Init;
					puts("logout success");
					chdir("..");
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
			else if(tok[0]=="D"){// download
				if(strcmp(recvline, SUCCESS)==0){
					write(connfd, SUCCESS, strlen(SUCCESS));
					system("mkdir tmp");// tmp folder

					receive(connfd, recvline);
					int numFile=atoi(recvline);
					// set TCP connection(write)
					int port=getRandPort();
					printf("open port = %d\n", port);
					struct sockaddr_in servaddr;
					int listenid, sockfd;
					TCPlisten(listenid, sockfd, servaddr, port);
					
					string str=toString(port);
					writeRecv(clientSock->connfd, str.c_str(), str.length());

					printf("numFile(owners) = %d\n", numFile);
					pthread_t *tid=(pthread_t*)malloc(sizeof(pthread_t)*numFile);
					ClientSock * downloadSock=(ClientSock*)malloc(sizeof(ClientSock)*numFile);
					DivideFileInfo *divideFileInfo=(DivideFileInfo*)malloc(sizeof(DivideFileInfo)*numFile);
					for(int i=0;i<numFile;i++){
						
						puts("listening ~");
						socklen_t clilen = sizeof(downloadSock[i].addr);
						downloadSock[i].connfd= accept(listenid, (struct sockaddr*)&(downloadSock[i].addr), &clilen);
						divideFileInfo[i].clientSock=downloadSock[i];
						printf("TCP connect from IP %s (port %d), connfd = %d\n", getIP(divideFileInfo[i].clientSock.addr), getPort(divideFileInfo[i].clientSock.addr), divideFileInfo[i].clientSock.connfd);
						//send file
						divideFileInfo[i].path=tok[1];
						

						pthread_create(&tid[i], NULL, &downloadDivideFile, (void *)&divideFileInfo[i]);
						printf("pthread create [%d]\n", i);
					}

					for(int i=0;i<numFile;i++){
						printf("pthread join [%d]\n", i);
						pthread_join(tid[i], NULL);
					}
					// free(divideFileInfo);
					// merge?
					cout<<"merge "<<tok[1]<<endl;
					mergeDivideFile(tok[1], numFile);


					// puts(recvline);
					// downloadFile(tok[1], fp, sockfd, pservaddr, servlen);
					// receiveArticle(fp, sockfd, pservaddr, servlen);
					// recvline[0]='\0';
					system("clear");
					showMsg();
				}
			}
			else if(tok[0]=="U"){// upload
				if(strcmp(recvline, SUCCESS)==0){

					int port=getRandPort();
					printf("open port = %d\n", port);

					string str=toString(port);
					writeRecv(clientSock->connfd, str.c_str(), str.length());

					// set TCP connection(write)
					struct sockaddr_in servaddr;
					int listenid, sockfd;
					TCPlisten(listenid, sockfd, servaddr, port);

					puts("listening ~");
					ClientSock * uploadSock=(ClientSock*)malloc(sizeof(ClientSock));
					socklen_t clilen = sizeof(clientSock->addr);
					uploadSock->connfd = accept(listenid, (struct sockaddr*)&(uploadSock->addr), &clilen);
					printf("TCP connect from IP %s (port %d)\n", getIP(uploadSock->addr), getPort(uploadSock->addr));
					//send file
					SendFileInfo sendFileInfo;
					sendFileInfo.path=tok[2];
					sendFileInfo.clientSock=*uploadSock;
					pthread_create(&tid, NULL, &sendFile, (void *)&sendFileInfo);
					system("clear");
				}
			}
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
					puts("type [EXIT] when you finish");
					// create a thread to read?
					
					// close(clientSock->connfd);
					
				}

				
			}
			showMsg();

		}
		else{

		}
		

	}
	if(n<0&&errno==EINTR){
		goto again;
	}
	else if(n<0){
		puts("str_cli error");
	}
	shutdown(clientSock->connfd, SHUT_WR);
	// close(clientSock->connfd);
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