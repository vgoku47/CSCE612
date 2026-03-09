/*
Gokulan Valavan
MS CEEN 2027
CSCE 612-600
Spring 2026
*/


#define BUF_SIZE 65536
#define THRESHOLD 1024
#define TIMEOUT 10

#include "pch.h"	
#pragma comment(lib, "ws2_32.lib")

//Constructor to initialize values and Generate Socket()
Socket::Socket() {
	
	sock = INVALID_SOCKET;
	pos = 0;
	allocsize = BUF_SIZE;
	// Allocate initial receive buffer.
	buff = new char[BUF_SIZE];
	//memset(&server, 0, sizeof(server));
	//server.sin_family = AF_INET;

}

//Destructor to close socket and free buffer
Socket::~Socket() {

	//If buffer isn't empty free the memory
	if (buff != nullptr) {
		delete[] buff;
		buff = nullptr;
	}
	if (sock != INVALID_SOCKET) {
		closesocket(sock);
		sock = INVALID_SOCKET;
	}
}


//Function for DNS
//bool Socket::DNS(const char* host, int port, int* dnsTime, char* ipfound) {
//
//	server.sin_port = htons(port);
//	server.sin_family = AF_INET;
//
//	struct addrinfo hints, *result = NULL;
//	memset(&hints, 0, sizeof(hints));
//	hints.ai_family = AF_INET;       
//	hints.ai_socktype = SOCK_STREAM;
//	hints.ai_protocol = IPPROTO_TCP;
//
//	//Using addrinfo due to gethostbyname being overwritten by multiple threads
//	clock_t dnsstart = clock();
//	int ret = getaddrinfo(host, NULL, &hints, &result);
//	clock_t dnsend = clock();
//
//	*dnsTime = (int)(1000.0 * (dnsend - dnsstart) / CLOCKS_PER_SEC);
//
//	if (ret != 0 || result == NULL) {
//		if (result) freeaddrinfo(result);
//		return false;
//	}
//
//	// Copy the resolved address into server struct
//	struct sockaddr_in* resolved = (struct sockaddr_in*)result->ai_addr;
//	server.sin_addr = resolved->sin_addr;
//
//	// inet_ntop is thread-safe
//	inet_ntop(AF_INET, &server.sin_addr, ipfound, 16);
//
//	freeaddrinfo(result);
//	return true;
//}

//connect() is defined here
bool Socket::connectSocket(int* connectTime, sockaddr_in server) {

	// Close any existing socket before creating a new one
	if (sock != INVALID_SOCKET) {
			closesocket(sock);
			sock = INVALID_SOCKET;
		}

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET) {
		return false;
	}

	clock_t constart = clock(); //Clock to measure connection time starts
	
	//connect() function 
	
	if (connect(sock, (struct sockaddr*)&server, sizeof(struct sockaddr_in)) == SOCKET_ERROR) {

		closesocket(sock);
		sock = INVALID_SOCKET;
		return false;
	}

	clock_t conend = clock();
	*connectTime = (int)(1000.0 * (conend - constart) / CLOCKS_PER_SEC);

	return true;

}

//Function to send requests
void Socket::sendMessage(const char* request) {
	int len = strlen(request);
	int sent;
	int total = 0;


	while (total < len) {

		sent = send(sock, request + total, len - total, 0); //send() function

		if (sent == SOCKET_ERROR) {
			//printf("Message not send Error %d \n", WSAGetLastError());
			return;
		}

		total += sent;

	}
}

//Function to read responses
bool Socket::readMessage(int* readTime, int maxsize) {

	pos = 0; 

	clock_t readstart = clock(); //Clock to measure read times

	//message is read in intervals where socket gets data so the loop waits for the data
	while (true) {

		// Calculating elapsed time
		int elapsed = (int)(1000.0 * (clock() - readstart) / CLOCKS_PER_SEC);
		if (elapsed > 10000) {  // 10 seconds total
			//printf("failed with slow download\n");
			return false;
		}

		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(sock, &readfds);

		// Calculate remaining time for select
		int remainingMs = 10000 - elapsed;
		struct timeval timeout;
		timeout.tv_sec = remainingMs / 1000;
		timeout.tv_usec = (remainingMs % 1000) * 1000;

		int wait = select(0, &readfds, NULL, NULL, &timeout); //select() function which waits for socket to get data

		if (wait > 0) {

			// Expand buffer if close to capacity.
			if (allocsize - pos < THRESHOLD + 1) {
				alterSize();
			}

			//When data is available

			int bytesPresent = allocsize - pos - 1;
			int receive = recv(sock, buff + pos, bytesPresent, 0); //pointer moved as data goes to buffer

			//Failed due to Socket Error
			if (receive == SOCKET_ERROR) {
				//printf("failed with %d on recv\n", WSAGetLastError());
				return false;
			}

			//No more data Normal Exit
			if (receive == 0) {

				if (pos >= allocsize) {
					alterSize();
				}

				buff[pos] = '\0';

				clock_t readend = clock();
				*readTime = (int)(1000.0 * (readend - readstart) / CLOCKS_PER_SEC);
				return true;
			}

			pos += receive;

			if (pos > maxsize) {
				//printf("failed with exceeding max\n");
				return false;
			}
		}
		//If no data arrives for than 10 seconds exit due to Timeout
		else if (!wait) {
			//printf("failed with slow download\n");
			return false;
		}	
		//Specific WSA Error while receiving data
		else {
			//printf("failed with %d on recv \n", WSAGetLastError());
			return false;
		}
	}	


}

//Helper function to alter buffer size size is added if the constraint is reached
void Socket::alterSize() {

	allocsize = allocsize*2; //doubling rather than incremental increase

	if (allocsize > 2 * 1024 * 1024 + 4096) {
		allocsize = 2 * 1024 * 1024 + 4096; //Capping at 2 MB
	}

	char* newbuff = new char[allocsize];
	memcpy(newbuff, buff, pos);
	delete[] buff;

	buff = newbuff;

}

//reset socket for next connections
void Socket::resetSocket() {
	if (sock != INVALID_SOCKET) {
		closesocket(sock);
		sock = INVALID_SOCKET;
	}

	if (buff != nullptr) {
		delete[] buff;
		buff = nullptr;
	}
	allocsize = BUF_SIZE;
	buff = new char[BUF_SIZE];

	pos = 0;


}

//Helper functions to determine unique URLs and IP addresses

//bool Socket::uniqueURL(const char* url) {
//
//	std::set<std::string>::const_iterator index = seenHosts.find(url);
//
//	if (index == seenHosts.end()) {
//		printf("passed \n");
//		seenHosts.insert(url);
//	}
//	else {
//		printf("failed \n");
//		return false;
//	}
//
//	return true;
//}

//bool Socket::uniqueIP(DWORD IP) {
//
//	std::set<DWORD>::const_iterator index = seenIPs.find(IP);
//
//	if (index == seenIPs.end()) {
//		printf("passed \n");
//		seenIPs.insert(IP);
//	}
//
//	else {
//		printf("failed \n");
//		return false;
//	}
//
//	return true;
//}
