#include "header.h"

int SERV_PORT=7122;

int WaitingAck;
Account me;
string currentAuthor;// current author
void showMsg(){

	if(me.state==Init){
		puts("************* Welcome *****************");
		puts("[R]egister [L]ogin");
		puts("----------------------------------------");
		puts("usage: [R]egister [User ID] [Password]");
	}
	else if(me.state==Normal){
		printf("************* Hello %s *****************\n", me.ID.c_str());
		puts("[SU]Show User [SA]Show Article [A]dd Article [E]nter Article");
		puts("[Y]ell [T]ell [L]ogout");
		puts("[Del]eteAccount");// change account?
		puts("----------------------------------------");
		puts("[Y]ell [廣播內容]");
		puts("[T]ell [User ID] [私密內容]");
		puts("[E]nter Article [文章 ID]");

	}
	else if(me.state==Article){
		puts("[R]esponse [D]ownload [U]pload");
		if(currentAuthor==me.ID)// current author is me
			puts("[A]dd/[D]el [B]lack list [DEL]ete");// only author
		puts("[R]eturn");
		puts("----------------------------------------");
		puts("[R]esponse [留言內容]");
		if(currentAuthor==me.ID)// current author is me
			puts("[A]dd/[D]el black list [User ID]");// only author
		puts("[D]ownload/[U]pload [File name]");
		
	}
	
}

void dg_cli(FILE *fp, int sockfd, const struct sockaddr *pservaddr, socklen_t servlen) {
	int n, maxfdp1;
	fd_set rset;
	char sendline[MAXLINE], recvline[MAXLINE + 1];
	int retVal;
	vector<string>tok;
	FD_ZERO(&rset); /*initial select*/
	me.state=Init;
	showMsg();// Login or Register

	for ( ; ; ) {
		
		struct timeval timeout;
		timeout.tv_sec=0;
		timeout.tv_usec=1000;
		if(!WaitingAck){
			FD_SET(fileno(fp), &rset); /*fp: I/O file pointer*/
			FD_SET(sockfd, &rset); /*fileno convers fp into descriptor*/
			maxfdp1 = max(fileno(fp), sockfd) + 1;
			retVal=select(maxfdp1, &rset, NULL, NULL, NULL);
			if (FD_ISSET(fileno(fp), &rset)) { /* input is readable */
			
				if (fgets(sendline, MAXLINE, fp) == NULL)
					return; /* all done */
				
				sendline[strlen(sendline)-1]='\0';// kill '\n'
				sendto(sockfd, sendline, strlen(sendline), 0, pservaddr, servlen);
				WaitingAck=1;

			}
			if (FD_ISSET(sockfd, &rset)) { /* socket is readable */
				if((n = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL))<0){
					puts("n<0");
				}
				else{
					recvline[n] = 0; /* null terminate */
					puts("recv:");
					tok.clear();
					tok=parse(sendline);
					if(me.state==Init){
						if(strcmp(recvline, SUCCESS)==0){
							me.state=Normal;
							
							me.ID=tok[0],me.pw=tok[1];

						}
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
						else if(tok[0]=="SA"){

						}
					}
					fputs(recvline, stdout);
					//client doesn't have to ack

				}
			}
			
		}
		else{
			while(1){
				FD_SET(fileno(fp), &rset); /*fp: I/O file pointer*/
				FD_SET(sockfd, &rset); /*fileno convers fp into descriptor*/
				maxfdp1 = max(fileno(fp), sockfd) + 1;
				retVal=select(maxfdp1, &rset, NULL, NULL, &timeout);
				if (FD_ISSET(sockfd, &rset)) { /* socket is readable */
					if((n = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL))<0){
						puts("n<0");
					}
					else{
						recvline[n] = 0; /* null terminate */
						puts("recv(waiting ack):");
						fputs(recvline, stdout);
						if(strcmp(recvline, ACK)==0){
							break;
						}

					}
				}
				if(retVal==0){// 0 => timeout
					// resend
					puts("resend");
					sendto(sockfd, sendline, strlen(sendline), 0, pservaddr, servlen);
					
				}
			}
			if(retVal<0){
				puts("error");
			}
			WaitingAck=0;

		}
		printf("retVal = %d\n", retVal);
		

		
	}
}

int main(int argc, char **argv) {
	int sockfd;
	struct sockaddr_in servaddr;
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