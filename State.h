#pragma once
#include <sys/time.h>
#include <map>
#include <stdint.h>
#include "Member.h"
#include "Message.h"

using namespace std;

struct MessageBlock {
	Message message;
	uint32_t destAddress;
	uint16_t destPort;
	int64_t timestamp;
	uint32_t count;
};

class State {
public:
	enum StateType {
		CONNECT = 1,
		LOGIN = 2,
		ELECTION = 3,
		BULLY = 4,
		CHAT = 5,
		MULTICAST = 6,
		NORMAL = 7,
		LEADER = 8
	};

	State();
	~State();
	void setStateType(StateType value);
	StateType getStateType();
	void setSequenceNumber(uint32_t value);
	uint32_t getSequenceNumber();
	map<MemberID, bool> * getAckStatus();
	MessageBlock * getConnectStatus();
	MessageBlock * getLoginStatus();
	MessageBlock * getMessageStatus();
private:
	StateType m_type;
	uint32_t m_sequenceNumber;
	map<MemberID, bool> *m_ackStatus;
	//Smap<uint32_t, MessageBlock *> * m_messageTracker;
	MessageBlock *m_connectTracker;
	MessageBlock *m_loginTracker;
	MessageBlock *m_messageTracker;
};
