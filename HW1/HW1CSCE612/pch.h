/*
Gokulan Valavan
MS CEEN 2027
CSCE 612-600
Spring 2026
*/

#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

//General Header Files
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <queue>
#include <fstream>
#include <set>
#include <ctime>
#include <stdio.h>
#include <string.h>
#include <thread>
#include <mutex>
#include <utility>
#include <cctype>
#ifdef _DEBUG
#include <crtdbg.h>
#endif

//Header files for Class Definitions
#include "HTMLParserBase.h"
#include "URL.h"
#include "socket.h"
#include "crawler.h"

//Loading Winsock2 libraries
#pragma comment(lib, "ws2_32.lib")

