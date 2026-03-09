/*
Gokulan Valavan
MS CEEN 2027
CSCE 612-600
Spring 2026
*/


#include "pch.h"


bool URLParse::URLParser(const char* url) {
	if (url == nullptr) {
		return false;
	}

	// Reset state for each parse.
	host[0] = '\0';
	path[0] = '\0';
	scheme[0] = '\0';
	query[0] = '\0';
	fragment[0] = '\0';
	port = 0;

	// Guard against excessively long or non-terminated input.
	size_t urlLen = strnlen(url, MAX_URL_LEN + 1);
	if (urlLen == 0 || urlLen > MAX_URL_LEN) {
		return false;
	}

	// Get scheme before the '://' delimiter.

	const char* pos = strstr(url, "://");
	if (pos == NULL) {
		//printf("Failed with no scheme \n");
		return false;
	}

	int schemeLen = pos - url;
	if (schemeLen <= 0 || schemeLen >= sizeof(scheme)) {
		return false;
	}
	strncpy(scheme, url, schemeLen);
	scheme[schemeLen] = '\0';

	// Only http is accepted for this assignment.
	if (strcmp(scheme, "http") != 0) {
		//printf("Failed with incorrect scheme \n");
		return false;
	}

	const char* postScheme = pos + 3; //3 steps after :// moving to find host

	pos = postScheme;

	// Traverse URL until host ends.

	while (*pos != '\0' && *pos != '/' && *pos != ':' && *pos != '?' && *pos != '#') {
		pos++;
	}
	
	int hostLen = pos - postScheme;

	if (hostLen <= 0 || hostLen >= sizeof(host)) {
		return false;
	}
	strncpy(host, postScheme, hostLen); 
	host[hostLen] = '\0';

	for (int i = 0; i < hostLen; ++i) {
		unsigned char ch = static_cast<unsigned char>(host[i]);
		if (!(std::isalnum(ch) || ch == '.' || ch == '-')) {
			return false;
		}
	}

	// Check optional port after ':'.

	if (*pos == ':') {
		pos++;
		port = atoi(pos);

		//checking the port
		if (port <= 0 || port > 65535) {
			//printf("Invalid port number \n");
			return false;
		}

		while (*pos >= '0' && *pos <= '9') {
			pos++;
		}
	}
	else {
		port = 80; //default port if no port specified
	}

	// Extract path (default "/").
	if (*pos == '/') {
		const char* pathStart = pos;
		const char* pathEnd = pathStart; //If path is just a '/' root path

		//If a non-root path exists
		while (*pathEnd != '?' && *pathEnd != '#' && *pathEnd != '\0') {
			pathEnd++;
		}

		int pathLen = pathEnd - pathStart;
		if (pathLen >= sizeof(path) - 1) {
			return false;
		}
		strncpy(path, pathStart, pathLen);
		path[pathLen] = '\0';
		pos = pathEnd;
	}
	else {
		strcpy(path, "/");
	}
	
	if (*pos == '?') {
		pos++;
		const char* queryStart = pos;
		const char* queryEnd = queryStart;

		//Query should end at '#'
		while (*queryEnd != '#' && *queryEnd != '\0') {
			queryEnd++;
		}

		int queryLen = queryEnd - queryStart;
		if (queryLen >= sizeof(query)) {
			return false;
		}

		strncpy(query, queryStart, queryLen);
		query[queryLen] = '\0';
		pos = queryEnd;

		// Check combined length fits in path buffer
		if (strlen(path) + 1 + queryLen >= sizeof(path)) { 
			return false; 
		}

		//Prepending ? to path
		char temp[MAX_URL_LEN];
		int w = snprintf(temp, sizeof(temp), "%s?%s", path, query);
		if (w < 0 || w >= sizeof(temp)) {
			return false;
		}
		strncpy(path, temp, sizeof(path) - 1);
		path[sizeof(path) - 1] = '\0';

		pos = queryEnd;
	}

	// If fragment exists, we ignore it for this HW.
	if (*pos == '#') {
		pos++;

		strncpy(fragment, pos, sizeof(fragment) - 1);
		fragment[sizeof(fragment) - 1] = '\0';
	}

	return true;

}

//Function to send GET requests to server
void URLParse::requestServerGet(char* request, int maxlen) {
	

	snprintf(request, maxlen,
		"GET %s HTTP/1.0\r\n"
		"User-Agent: kakarot_crawler/1.0\r\n"
		"Host: %s\r\n"
		"Connection: close\r\n"
		"\r\n",
		path, host);

}

//Function to send HEAD requests to server
void URLParse::requestServerHead(char* request, int maxlen) {


	snprintf(request, maxlen,
		"HEAD /robots.txt HTTP/1.0\r\n"
		"User-Agent: kakarot_crawler/1.0\r\n"
		"Host: %s\r\n"
		"Connection: close\r\n"
		"\r\n",
		 host);

}
