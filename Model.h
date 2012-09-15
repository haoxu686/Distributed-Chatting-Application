#pragma once
#include <string>
#include <map>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <deque>
#include <iostream>
#include "Member.h"
#include "Message.h"
#include "State.h"
#include "Timer.h"

#include "ConnectTimer.h"
#include "LoginTimer.h"
#include "ElectionTimer.h"
#include "BullyTimer.h"
#include "HelloTimer.h"
#include "LeaderTimer.h"
#include "ChatTimer.h"
#include "MulticastTimer.h"

#include "crypto_box.h"

class Timer;
class Model {
private:
	int m_socketfd;
	uint32_t m_currentSequenceNumber;

	pthread_mutex_t m_mutexEvent;
	pthread_cond_t m_condState;
	pthread_cond_t m_condQueue;
	Timer *m_timer;
	map<MemberID, Member *> *m_listMembers;
	map<uint32_t,MessageBlock *> m_messageTracker;
	deque<Message> m_messageQueue;
	MemberID m_leader;
	int64_t m_leaderTimestamp;
	uint32_t m_leaderLastSequenceNumber;
	Member *m_me;
	State m_state;
	string pub_leader;
	string priv_key;
	string pub_key;
	uint32_t password;
	string nonce;

	uint32_t CONNECT_TIMER_ID;
	uint32_t LOGIN_TIMER_ID;
	uint32_t ELECTION_TIMER_ID;
	uint32_t BULLY_TIMER_ID;
	uint32_t HELLO_TIMER_ID;
	uint32_t LEADER_TIMER_ID;
	uint32_t CHAT_TIMER_ID;
	uint32_t MULTICAST_TIMER_ID;

	void init(string);
	void getMyIpAddress(struct sockaddr_in &);
	void sendMessage(Message &, uint32_t, uint16_t);
	Message recvMessage();
	uint32_t getNextSequenceNumber();

	void handleConnectMsg(Message &message);
	void handleConnectOkMsg(Message &message);
	void handleLoginMsg(Message &message);
	void handleLoginOkMsg(Message &message);
	void handleHelloMsg(Message &message);
	void handleElectionMsg(Message &message);
	void handleBullyMsg(Message &message);
	void handleLeaderMsg(Message &message);
	void handleChatMsg(Message &message);
	void handleMulticastMsg(Message &message);
	void handleAckMsg(Message &message);

	void login();
public:
	Model(string, Timer *);
	Model(string, string, uint16_t, Timer *);
	~Model();
	void handleConnectTimeout();
	void handleLoginTimeout();
	void handleElectionTimeout();
	void handleBullyTimeout();
	void handleHelloTimeout();
	void handleLeaderTimeout();
	void handleChatTimeout();
	void handleMulticastTimeout();

	void doListenMessage();
	void doSend();
	void chat(string);

	int64_t static now();

	int64_t MEMBER_TIMEOUT;
};

