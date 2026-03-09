
//Gokulan Valavan
//MS CEEN 2027
//CSCE 612-600
//Spring 2026

#pragma once
#include "pch.h"

//Socket class used to communicate requests with server
class Socket {

private:
	SOCKET sock;
	char* buff;
	int allocsize;
	int pos;
	//struct sockaddr_in server;
	
public:
	Socket(); //Constructor
	~Socket(); //Destructor

	bool connectSocket(int* connectTime, sockaddr_in server); 
	//bool DNS(const char* host, int port, int* dnsTime, char* ipfound);
	void sendMessage(const char* request);
	bool readMessage(int* readTime, int maxsize);
	void resetSocket();

	//Accessor Function
	char* retBuff() { return buff; }
	int retBytesRecv() { return pos; }
	//DWORD getIP() { return server.sin_addr.S_un.S_addr; }

	//Altering buffer size
	void alterSize();
	
};

