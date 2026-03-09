/*
Gokulan Valavan
MS CEEN 2027
CSCE 612-600
Spring 2026
*/

#include "pch.h"

// Default is 1MB which would be 500MB of stack alone with 500 threads
#define THREAD_STACK_SIZE (64 * 1024)

int main(int argc, char* argv[]) {
	// Check arguments: <num_threads> <URL-input.txt>
	if (argc != 3) {
		printf("Usage: %s <num_threads> <input_file>\n", argv[0]);
		return 0;
	}

	int numThreads = atoi(argv[1]);
	if (numThreads <= 0) {
		printf("Error: number of threads must be positive\n");
		return 0;
	}

	const char* filename = argv[2];

	// Open and check file size
	FILE* fp = fopen(filename, "rb");
	if (!fp) {
		printf("Failed to open file %s\n", filename);
		return 0;
	}

	fseek(fp, 0, SEEK_END);
	long fileSize = ftell(fp);
	fclose(fp);

	printf("Opened %s with size %ld\n", filename, fileSize);

	// Initialize Winsock
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
		printf("WSAStartup failed\n");
		return 0;
	}

	// Initialize Crawler with shared data structures
	Crawler crawler(numThreads);

	// Producer thread streams URLs to keep memory bounded.
	crawler.StreamURLs(filename);

	// Start stats thread
	HANDLE statsThread = CreateThread(NULL,0,Crawler::StaticStatsThread,&crawler,0,NULL);

	// Start N crawling threads
	HANDLE* threadHandles = new HANDLE[numThreads];
	for (int i = 0; i < numThreads; i++) {

		threadHandles[i] = CreateThread(NULL, THREAD_STACK_SIZE,Crawler::StaticCrawlerThread,&crawler,0,NULL);

	}

	int remaining = numThreads;
	int offset = 0;
	while (remaining > 0) {
		int batch = (remaining > 64) ? 64 : remaining;
		WaitForMultipleObjects(batch, threadHandles + offset, TRUE, INFINITE);
		offset += batch;
		remaining -= batch;
	}

	// Clean up crawler thread handles
	for (int i = 0; i < numThreads; i++) {
		CloseHandle(threadHandles[i]);
	}
	delete[] threadHandles;

	// Ensure producer finishes before crawler is destroyed.
	crawler.WaitProducer();

	// Signal stats thread to quit and wait for it to terminate
	crawler.Shutdown();
	WaitForSingleObject(statsThread, INFINITE);
	CloseHandle(statsThread);

	// Cleanup
	WSACleanup();

	return 0;
}
