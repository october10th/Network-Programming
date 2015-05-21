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
#define LISTENQ 1024
#define MAXLINE 2048
#define PN 5
using namespace std;

enum{
	Init,
	Normal,
	Article,

};
const char* getIP(struct sockaddr_in cliaddr){
	return inet_ntoa(((struct sockaddr_in *)&cliaddr)->sin_addr);
}
int getPort(struct sockaddr_in cliaddr){
	return ntohs(((struct sockaddr_in *)&cliaddr)->sin_port);		
}
struct User{
	char IP[25];
	int port;
	struct sockaddr_in cliaddr;
	User(){}
	User(const char _IP[], int _port, struct sockaddr_in _cliaddr){
		strcpy(IP, _IP);
		port=_port;
		cliaddr=_cliaddr;
	}
	void setCliaddr(struct sockaddr_in _cliaddr){
		cliaddr=_cliaddr;
	}
	bool operator<(const User &xd)const{
		if(strcmp(IP, xd.IP)==0){
			return port<xd.port;
		}
		return strcmp(IP, xd.IP)<0;
	}
};
struct Account{
	string ID, pw;
	int state;
	Account(){state=0;}
	Account(string _username, string _pw){ID=_username, pw=_pw;state=0;}
	Account(const char * _username, const char * _pw){ID=_username, pw=_pw;state=0;}
	bool operator<(const Account &xd)const{
		if(strcmp(ID.c_str(), xd.ID.c_str())==0)
			return strcmp(pw.c_str(), xd.pw.c_str())<0;
		return strcmp(ID.c_str(), xd.ID.c_str())<0;

	}
};

typedef struct sockaddr SA;
const char ACK[]="ACK";
const char SUCCESS[]="Success!";
const char FAIL[]="Fail!";
const char create_sql[6][512]={
	"CREATE TABLE user(ID VARCHAR(64) PRIMARY KEY, pw VARCHAR(64));",
	"CREATE TABLE article( aid INTEGER PRIMARY KEY, title VARCHAR(64), content VARCHAR(1024), hit INTEGER DEFAULT 0 ) ;",
	"CREATE TABLE response( rid INTEGER PRIMARY KEY, aid INTEGER , content VARCHAR(1024)) ;",
	"CREATE TABLE blacklist( ID VARCHAR(64) PRIMARY KEY, aid INTEGER) ;",
	"CREATE TABLE filelist( fid INTEGER PRIMARY KEY, path VARCHAR(1024), aid INTEGER, rid INTEGER) ;"
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