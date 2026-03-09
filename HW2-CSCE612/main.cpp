//Gokulan Valavan
//CSCE 612 Spring 2026


#include "pch.h"

//Forward declaration
int buildPacket(char* host, char* packet);

int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("Usage: hw2.exe <lookup string> <DNS server IP>\n");
        return 0;
    }

    char* host = argv[1];
    char* remoteIP = argv[2];


    // print summary
    printf("Lookup  : %s\n", host);
  
    //Winsock init
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }

    Socket s;
    char packet[MAX_DNS_LEN];

    if (!s.openSocket())
        return -1;


    srand((unsigned)time(NULL));

    int pkt_size = buildPacket(host, packet);
    if (pkt_size < 0) {
        return -1;
    }
    printf("Server  : %s\n", remoteIP);
    printf("********************************\n");

    if (!s.sendRequest(remoteIP, packet, pkt_size)) {
        return -1;
    }

    WSACleanup();
    return 0;
}
