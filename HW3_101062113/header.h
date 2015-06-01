#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <algorithm>
#include <errno.h>
#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <sstream>
#include <pthread.h>
#include <dirent.h> 
#define LISTENQ 1024
#define MAXLINE 2048
// #define PN 5
using namespace std;

enum{
	Init,
	Normal,
	Tell,

};
const char* getIP(struct sockaddr_in cliaddr){
	return inet_ntoa(((struct sockaddr_in *)&cliaddr)->sin_addr);
}
int getPort(struct sockaddr_in cliaddr){
	return ntohs(((struct sockaddr_in *)&cliaddr)->sin_port);		
}
struct User{
	string IP;
	int port;
	struct sockaddr_in cliaddr;
	int connfd;
	User(){}
	User(const char _IP[], int _port, struct sockaddr_in _cliaddr){
		IP=_IP;
		port=_port;
		cliaddr=_cliaddr;
	}
	User(const char _IP[], int _port, struct sockaddr_in _cliaddr, int _connfd){
		IP=_IP;
		port=_port;
		cliaddr=_cliaddr;
		connfd=_connfd;
	}
	User(string _IP, int _port, struct sockaddr_in _cliaddr){
		IP=_IP;
		port=_port;
		cliaddr=_cliaddr;
	}
	User(string _IP, int _port, struct sockaddr_in _cliaddr, int _connfd){
		IP=_IP;
		port=_port;
		cliaddr=_cliaddr;
		connfd=_connfd;
	}
	void setCliaddr(struct sockaddr_in _cliaddr){
		cliaddr=_cliaddr;
	}
	bool operator<(const User &xd)const{
		if(strcmp(IP.c_str(), xd.IP.c_str())==0){
			return port<xd.port;
		}
		return strcmp(IP.c_str(), xd.IP.c_str())<0;
	}
};
struct Account{
	string ID, pw;
	int state;

	Account(){state=0;}
	Account(string _username){ID=_username;}
	Account(string _username, string _pw){ID=_username, pw=_pw;state=0;}
	Account(const char * _username, const char * _pw){ID=_username, pw=_pw;state=0;}
	bool operator<(const Account &xd)const{
		// if(strcmp(ID.c_str(), xd.ID.c_str())==0)
		// 	return strcmp(pw.c_str(), xd.pw.c_str())<0;
		return strcmp(ID.c_str(), xd.ID.c_str())<0;

	}
};
struct ClientSock{
	int connfd;
	struct sockaddr_in addr;
};
typedef struct sockaddr SA;
const char ACK[]="ACK";
const char SUCCESS[]="Success!";
const char FAIL[]="Fail!";
const char OK[]="OK";
const char TALK[]="TALK$#@#$@#$########";
const char EXIT[]="EXIT";
const int TABLE_NUM=2;
const char create_sql[TABLE_NUM][512]={
	"CREATE TABLE user(ID VARCHAR(64) PRIMARY KEY, pw VARCHAR(64));",
	"CREATE TABLE filelist( fid INTEGER PRIMARY KEY, path VARCHAR(1024), author VARCHAR(64), time timestamp DEFAULT CURRENT_TIMESTAMP) ;"
};
vector<string>parse(string str){
	string item;
	stringstream ss(str);
    vector<string>elems;
    while(ss>>item){
    	elems.push_back(item);
        // cout<<item<<endl;
    }
    return elems;
}
string toString(int a){
	char str[20];
	sprintf(str, "%d", a);
	return string(str);
}
int random(int a, int b){// a <= x < b
	return a+rand()%(b-a);
}
int getRandPort(){
	return random(8000, 10000);
}
int receive(int connfd, char recvline[]){
	int n;
	n=read(connfd, recvline, MAXLINE);
	if(n>=0)
		recvline[n]=0;
	return n;
}
int recvWrite(int connfd, char recvline[]){
	int n;
	n=read(connfd, recvline, MAXLINE);
	if(n>=0)
		recvline[n]=0;
	write(connfd, OK, strlen(OK));
	return n;
}
void writeRecv(int connfd, const char sendline[], int length){
	char recvline[MAXLINE];
	int n;
	write(connfd, sendline, length);
	n=read(connfd, recvline, MAXLINE);
	if(n>=0)recvline[n]=0;
	// puts(sendline);
	// usleep(1000);
}
void writeWithSleep(int connfd, const char sendline[], int length){
	write(connfd, sendline, length);
	// puts(sendline);
	usleep(1000);
}



// server
void TCPlisten(int &listenid, int &connfd, struct sockaddr_in & servaddr, int port){
	listenid=socket(AF_INET, SOCK_STREAM,0);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	servaddr.sin_port=htons(port);
	bind(listenid, (struct sockaddr *)&servaddr, sizeof(servaddr));
	listen(listenid, LISTENQ); /*LISTENQ=1024*/

}
// client
void TCPconnect(int &sockfd, struct sockaddr_in &servaddr, const char IP[], int port){
	if((sockfd=socket(AF_INET, SOCK_STREAM,0)) < 0) puts("socket error");//ceate an Internet(AF_INET) stream(SOCK_STREAM) socket
	bzero(&servaddr,sizeof(servaddr)); //reset address to zero
	servaddr.sin_family=AF_INET; //IPv4
	servaddr.sin_port=htons(port);
	if(inet_pton(AF_INET, IP, &servaddr.sin_addr) <= 0)printf("inet_ption error for %s\n", IP);
	printf("connect to %s %d\n", IP, port);
	if(connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))<0)puts("connect error");
}
// other
vector<string> getCurFilelist(){
	DIR *d;
	struct dirent *dir;
	vector<string>filelist;
	filelist.clear();
	d = opendir(".");
	if (d){
		while ((dir = readdir(d)) != NULL){
			// no "." or ".."
			if(strcmp(dir->d_name, ".")&&strcmp(dir->d_name, ".."))
				filelist.push_back(dir->d_name);
			printf("%s\n", dir->d_name);
		}
		closedir(d);
	}
	return filelist;
}
