//Gokulan Valavan
//CSCE 612 Spring 2026

#include "pch.h"

//Function to convert host string to a DNS friendly query
void makeDNSquestion(char* buf, char* host) {

	int i = 0;
	char* word = host;

	while (*word) {

		char* p = word;
		while (*p != '.' && *p != '\0') {
			p++;
		}

		int len = p - word;

		buf[i++] = len;
		memcpy(buf + i, word, len);
		i += len;

		if (*p == '.') {
			word = p + 1;
		}
		else {
			break;
		}

	}
	buf[i] = 0;
}

//Building a packet to send to server
int buildPacket(char* host, char* packet) {

	QueryHeader* qh;
	FixedDNSheader* fdh;
	int queryType;
	char queryName[256];

	DWORD ip = inet_addr(host);
	if (ip != INADDR_NONE)
	{
		// reverse the octets and append .in-addr.arpa
		struct in_addr addr;
		addr.s_addr = ip;
		// ip is already in network byte order from inet_addr
		unsigned char* bytes = (unsigned char*)&ip;
		sprintf_s(queryName, "%d.%d.%d.%d.in-addr.arpa",bytes[3], bytes[2], bytes[1], bytes[0]);
		queryType = DNS_PTR;
	}
	else
	{
		strcpy_s(queryName, host);
		queryType = DNS_A;
	}

	int pkt_size = strlen(queryName) + 2 + sizeof(FixedDNSheader) + sizeof(QueryHeader); //Packet size as per notes
	
	fdh = (FixedDNSheader*)packet;
	qh = (QueryHeader*)(packet + pkt_size - sizeof(QueryHeader));

	//Setting headers and query bytes
	fdh->TXID = htons(rand() % 0xFFFF);
	fdh->flags = htons(DNS_QUERY | DNS_RD | DNS_STDQUERY);
	fdh->nQuestions = htons(1);
	fdh->nAnswers = 0;
	fdh->nAdd = 0;
	fdh->nAuth = 0;

	qh->qType = htons(queryType);
	qh->qClass = htons(DNS_INET);
	
	printf("Query   : %s, type %d, TXID 0x%.4X\n", queryName, queryType, ntohs(fdh->TXID));

	makeDNSquestion((char*)(fdh + 1), queryName); //Calling function to convert the host to query

	if (pkt_size > MAX_DNS_LEN) {
		printf("Packet size exceeded Maximum: %d", sizeof(packet));
		return -1;
	}

	return pkt_size;
}

//Function to parse qname to hostname
int parseName(char* buf, int bytes, int pos, char* result)
{
	int jumped = 0;
	int savedPos = -1;
	int nameLen = 0;
	int steps = 0;

	while (pos < bytes && steps < bytes)
	{
		unsigned char len = (unsigned char)buf[pos];

		if (len == 0)
		{
			pos++;
			break;
		}
		else if (len >= 0xC0)
		{
			// check second byte exists
			if (pos + 1 >= bytes) {
				printf("  ++ invalid record: truncated jump offset\n");
				return -1;
			}

			// save return position on first jump only
			if (!jumped)
				savedPos = pos + 2;

			// compute 14-bit offset
			int offset = ((len & 0x3F) << 8) | (unsigned char)buf[pos + 1];

			
			if (offset < (int)sizeof(FixedDNSheader)) {
				printf("  ++ invalid record: jump into fixed DNS header\n");
				return -1;
			}
			if (offset >= bytes) {
				printf("  ++ invalid record: jump beyond packet boundary\n");
				return -1;
			}

			pos = offset;
			jumped = 1;
		}
		else
		{
			// literal label
			pos++;

			// check enough bytes remain for this label
			if (pos + len > bytes) {
				printf("  ++ invalid record: truncated name\n");
				return -1;
			}

			// add dot between labels
			if (nameLen > 0)
				result[nameLen++] = '.';

			memcpy(result + nameLen, buf + pos, len);
			nameLen += len;
			pos += len;
		}

		steps++;
	}

	// detect infinite loop
	if (steps >= bytes) {
		printf("  ++ invalid record: jump loop\n");
		return -1;
	}

	result[nameLen] = '\0';

	if (jumped)
		return savedPos;
	else
		return pos;
}

//Parsing resource record sections of received packet
int parseRRSection(char* buf, int bytes, int curPos, int count)
{
	char name[256];
	char rdataName[256];

	for (int i = 0; i < count; i++)
	{	
		// parse RR name
		curPos = parseName(buf, bytes, curPos, name);
		if (curPos < 0) {
			printf("  ++ invalid section: not enough records\n");
			return -1;
		}

		// check room for RR header
		if (curPos + (int)sizeof(DNSanswerHdr) > bytes) {
			printf("  ++ invalid record: truncated RR answer header\n");
			return -1;
		}

		// read fixed RR header
		DNSanswerHdr* rr = (DNSanswerHdr*)(buf + curPos);
		u_short type = ntohs(rr->type);
		u_short cls = ntohs(rr->aClass);
		u_int   ttl = ntohl(rr->ttl);
		u_short rdlen = ntohs(rr->len);
		curPos += sizeof(DNSanswerHdr);

		if (curPos + rdlen > bytes) {
			printf("  ++ invalid record: RR value length stretches the answer beyond packet\n");
			return -1;
		}

		// read RDATA based on type
		if (type == DNS_A && rdlen == 4)
		{
			struct in_addr addr;
			memcpy(&addr, buf + curPos, 4);
			printf("    %s A %s TTL = %u\n", name, inet_ntoa(addr), ttl);
		}
		else if (type == DNS_CNAME)
		{
			int ret = parseName(buf, bytes, curPos, rdataName);
			if (ret < 0) {
				curPos += rdlen;
				continue;  // skip this record
			}
			printf("    %s CNAME %s TTL = %u\n", name, rdataName, ttl);
		}
		else if (type == DNS_NS)
		{
			int ret = parseName(buf, bytes, curPos, rdataName);
			if (ret < 0) {
				curPos += rdlen;
				continue;  // skip this record
			}
			printf("    %s NS %s TTL = %u\n", name, rdataName, ttl);
		}
		else if (type == DNS_PTR)
		{
			int ret = parseName(buf, bytes, curPos, rdataName);
			if (ret < 0) {
				curPos += rdlen;
				continue;  // skip this record
			}
			printf("    %s PTR %s TTL = %u\n", name, rdataName, ttl);
		}

		// always advance by rdlen
		curPos += rdlen;
	}

	return curPos;
}