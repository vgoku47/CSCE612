
//Gokulan Valavan
//MS CEEN 2027
//CSCE 612-600
//Spring 2026

#pragma once
#include "pch.h"

//Class to break down the URL input to various parts which will then be used to send requests and form connections with the server
class URLParse {

public:

	char host[MAX_HOST_LEN];
	char path[MAX_URL_LEN];
	char scheme[16];
	char query[MAX_URL_LEN];
	int port;
	char fragment[MAX_URL_LEN];

	//Constructor to initialize the Class
	URLParse() {
		host[0] = '\0';
		path[0] = '\0';
		scheme[0] = '\0';
		query[0] = '\0';
		fragment[0] = '\0';
		port = 0;
	}

	bool URLParser(const char* url); //Function that progressively splits the URL by various parts
	void requestServerHead(char* request, int maxlen); //Function to setup an HTTP Head request which will be used in main.cpp
	void requestServerGet(char* request, int maxlen); 


};


