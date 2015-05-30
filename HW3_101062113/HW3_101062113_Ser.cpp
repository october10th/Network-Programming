#include "header.h"
#include <sqlite3.h>
// int listenfd, connfd, udpfd, nready, maxfdp1;

pid_t childpid;
int SERV_PORT=7122;
// fd_set rset;
// const int on = 1;

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

	// DROP TABLE FROM filelist

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

	showDB();

}

void *run(void *arg){
	int n;
	char  mesg[MAXLINE], sendline[MAXLINE], recvline[MAXLINE];
	pthread_detach(pthread_self());
	ClientSock clientSock=*(ClientSock*)arg;
	free(arg);
	int connfd=clientSock.connfd;

	while((n=read(connfd, recvline, MAXLINE)) >0) {

		recvline[n]=0; /* null terminate */
		puts("from client");
		puts(recvline);
		
		
		write(connfd, recvline, strlen(recvline));
	}


	return NULL;
}




int main(int argc, char **argv) {
	pthread_t tid;	
	int listenid, connfd;

	ssize_t n;
	socklen_t len;
	struct sockaddr_in cliaddr, servaddr;

	init();
	system("mkdir Upload");
	chdir("Upload");
	userAccount.clear();
	accountUser.clear();
	SERV_PORT=argc>1?atoi(argv[1]):7122;
	// TCP 
	listenid=socket(AF_INET, SOCK_STREAM,0);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	servaddr.sin_port=htons(SERV_PORT);
	bind(listenid, (struct sockaddr *)&servaddr, sizeof(servaddr));
	listen(listenid, LISTENQ); /*LISTENQ=1024*/
	

	while(1){
		ClientSock * cliSock; 
		cliSock = (ClientSock*)malloc(sizeof(ClientSock));
		socklen_t clilen = sizeof(cliSock->cliaddr);
		cliSock->connfd = accept(listenid, (struct sockaddr*)&(cliSock->cliaddr), &clilen);
		printf("TCP connect from IP %s (port %d)\n", getIP(cliSock->cliaddr), getPort(cliSock->cliaddr));
		pthread_create(&tid, NULL, &run, (void *)cliSock);
	}	


	sqlite3_close(db);

	return 0;
}

