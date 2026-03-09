/*
Gokulan Valavan
MS CEEN 2027
CSCE 612-600
Spring 2026
*/


#include "pch.h"

int parseHTML(char* htmlBody, int bodyLen, const char* baseURL, int* parseTime, bool* tamuFound) {
    // Validate inputs
    if (htmlBody == nullptr || bodyLen == 0 || baseURL == nullptr) {
        *parseTime = 0;
        if (tamuFound) *tamuFound = false;
        return 0;
    }

    // Start timer
    clock_t start = clock();

    // Create parser instance
    HTMLParserBase parser;

    // Parse HTML
    int nLinks = 0;
    char* linkBuffer = parser.Parse(htmlBody,bodyLen,(char*)baseURL,(int)strlen(baseURL),&nLinks);

    // End timer
    clock_t end = clock();
    *parseTime = (int)(1000*(end - start)/CLOCKS_PER_SEC);

    if (linkBuffer == NULL || nLinks < 0) {
        if (tamuFound) *tamuFound = false;
        return 0;
    }

    // Check parsed links for tamu.edu
    // Links are null-terminated strings packed back-to-back in linkBuffer
    bool foundTamu = false;
    if (tamuFound != nullptr) {
        char* ptr = linkBuffer;
        for (int i = 0; i < nLinks; i++) {
            int linkLen = (int)strlen(ptr);

            // Case-insensitive search for "tamu.edu" anywhere in the link
            for (int j = 0; j <= linkLen - 8; j++) {
                if (_strnicmp(ptr + j, "tamu.edu", 8) == 0) {
                    foundTamu = true;
                    break;
                }
            }

            ptr += linkLen + 1;  // advance past null terminator to next link

            if (foundTamu) break;
        }
        *tamuFound = foundTamu;
    }

    // Do NOT delete linkBuffer - owned by HTMLParserBase, freed when parser destructs

    return nLinks;

}

int extractStatusCode(char* response, int responseLen) {
    // Check if response starts with "HTTP/"
    if (responseLen < 12 || strncmp(response, "HTTP/", 5) != 0) {
        return -1;  // Invalid response
    }

    char* statusStart = strchr(response, ' ');
    if (statusStart == NULL) {
        return -1;
    }

    statusStart++;  // Move past the space
    return atoi(statusStart);  // Convert status code to int
}