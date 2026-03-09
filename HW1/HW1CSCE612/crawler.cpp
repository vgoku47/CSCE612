///*
//Gokulan Valavan
//MS CEEN 2027
//CSCE 612-600
//Spring 2026
//*/

//SRW Locks are extensively used in this homework due to its nature of having zero memory allocation
//Thus being nearly immune to violent crashes caused by memory pressure seen while using CRITICAL_SECTION locks
//MSDN: https://learn.microsoft.com/en-us/windows/win32/sync/slim-reader-writer--srw--locks

#include "crawler.h"

//Forward definitions of HTML parsing functions here that were previously in main
int extractStatusCode(char* response, int responselen);

int parseHTML(char* htmlBody, int bodyLen, const char* baseURL, int* parseTime, bool* tamuFound);

//Separated DNS from socket so it can be called by multiple threads 
bool doDNS(const char* host, int port, sockaddr_in& server, DWORD& outIP) {

	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);

	// First check if host is already a dotted IP address
	DWORD IP = inet_addr(host);
	if (IP != INADDR_NONE) {
		server.sin_addr.S_un.S_addr = IP;
		outIP = IP;
		return true;
	}

	// Host is a name now resolved with getaddrinfo
	struct addrinfo hints = {0}, * result = NULL;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;         // IPv4 only
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;


	int ret = getaddrinfo(host, NULL, &hints, &result);

	if (ret != 0 || result == NULL) {
		//printf("getaddrinfo failed for %s: %d\n", host, ret);
		if (result) freeaddrinfo(result);
		return false;
	}

	// Copy resolved address into the caller's local server struct
	struct sockaddr_in* resolved = (struct sockaddr_in*)result->ai_addr;
	server.sin_addr = resolved->sin_addr;
	outIP = server.sin_addr.S_un.S_addr;

	freeaddrinfo(result);
	return true;
}

//Constructer for Crawler Class
Crawler::Crawler(int numThreads) {

	E = 0;
	H = 0;
	D = 0;
	I = 0;
	R = 0;
	C = 0;
	L = 0;

	pagesLastReport = 0;
	bytesLastReport = 0;
	totalBytes = 0;

	http2xx = 0;
	http3xx = 0;
	http4xx = 0;
	http5xx = 0;
	httpOther = 0;

	activeThreads = numThreads;
	this->numThreads = numThreads;

	// Initialize critical sections
	InitializeSRWLock(&qlock);
	InitializeSRWLock(&setslock);

	// Events used to coordinate stats and queue backpressure.
	eventQuit = CreateEvent(NULL, true, false, NULL);
	queueEvent = CreateEvent(NULL, false, false, NULL);
	queueSpaceEvent = CreateEvent(NULL, false, true, NULL);
	producerThread = NULL;
	producerDone = 0;


	startTime = GetTickCount64();
}

Crawler::~Crawler() {
	
	if (producerThread) {
		CloseHandle(producerThread);
		producerThread = NULL;
	}
	if (queueEvent) {
		CloseHandle(queueEvent);
		queueEvent = NULL;
	}
	if (queueSpaceEvent) {
		CloseHandle(queueSpaceEvent);
		queueSpaceEvent = NULL;
	}
	CloseHandle(eventQuit);

}

//Function to spawn producer threads that run ReadFile functions
void Crawler::StreamURLs(const char* filename) {
	// Producer runs in its own thread to stream URLs into the queue.
	producerThread = CreateThread(NULL, 0, [](LPVOID param) -> DWORD {
			auto* args = static_cast<std::pair<Crawler*, const char*>*>(param);
			args->first->ReadFile(args->second);
			delete args;
			return 0;
		},
		new std::pair<Crawler*, const char*>(this, filename),
		0, NULL);
}

//Producer function that reads URLs line-by-line from the file and queues them. The queue maxes out QUEUE_LIMIT number of URLs
void Crawler::ReadFile(const char* filename) {
	std::ifstream file(filename);
	std::string url;
	if (!file.is_open()) {
		printf("Failed to open file %s\n", filename);
		InterlockedExchange(&producerDone, 1);
		SetEvent(queueEvent);
		return;
	}

	int count = 0;
	while (std::getline(file, url)) {
		if (url.empty()) continue;
		if (url.back() == '\r') url.pop_back();
		if (url.empty()) continue;

		while (true) {
			// Backpressure when queue is full.
			AcquireSRWLockExclusive(&qlock);
			if (pendingURL.size() < QUEUE_LIMIT) {
				pendingURL.push(std::move(url));
				ReleaseSRWLockExclusive(&qlock);
				SetEvent(queueEvent);
				count++;
				break;
			}
			ReleaseSRWLockExclusive(&qlock);
			WaitForSingleObject(queueSpaceEvent, 5);
		}
	}
	file.close();
	//printf("Streamed %d URLs into queue\n", count);
	InterlockedExchange(&producerDone, 1);
	SetEvent(queueEvent);
}

//Ensures producer thread exits completely before initiating the desctructor
void Crawler::WaitProducer() {
	if (producerThread) {
		WaitForSingleObject(producerThread, INFINITE);
	}
}

//Check for robots (moved from main)
bool Crawler::CheckRobots(Socket& socket, URLParse& urlParser, sockaddr_in& server) {

	
	socket.resetSocket();
	int connectTime, loadTime;
	if (!socket.connectSocket(&connectTime, server)) {
		return false;
	}

	char robotsRequest[512];
	urlParser.requestServerHead(robotsRequest, sizeof(robotsRequest));
	socket.sendMessage(robotsRequest);

	if (!socket.readMessage(&loadTime, 16 * 1024)) {
		return false;
	}

	// Parse robots status code
	int robotsStatus = extractStatusCode(socket.retBuff(), socket.retBytesRecv());
	if (robotsStatus == -1) {
		return false;
	}

	// Check if status is 400-499 to ensure robots.txt doesn't exist
	if (robotsStatus >= 400 && robotsStatus < 500) {
		return true;
	}

	return false;
}

//function that crawls HTML pages (moved from main)
bool Crawler::CrawlPage(Socket& socket, URLParse& urlParser, sockaddr_in &server, int& bytesDownloaded, int& linksFound, int& httpStatus, bool& tamuFound) {

	int connectTime;
	int loadTime;
	tamuFound = false;

	if (!socket.connectSocket(&connectTime, server)) {
		return false;
	}

	char pageRequest[MAX_REQUEST_LEN];
	urlParser.requestServerGet(pageRequest, sizeof(pageRequest));
	socket.sendMessage(pageRequest);

	if (!socket.readMessage(&loadTime, 2 * 1024 * 1024)) {
		return false;
	}

	bytesDownloaded = socket.retBytesRecv();

	// Parse page status
	httpStatus = extractStatusCode(socket.retBuff(), socket.retBytesRecv());
	if (httpStatus == -1) {
		return false;
	}

	if (httpStatus >= 200 && httpStatus < 300) {
		InterlockedIncrement(&http2xx);
	}
	else if (httpStatus >= 300 && httpStatus < 400) {
		InterlockedIncrement(&http3xx);
	}
	else if (httpStatus >= 400 && httpStatus < 500) {
		InterlockedIncrement(&http4xx);
	}
	else if (httpStatus >= 500 && httpStatus < 600) {
		InterlockedIncrement(&http5xx);
	}
	else
		InterlockedIncrement(&httpOther);

	int nLinks = 0;
	// Parse HTML if 2xx
	if (httpStatus >= 200 && httpStatus < 300) {

		char* response = socket.retBuff();
		int responseLen = socket.retBytesRecv();

		char* headerEnd = strstr(response, "\r\n\r\n");
		if (headerEnd != NULL) {
			char* body = headerEnd + 4;
			int bodyLen = (int) (responseLen - (body - response));

			char baseURL[MAX_URL_LEN];
			snprintf(baseURL, sizeof(baseURL),"http://%s%s", urlParser.host, urlParser.path);

			int parseTime;
			nLinks = parseHTML(body, bodyLen, baseURL, &parseTime, &tamuFound);

			if (nLinks < 0) {
				nLinks = 0;
			}
		}
	}
	linksFound = nLinks;

	return true;

}

//Main consumer function that dequeues to get URL and works on it in different threads stats are updated using InterlockedIncrements
void Crawler::Run() {
	//printf("Thread started\n");

	while (true) {
		std::string url;

		//printf("Checking queue, size = %d\n", (int)pendingURL.size());

		// Pop work from the shared queue.
		AcquireSRWLockExclusive(&qlock);
		if (pendingURL.empty()) {
			ReleaseSRWLockExclusive(&qlock);
			if (InterlockedCompareExchange(&producerDone, 1, 1) == 1) {
				break;
			}
			WaitForSingleObject(queueEvent, 10);
			continue;
		}
		url = pendingURL.front();
		pendingURL.pop();
		ReleaseSRWLockExclusive(&qlock);
		SetEvent(queueSpaceEvent);

		InterlockedIncrement(&E);

		// ---- Parse URL ----
		URLParse u;
		if (!u.URLParser(url.c_str())) {
			continue;
		}
		size_t hostLen = strnlen(u.host, MAX_HOST_LEN);
		if (hostLen == 0 || hostLen >= MAX_HOST_LEN) {

			continue;
		}


		// ---- Host uniqueness ----
		AcquireSRWLockShared(&setslock);
		bool hostSeen = (seenhosts.count(u.host) > 0);
		ReleaseSRWLockShared(&setslock);
		if (!hostSeen) {
			AcquireSRWLockExclusive(&setslock);
			hostSeen = (seenhosts.count(u.host) > 0);
			if (!hostSeen) seenhosts.insert(u.host);
			ReleaseSRWLockExclusive(&setslock);
		}
		if (hostSeen) continue;

		InterlockedIncrement(&H);

		// ---- DNS  ----
		sockaddr_in server;
		DWORD ipAddr = 0;
		char hostCopy[MAX_HOST_LEN];
		strncpy(hostCopy, u.host, sizeof(hostCopy) - 1);
		hostCopy[sizeof(hostCopy) - 1] = '\0';
		if (hostCopy[0] == '\0') continue;
		if (!doDNS(hostCopy, u.port, server, ipAddr)) continue;

		InterlockedIncrement(&D);

		// ---- IP uniqueness ----
		AcquireSRWLockShared(&setslock);
		bool ipSeen = (seenIPs.count(ipAddr) > 0);
		ReleaseSRWLockShared(&setslock);
		if (!ipSeen) {
			AcquireSRWLockExclusive(&setslock);
			ipSeen = (seenIPs.count(ipAddr) > 0);
			if (!ipSeen) seenIPs.insert(ipAddr);
			ReleaseSRWLockExclusive(&setslock);
		}
		if (ipSeen) continue;

		InterlockedIncrement(&I);
		// Passed IP check

		// Robots check
		Socket s;
		if (!CheckRobots(s, u, server)) {
			continue;  // Robots disallowed
		}
		InterlockedIncrement(&R);  // Passed robots

		s.resetSocket();
		// Crawl page
		int bytesDownloaded = 0;
		int linksFound = 0;
		int httpStatus = 0;
		bool tamuFound = false;

		if (CrawlPage(s, u, server, bytesDownloaded, linksFound, httpStatus, tamuFound)) {
			InterlockedIncrement(&C);
			InterlockedAdd(&bytesLastReport, bytesDownloaded);
			InterlockedIncrement(&pagesLastReport);
			InterlockedAdd(&L, linksFound);
			InterlockedAdd64(&totalBytes, bytesDownloaded);

			// Track tamu.edu in-degree (only for 2xx pages)
			if (tamuFound && httpStatus >= 200 && httpStatus < 300) {
				InterlockedIncrement(&tamuPages);
				// Check if source host is NOT tamu.edu
				int hostLen = (int)strlen(u.host);
				bool isTamu = false;
				// Check if source host is NOT tamu.edu
				if (hostLen >= 8) {
					isTamu = (_strnicmp(u.host + hostLen - 8, "tamu.edu", 8) == 0);
				}
				if (!isTamu) {
					InterlockedIncrement(&tamuExternal);
				}
			}
			
		}
	}
	InterlockedDecrement(&activeThreads);

}

//Function to calculate, print stats
void Crawler::StatsRun() {
	int elapsedSeconds = 0;

	while (WaitForSingleObject(eventQuit, 2000) == WAIT_TIMEOUT) {
		elapsedSeconds += 2;

		// Read queue size
		AcquireSRWLockExclusive(&qlock);
		int queueSize = (int)pendingURL.size();
		ReleaseSRWLockExclusive(&qlock);

		// Read counters
		LONG extracted = E;
		LONG hostUnique = H;
		LONG dnsSuccess = D;
		LONG ipUnique = I;
		LONG robotsOK = R;
		LONG crawled = C;
		LONG links = L;
		LONG active = activeThreads;

		// Calculate rates (reset counters atomically)
		LONG pagesSinceLastReport = InterlockedExchange(&pagesLastReport, 0);
		LONG bytesSinceLastReport = InterlockedExchange(&bytesLastReport, 0);

		double pps = pagesSinceLastReport / 2.0;  // 2 second interval
		double mbps = (bytesSinceLastReport * 8.0) / (2.0 * 1000000.0);

		// Print stats
		printf("[%3d] %4d Q %6d E %7d H %6d D %6d I %5d R %5d C %5d L %4dK\n",
			elapsedSeconds, active, queueSize, extracted, hostUnique,
			dnsSuccess, ipUnique, robotsOK, crawled, links / 1000);
		printf(" *** crawling %.1f pps @ %.1f Mbps\n", pps, mbps);
	}

	// Print final summary
	DWORD64 endTime = GetTickCount64();
	double totalSeconds = (double)(endTime - startTime) / 1000;

	if (totalSeconds < 0.001) totalSeconds = 0.001;

	printf("\nExtracted %ld URLs @ %.0f/s\n", E, E / totalSeconds);
	printf("Looked up %ld DNS names @ %.0f/s\n", D, D / totalSeconds);
	printf("Attempted %ld robots @ %.0f/s\n", R, R / totalSeconds);
	printf("Crawled %ld pages @ %.0f/s (%.2f MB)\n", C, C / totalSeconds,
		(double)totalBytes / (1024 * 1024));
	printf("Parsed %ld links @ %.0f/s\n", L, L / totalSeconds);
	printf("HTTP codes: 2xx = %ld, 3xx = %ld, 4xx = %ld, 5xx = %ld, other = %ld\n",
		http2xx, http3xx, http4xx, http5xx, httpOther);
	printf("\nTamu.edu in-degree: %ld pages link to tamu.edu (%ld from outside tamu.edu)\n",
		tamuPages, tamuExternal);
}

void Crawler::Shutdown() {
	// Signal stats thread to quit
	SetEvent(eventQuit);
}

// Static thread wrapper for stats thread
DWORD WINAPI Crawler::StaticStatsThread(LPVOID param) {
	Crawler* crawler = (Crawler*)param;
	crawler->StatsRun();
	return 0;
}

// Static thread wrapper for crawler threads
DWORD WINAPI Crawler::StaticCrawlerThread(LPVOID param) {
	Crawler* crawler = (Crawler*)param;
	crawler->Run();
	return 0;
}
