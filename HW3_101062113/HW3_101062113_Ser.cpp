#include "header.h"
#include <sqlite3.h>
// int listenfd, connfd, udpfd, nready, maxfdp1;

pid_t childpid;
int SERV_PORT=7122;
// fd_set rset;
// const int on = 1;


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


void showDB(){// show user list at init
	int rows, cols, i, j;
	char *errMsg = NULL;
	char **result;
	string sql="SELECT * FROM user;";
	sqlite3_get_table(db , sql.c_str(), &result , &rows, &cols, &errMsg);
	for (i=0;i<=rows;i++) {
	    for (j=0;j<cols;j++) {
	        printf("%s\t", result[i*cols+j]);
	    }
	    printf("\n");
	}
}
//------------------------------------------------
void showResult(){
	for(int i=0;i<result.size();i++){
		for (std::map<string, string>::iterator it=result[i].begin(); it!=result[i].end(); ++it)
    		cout << it->first << " => " << it->second << '\n';

	}
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
int insertUser(string ID, string pw){
	return insertUser(Account(ID, pw));
}
//------------------------------------------------
void clearFilelist(){
	string sql="DELETE FROM filelist;";
	cout<<sql<<endl;
	sql_exec(sql);
}
void showFilelist(){
	string sql="SELECT * FROM filelist;";
	cout<<sql<<endl;
	sql_exec(sql);
}
int addFile(string ID, string path){
	string sql="SELECT MAX(fid) FROM filelist;";
	int fid=0;
	cout<<sql<<endl;
	sql_exec(sql);
	showResult();
	if(result[0][string("MAX(fid)")]!="NULL")fid=1+atoi(result[0][string("MAX(fid)")].c_str());
	sql="INSERT INTO filelist (fid,author,path) VALUES('"+toString(fid)+"', '"+ID+"', '"+path+"');";
	cout<<sql<<endl;
	sql_exec(sql);
	showResult();
	printf("fid = %d\n", fid);
	return fid;
}
void deleteFile(string ID){
	string sql="DELETE FROM filelist WHERE author='"+ID+"';";
	cout<<sql<<endl;
	sql_exec(sql);
	showResult();
}
int existNumFile(string path){
	string sql="SELECT * FROM filelist WHERE path='"+path+"';";
	cout<<sql<<endl;
	sql_exec(sql);
	if(result.size()){
		showResult();
		return result.size();
	}
	return 0;
}
vector<string>getFileAuthor(){
	vector<string>author;
	author.clear();
	for(int i=0;i<result.size();i++){
		for (std::map<string, string>::iterator it=result[i].begin(); it!=result[i].end(); ++it){
			if(it->first=="author"){
				author.push_back(it->second);
			}
		}
	}
	return author;
}
void initServerFilelist(){
	vector<string>filelist=getCurFilelist();
	for(int i=0;i<filelist.size();i++){
		addFile("server", filelist[i]);
	}
}
void recvFilelist(int connfd, string ID){
	char recvline[MAXLINE];
	int n=recvWrite(connfd, recvline);
	int totallines=atoi(recvline);
	for(int i=0;i<totallines;i++){
		n=recvWrite(connfd, recvline);
		addFile(ID, recvline);
	}
}
//------------------------------------------------
void init(){
	int rc, i;
	char *zErrMsg = NULL;
	
	/* Open database */
	rc = sqlite3_open("hw3.db", &db);
	if( rc ){
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		exit(0);
	}
	else{
		fprintf(stdout, "Opened database successfully\n");
	}
	/* Create SQL statement */
   	
   	// clear filelist table?
	// DROP TABLE FROM filelist?
	// clear filelist
	clearFilelist();

   	// build table
   	for(i=0;i<TABLE_NUM;i++){
		rc=sqlite3_exec(db, create_sql[i], 0, 0, &zErrMsg);
		if(rc!=SQLITE_OK){
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
      		sqlite3_free(zErrMsg);
		}else{
			fprintf(stdout, "Records created successfully\n");
		}
	}
	// account for server
	insertUser("server", "root");
	showDB();

}
void deleteUser(Account acc){
	string sql="DELETE FROM user WHERE ID='"+acc.ID+"' AND pw='"+acc.pw+"';";
	sql_exec(sql);
	showResult();// print nothing
	// del file
	deleteFile(acc.ID);
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


//------------------------------------------------
void sendResult(int connfd, int scale, vector<map<string, string> > result){
	char recvline[MAXLINE];
	string str=toString(result.size());
	writeRecv(connfd, str.c_str(), str.length());

	for(int i=0;i<result.size();i++){
		str=toString(result[i].size()*scale);
		writeRecv(connfd, str.c_str(), str.length());

		for (std::map<string, string>::iterator it=result[i].begin(); it!=result[i].end(); ++it){
			writeRecv(connfd, it->first.c_str(), it->first.length());
			writeRecv(connfd, it->second.c_str(), it->second.length());
		}
	}
}
//------------------------------------------------
void sendUser(int connfd){
	string str=toString(accountUser.size());
	writeRecv(connfd, str.c_str(), str.length());cout<<str;
	
			
	for (std::map<Account, User>::iterator it=accountUser.begin(); it!=accountUser.end(); ++it){
		str=toString(3*2);// ID IP port
		writeRecv(connfd, str.c_str(), str.length());
		
		
		str="ID";
		writeRecv(connfd, str.c_str(), str.length());
		
		str=it->first.ID;
		writeRecv(connfd, str.c_str(), str.length());
		
		
		str="IP";
		writeRecv(connfd, str.c_str(), str.length());
		
		str=it->second.IP;
		writeRecv(connfd, str.c_str(), str.length());
		
		str="port";
		writeRecv(connfd, str.c_str(), str.length());

		str=toString(it->second.port);
		writeRecv(connfd, str.c_str(), str.length());
		
	}
}


void *run(void *arg){
	pthread_detach(pthread_self());
	vector<string>tok;
	Account currentUser;
	int n;
	char  mesg[MAXLINE], sendline[MAXLINE], recvline[MAXLINE];
	ClientSock clientSock=*(ClientSock*)arg;
	int connfd=clientSock.connfd;
	struct sockaddr_in cliaddr=clientSock.addr;
	free(arg);

	while((n=receive(connfd, recvline)) >0) {

		recvline[n]=0; /* null terminate */
		printf("from client IP %s port %d\n", getIP(cliaddr), getPort(cliaddr));
		printf("connfd = %d\n", connfd);
		tok.clear();
		tok=parse(recvline);
		

		if(userAccount.find(User(getIP(cliaddr), getPort(cliaddr), cliaddr))==userAccount.end()){
			if(tok[0][0]=='R'){
				writeRecv(connfd, recvline, strlen(recvline));// send to client
				//register
				currentUser=Account(tok[1], tok[2]);
				currentUser.state=Normal;
				userAccount[User(getIP(cliaddr), getPort(cliaddr), cliaddr, connfd)]=currentUser;
				accountUser[currentUser]=User(getIP(cliaddr), getPort(cliaddr), cliaddr, connfd);
				
				if(insertUser(currentUser)){
					puts("register sucess");
					write(connfd, SUCCESS, strlen(SUCCESS));

					// update filelist
					recvFilelist(connfd, currentUser.ID);


				}
				else{
					puts("register fail");

					write(connfd, FAIL, strlen(FAIL));
				}
			}
			else if(tok[0][0]=='L'){
				writeRecv(connfd, recvline, strlen(recvline));// send to client
				if(login(Account(tok[1], tok[2]))){
					currentUser=Account(tok[1], tok[2]);
					currentUser.state=Normal;
					userAccount[User(getIP(cliaddr), getPort(cliaddr), cliaddr, connfd)]=currentUser;
					accountUser[currentUser]=User(getIP(cliaddr), getPort(cliaddr), cliaddr, connfd);
					puts("login sucess");
					write(connfd, SUCCESS, strlen(SUCCESS));	
					// update filelist
					recvFilelist(connfd, currentUser.ID);

				}
				else{
					puts("login fail");
					write(connfd, FAIL, strlen(FAIL));
				}
			}
		}
		else{ // login already
			currentUser=userAccount[User(getIP(cliaddr), getPort(cliaddr), cliaddr)];
			accountUser[currentUser]=User(getIP(cliaddr), getPort(cliaddr), cliaddr);
			if(currentUser.state==Normal){
				if(tok[0]=="Del"){// delete account
					writeRecv(connfd, recvline, strlen(recvline));// send to client
					puts("Delete account");
					deleteUser(currentUser);// delete in db
					userAccount.erase(User(getIP(cliaddr), getPort(cliaddr), cliaddr));
					accountUser.erase(currentUser);
					
					write(connfd, SUCCESS, strlen(SUCCESS));
				}
				else if(tok[0]=="L"){// logout
					writeRecv(connfd, recvline, strlen(recvline));// send to client
					puts("Logout");
					userAccount.erase(User(getIP(cliaddr), getPort(cliaddr), cliaddr));
					accountUser.erase(currentUser);
					deleteFile(currentUser.ID);
					write(connfd, SUCCESS, strlen(SUCCESS));
				}
			
				//------------------------------------------------
				// show filelist/ show user
				else if(tok[0]=="SU"){// show user
					writeRecv(connfd, recvline, strlen(recvline));// send to client
					writeRecv(connfd, SUCCESS, strlen(SUCCESS));
					sendUser(connfd);
				}
				else if(tok[0]=="SF"){// show filelist
					writeRecv(connfd, recvline, strlen(recvline));// send to client
					writeRecv(connfd, SUCCESS, strlen(SUCCESS));
					showFilelist();
					sendResult(connfd, 2, result);
				}
				//------------------------------------------------
				// upload download
				else if(tok[0]=="D"){// download
					writeRecv(connfd, recvline, strlen(recvline));// send to client
					int numFile=existNumFile(tok[1]);
					printf("number of file = %d\n", numFile);
					
					// total authors
					if(numFile){
						vector<string>author=getFileAuthor();
						writeRecv(connfd, SUCCESS, strlen(SUCCESS));
						string str=toString((int)author.size());
						// how many author
						write(connfd, str.c_str(), str.length());
						// to user A
						recvWrite(connfd, recvline);// tell B connect to "port"
						// to user A
						User userA=accountUser[currentUser];
						for(int i=0;i<author.size();i++){
							cout<<author[i]<<endl;

							
							User userB=accountUser[Account(author[i])];
							
							// user B
							str=string(DOWNLOAD)+" "+userA.IP+" "+recvline+" "+tok[1]+" "+toString(i)+" "+toString((int)author.size());
							write(userB.connfd, str.c_str(), str.length());// tell B :(A's) IP port path idx total

						}

						// update filelist
						puts("update filelist");
						addFile(currentUser.ID, tok[1]);
					}
					else{
						write(connfd, FAIL, strlen(FAIL));
					}
					


				}
				else if(tok[0]=="U"){// upload
					writeRecv(connfd, recvline, strlen(recvline));// send to client
					if(accountUser.find(Account(tok[1]))!=accountUser.end()){
						// to user A
						write(connfd, SUCCESS, strlen(SUCCESS));
						User userA=accountUser[currentUser];
						User userB=accountUser[Account(tok[1])];
						string str;
						// to user A
						recvWrite(connfd, recvline);// tell B connect to "port"
						// user B
						str=string(UPLOAD)+" "+userA.IP+" "+recvline+" "+tok[2];
						write(userB.connfd, str.c_str(), str.length());// tell B :(A's) IP port

						// update B's filelist
						addFile(tok[1], tok[2]);
					}
					else{
						write(connfd, FAIL, strlen(FAIL));
					}
					
				}
				//------------------------------------------------
				else if(tok[0]=="T"){// A tell B
					writeRecv(connfd, recvline, strlen(recvline));// send to client
					if(accountUser.find(Account(tok[1]))!=accountUser.end()){
						currentUser.state=Tell;
						userAccount[User(getIP(cliaddr), getPort(cliaddr), cliaddr, connfd)]=currentUser;
						// to user A
						write(connfd, SUCCESS, strlen(SUCCESS));
						User userA=accountUser[currentUser];
						User userB=accountUser[Account(tok[1])];
						Account accountB=userAccount[userB];
						accountB.state=Tell;
						userAccount[userB]=accountB;
						
						string str;
						// to user A
						recvWrite(connfd, recvline);// tell B connect to "port"
						// user B
						printf("user B connfd = %d\n", userB.connfd);
						str=string(TALK)+" "+userA.IP+" "+recvline;
						// write(userB.connfd, TALK, strlen(TALK));
						
						
						write(userB.connfd, str.c_str(), str.length());// tell B :(A's) IP port

						// string str=userB.IP+" "+toString(userB.port);
						// writeWithSleep(connfd, str.c_str(), str.length());

					}
					else{
						write(connfd, FAIL, strlen(FAIL));
					}
					
				}
			
			}
			else if(currentUser.state==Tell){
				if(tok[0]=="EXIT"){
					printf("%s EXIT~\n", currentUser.ID.c_str());
					currentUser.state=Normal;
					userAccount[User(getIP(cliaddr), getPort(cliaddr), cliaddr, connfd)]=currentUser;
				}
				else{
					printf("%s tell~\n", currentUser.ID.c_str());
				}
			}		
		}
		puts(recvline);
		
		
		// write(connfd, recvline, strlen(recvline));
	}
	close(connfd);

	return NULL;
}




int main(int argc, char **argv) {
	pthread_t tid;	
	int listenid, connfd;

	ssize_t n;
	// socklen_t len;
	struct sockaddr_in servaddr;

	init();
	userAccount.clear();
	accountUser.clear();

	system("mkdir Folder_server");
	chdir("Folder_server");
	// server filelist
	initServerFilelist();

	userAccount.clear();
	accountUser.clear();
	SERV_PORT=argc>1?atoi(argv[1]):7122;
	// TCP 
	TCPlisten(listenid, connfd, servaddr, SERV_PORT);
	
	signal(SIGCHLD, sig_chld);/* must call waitpid() */
	while(1){
		ClientSock * clientSock; 
		clientSock = (ClientSock*)malloc(sizeof(ClientSock));
		socklen_t clilen = sizeof(clientSock->addr);
		clientSock->connfd = accept(listenid, (struct sockaddr*)&(clientSock->addr), &clilen);
		printf("TCP connect from IP %s (port %d)\n", getIP(clientSock->addr), getPort(clientSock->addr));
		pthread_create(&tid, NULL, &run, (void *)clientSock);
	}


	sqlite3_close(db);

	return 0;
}

