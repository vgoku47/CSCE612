//Gokulan Valavan
//CSCE 612 Spring 2026

#include "pch.h"

//Forward declaring functions from message file
int parseName(char* buf, int bytes, int pos, char* result);
int parseRRSection(char* buf, int bytes, int curPos, int count);

//Constructor
Socket::Socket() {

	sock = INVALID_SOCKET;
}

//Destructor
Socket::~Socket() {


	if (sock != INVALID_SOCKET) {
		closesocket(sock);
		sock = INVALID_SOCKET;
	}
}

//Function to open socket and bind to port (UDP socket)
bool Socket::openSocket() {

	if (sock != INVALID_SOCKET) {
		closesocket(sock);
		sock = INVALID_SOCKET;
	}

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (sock == INVALID_SOCKET) {
		printf("Socket invalid %d", WSAGetLastError());
		return false;
	}

	struct sockaddr_in local;
	memset(&local, 0, sizeof(local));
	local.sin_family = AF_INET;
	local.sin_port = htons(0);
	local.sin_addr.S_un.S_addr = INADDR_ANY;

	if (bind(sock, (struct sockaddr*)&local, (sizeof(local))) == SOCKET_ERROR) {

		printf("Error in binding socket %d", WSAGetLastError());
		return false;
	}

	return true;
}


//Sending request to DNS server and gets back packets with resolved details of the host
bool Socket::sendRequest(const char* server_ip, char* packet, int pkt_size) {

    FixedDNSheader* sentHeader = (FixedDNSheader*)packet;
    u_short sentTXID = ntohs(sentHeader->TXID);
	memset(&remote, 0, sizeof(remote));
	remote.sin_family = AF_INET;
	remote.sin_port = htons(53);
	remote.sin_addr.S_un.S_addr = inet_addr(server_ip);
 

    int count = 0;
    while (count < MAX_ATTEMPTS) //3 attempts each 10s timeout
    {
        printf("Attempt %d with %d bytes... ", count, pkt_size);

        DWORD startTime = GetTickCount64();

        if (sendto(sock, packet, pkt_size, 0,
            (struct sockaddr*)&remote, sizeof(remote)) == SOCKET_ERROR)
        {
            printf("socket error %d\n", WSAGetLastError());
            return false;
        }

       
        // set up timeout with select()
        struct timeval tp;
        tp.tv_sec = TIMEOUT;
        tp.tv_usec = 0;

        fd_set fd;
        FD_ZERO(&fd);
        FD_SET(sock, &fd);

        int available = select(0, &fd, NULL, NULL, &tp);

        if (available > 0)
        {
            // response arrived — receive it
            struct sockaddr_in response;
            int responseLen = sizeof(response);
            char recvBuf[MAX_DNS_LEN];

            int bytes = recvfrom(sock, recvBuf, MAX_DNS_LEN, 0, (struct sockaddr*)&response, &responseLen);

            

            if (bytes == SOCKET_ERROR)
            {
                printf("socket error %d\n", WSAGetLastError());
                return false;
            }

            DWORD elapsed = GetTickCount64() - startTime;
            printf("response in %d ms with %d bytes\n", elapsed, bytes);

          
            
            if (!readPacket(response, recvBuf, bytes, sentTXID)) {
                return false;
            }

            return true;
        }
        else if (available == 0)
        {
            printf("timeout in 10000 ms\n");
            count++;
        }
        else
        {
            printf("socket error %d\n", WSAGetLastError());
            return false;
        }
    }
    return false;
}

//Parse through packet header, query, answers originating in the server
bool Socket::readPacket(struct sockaddr_in response, char* packet, int bytes, u_short sentTXID) {



    if (bytes < (int)sizeof(FixedDNSheader))
    {
        printf("  ++ invalid reply: packet smaller than fixed DNS header\n");
        return false;
    }

    if (response.sin_addr.S_un.S_addr != remote.sin_addr.S_un.S_addr || response.sin_port != remote.sin_port) {
        printf("  ++ bogus reply from unexpected source\n");
        return false;
    }
   
    FixedDNSheader* fdh = (FixedDNSheader*)packet;
    u_short rxTXID = ntohs(fdh->TXID);
    printf("  TXID 0x%.4X flags 0x%.4X questions %d, answers %d authority %d additional %d\n", rxTXID, ntohs(fdh->flags), ntohs(fdh->nQuestions), ntohs(fdh->nAnswers), ntohs(fdh->nAuth), ntohs(fdh->nAdd));
    
    //Checking TXID consistency
    if (rxTXID != sentTXID) {
        printf("  ++ invalid reply: TXID mismatch, sent 0x%.4X, received 0x%.4X\n",
            sentTXID, rxTXID);
        return false;
    }
    

    int rcode = ntohs(fdh->flags) & 0x000F;
    if (rcode == DNS_OK)
        printf("  succeeded with Rcode = %d\n", rcode);
    else
        printf("  failed with Rcode = %d\n", rcode);

    int curPos = sizeof(FixedDNSheader);
    char name[256];


    // ---- questions ----
    printf("  ------------ [questions] ----------\n");
    for (int i = 0; i < ntohs(fdh->nQuestions); i++)
    {
        curPos = parseName(packet, bytes, curPos, name);
        if (curPos == -1) {
            return false;
        }
        if (curPos + 4 > bytes) return false;
        u_short qType = ntohs(*(u_short*)(packet + curPos));
        u_short qClass = ntohs(*(u_short*)(packet + curPos + 2));
        curPos += 4;
        printf("    %s type %d class %d\n", name, qType, qClass);
    }

    
        // ---- answers ----
        printf("  ------------ [answers] ------------\n");
        curPos = parseRRSection(packet, bytes, curPos, ntohs(fdh->nAnswers));
        if (curPos < 0) return false;
   

   
        // ---- authority ----
        printf("  ------------ [authority] ----------\n");
        curPos = parseRRSection(packet, bytes, curPos, ntohs(fdh->nAuth));
        if (curPos < 0) return false;
    

    
        // ---- additional ----
        printf("  ------------ [additional] ---------\n");
        curPos = parseRRSection(packet, bytes, curPos, ntohs(fdh->nAdd));
        if (curPos < 0) return false;
    

    return true;
}

void Socket::resetSocket() {

	if (sock != SOCKET_ERROR) {
		closesocket(sock);
		sock = INVALID_SOCKET;
	}

}