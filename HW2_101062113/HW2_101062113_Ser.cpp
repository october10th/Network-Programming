#include "header.h"
#include <sqlite3.h>
int listenfd, connfd, udpfd, nready, maxfdp1;
char mesg[MAXLINE], sendline[MAXLINE];
pid_t childpid;
int SERV_PORT=7122;
fd_set rset;
ssize_t n;
socklen_t len;
const int on = 1;
struct sockaddr_in cliaddr, servaddr;
vector<string>tok;
Account currentUser;
//
map<User, Account>userAccount;
map<Account, User>accountUser;
sqlite3 *db;
vector< map < string, string > >result;
int totalbytes, sendbytes;

void sig_chld(int signo){
	pid_t pid;
	int stat;
	while((pid = waitpid(-1, &stat, WNOHANG))>0){
		printf("stat = %d\n", stat);
		printf("child %d terminated\n", (int)pid);
	}
	return;
}

int callback(void *data, int argc, char **argv, char **azColName){
	int i;
	map<string, string>m;
	m.clear();
	fprintf(stderr, "%s: ", (const char*)data);
	for(i=0; i<argc; i++){
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
		m[azColName[i]]=argv[i] ? argv[i] : "NULL";
		
	}
	printf("\n");

	result.push_back(m);
	return 0;
}
void sql_exec(string sql){
	result.clear();
	char *zErrMsg = NULL;
	const char* data = "Callback function called";
	int rc=sqlite3_exec(db, sql.c_str(), callback, (void*)data, &zErrMsg);
	if( rc != SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}else{
		fprintf(stdout, "Operation done successfully\n");
	}
}


void showDB(){
	int rows, cols, i, j;
	char *errMsg = NULL;
	char **result;
	string sql="SELECT * FROM user;";
	sqlite3_get_table(db , sql.c_str(), &result , &rows, &cols, &errMsg);
	/* 列出所有資料 */
	for (i=0;i<=rows;i++) {
	    for (j=0;j<cols;j++) {
	        printf("%s\t", result[i*cols+j]);
	    }
	    printf("\n");
	}
}
void showResult(){
	for(int i=0;i<result.size();i++){
		for (std::map<string, string>::iterator it=result[i].begin(); it!=result[i].end(); ++it)
    		cout << it->first << " => " << it->second << '\n';

	}
}
void init(){
	int rc, i;
	char *zErrMsg = NULL;
	
	/* Open database */
	rc = sqlite3_open("hw2.db", &db);
	if( rc ){
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		exit(0);
	}
	else{
		fprintf(stdout, "Opened database successfully\n");
	}
	/* Create SQL statement */
   	// build table
   	for(i=0;i<5;i++){
		rc=sqlite3_exec(db, create_sql[i], 0, 0, &zErrMsg);
		if(rc!=SQLITE_OK){
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
      		sqlite3_free(zErrMsg);
		}else{
			fprintf(stdout, "Records created successfully\n");
		}
	}
	showDB();

}

int insertUser(Account acc){
	string sql="SELECT * FROM user WHERE ID='"+acc.ID+"';";
	cout<<sql<<endl;
	sql_exec(sql);
	if(result.size()){
		showResult();
		return 0;
	}
	sql="INSERT INTO user (ID, pw) VALUES('"+acc.ID+"', '"+acc.pw+"');";
	sql_exec(sql);
	showResult();

	return 1;
}
void deleteUser(Account acc){
	string sql="DELETE FROM user WHERE ID='"+acc.ID+"' AND pw='"+acc.pw+"';";
	sql_exec(sql);
	showResult();// print nothing
}
int login(Account acc){
	
	string sql="SELECT * FROM user WHERE ID='"+acc.ID+"' AND pw='"+acc.pw+"';";
	cout<<sql<<endl;
	sql_exec(sql);
	if(result.size()){
		showResult();
		return 1;
	}
	return 0;
}
int addArticle(string ID, string title, string content, string IP, int port){
	string sql="SELECT MAX(aid) FROM article;";
	int aid=0;
	cout<<sql<<endl;
	sql_exec(sql);
	showResult();
	if(result[0][string("MAX(aid)")]!="NULL")aid=1+atoi(result[0][string("MAX(aid)")].c_str());
	sql="INSERT INTO article (aid,author,title,content,IP,port) VALUES('"+toString(aid)+"', '"+ID+"', '"+title+"', '"+content+"', '"+IP+"', '"+toString(port)+"');";
	cout<<sql<<endl;
	sql_exec(sql);
	showResult();
	printf("aid = %d\n", aid);
	return aid;
}
void showArticle(){
	string sql="SELECT aid,author,title,hit FROM article;";
	cout<<sql<<endl;
	sql_exec(sql);
	showResult();
}
void showArticle(string ID){
	string sql="SELECT aid,author,title,hit FROM article WHERE author='"+ID+"' ;";
	cout<<sql<<endl;
	sql_exec(sql);
	showResult();
}
void enterArticle(int aid){
	string sql="SELECT * FROM article WHERE aid='"+toString(aid)+"' ;";
	cout<<sql<<endl;
	sql_exec(sql);
	showResult();
}
void deleteArticle(string ID, int aid){
	string sql="DELETE FROM article WHERE author='"+ID+"' AND aid='"+toString(aid)+"';";
	cout<<sql<<endl;
	sql_exec(sql);
	showResult();
}
void updateHit(int aid){
	string sql="SELECT hit FROM article WHERE aid='"+toString(aid)+"';";
	cout<<sql<<endl;
	sql_exec(sql);
	showResult();
	int hit=atoi(result[0].begin()->second.c_str());
	sql="UPDATE article SET hit='"+toString(hit+1)+"' WHERE aid='"+toString(aid)+"';";
	cout<<sql<<endl;
	sql_exec(sql);
	showResult();
}
int addResponse(string ID, int aid, string content, string IP, int port){
	string sql="SELECT MAX(rid) FROM response;";
	int rid=0;
	cout<<sql<<endl;
	sql_exec(sql);
	showResult();
	if(result[0][string("MAX(rid)")]!="NULL")rid=1+atoi(result[0][string("MAX(rid)")].c_str());
	sql="INSERT INTO response (aid,rid,author,content,IP,port) VALUES('"+toString(aid)+"', '"+toString(rid)+"', '"+ID+"', '"+content+"', '"+IP+"', '"+toString(port)+"');";
	cout<<sql<<endl;
	sql_exec(sql);
	showResult();
	printf("rid = %d\n", rid);
	return rid;
}
void enterResponse(int aid){
	string sql="SELECT * FROM response WHERE aid='"+toString(aid)+"' ;";
	cout<<sql<<endl;
	sql_exec(sql);
	showResult();
}
void deleteResponse(int aid){
	string sql="DELETE FROM response WHERE aid='"+toString(aid)+"';";
	cout<<sql<<endl;
	sql_exec(sql);
	showResult();
}
int isBlackList(string ID, int aid){
	string sql="SELECT * FROM blacklist WHERE aid='"+toString(aid)+"' AND ID='"+ID+"';";
	cout<<sql<<endl;
	sql_exec(sql);
	showResult();
	cout<<"blacklist:"<<result.size()<<endl;
	return (int)result.size();
}
void addBlacklist(string ID, int aid){
	string sql="INSERT INTO blacklist (ID,aid) VALUES ('"+ID+"','"+toString(aid)+"');";
	cout<<sql<<endl;
	sql_exec(sql);
	showResult();
}
void deleteBlacklist(string ID, int aid){
	string sql="DELETE FROM blacklist WHERE ID='"+ID+"' AND aid='"+toString(aid)+"';";
	cout<<sql<<endl;
	sql_exec(sql);
	showResult();
}
void deleteBlacklistAll(int aid){
	string sql="DELETE FROM blacklist WHERE aid='"+toString(aid)+"';";
	cout<<sql<<endl;
	sql_exec(sql);
	showResult();
}

int isAuthor(string ID, int aid){
	string sql="SELECT * FROM article WHERE author='"+ID+"' AND aid='"+toString(aid)+"';";
	cout<<sql<<endl;
	sql_exec(sql);
	showResult();
	return (int)result.size();
}
void sendResult(){
	string str=toString(result.size());
	sendto(udpfd, str.c_str(), str.length(), 0, (struct sockaddr *) &cliaddr, len);
	for(int i=0;i<result.size();i++){
		str=toString(result[i].size()*2);
		sendto(udpfd, str.c_str(), str.length(), 0, (struct sockaddr *) &cliaddr, len);
		
		for (std::map<string, string>::iterator it=result[i].begin(); it!=result[i].end(); ++it){
			sendto(udpfd, it->first.c_str(), it->first.length(), 0, (struct sockaddr *) &cliaddr, len);
			sendto(udpfd, it->second.c_str(), it->second.length(), 0, (struct sockaddr *) &cliaddr, len);
		}
	}
}
int addFile(string ID, int aid, string path, string IP, int port){
	string sql="SELECT MAX(fid) FROM filelist;";
	int fid=0;
	cout<<sql<<endl;
	sql_exec(sql);
	showResult();
	if(result[0][string("MAX(rid)")]!="NULL")fid=1+atoi(result[0][string("MAX(fid)")].c_str());
	sql="INSERT INTO filelist (aid,fid,author,path,IP,port) VALUES('"+toString(aid)+"', '"+toString(fid)+"', '"+ID+"', '"+path+"', '"+IP+"', '"+toString(port)+"');";
	cout<<sql<<endl;
	sql_exec(sql);
	showResult();
	printf("fid = %d\n", fid);
	return fid;
}
void enterFile(int aid){
	string sql="SELECT * FROM filelist WHERE aid='"+toString(aid)+"' ;";
	cout<<sql<<endl;
	sql_exec(sql);
	showResult();
}
void deleteFile(int aid){
	//delete file from computer
	enterFile(aid);
	for(int i=0;i<result.size();i++){
		for (std::map<string, string>::iterator it=result[i].begin(); it!=result[i].end(); ++it){
    		if(it->first=="path"){
    			cout<<"delete file : "<<it->second<<endl;
    			string str="rm "+it->second;
				system(str.c_str());
    		}
    	}
	}
	//---
	string sql="DELETE FROM filelist WHERE aid='"+toString(aid)+"';";
	cout<<sql<<endl;
	sql_exec(sql);
	showResult();
	
}
void deleteFile(string filename){
	string sql="DELETE FROM filelist WHERE path='"+filename+"';";
	cout<<sql<<endl;
	sql_exec(sql);
	showResult();
}
int getTotalbytes(string filename){
	FILE *fin=fopen(filename.c_str(), "rb");
	int totalbytes=0;
	int sendbytes;
	while (!feof(fin)) {
		
        sendbytes=fread(sendline, sizeof(char), sizeof(sendline), fin);
        totalbytes+=sendbytes;
    }
    printf("totalbytes = %d\n", totalbytes);
	fclose(fin);
	return totalbytes;
}

void receiveFile(string filename){
	int recvbytes=0;
	if((n = recvfrom(udpfd, mesg, MAXLINE, 0, (struct sockaddr *) &cliaddr, &len))<0){
		printf("UDP connected from %d (%s)\n", getPort(cliaddr), getIP(cliaddr));
		puts("n<0");	
	}
	else{
		printf("UDP connected from %d (%s)\n", getPort(cliaddr), getIP(cliaddr));
		mesg[n]=0;
		printf("from UDP:[%s]\n", mesg);
		totalbytes=atoi(mesg);
		puts("send back ACK");
		sendto(udpfd, ACK, strlen(ACK), 0, (struct sockaddr *) &cliaddr, len);
		
		FILE *fout=fopen(filename.c_str(), "wb");
		while(recvbytes<totalbytes){

			if((n = recvfrom(udpfd, mesg, MAXLINE, 0, (struct sockaddr *) &cliaddr, &len))<0){
				// printf("UDP connected from %d (%s)\n", getPort(cliaddr), getIP(cliaddr));
				puts("n<0");	
			}
			else{
				// printf("UDP connected from %d (%s)\n", getPort(cliaddr), getIP(cliaddr));
				
				sendto(udpfd, ACK, strlen(ACK), 0, (struct sockaddr *) &cliaddr, len);
				mesg[n]=0;
				fwrite(mesg, sizeof(char), n, fout);

				printf("recvbytes = %d (%lu)\n", recvbytes+=n, n);
			}
		}
		if(recvbytes!=totalbytes){
			string str="rm "+filename;
			system(str.c_str());deleteFile(filename);
			printf("recvbytes %d totalbytes %d\n", recvbytes, totalbytes);
			puts("packet loss delete file");
		}
		else{
			puts("no packet loss");
		}
		fclose(fout);
	}
}
void sendFile(string filename){
	int tot=0;
	FILE *fin=fopen(filename.c_str(), "rb");
	totalbytes=getTotalbytes(filename);
	printf("start sending %d bytes\n", totalbytes);
	sprintf(sendline, "%d", totalbytes);
    sendto(udpfd, sendline, strlen(sendline), 0, (struct sockaddr *) &cliaddr, len);
	while (!feof(fin)) {
        sendbytes=fread(sendline, sizeof(char), sizeof(sendline), fin);
        sendto(udpfd, sendline, sendbytes, 0, (struct sockaddr *) &cliaddr, len);
        printf("sendbytes=%d (%d)\n", sendbytes, tot+=sendbytes);
    }

    fclose(fin);
}
void sendArticle(){
	// article
	enterArticle(currentUser.currentAid);
	sendResult();
	
	// response
	enterResponse(currentUser.currentAid);
	sendResult();

	// file
	enterFile(currentUser.currentAid);
	sendResult();
}
int main(int argc, char **argv) {
	
	init();
	system("mkdir Upload");
	chdir("Upload");
	userAccount.clear();
	accountUser.clear();
	if(argc>1)SERV_PORT=atoi(argv[1]);
	/* for create listening TCP socket */
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	bzero(&servaddr, sizeof(servaddr)); servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
	listen(listenfd, LISTENQ);
	/* for create UDP socket */
	udpfd = socket(AF_INET, SOCK_DGRAM, 0);
	bzero(&servaddr, sizeof(servaddr)); servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);
	bind(udpfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
	signal(SIGCHLD, sig_chld);/* must call waitpid() */

	FD_ZERO(&rset);
	maxfdp1 = max(listenfd, udpfd) + 1;
	for ( ; ; ) {
		FD_SET(listenfd, &rset);
		FD_SET(udpfd, &rset);
		if ( (nready = select(maxfdp1, &rset, NULL, NULL, NULL)) < 0)
		{
			if (errno == EINTR)
				continue; /* back to for() */
			else
				printf("select error");
		}
		if (FD_ISSET(udpfd, &rset)) {
			len = sizeof(cliaddr);
			if((n = recvfrom(udpfd, mesg, MAXLINE, 0, (struct sockaddr *) &cliaddr, &len))<0){
				printf("UDP connected from %d (%s)\n", getPort(cliaddr), getIP(cliaddr));
				puts("n<0");	
			}
			else{
				printf("UDP connected from %d (%s)\n", getPort(cliaddr), getIP(cliaddr));
				mesg[n]=0;
				printf("from UDP:[%s]\n", mesg);

				usleep(500);
				// ACK
				puts("send back ACK");
				sendto(udpfd, ACK, strlen(ACK), 0, (struct sockaddr *) &cliaddr, len);
				// get token
				tok.clear();
				tok=parse(mesg);
				if(userAccount.find(User(getIP(cliaddr), getPort(cliaddr), cliaddr))==userAccount.end()){
					// not login

					// sscanf(mesg, "%s%s%s", cmd, tok[0], tok[1]);
					
					if(tok[0][0]=='R'){
						//register
						currentUser=Account(tok[1], tok[2]);
						currentUser.state=Normal;
						currentUser.currentAid=-1;
						userAccount[User(getIP(cliaddr), getPort(cliaddr), cliaddr)]=currentUser;
						accountUser[currentUser]=User(getIP(cliaddr), getPort(cliaddr), cliaddr);
						if(insertUser(currentUser)){
							puts("register sucess");
							sendto(udpfd, SUCCESS, strlen(SUCCESS), 0, (struct sockaddr *) &cliaddr, len);
						}
						else{
							puts("register fail");
							sendto(udpfd, FAIL, strlen(FAIL), 0, (struct sockaddr *) &cliaddr, len);
						}
					}
					else if(tok[0][0]=='L'){
						if(login(Account(tok[1], tok[2]))){

							currentUser=Account(tok[1], tok[2]);
							currentUser.state=Normal;
							userAccount[User(getIP(cliaddr), getPort(cliaddr), cliaddr)]=currentUser;
							accountUser[currentUser]=User(getIP(cliaddr), getPort(cliaddr), cliaddr);
							
							puts("login sucess");
							sendto(udpfd, SUCCESS, strlen(SUCCESS), 0, (struct sockaddr *) &cliaddr, len);
						}
						else{
							puts("login fail");
							sendto(udpfd, FAIL, strlen(FAIL), 0, (struct sockaddr *) &cliaddr, len);
							
						}
					}
				}
				else{ // login already
					currentUser=userAccount[User(getIP(cliaddr), getPort(cliaddr), cliaddr)];
					accountUser[currentUser]=User(getIP(cliaddr), getPort(cliaddr), cliaddr);
					if(currentUser.state==Normal){
						if(tok[0]=="Del"){// delete account
							puts("Delete account");
							deleteUser(currentUser);// delete in db
							userAccount.erase(User(getIP(cliaddr), getPort(cliaddr), cliaddr));
							accountUser.erase(currentUser);
							sendto(udpfd, SUCCESS, strlen(SUCCESS), 0, (struct sockaddr *) &cliaddr, len);
						}
						else if(tok[0]=="L"){// logout
							puts("Logout");
							userAccount.erase(User(getIP(cliaddr), getPort(cliaddr), cliaddr));
							accountUser.erase(currentUser);
							sendto(udpfd, SUCCESS, strlen(SUCCESS), 0, (struct sockaddr *) &cliaddr, len);
						}
						//------------------------------------------------
						else if(tok[0]=="SU"){// show user
							for (std::map<Account, User>::iterator it=accountUser.begin(); it!=accountUser.end(); ++it){
	    						sendto(udpfd, (it->first).ID.c_str(), (it->first).ID.length(), 0, (struct sockaddr *) &cliaddr, len);
	    					}
	    					sendto(udpfd, SUCCESS, strlen(SUCCESS), 0, (struct sockaddr *) &cliaddr, len);
						}
						else if(tok[0]=="SA"){// show article
							puts("show article");

							showArticle();
							cout<<"result size: "<<result.size()<<endl;
							sendto(udpfd, SUCCESS, strlen(SUCCESS), 0, (struct sockaddr *) &cliaddr, len);
							
							// send all article aid title author
							string str=toString(result.size());
							sendto(udpfd, str.c_str(), str.length(), 0, (struct sockaddr *) &cliaddr, len);
							for(int i=0;i<result.size();i++){
								for (std::map<string, string>::iterator it=result[i].begin(); it!=result[i].end(); ++it){
									str=it->first+" : "+it->second;
		    						sendto(udpfd, str.c_str(), str.length(), 0, (struct sockaddr *) &cliaddr, len);
		    					}
							}

						}
						else if(tok[0]=="AU"){// show article by author
							puts("show article");

							showArticle(tok[1]);//author ID
							cout<<"result size: "<<result.size()<<endl;
							sendto(udpfd, SUCCESS, strlen(SUCCESS), 0, (struct sockaddr *) &cliaddr, len);
							
							// send all article aid title author
							string str=toString(result.size());
							sendto(udpfd, str.c_str(), str.length(), 0, (struct sockaddr *) &cliaddr, len);
							for(int i=0;i<result.size();i++){
								for (std::map<string, string>::iterator it=result[i].begin(); it!=result[i].end(); ++it){
									str=it->first+" : "+it->second;
		    						sendto(udpfd, str.c_str(), str.length(), 0, (struct sockaddr *) &cliaddr, len);
		    					}
							}

						}
						else if(tok[0]=="A"){// add article
							puts("add article");
							string str=tok[2];
							for(int i=3;i<tok.size();i++){
								str+=" ";
								str+=tok[i];
							}
							addArticle(currentUser.ID, tok[1], str, getIP(cliaddr), getPort(cliaddr));
							sendto(udpfd, SUCCESS, strlen(SUCCESS), 0, (struct sockaddr *) &cliaddr, len);
							// receiving files
							// puts("receiving files");
							// FD_SET(listenfd, &rset);
							// FD_SET(udpfd, &rset);

						}
						else if(tok[0]=="E"){// enter article
							printf("enter article: %s\n", tok[1].c_str());
							updateHit(atoi(tok[1].c_str()));
							sendto(udpfd, SUCCESS, strlen(SUCCESS), 0, (struct sockaddr *) &cliaddr, len);
							
							if(isBlackList(currentUser.ID, atoi(tok[1].c_str()))){
								string str=toString(0);
								sendto(udpfd, str.c_str(), str.length(), 0, (struct sockaddr *) &cliaddr, len);
							}
							else{
								
								currentUser.currentAid=atoi(tok[1].c_str());
								currentUser.state=Article;
								userAccount[User(getIP(cliaddr), getPort(cliaddr), cliaddr)]=currentUser;
								sendArticle();
							}
						}
						//------------------------------------------------
						else if(tok[0]=="Y"){// yell
							
							string str=currentUser.ID+" yell : "+tok[1];
							for (std::map<Account, User>::iterator it=accountUser.begin(); it!=accountUser.end(); ++it){
	    						sendto(udpfd, str.c_str(), str.length(), 0, (struct sockaddr *) &(it->second.cliaddr), sizeof(it->second.cliaddr));
	    					}
						}
						else if(tok[0]=="T"){// tell
							if(accountUser.find(Account(tok[1]))!=accountUser.end()){
								User usr=accountUser[Account(tok[1])];
								string str=currentUser.ID+" tell : "+tok[2];
								sendto(udpfd, str.c_str(), str.length(), 0, (struct sockaddr *) &usr.cliaddr, sizeof(usr.cliaddr));
	    					
							}
							
						}
					
					}
					else if(currentUser.state==Article){
						if(tok[0]=="R"){// response
							string str=tok[1];
							for(int i=2;i<tok.size();i++){
								str+=" ";
								str+=tok[i];
							}
							addResponse(currentUser.ID, currentUser.currentAid, str, getIP(cliaddr), getPort(cliaddr));
							sendto(udpfd, SUCCESS, strlen(SUCCESS), 0, (struct sockaddr *) &cliaddr, len);
							sendArticle();
						}
						else if(tok[0]=="D"){// download
							sendto(udpfd, SUCCESS, strlen(SUCCESS), 0, (struct sockaddr *) &cliaddr, len);
							sendFile(tok[1]);
							sendArticle();
						}
						else if(tok[0]=="U"){// upload
							sendto(udpfd, SUCCESS, strlen(SUCCESS), 0, (struct sockaddr *) &cliaddr, len);
							
							addFile(currentUser.ID, currentUser.currentAid, tok[1], getIP(cliaddr), getPort(cliaddr));
							receiveFile(tok[1]);
							sendArticle();
						}
						else if(tok[0]=="Add"){// add black list
							addBlacklist(tok[1], currentUser.currentAid);
							puts("add black list");
							sendto(udpfd, SUCCESS, strlen(SUCCESS), 0, (struct sockaddr *) &cliaddr, len);
						}
						else if(tok[0]=="Del"){// delete black list
							deleteBlacklist(tok[1], currentUser.currentAid);
							puts("delete black list");
							sendto(udpfd, SUCCESS, strlen(SUCCESS), 0, (struct sockaddr *) &cliaddr, len);
							
						}
						else if(tok[0]=="DELETE"){// delete article
							if(isAuthor(currentUser.ID, currentUser.currentAid)){
								// article
								deleteArticle(currentUser.ID, currentUser.currentAid);
								// response
								deleteResponse(currentUser.currentAid);
								// files
								deleteFile(currentUser.currentAid);
								// black all
								deleteBlacklistAll(currentUser.currentAid);
								
								currentUser.currentAid=-1;
								currentUser.state=Normal;
								userAccount[User(getIP(cliaddr), getPort(cliaddr), cliaddr)]=currentUser;
								puts("delete article");
								sendto(udpfd, SUCCESS, strlen(SUCCESS), 0, (struct sockaddr *) &cliaddr, len);	
							}
							
							
						}
						else if(tok[0]=="Ret"){// return
							currentUser.currentAid=-1;
							currentUser.state=Normal;
							userAccount[User(getIP(cliaddr), getPort(cliaddr), cliaddr)]=currentUser;
							sendto(udpfd, SUCCESS, strlen(SUCCESS), 0, (struct sockaddr *) &cliaddr, len);
							
						}

					}
				}
				// send back (debug	
				// sendto(udpfd, mesg, n, 0, (struct sockaddr *) &cliaddr, len);
			}
			
		}
	}
	sqlite3_close(db);
}

