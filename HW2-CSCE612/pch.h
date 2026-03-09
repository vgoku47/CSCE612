//Gokulan Valavan
//CSCE 612 Spring 2026


#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS   

//Essential header files
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <ctime>
#include <stdio.h>
#include <string.h>

//Class definitions
#include "socket.h"
#include "message.h"

#pragma comment(lib, "ws2_32.lib")

//Constants needed incase
#define MAX_DNS_LEN 512
#define BUF_SIZE 65536
#define THRESHOLD 1024
#define TIMEOUT 10
#define MAX_ATTEMPTS 3
#define _WINSOCK_DEPRECATED_NO_WARNINGS

//DNS query types
#define DNS_A       1       // name -> IP
#define DNS_NS      2       // name server
#define DNS_CNAME   5       // canonical name
#define DNS_PTR     12      // IP -> name
#define DNS_HINFO   13      // host info
#define DNS_MX      15      // mail exchange
#define DNS_AXFR    252     // request for zone transfer
#define DNS_ANY     255     // all records

//query class
#define DNS_INET    1

// DNS flags
#define DNS_QUERY    (0 << 15)   // 0 = query
#define DNS_RESPONSE (1 << 15)   // 1 = response
#define DNS_STDQUERY (0 << 11)   // opcode standard query
#define DNS_AA       (1 << 10)   // authoritative answer
#define DNS_TC       (1 << 9)    // truncated
#define DNS_RD       (1 << 8)    // recursion desired
#define DNS_RA       (1 << 7)    // recursion available

// Rcode values 
#define DNS_OK         0
#define DNS_FORMAT     1
#define DNS_SERVERFAIL 2
#define DNS_ERROR      3
#define DNS_NOTIMPL    4
#define DNS_REFUSED    5
