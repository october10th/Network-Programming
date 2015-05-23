#include "header.h"

int SERV_PORT=7122;
struct timeval timeout;
int WaitingAck;
Account me;
int isCurrentAuthor;// current author
char sendline[MAXLINE], recvline[MAXLINE + 1], buf[MAXLINE];
int retVal;
int n, maxfdp1;
int totalbytes, totallines, recvbytes, sendbytes;
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
		puts("[SU]Show User [SA]Show Article [A]dd Article [E]nter Article");
		puts("[Y]ell [T]ell [L]ogout");
		puts("[Del]eteAccount");// change account?
		puts("[AU]thor");
		puts("----------------------------------------");
		puts("[A]ddArticle [title(at most 64 bytes)] [content(at most 300bytes)]");
		puts("[Y]ell [廣播內容]");
		puts("[T]ell [User ID] [私密內容]");
		puts("[E]nter Article [文章 ID]");
		puts("[AU]thor [author ID]   (show articles by that author)");

	}
	else if(me.state==Article){
		puts("[R]esponse [D]ownload [U]pload");
		if(isCurrentAuthor){// current author is me
			puts("[Add]/[Del] black list");puts("[DELETE] article");// only author
		}
		puts("[Ret]urn");
		puts("----------------------------------------");
		puts("[R]esponse [留言內容]");
		if(isCurrentAuthor)// current author is me
			puts("[Add]/[Del] black list [User ID]");// only author
		puts("[D]ownload/[U]pload [File name]");
		puts("(you have to put the file in current directory Download)");
		puts("(and the result will be in Upload)");
		
	}
	
}
void dumplines(){
	for(int i=0;i<recvBuff.size();i++){
		printf("[%d] %s\n", i, recvBuff[i].c_str());
	}
	recvBuff.clear();
}
void addArticle(){//unused
	// A+ title ?
	puts("EOF (ctrl + D) when you finish");
	FILE *fout=fopen("client/client_article.txt", "w");
	char str[MAXLINE];
	
	sendBuff.clear();
	while(fgets(str, MAXLINE, stdin) != NULL){
		puts(str);
		fprintf(fout, "%s", str);

	}
	fclose(fout);
	puts("article finish");
	

}
void addFile(){//unused
	int nfiles;
	char str[MAXLINE];
	puts("add any file? (enter a number)");
	scanf("%d", &nfiles);
	for(int i=0;i<nfiles;i++){
		printf("[%d] enter a path\n", i);
		fgets(str, MAXLINE, stdin);
	}
}
void waitingAck(FILE *fp, int sockfd, const struct sockaddr *pservaddr, socklen_t servlen){


	while(1){
		FD_SET(fileno(fp), &rset); /*fp: I/O file pointer*/
		FD_SET(sockfd, &rset); /*fileno convers fp into descriptor*/
		maxfdp1 = max(fileno(fp), sockfd) + 1;

		timeout.tv_sec=0;
		timeout.tv_usec=100000;
		retVal=select(maxfdp1, &rset, NULL, NULL, &timeout);
		if (FD_ISSET(sockfd, &rset)) { /* socket is readable */
			if((n = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL))<0){
				puts("n<0");
			}
			else{
				recvline[n] = 0; /* null terminate */
				puts("recv(waiting ack):");
				puts(recvline);
				if(strcmp(recvline, ACK)==0){
					break;
				}

			}
		}
		if(retVal==0){// 0 => timeout
			// resend
			puts("resend");
			sendto(sockfd, sendline, sendbytes, 0, pservaddr, servlen);
			
		}
	}
	if(retVal<0){
		puts("error");
	}
	WaitingAck=0;

}
void showResult(FILE *fp, int sockfd, const struct sockaddr *pservaddr, socklen_t servlen){
	if((n = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL))<0){
		puts("n<0");
	}
	else{
		recvline[n] = 0; /* null terminate */
		totalbytes=atoi(recvline);
		
		for(int i=0;i<totalbytes;i++){
			totallines=10;
			if((n = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL))<0){
				puts("n<0");
			}
			else{
				recvline[n] = 0;
				totallines=atoi(recvline);
			}
			for(int j=0;j<totallines;j++){
				if((n = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL))<0){
					puts("n<0");
				}
				else{
					recvline[n] = 0; /* null terminate */
					if(j%2==0 && totallines>1)printf("%s:", recvline);
					else puts(recvline);
				}
			}
			puts("");
		}
	}
}

void receiveArticle(FILE *fp, int sockfd, const struct sockaddr *pservaddr, socklen_t servlen){
	// article
	if((n = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL))<0){
		puts("n<0");
	}
	else{
		recvline[n] = 0; /* null terminate */
		totalbytes=atoi(recvline);
		if(totalbytes==0){
			puts("(black list)permission denied!");
			return;
		}
		else{
			me.state=Article;
		}
		puts("----------article: ----------");
		for(int i=0;i<totalbytes;i++){
			totallines=10;
			if((n = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL))<0){
				puts("n<0");
			}
			else{
				recvline[n] = 0;
				totallines=atoi(recvline);
			}
			int findAuthor=0;
			for(int j=0;j<totallines;j++){
				if((n = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL))<0){
					puts("n<0");
				}
				else{
					recvline[n] = 0; /* null terminate */
					if(j%2==0 && totallines>1){
						printf("%s:", recvline);
						if(strcmp(recvline, "author")==0){
							findAuthor=1;
						}
					}
					else puts(recvline);

					if(findAuthor&&strcmp(recvline, me.ID.c_str())==0){
						isCurrentAuthor=1;
					}

				}
			}
			puts("");
		}
	}
	

	// response
	puts("----------reponse: ----------");
	showResult(fp, sockfd, pservaddr, servlen);
	// file
	puts("----------file: ----------");
	showResult(fp, sockfd, pservaddr, servlen);
}
int getTotalbytes(string filename){
	FILE *fin=fopen(filename.c_str(), "rb");
	int totalbytes=0;
	int sendbytes;
	while (!feof(fin)) {
		
        sendbytes=fread(buf, sizeof(char), sizeof(buf), fin);
        totalbytes+=sendbytes;
    }
    printf("totalbytes = %d\n", totalbytes);
	fclose(fin);
	return totalbytes;
}
void sendFile(string filename, FILE *fp, int sockfd, const struct sockaddr *pservaddr, socklen_t servlen){
	int tot=0;
	FILE *fin=fopen(filename.c_str(), "rb");
	totalbytes=getTotalbytes(filename);
	// send total bytes (with ack)
	sprintf(sendline, "%d", totalbytes);
	sendto(sockfd, sendline, strlen(sendline), 0, pservaddr, servlen);
	WaitingAck=1;
	sendbytes=strlen(sendline);
    waitingAck(fp, sockfd, pservaddr, servlen);
    // start sending
	while (!feof(fin)) {
		
        sendbytes=fread(sendline, sizeof(char), sizeof(sendline), fin);
        sendto(sockfd, sendline, sendbytes, 0, pservaddr, servlen);
        printf("sendbytes=%d (%d)\n", sendbytes, tot+=sendbytes);
        WaitingAck=1;
        waitingAck(fp, sockfd, pservaddr, servlen);
    }
    fclose(fin);

}
void downloadFile(string filename, FILE *fp, int sockfd, const struct sockaddr *pservaddr, socklen_t servlen){
	FILE *fout=fopen(filename.c_str(), "wb");
	int recvbytes=0;
	if((n = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL))<0){
		puts("n<0");
	}
	else{
		recvline[n] = 0; /* null terminate */
		totalbytes=atoi(recvline);
		printf("start download (%d bytes)\n", totalbytes);
		if(totalbytes==0){
			puts("NO file");
			return ;
		}
		
		while(recvbytes<totalbytes){
			
			if((n = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL))<0){
				puts("n<0");
			}
			else{
				recvline[n] = 0; /* null terminate */
				fwrite(recvline, sizeof(char), n, fout);
				printf("download %d (%d) bytes\n", recvbytes+=n, totalbytes);
				
			}
		}
		puts("download fin");
		
	}
	if(recvbytes!=totalbytes){
		string str="rm "+filename;
		system(str.c_str());
		printf("recvbytes %d totalbytes %d\n", recvbytes, totalbytes);
		puts("packet loss delete file");
	}
	else{
		puts("no packet loss");
	}
	fclose(fout);
	
}
void dg_cli(FILE *fp, int sockfd, const struct sockaddr *pservaddr, socklen_t servlen) {
	
	
	
	timeout.tv_sec=0;
	timeout.tv_usec=100000;
	FD_ZERO(&rset); /*initial select*/
	me.state=Init;
	showMsg();// Login or Register

	for ( ; ; ) {
		
		
		
		if(!WaitingAck){
			FD_SET(fileno(fp), &rset); /*fp: I/O file pointer*/
			FD_SET(sockfd, &rset); /*fileno convers fp into descriptor*/
			maxfdp1 = max(fileno(fp), sockfd) + 1;
			retVal=select(maxfdp1, &rset, NULL, NULL, NULL);
			if (FD_ISSET(fileno(fp), &rset)) { /* input is readable */
			
				if (fgets(sendline, MAXLINE, fp) == NULL)
					return; /* all done */
				
				sendline[strlen(sendline)-1]='\0';// remove '\n'

				sendto(sockfd, sendline, strlen(sendline), 0, pservaddr, servlen);
				recvBuff.clear();
				WaitingAck=1;

			}

			if (FD_ISSET(sockfd, &rset)) { /* socket is readable */
				if((n = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL))<0){
					puts("n<0");
				}
				else{
					recvline[n] = 0; /* null terminate */
					puts("recv:");
					system("clear");
					tok.clear();
					tok=parse(sendline);
					if(tok.size()==0){
						puts(recvline);
						continue;
					}
					if(me.state==Init){
						if(strcmp(recvline, SUCCESS)==0){// register or login success
							me.state=Normal;
							me.ID=tok[1],me.pw=tok[2];
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
						else if(tok[0]=="SU"){// show user
							if(strcmp(recvline, SUCCESS)==0){
								puts("show users:");
								dumplines();
								puts("------end------");
								sendline[0]=0;
								
							}
							else{
								recvBuff.push_back(recvline);
								continue;
							}
							recvline[0]=0;
						}
						else if(tok[0]=="SA"){// show article
							if(strcmp(recvline, SUCCESS)==0){
								puts("show article:");
								if((n = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL))<0){
									puts("n<0");
								}
								else{
									recvline[n] = 0; /* null terminate */
									totalbytes=atoi(recvline);
									for(int i=0;i<totalbytes;i++){
										for(int j=0;j<4;j++){
											if((n = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL))<0){
												puts("n<0");
											}
											else{
												recvline[n] = 0; /* null terminate */
												printf("%s |", recvline);
											}
										}
										puts("");
									}
									recvline[0]='\0';
								}
							}
						}
						else if(tok[0]=="AU"){// show article by author
							if(strcmp(recvline, SUCCESS)==0){
								puts("show article by author:");
								if((n = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL))<0){
									puts("n<0");
								}
								else{
									recvline[n] = 0; /* null terminate */
									totalbytes=atoi(recvline);
									for(int i=0;i<totalbytes;i++){
										for(int j=0;j<4;j++){
											if((n = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL))<0){
												puts("n<0");
											}
											else{
												recvline[n] = 0; /* null terminate */
												printf("%s |", recvline);
											}
										}
										puts("");
									}
									recvline[0]='\0';
								}
							}
						}
						else if(tok[0]=="A"){// add article
							if(strcmp(recvline, SUCCESS)==0){
								// addArticle(); // send txt (not going to implement) 

								// send file
								// addFile();
								// // send how many file? ( ack
								// sprintf(sendline, "%d", (int)sendBuff.size());
								// sendto(sockfd, sendline, strlen(sendline), 0, pservaddr, servlen);
								// WaitingAck=1;
								// waitingAck();
								// for(int i=0;i<sendBuff.size();i++){
									
								// 	// send file ( send totalbytes with ack and others no ack

								// }
								// sendBuff.clear();
							}
							// 
						}
						else if(tok[0]=="E"){// enter article
							puts("enter article");
							isCurrentAuthor=0;
							if(strcmp(recvline, SUCCESS)==0){
								receiveArticle(fp, sockfd, pservaddr, servlen);
								recvline[0]='\0';
							}
						}
						//------------------------------------------------
						else if(tok[0]=="Y"){// yell
							// do nothing ( maybe puts
							// puts(recvline);
						}
						else if(tok[0]=="T"){// tell
							// do nothing ( maybe puts
							// puts(recvline);
						}
					}
					else if(me.state==Article){
						if(tok[0]=="R"){// response

							if(strcmp(recvline, SUCCESS)==0){
								// puts(recvline);
								receiveArticle(fp, sockfd, pservaddr, servlen);
								recvline[0]='\0';
							}

						}
						else if(tok[0]=="D"){// download
							if(strcmp(recvline, SUCCESS)==0){
								// puts(recvline);
								downloadFile(tok[1], fp, sockfd, pservaddr, servlen);
								receiveArticle(fp, sockfd, pservaddr, servlen);
								recvline[0]='\0';
							}
						}
						else if(tok[0]=="U"){// upload
							if(strcmp(recvline, SUCCESS)==0){
								// puts(recvline);
								// send file ( still ack )
								sendFile(tok[1], fp, sockfd, pservaddr, servlen);
								receiveArticle(fp, sockfd, pservaddr, servlen);
								recvline[0]='\0';
							}
						}
						else if(tok[0]=="Add"){// add black list
							if(strcmp(recvline, SUCCESS)==0){
								// puts(recvline);
							}
						}
						else if(tok[0]=="Del"){// delete black list
							if(strcmp(recvline, SUCCESS)==0){
								// puts(recvline);
							}
						}
						else if(tok[0]=="DELETE"){// delete article
							if(strcmp(recvline, SUCCESS)==0){
								// puts(recvline);
								puts("if you didn't delete success you will still be sent to the prev page");
								isCurrentAuthor=0;
								me.state=Normal;
							}

						}
						else if(tok[0]=="Ret"){// return
							if(strcmp(recvline, SUCCESS)==0){
								// puts(recvline);
								isCurrentAuthor=0;
								me.state=Normal;

							}
						}
					}
					puts(recvline);
					//client doesn't have to ack
					showMsg();

				}
				
			}
			// sth in buffer has to be send
			// if(sendBuff.size()){
			// 	// no ack
			// }
			
		}
		else{
			sendbytes=strlen(sendline);
			waitingAck(fp, sockfd, pservaddr, servlen);
		}
		// printf("retVal = %d\n", retVal);
	}
}

int main(int argc, char **argv) {
	int sockfd;
	struct sockaddr_in servaddr;
	system("mkdir Download");
	chdir("Download");
	if (argc < 2){
		puts("usage: udpcli <IPaddress> <port>");
	}
	else{
		SERV_PORT=atoi(argv[2]);
	}
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
	inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	WaitingAck=0;
	dg_cli(stdin, sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
	exit(0);

	return 0;
}