#pragma once
#include "Member.h"
#include <stdint.h>
#include <map>

class OutputBuffer {
private:
	char *m_buffer;
	char *m_current;
public:
	OutputBuffer(uint32_t size);
	~OutputBuffer();
	void writeU8(uint8_t value);
	void writeU16(uint16_t value);
	void writeU32(uint32_t value);
	void write(const char *value, uint32_t length);
	char * toByteArray();
};

class InputBuffer {
private:
	char *m_buffer;
	char *m_current;
public:
	InputBuffer(char *buffer);
	~InputBuffer();
	uint8_t readU8();
	uint32_t readU32();
	uint16_t readU16();
	char * read(uint32_t length);
};

class Message {
public:
	enum MessageType {
		CONNECT = 1,
		CONNECT_OK = 2,
		LOGIN = 3,
		LOGIN_OK = 4,
		HELLO = 5,
		LEADER = 6,
		ELECTION = 7,
		BULLY = 8,
		CHAT = 9,
		MULTICAST = 10,
		ACK = 11,
	};

	struct MemberList {
		map<MemberID, Member *> *list;
		uint32_t getSerializedSize();
		void serialize(OutputBuffer *buffer);
		void deserialize(InputBuffer *buffer);
	};

	Message();
	Message(InputBuffer *buffer);
	Message(Message::MessageType, uint32_t, MemberID);
	void setMessageType(MessageType);
	MessageType getMessageType();
	uint32_t getSequenceNumber();
	uint32_t getSerializedSize();
	void serialize(OutputBuffer *buffer);
	MemberID getOriginator();

	void setName(string);
	string getName();
	void setLeader(MemberID id);
	MemberID getLeader();
	void setMemberList(map<MemberID, Member *> *);
	map<MemberID, Member *> * getMemberList();
	void setChatContent(string);
	string getChatContent();
	void setSender(MemberID);
	MemberID getSender();
	void setPublicKey(string);
	string getPublicKey();
	void setDigest(string);
	string getDigest();
	void setContentDigest(string);
	string getContentDigest();
private:
	MessageType m_type;
	MemberID m_originator;
	uint32_t m_sequenceNumber;

	string m_name;
	MemberList m_memberList;
	MemberID m_leader;
	string m_chatContent;
	MemberID m_sender;
	string m_publicKey;
	string m_digest;
	string m_contentDigest;
};
