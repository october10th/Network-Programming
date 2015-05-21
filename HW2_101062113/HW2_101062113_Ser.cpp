#include "header.h"
#include <sqlite3.h>
map<User, Account>userAccount;
map<Account, User>accountUser;
sqlite3 *db;
vector< map < string, string > >result;
int WaitingAck;
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
		m[azColName[i]]=argv[i];
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

void insertUser(Account acc){
	string sql="INSERT INTO user VALUES('"+acc.ID+"', '"+acc.pw+"');";
	sql_exec(sql);
	showResult();
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
void addArticle(){

}
int main(int argc, char **argv) {
	system("mkdir Upload");
	int listenfd, connfd, udpfd, nready, maxfdp1;
	char mesg[MAXLINE];
	pid_t childpid;
	int SERV_PORT=7122;
	fd_set rset;
	ssize_t n;
	socklen_t len;
	const int on = 1;
	struct sockaddr_in cliaddr, servaddr;
	vector<string>tok;
	Account currentUser;
	init();
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
	WaitingAck=0;
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
				puts("send back ACK");
				sendto(udpfd, ACK, strlen(ACK), 0, (struct sockaddr *) &cliaddr, len);
				tok.clear();
				tok=parse(mesg);
				if(userAccount.find(User(getIP(cliaddr), getPort(cliaddr), cliaddr))==userAccount.end()){
					// not login

					// sscanf(mesg, "%s%s%s", cmd, tok[0], tok[1]);
					
					if(tok[0][0]=='R'){
						//register
						currentUser=Account(tok[1], tok[2]);
						currentUser.state=Normal;
						userAccount[User(getIP(cliaddr), getPort(cliaddr), cliaddr)]=currentUser;
						accountUser[currentUser]=User(getIP(cliaddr), getPort(cliaddr), cliaddr);
						insertUser(currentUser);

						puts("register sucess");
						sendto(udpfd, SUCCESS, strlen(SUCCESS), 0, (struct sockaddr *) &cliaddr, len);
						
					}
					else if(tok[0][0]=='L'){
						if(login(Account(tok[1], tok[2]))){

							currentUser=Account(tok[1], tok[2]);
							currentUser.state=Normal;
							userAccount[User(getIP(cliaddr), getPort(cliaddr), cliaddr)]=currentUser;
							accountUser[currentUser]=User(getIP(cliaddr), getPort(cliaddr), cliaddr);
							insertUser(currentUser);
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
							// char sendline[MAXLINE];
							// sprintf(sendline,"%lu", accountUser.size());
	    					// sendto(udpfd, sendline, strlen(sendline), 0, (struct sockaddr *) &cliaddr, len);
							for (std::map<Account, User>::iterator it=accountUser.begin(); it!=accountUser.end(); ++it){
	    						sendto(udpfd, (it->first).ID.c_str(), (it->first).ID.length(), 0, (struct sockaddr *) &cliaddr, len);
	    					}
	    					sendto(udpfd, SUCCESS, strlen(SUCCESS), 0, (struct sockaddr *) &cliaddr, len);
						}
						else if(tok[0]=="SA"){// show article

						}
						else if(tok[0]=="A"){// add article

						}
						else if(tok[0]=="E"){// enter article

						}
						//------------------------------------------------
						else if(tok[0]=="Y"){// yell

						}
						else if(tok[0]=="T"){// tell

						}
					
					}
					else if(currentUser.state==Article){

					}
				}
				// send back (debug	
				// sendto(udpfd, mesg, n, 0, (struct sockaddr *) &cliaddr, len);
			}
			
		}
	}
	sqlite3_close(db);
}

