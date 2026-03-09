
//Gokulan Valavan
//CSCE 612 Spring 2026

#pragma once
#pragma pack(push,1)

class QueryHeader
{
public:
	
	u_short qType;
	u_short qClass;

};

class FixedDNSheader {
public:
	u_short TXID;
	u_short flags;
	u_short nQuestions;
	u_short nAnswers;
	u_short nAuth;
	u_short nAdd;

};

class DNSanswerHdr {
public:
	u_short type;
	u_short aClass;
	u_int ttl;
	u_short len;
};

#pragma pack(pop)


