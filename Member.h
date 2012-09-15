#pragma once
#include <string>
#include <stdint.h>
#include <sys/time.h>

using namespace std;

struct MemberID {
	uint32_t m_ipAddress;
	uint16_t m_port;
	bool operator<(const MemberID &id) const;
	bool operator<=(const MemberID &id) const;
	bool operator>(const MemberID &id) const;
	bool operator>=(const MemberID &id) const;
	bool operator==(const MemberID &id) const;
	bool operator!=(const MemberID &id) const;
};

class Member {
private:
	MemberID m_id;
	string m_name;
	uint32_t m_lastSequenceNumber;
	int64_t m_timestamp;
	uint32_t m_password;
	string m_publicKey;
public:
	Member(uint32_t ipv4Address, uint16_t port, string name);
	MemberID getId();
	string getName();
	void setLastSequenceNumber(uint32_t);
	uint32_t getLastSequenceNumber();
	void setTimestamp(int64_t);
	int64_t getTimestamp();
	void setPassword(uint32_t);
	uint32_t getPassword();
	void setPublicKey(string);
	string getPublicKey();
};
