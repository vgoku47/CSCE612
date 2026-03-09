//
//Gokulan Valavan
//MS CEEN 2027
//CSCE 612-600
//Spring 2026


#pragma once
#include "pch.h"

class Crawler
{
private:

	// Bounded queue to limit memory usage.
	std::queue<std::string> pendingURL;
	SRWLOCK qlock;
	HANDLE queueEvent;
	HANDLE queueSpaceEvent;
	HANDLE producerThread;
	volatile LONG producerDone;
	// Upper bound for queued URLs
	static const size_t QUEUE_LIMIT = 50000;

	// Stats counters (updated across threads).

	volatile LONG E;  // Extracted URLs from queue
	volatile LONG H;  // Passed host uniqueness
	volatile LONG D;  // Successful DNS lookups
	volatile LONG I;  // Passed IP uniqueness
	volatile LONG R;  // Passed robots checks
	volatile LONG C;  // Successfully crawled pages
	volatile LONG L;  // Total links found

	volatile LONG http2xx;
	volatile LONG http3xx;
	volatile LONG http4xx;
	volatile LONG http5xx;
	volatile LONG httpOther;

	// rate calculations (pps and Mbps)

	volatile LONG pagesLastReport;
	volatile LONG bytesLastReport;

	volatile LONGLONG totalBytes;

	// tamu.edu in-degree tracking
	volatile LONG tamuPages;      // 2xx pages with at least one link to tamu.edu
	volatile LONG tamuExternal;   // subset where source host is NOT tamu.edu

	// Uniqueness tracking across the crawl.
	std::set<std::string> seenhosts;
	std::set<DWORD> seenIPs;
	SRWLOCK setslock;

	// Thread control

	HANDLE eventQuit;
	int numThreads;
	volatile LONG activeThreads;
	

	// Timing
	DWORD64 startTime;

public:

	Crawler(int numThreads);
	~Crawler();
	void StreamURLs(const char* filename);
	void ReadFile(const char* filename);
	void WaitProducer();
	void Run();
	void StatsRun();
	bool CheckRobots(Socket& socket, URLParse& u, sockaddr_in& server);
	bool CrawlPage(Socket& socket, URLParse& urlParser, sockaddr_in& server, int& bytesDownloaded, int& linksFound, int& httpStatus, bool& tamuFound);
	void Shutdown();

	// Thread wrappers (static functions for CreateThread)
	static DWORD WINAPI StaticStatsThread(LPVOID param);
	static DWORD WINAPI StaticCrawlerThread(LPVOID param);

};

