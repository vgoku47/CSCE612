//Gokulan Valavan
//CSCE 612 Spring 2026

#pragma once
#include "pch.h"


class Socket
{
private:

	SOCKET sock;
	struct sockaddr_in remote;

public:

	Socket();
	~Socket();

	bool openSocket();
	bool sendRequest(const char* server_ip, char* packet, int pkt_size);
	bool readPacket(struct sockaddr_in response, char* packet, int bytes, u_short sentTXID);
	void resetSocket();


};

