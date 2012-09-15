#include "Model.h"
#include <string.h>
#include <iostream>
#include <ifaddrs.h>
#include <sys/time.h>
#include <stdlib.h>
#include <sstream>

using namespace std;

Model::Model(string name, Timer *timer) {
	m_timer = timer;
	init(name);
	char address[100];
	inet_ntop(AF_INET, (void *) &m_me->getId().m_ipAddress, address, INET_ADDRSTRLEN);
	cout << name << " started a new chat, listening on ";
	cout << address << ":" << ntohs(m_me->getId().m_port) << endl;
	cout << "Succeeded, current users:" << endl;
	cout << m_me->getName() << " " << address << ":" << ntohs(m_me->getId().m_port) << " (Leader)" << endl;
	cout << "Waiting for others to join..." << endl;
	m_state.setStateType(State::LEADER);
	m_timer->schedule(LEADER_TIMER_ID, 1);

    pub_leader=pub_key;

}

Model::Model(string name, string contactIpAddress, uint16_t contactPort, Timer *timer) {
	m_timer = timer;
	init(name);
	char address[100];
	inet_ntop(AF_INET, (void *) &m_me->getId().m_ipAddress, address, INET_ADDRSTRLEN);
	cout << name << " joining a new chat on ";
	cout << contactIpAddress << ":" << ntohs(contactPort) << ", listening on ";
	cout << address << ":" << ntohs(m_me->getId().m_port) << endl;

	Message message = Message(Message::CONNECT, getNextSequenceNumber(), m_me->getId());
	uint32_t ipAddress;
	inet_pton(AF_INET, contactIpAddress.c_str(), &ipAddress);
	this->sendMessage(message, ipAddress, contactPort);
	m_state.setStateType(State::CONNECT);
	m_state.setSequenceNumber(message.getSequenceNumber());
	MessageBlock *block = m_state.getConnectStatus();
	block->message = message;
	block->destAddress = ipAddress;
	block->destPort = contactPort;
	block->count = 2;
	m_timer->schedule(CONNECT_TIMER_ID, 1);
}

Model::~Model() {
	close(m_socketfd);
	delete m_me;
	map<MemberID, Member *>::iterator it;
	for (it = m_listMembers->begin(); it != m_listMembers->end(); it++) {
		delete it->second;
	}
	delete m_listMembers;
	pthread_mutex_destroy(&m_mutexEvent);
	pthread_cond_destroy(&m_condState);
	pthread_cond_destroy(&m_condQueue);
}

void Model::init(string name) {
	m_socketfd = socket(AF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in interface;
	bzero((char *) &interface, sizeof(interface));
	getMyIpAddress(interface);
	interface.sin_family = AF_INET;
	bind(m_socketfd, (struct sockaddr *) &interface, sizeof(interface));
	struct sockaddr_in me;
	int size = sizeof(me);
	getsockname(m_socketfd, (struct sockaddr *) &me, (socklen_t *) &size);
	m_me = new Member(me.sin_addr.s_addr, me.sin_port, name);

	m_leader = m_me->getId();
	m_listMembers = new map<MemberID, Member *>();
	Member *tmpMe = new Member(me.sin_addr.s_addr, me.sin_port, name);
	m_listMembers->insert(make_pair(m_me->getId(), tmpMe));

	Runnable *r;
	r = new ConnectTimer(this);
	CONNECT_TIMER_ID = m_timer->add(r);
	r = new LoginTimer(this);
	LOGIN_TIMER_ID = m_timer->add(r);
	r = new ElectionTimer(this);
	ELECTION_TIMER_ID = m_timer->add(r);
	r = new BullyTimer(this);
	BULLY_TIMER_ID = m_timer->add(r);
	r = new HelloTimer(this);
	HELLO_TIMER_ID = m_timer->add(r);
	r = new LeaderTimer(this);
	LEADER_TIMER_ID = m_timer->add(r);
	r = new MulticastTimer(this);
	MULTICAST_TIMER_ID = m_timer->add(r);
	r = new ChatTimer(this);
	CHAT_TIMER_ID = m_timer->add(r);

	MEMBER_TIMEOUT = 4000000;
	m_currentSequenceNumber = 1;

	pthread_mutex_init(&m_mutexEvent, NULL);
	pthread_cond_init(&m_condState, NULL);
	pthread_cond_init(&m_condQueue, NULL);

	struct timeval tv;
	gettimeofday(&tv, NULL);
	srand(tv.tv_sec*tv.tv_usec);

	pub_key = crypto_box_keypair(&priv_key);

	for (uint32_t i=0;i<crypto_box_NONCEBYTES;i++){
		nonce.append("a");
	}
	password=rand()%100;
}

void Model::getMyIpAddress(struct sockaddr_in &sa) {
	struct ifaddrs *begin;
	struct ifaddrs *iterator;
	getifaddrs(&begin);
	char address[100];
	for (iterator = begin; iterator != NULL; iterator = iterator->ifa_next) {
		if (iterator->ifa_addr->sa_family != AF_INET) {
			continue;
		}
		void *tmp = &((struct sockaddr_in *) iterator->ifa_addr)->sin_addr;
		inet_ntop(AF_INET, tmp, address, INET_ADDRSTRLEN);
		if (address[0] == '1' && address[1] == '5' && address[2] == '8') {
			sa.sin_addr = ((struct sockaddr_in *) iterator->ifa_addr)->sin_addr;
			break;
		}
	}
	freeifaddrs(begin);
}

void Model::sendMessage(Message &message, uint32_t ipAddress, uint16_t port) {
	uint32_t flag = rand();
	flag %= 100;
	//if (flag <= 20) {
	//	return;
	//}
	struct sockaddr_in sa;
	bzero((char *) &sa, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = ipAddress;
	sa.sin_port = port;
	int length = message.getSerializedSize();
	OutputBuffer *buffer = new OutputBuffer(length);
	message.serialize(buffer);
	sendto(m_socketfd, buffer->toByteArray(), length, 0, (struct sockaddr *) &sa, sizeof(sa));
}

Message Model::recvMessage() {
	char *input = new char[2000];
	struct sockaddr_in sa;
	bzero((char *) &sa, sizeof(sa));
	int senderlen = sizeof(sa);
	recvfrom(m_socketfd, input, 2000, 0, (struct sockaddr *) &sa, (socklen_t *) &senderlen);
	InputBuffer *buffer = new InputBuffer(input);
	Message message = Message(buffer);
	delete buffer;
	return message;
}

uint32_t Model::getNextSequenceNumber() {
	return m_currentSequenceNumber++;
}

void Model::handleConnectMsg(Message &message) {
	if (m_state.getStateType() == State::LEADER || m_state.getStateType() == State::NORMAL
			|| m_state.getStateType() == State::CHAT || m_state.getStateType() == State::MULTICAST) {
		Message msg1(Message::CONNECT_OK, message.getSequenceNumber(),m_me->getId());
		msg1.setLeader(m_leader);
		msg1.setMemberList(m_listMembers);
		msg1.setPublicKey(pub_leader);
		sendMessage(msg1, message.getOriginator().m_ipAddress,message.getOriginator().m_port);
	}
}

void Model::handleConnectOkMsg(Message &message) {
	pthread_mutex_lock(&m_mutexEvent);
	if (m_state.getStateType() == State::CONNECT) {
		//Modify local variables
		m_leader = message.getLeader();
		pub_leader=message.getPublicKey();
		map<MemberID, Member *>::iterator it;
		for (it = m_listMembers->begin(); it != m_listMembers->end(); it++) {
			delete it->second;
		}
		delete m_listMembers;
		m_listMembers = message.getMemberList();
		m_timer->suspend(CONNECT_TIMER_ID);

		login();
	}
	pthread_mutex_unlock(&m_mutexEvent);
}

void Model::handleLoginMsg(Message &message) {
	if (m_state.getStateType() == State::LEADER || m_state.getStateType() == State::MULTICAST) {
		map<MemberID, Member *>::iterator it = m_listMembers->find(message.getOriginator());
		Member *member;
		if (it == m_listMembers->end()) {
			member = new Member(message.getOriginator().m_ipAddress, message.getOriginator().m_port, message.getName());
		} else {
			member = it->second;
		}
		member->setTimestamp(Model::now());
		member->setLastSequenceNumber(0);
		member->setPublicKey(message.getPublicKey());
		string digest = message.getDigest();
		string password = crypto_box_open(digest, nonce, message.getPublicKey(), priv_key);
		member->setPassword(atoi(password.c_str()));
		m_listMembers->insert(make_pair(member->getId(), member));
		Message msg1(Message::LOGIN_OK, message.getSequenceNumber(),m_me->getId());
		msg1.setMemberList(m_listMembers);
		msg1.setPublicKey(pub_key);
		sendMessage(msg1, message.getOriginator().m_ipAddress,message.getOriginator().m_port);
		char address[100];
		inet_ntop(AF_INET, (void *) &message.getOriginator().m_ipAddress, address, INET_ADDRSTRLEN);
		cout << "NOTICE " << message.getName() << " joined on " << address << ":" << ntohs(message.getOriginator().m_port) << endl;
	}
}

void Model::handleLoginOkMsg(Message &message) {
	pthread_mutex_lock(&m_mutexEvent);
	if (m_state.getStateType() == State::LOGIN) {

		//Modify local variables
		m_state.setStateType(State::NORMAL);
		map<MemberID, Member *>::iterator it;
		for (it = m_listMembers->begin(); it != m_listMembers->end(); it++) {
			delete it->second;
		}
		delete m_listMembers;
		m_listMembers = message.getMemberList();
		pub_leader = message.getPublicKey();
		m_leaderTimestamp = Model::now();
		cout << "Succeeded, current users:" << endl;
		char address[100];
		for (it = m_listMembers->begin(); it != m_listMembers->end(); it++) {
			inet_ntop(AF_INET, (void *) &it->first.m_ipAddress, address, INET_ADDRSTRLEN);
			cout << it->second->getName() << " " << address << ":" << ntohs(it->first.m_port);
			if (it->first == m_leader) {
				cout << " (Leader)";
			}
			cout << endl;
		}
		pthread_cond_broadcast(&m_condState);

		//Manipulate timers
		m_timer->suspend(LOGIN_TIMER_ID);
		m_timer->schedule(HELLO_TIMER_ID, 1);
	}
	pthread_mutex_unlock(&m_mutexEvent);
}

void Model::handleHelloMsg(Message &message) {
	pthread_mutex_lock(&m_mutexEvent);
	if (m_state.getStateType() == State::LEADER || m_state.getStateType() == State::MULTICAST) {
		map<MemberID, Member *>::iterator itr;
		itr = m_listMembers->find(message.getOriginator());
		if (itr != m_listMembers->end()) {
			itr->second->setTimestamp(Model::now());
			itr->second->setPublicKey(message.getPublicKey());
			string digest = message.getDigest();
			string password = crypto_box_open(digest, nonce, message.getPublicKey(), priv_key);
			itr->second->setPassword(atoi(password.c_str()));
		}
	}
	pthread_mutex_unlock(&m_mutexEvent);
}

void Model::handleElectionMsg(Message &message) {
	pthread_mutex_lock(&m_mutexEvent);
	map<MemberID, Member *>::iterator it = m_listMembers->find(message.getOriginator());
	if (it == m_listMembers->end()) {
		pthread_mutex_unlock(&m_mutexEvent);
		return;
	}
	switch (m_state.getStateType()) {
	case State::LEADER: {
		if (m_me->getId() > message.getOriginator()) {
			//Change local variables
			m_state.setStateType(State::ELECTION);
			//Send message
			Message msg1(Message::BULLY, message.getSequenceNumber(), m_me->getId());
			sendMessage(msg1, message.getOriginator().m_ipAddress, message.getOriginator().m_port);

			Message msg2(Message::ELECTION, getNextSequenceNumber(), m_me->getId());
			for (it = m_listMembers->begin(); it != m_listMembers->end(); it++) {
				if (m_me->getId() < it->first) {
					sendMessage(msg2, it->first.m_ipAddress, it->first.m_port);
				}
			}
			//Manipulate timers
			m_timer->suspend(LEADER_TIMER_ID);
			m_timer->schedule(ELECTION_TIMER_ID, 3);
		} else {
			m_state.setStateType(State::BULLY);
			m_timer->suspend(LEADER_TIMER_ID);
			m_timer->schedule(BULLY_TIMER_ID, 3);
		}
		break;
	}
	case State::MULTICAST: {
		if (m_me->getId() > message.getOriginator()) {
			//Change local variables
			m_state.setStateType(State::ELECTION);
			//Send message
			Message msg1(Message::BULLY, message.getSequenceNumber(), m_me->getId());
			sendMessage(msg1, message.getOriginator().m_ipAddress, message.getOriginator().m_port);

			Message msg2(Message::ELECTION, getNextSequenceNumber(), m_me->getId());
			for (it = m_listMembers->begin(); it != m_listMembers->end(); it++) {
				if (m_me->getId() < it->first) {
					sendMessage(msg2, it->first.m_ipAddress, it->first.m_port);
				}
			}
			//Manipulate timers
			m_timer->suspend(LEADER_TIMER_ID);
			m_timer->suspend(MULTICAST_TIMER_ID);
			m_timer->schedule(ELECTION_TIMER_ID, 3);
		} else {
			m_state.setStateType(State::BULLY);
			m_timer->suspend(LEADER_TIMER_ID);
			m_timer->suspend(MULTICAST_TIMER_ID);
			m_timer->schedule(BULLY_TIMER_ID, 3);
		}
		break;
	}
	case State::NORMAL: {
		if (m_me->getId() > message.getOriginator()) {
			//Change local variables
			m_state.setStateType(State::ELECTION);
			//Send message
			Message msg1(Message::BULLY, message.getSequenceNumber(), m_me->getId());
			sendMessage(msg1, message.getOriginator().m_ipAddress, message.getOriginator().m_port);

			Message msg2(Message::ELECTION, getNextSequenceNumber(), m_me->getId());
			for (it = m_listMembers->begin(); it != m_listMembers->end(); it++) {
				if (m_me->getId() < it->first) {
					sendMessage(msg2, it->first.m_ipAddress, it->first.m_port);
				}
			}
			//Manipulate timers
			m_timer->suspend(HELLO_TIMER_ID);
			m_timer->schedule(ELECTION_TIMER_ID, 3);
		} else {
			m_state.setStateType(State::BULLY);
			m_timer->suspend(HELLO_TIMER_ID);
			m_timer->schedule(BULLY_TIMER_ID, 3);
		}
		break;
	}
	case State::CHAT: {
		if (m_me->getId() > message.getOriginator()) {
			//Change local variables
			m_state.setStateType(State::ELECTION);
			//Send message
			Message msg1(Message::BULLY, message.getSequenceNumber(), m_me->getId());
			sendMessage(msg1, message.getOriginator().m_ipAddress, message.getOriginator().m_port);

			Message msg2(Message::ELECTION, getNextSequenceNumber(), m_me->getId());
			for (it = m_listMembers->begin(); it != m_listMembers->end(); it++) {
				if (m_me->getId() < it->first) {
					sendMessage(msg2, it->first.m_ipAddress, it->first.m_port);
				}
			}
			//Manipulate timers
			m_timer->suspend(HELLO_TIMER_ID);
			m_timer->suspend(CHAT_TIMER_ID);
			m_timer->schedule(ELECTION_TIMER_ID, 3);
		} else {
			m_state.setStateType(State::BULLY);
			m_timer->suspend(HELLO_TIMER_ID);
			m_timer->suspend(CHAT_TIMER_ID);
			m_timer->schedule(BULLY_TIMER_ID, 3);
		}
		break;
	}
	case State::ELECTION: {
		if (m_me->getId() > message.getOriginator()) {
			//Send message
			Message msg1(Message::BULLY, message.getSequenceNumber(), m_me->getId());
			sendMessage(msg1, message.getOriginator().m_ipAddress, message.getOriginator().m_port);
		} else {
			m_state.setStateType(State::BULLY);
			m_timer->suspend(ELECTION_TIMER_ID);
			m_timer->schedule(BULLY_TIMER_ID, 3);
		}
		break;
	}
	case State::BULLY: {
		if (m_me->getId() > message.getOriginator()) {
			//Send message
			Message msg1(Message::BULLY, message.getSequenceNumber(), m_me->getId());
			sendMessage(msg1, message.getOriginator().m_ipAddress, message.getOriginator().m_port);
		} else {
			m_state.setStateType(State::BULLY);
			m_timer->suspend(BULLY_TIMER_ID);
			m_timer->schedule(BULLY_TIMER_ID, 3);
		}
		break;
	}
 	default:
		break;
	}
	pthread_mutex_unlock(&m_mutexEvent);
}

void Model::handleBullyMsg(Message &message) {
	pthread_mutex_lock(&m_mutexEvent);
	map<MemberID, Member *>::iterator it = m_listMembers->find(message.getOriginator());
	if (it == m_listMembers->end()) {
		pthread_mutex_unlock(&m_mutexEvent);
		return;
	}
	switch (m_state.getStateType()) {
	case State::ELECTION: {
		m_state.setStateType(State::BULLY);
		m_timer->suspend(ELECTION_TIMER_ID);
		m_timer->schedule(BULLY_TIMER_ID, 3);
		break;
	}
	case State::LEADER: {
		m_timer->suspend(LEADER_TIMER_ID);
		m_state.setStateType(State::BULLY);
		m_timer->schedule(BULLY_TIMER_ID, 3);
		break;
	}
	case State::MULTICAST: {
		m_timer->suspend(MULTICAST_TIMER_ID);
		m_state.setStateType(State::BULLY);
		m_timer->schedule(BULLY_TIMER_ID, 3);
		break;
	}
	default:
		break;
	}
	pthread_mutex_unlock(&m_mutexEvent);
}

void Model::handleLeaderMsg(Message &message) {
	pthread_mutex_lock(&m_mutexEvent);
	map<MemberID, Member *>::iterator it = m_listMembers->find(message.getOriginator());
	if (it == m_listMembers->end()) {
		pthread_mutex_unlock(&m_mutexEvent);
		return;
	}
	switch (m_state.getStateType()) {
	case State::LEADER: {
		if (m_leader < message.getOriginator()) {
			//Modify local variables
			m_state.setStateType(State::NORMAL);
			map<MemberID, Member *>::iterator it;
			for (it = m_listMembers->begin(); it != m_listMembers->end(); it++) {
				delete it->second;
			}
			delete m_listMembers;
			m_leader = message.getOriginator();
			pub_leader = message.getPublicKey();
			m_listMembers = message.getMemberList();
			m_leaderTimestamp = Model::now();
			m_leaderLastSequenceNumber = 0;
			Member *leader = m_listMembers->find(m_leader)->second;
			cout << "NOTICE " << leader->getName() << " becomes the new leader" << endl;
			//Manipulate timers
			m_timer->suspend(LEADER_TIMER_ID);
			//m_timer->schedule(HELLO_TIMER_ID, 1);
			login();
		} else if (m_leader > message.getOriginator()) {
			Message msg1(Message::BULLY, message.getSequenceNumber(), m_me->getId());
			sendMessage(msg1, message.getOriginator().m_ipAddress, message.getOriginator().m_port);
		}
		break;
	}
	case State::MULTICAST: {
		if (m_leader < message.getOriginator()) {
			//Modify local variables
			m_state.setStateType(State::NORMAL);
			map<MemberID, Member *>::iterator it;
			for (it = m_listMembers->begin(); it != m_listMembers->end(); it++) {
				delete it->second;
			}
			delete m_listMembers;
			m_leader = message.getOriginator();
			pub_leader = message.getPublicKey();
			m_listMembers = message.getMemberList();
			m_leaderTimestamp = Model::now();
			m_leaderLastSequenceNumber = 0;
			Member *leader = m_listMembers->find(m_leader)->second;
			cout << "NOTICE " << leader->getName() << " becomes the new leader" << endl;
			pthread_cond_broadcast(&m_condState);
			//Manipulate timers
			m_timer->suspend(LEADER_TIMER_ID);
			m_timer->suspend(MULTICAST_TIMER_ID);
			//m_timer->schedule(HELLO_TIMER_ID, 1);
			login();
		} else if (m_leader > message.getOriginator()) {
			Message msg1(Message::BULLY, message.getSequenceNumber(), m_me->getId());
			sendMessage(msg1, message.getOriginator().m_ipAddress, message.getOriginator().m_port);
		}
		break;
	}
	case State::NORMAL: {
		if (m_leader < message.getOriginator()) {
			//Modify local variables
			m_state.setStateType(State::NORMAL);
			map<MemberID, Member *>::iterator it;
			for (it = m_listMembers->begin(); it != m_listMembers->end(); it++) {
				delete it->second;
			}
			delete m_listMembers;
			m_leader = message.getOriginator();
			pub_leader = message.getPublicKey();
			m_listMembers = message.getMemberList();
			m_leaderTimestamp = Model::now();
			m_leaderLastSequenceNumber = 0;
			Member *leader = m_listMembers->find(m_leader)->second;
			cout << "NOTICE " << leader->getName() << " becomes the new leader" << endl;
			//Manipulate timers
			//m_timer->schedule(HELLO_TIMER_ID, 1);
			login();
		} else if (m_leader > message.getOriginator()) {
			Message msg1(Message::BULLY, message.getSequenceNumber(), m_me->getId());
			sendMessage(msg1, message.getOriginator().m_ipAddress, message.getOriginator().m_port);
		} else {
			map<MemberID, Member *> *newList = message.getMemberList();
			map<MemberID, Member *>::iterator it;
			for (it = newList->begin(); it != newList->end(); it++) {
				if (m_listMembers->find(it->first) == m_listMembers->end()) {
					char address[100];
					inet_ntop(AF_INET, (void *) &it->second->getId().m_ipAddress, address, INET_ADDRSTRLEN);
					cout << "NOTICE " << it->second->getName() << " joined on " << address << ":" << ntohs(it->second->getId().m_port) << endl;
				}
			}
			for (it = m_listMembers->begin(); it != m_listMembers->end(); it++) {
				if (newList->find(it->first) == newList->end()) {
					cout << "NOTICE " << it->second->getName() << " left the chat or crashed" << endl;
				}
				delete it->second;
			}
			delete m_listMembers;
			m_listMembers = message.getMemberList();
			pub_leader = message.getPublicKey();
			m_leaderTimestamp = Model::now();
		}
		break;
	}
	case State::CHAT: {
		if (m_leader < message.getOriginator()) {
			//Modify local variables
			m_state.setStateType(State::NORMAL);
			map<MemberID, Member *>::iterator it;
			for (it = m_listMembers->begin(); it != m_listMembers->end(); it++) {
				delete it->second;
			}
			delete m_listMembers;
			m_leader = message.getOriginator();
			m_listMembers = message.getMemberList();
			pub_leader = message.getPublicKey();
			m_leaderTimestamp = Model::now();
			m_leaderLastSequenceNumber = 0;
			Member *leader = m_listMembers->find(m_leader)->second;
			cout << "NOTICE " << leader->getName() << " becomes the new leader" << endl;
			pthread_cond_broadcast(&m_condState);
			//Manipulate timers
			m_timer->suspend(CHAT_TIMER_ID);
			//m_timer->schedule(HELLO_TIMER_ID, 1);
			login();
		} else if (m_leader > message.getOriginator()) {
			Message msg1(Message::BULLY, message.getSequenceNumber(), m_me->getId());
			sendMessage(msg1, message.getOriginator().m_ipAddress, message.getOriginator().m_port);
		} else {
			map<MemberID, Member *> *newList = message.getMemberList();
			map<MemberID, Member *>::iterator it;
			for (it = newList->begin(); it != newList->end(); it++) {
				if (m_listMembers->find(it->first) == m_listMembers->end()) {
					char address[100];
					inet_ntop(AF_INET, (void *) &it->second->getId().m_ipAddress, address, INET_ADDRSTRLEN);
					cout << "NOTICE " << message.getName() << " joined on " << address << ":" << ntohs(it->second->getId().m_port) << endl;
				}
			}
			for (it = m_listMembers->begin(); it != m_listMembers->end(); it++) {
				if (newList->find(it->first) == newList->end()) {
					cout << "NOTICE " << it->second->getName() << " left the chat or crashed" << endl;
				}
				delete it->second;
			}
			delete m_listMembers;
			m_listMembers = message.getMemberList();
			pub_leader = message.getPublicKey();
			m_leaderTimestamp = Model::now();
		}
		break;
	}
	case State::ELECTION: {
		if (m_me->getId() < message.getOriginator()) {
			//Modify local variables
			m_state.setStateType(State::NORMAL);
			map<MemberID, Member *>::iterator it;
			for (it = m_listMembers->begin(); it != m_listMembers->end(); it++) {
				delete it->second;
			}
			delete m_listMembers;
			m_leader = message.getOriginator();
			pub_leader = message.getPublicKey();
			m_listMembers = message.getMemberList();
			m_leaderTimestamp = Model::now();
			m_leaderLastSequenceNumber = 0;
			Member *leader = m_listMembers->find(m_leader)->second;
			cout << "NOTICE " << leader->getName() << " becomes the new leader" << endl;
			pthread_cond_broadcast(&m_condState);
			//Manipulate timers
			m_timer->suspend(ELECTION_TIMER_ID);
			//m_timer->schedule(HELLO_TIMER_ID, 1);
			login();
		} else if (m_me->getId() > message.getOriginator()) {
			Message msg1(Message::BULLY, message.getSequenceNumber(), m_me->getId());
			sendMessage(msg1, message.getOriginator().m_ipAddress, message.getOriginator().m_port);
		}
		break;
	}
	case State::BULLY: {
		if (m_me->getId() < message.getOriginator()) {
			//Modify local variables
			m_state.setStateType(State::NORMAL);
			map<MemberID, Member *>::iterator it;
			for (it = m_listMembers->begin(); it != m_listMembers->end(); it++) {
				delete it->second;
			}
			delete m_listMembers;
			m_leader = message.getOriginator();
			pub_leader = message.getPublicKey();
			m_listMembers = message.getMemberList();
			m_leaderTimestamp = Model::now();
			m_leaderLastSequenceNumber = 0;
			Member *leader = m_listMembers->find(m_leader)->second;
			cout << "NOTICE " << leader->getName() << " becomes the new leader" << endl;
			pthread_cond_broadcast(&m_condState);
			//Manipulate timers
			m_timer->suspend(BULLY_TIMER_ID);
			//m_timer->schedule(HELLO_TIMER_ID, 1);
			login();
		} else if (m_me->getId() > message.getOriginator()) {
			Message msg1(Message::BULLY, message.getSequenceNumber(), m_me->getId());
			sendMessage(msg1, message.getOriginator().m_ipAddress, message.getOriginator().m_port);
		}
		break;
	}
	default:
		break;
	}
	pthread_mutex_unlock(&m_mutexEvent);
}

void Model::handleChatMsg(Message &message) {
	pthread_mutex_lock(&m_mutexEvent);
	if (m_state.getStateType() == State::LEADER || m_state.getStateType() == State::MULTICAST) {
		map<MemberID, Member *>::iterator it = m_listMembers->find(message.getOriginator());
		if (it != m_listMembers->end()) {
			Member *member = it->second;
			if (message.getSequenceNumber() > member->getLastSequenceNumber()) {
				string digest = message.getDigest();
				string password = crypto_box_open(digest, nonce, member->getPublicKey(), priv_key);
				if ((uint32_t)atoi(password.c_str()) != member->getPassword()) {
					pthread_mutex_unlock(&m_mutexEvent);
					return;
				}
				string contentDigest = message.getContentDigest();
				string content = crypto_box_open(contentDigest, nonce, member->getPublicKey(), priv_key);
				Message msg(Message::MULTICAST, getNextSequenceNumber(), m_me->getId());
				msg.setSender(message.getOriginator());
				msg.setChatContent(content);
				m_messageQueue.push_back(msg);
				member->setLastSequenceNumber(message.getSequenceNumber());
				pthread_cond_broadcast(&m_condQueue);
			}
			Message msg(Message::ACK, message.getSequenceNumber(), m_me->getId());
			sendMessage(msg, message.getOriginator().m_ipAddress, message.getOriginator().m_port);
		}
	}
	pthread_mutex_unlock(&m_mutexEvent);
}

void Model::handleMulticastMsg(Message &message) {
	pthread_mutex_lock(&m_mutexEvent);
	string name = "Unknown Client";
	switch (m_state.getStateType()) {
	case State::NORMAL:
	case State::CHAT:
	case State::LEADER:
	case State::MULTICAST: {
		if (m_leader == message.getOriginator()) {
			string digest = message.getContentDigest();
			string content;
			try{
			content = crypto_box_open(digest, nonce, pub_leader, priv_key);
			}catch(exception& e) {
				cout << e.what() << endl;
			}
			if (message.getSequenceNumber() > m_leaderLastSequenceNumber) {
				map<MemberID, Member *>::iterator itr;
				itr = m_listMembers->find(message.getSender());
				if (itr != m_listMembers->end()) {
					name = itr->second->getName();
				}
				cout << name + ":" + content << endl;
				m_leaderLastSequenceNumber = message.getSequenceNumber();
			}
			Message msg1(Message::ACK, message.getSequenceNumber(), m_me->getId());
			sendMessage(msg1, message.getOriginator().m_ipAddress, message.getOriginator().m_port);
		}
		break;
	}
	default:
		break;
	}
	pthread_mutex_unlock(&m_mutexEvent);
}

void Model::handleAckMsg(Message &message) {
	pthread_mutex_lock(&m_mutexEvent);
	if (m_state.getStateType() == State::CHAT) {
		MessageBlock *block = m_state.getMessageStatus();
		if (block->message.getSequenceNumber() != message.getSequenceNumber()) {
			return;
		}
		m_state.setStateType(State::NORMAL);
		pthread_cond_broadcast(&m_condState);
	} else if (m_state.getStateType() == State::MULTICAST) {
		MessageBlock *block = m_state.getMessageStatus();
		map<MemberID, bool> *list = m_state.getAckStatus();
		map<MemberID, bool>::iterator itr;
		itr = list->find(message.getOriginator());
		if (itr == list->end()) {
			pthread_mutex_unlock(&m_mutexEvent);
			return;
		}
		if (message.getSequenceNumber() != block->message.getSequenceNumber()) {
			pthread_mutex_unlock(&m_mutexEvent);
			return;
		}
		list->erase(itr);
		if (list->size() == 0) {
			m_state.setStateType(State::LEADER);
			pthread_cond_broadcast(&m_condState);
		}
	}
	pthread_mutex_unlock(&m_mutexEvent);
}

void Model::handleConnectTimeout() {
	pthread_mutex_lock(&m_mutexEvent);
	if (m_state.getStateType() != State::CONNECT) {
		m_timer->suspend(CONNECT_TIMER_ID);
		pthread_mutex_unlock(&m_mutexEvent);
		return;
	}
	MessageBlock *block = m_state.getConnectStatus();
	if (block->count == 0) {
		char address[100];
		inet_ntop(AF_INET, (void *) &block->destAddress, address, INET_ADDRSTRLEN);
		cout << "Sorry, no chat is active on " << address << ":" << ntohs(block->destPort);
		cout << " try again later." << endl;
		cout << "Bye." << endl;
		exit(0);
	} else {
		block->count--;
		sendMessage(block->message, block->destAddress, block->destPort);
		m_timer->schedule(CONNECT_TIMER_ID, 1);
	}
	pthread_mutex_unlock(&m_mutexEvent);
}

void Model::handleLoginTimeout() {
	pthread_mutex_lock(&m_mutexEvent);
	if (m_state.getStateType() != State::LOGIN) {
		m_timer->suspend(LOGIN_TIMER_ID);
		pthread_mutex_unlock(&m_mutexEvent);
		return;
	}
	MessageBlock *block = m_state.getLoginStatus();
	if (block->count == 0) {
		map<MemberID, Member *>::iterator it = m_listMembers->begin();
		uint32_t ipAddress = it->first.m_ipAddress;
		uint16_t port = it->first.m_port;
		uint32_t sequenceNumber = getNextSequenceNumber();
		Message message(Message::CONNECT, sequenceNumber, m_me->getId());
		sendMessage(message, ipAddress, port);
		m_state.setStateType(State::CONNECT);
		m_timer->schedule(CONNECT_TIMER_ID, 1);
		m_state.setSequenceNumber(sequenceNumber);
		MessageBlock *mb = m_state.getConnectStatus();
		mb->message = message;
		mb->destAddress = ipAddress;
		mb->destPort = port;
		mb->timestamp = Model::now();
	} else {
		block->count--;
		sendMessage(block->message, block->destAddress, block->destPort);
		m_timer->schedule(LOGIN_TIMER_ID, 1);
	}
	pthread_mutex_unlock(&m_mutexEvent);
}

void Model::handleElectionTimeout() {
	pthread_mutex_lock(&m_mutexEvent);
	if (m_state.getStateType() != State::ELECTION) {
		m_timer->suspend(ELECTION_TIMER_ID);
		pthread_mutex_unlock(&m_mutexEvent);
		return;
	}
	//Modify local varibles
	m_state.setStateType(State::LEADER);
	m_leader = m_me->getId();
	pub_leader = pub_key;
	//Send messages
	Message message(Message::LEADER, getNextSequenceNumber(), m_leader);
	message.setMemberList(m_listMembers);
	message.setPublicKey(pub_key);
	map<MemberID, Member *>::iterator itr;
	for (itr = m_listMembers->begin(); itr != m_listMembers->end(); itr++) {
		if (m_me->getId() != itr->first) {
			sendMessage(message, itr->first.m_ipAddress, itr->first.m_port);
			itr->second->setTimestamp(Model::now());
			itr->second->setLastSequenceNumber(0);
		}
	}
	cout << "Now I become the leader. Haha" << endl;
	pthread_cond_broadcast(&m_condState);
	//Manipulate timer
	m_timer->schedule(LEADER_TIMER_ID, 1);
	pthread_mutex_unlock(&m_mutexEvent);
}

void Model::handleBullyTimeout() {
	pthread_mutex_lock(&m_mutexEvent);
	if (m_state.getStateType() != State::BULLY) {
		m_timer->suspend(BULLY_TIMER_ID);
		pthread_mutex_unlock(&m_mutexEvent);
		return;
	}
	m_state.setStateType(State::ELECTION);
	map<MemberID, Member *>::iterator itr;
	for (itr = m_listMembers->begin(); itr != m_listMembers->end(); itr++) {
		if (m_me->getId() < itr->first) {
			Message msg(Message::ELECTION, getNextSequenceNumber(), m_me->getId());
			sendMessage(msg, itr->first.m_ipAddress, itr->first.m_port);
		}
	}
	m_timer->schedule(ELECTION_TIMER_ID, 3);
	pthread_mutex_unlock(&m_mutexEvent);
}

void Model::handleHelloTimeout() {
	pthread_mutex_lock(&m_mutexEvent);
	if (m_state.getStateType() != State::NORMAL && m_state.getStateType() != State::CHAT) {
		m_timer->suspend(HELLO_TIMER_ID);
		pthread_mutex_unlock(&m_mutexEvent);
		return;
	}
	if (m_leaderTimestamp + MEMBER_TIMEOUT <= Model::now()) {
		m_state.setStateType(State::ELECTION);
		m_listMembers->erase(m_leader);
		map<MemberID, Member *>::iterator itr;
		for (itr = m_listMembers->begin(); itr != m_listMembers->end(); itr++) {
			if (m_me->getId() < itr->first) {
				Message msg(Message::ELECTION, getNextSequenceNumber(), m_me->getId());
				sendMessage(msg, itr->first.m_ipAddress, itr->first.m_port);
			}
		}
		m_timer->suspend(CHAT_TIMER_ID);
		m_timer->schedule(ELECTION_TIMER_ID, 1);
	} else {
		Message msg(Message::HELLO, getNextSequenceNumber(), m_me->getId());
		msg.setPublicKey(pub_key);
		ostringstream os;
		os << password;
		string digest = crypto_box(os.str(), nonce, pub_leader, priv_key);
		msg.setDigest(digest);
		sendMessage(msg, m_leader.m_ipAddress, m_leader.m_port);
		m_timer->schedule(HELLO_TIMER_ID, 1);
	}
	pthread_mutex_unlock(&m_mutexEvent);
}

void Model::handleLeaderTimeout() {
	pthread_mutex_lock(&m_mutexEvent);
	if (m_state.getStateType() != State::LEADER && m_state.getStateType() != State::MULTICAST) {
		m_timer->suspend(LEADER_TIMER_ID);
		pthread_mutex_unlock(&m_mutexEvent);
		return;
	}
	map<MemberID, Member *>::iterator itr;
	Message msg(Message::LEADER, getNextSequenceNumber(), m_me->getId());
	for (itr = m_listMembers->begin(); itr != m_listMembers->end();) {
		if (m_me->getId() == itr->first) {
			++itr;
			continue;
		}
		if (itr->second->getTimestamp() + MEMBER_TIMEOUT <= Model::now()) {
			if (m_state.getStateType() == State::MULTICAST) {
				map<MemberID, bool> *list = m_state.getAckStatus();
				list->erase(itr->first);
			}
			cout << "NOTICE " << itr->second->getName() << " left the chat or crashed" << endl;
			m_listMembers->erase(itr++);
		} else {
			++itr;
		}
	}
	msg.setMemberList(m_listMembers);
	msg.setPublicKey(pub_key);
	for (itr = m_listMembers->begin(); itr != m_listMembers->end(); itr++) {
		if (m_me->getId() == itr->first) {
			continue;
		}
		sendMessage(msg, itr->first.m_ipAddress, itr->first.m_port);
	}
	m_timer->schedule(LEADER_TIMER_ID, 1);
	pthread_mutex_unlock(&m_mutexEvent);
}

void Model::handleChatTimeout() {
	pthread_mutex_lock(&m_mutexEvent);
	if (m_state.getStateType() != State::CHAT) {
		m_timer->suspend(CHAT_TIMER_ID);
		pthread_mutex_unlock(&m_mutexEvent);
		return;
	}
	MessageBlock *block = m_state.getMessageStatus();
	if (block->count == 0) {
		m_state.setStateType(State::ELECTION);
		map<MemberID, Member *>::iterator itr;
		for (itr = m_listMembers->begin(); itr != m_listMembers->end(); itr++) {
			if (m_me->getId() < itr->first) {
				Message msg(Message::ELECTION, getNextSequenceNumber(), m_me->getId());
				sendMessage(msg, itr->first.m_ipAddress, itr->first.m_port);
			}
		}
		m_timer->schedule(ELECTION_TIMER_ID, 1);
	} else {
		block->count--;
		ostringstream os;
		os << password;
		string digest = crypto_box(os.str(), nonce, pub_leader, priv_key);
		string contentDigest = crypto_box(block->message.getChatContent(), nonce, pub_leader, priv_key);
		block->message.setDigest(digest);
		block->message.setContentDigest(contentDigest);
		sendMessage(block->message, m_leader.m_ipAddress, m_leader.m_port);
		m_timer->schedule(CHAT_TIMER_ID, 1);
	}
	pthread_mutex_unlock(&m_mutexEvent);
}

void Model::handleMulticastTimeout() {
	pthread_mutex_lock(&m_mutexEvent);
	if (m_state.getStateType() != State::MULTICAST) {
		pthread_mutex_unlock(&m_mutexEvent);
		return;
	}
	MessageBlock *block = m_state.getMessageStatus();
	map<MemberID, bool> *list = m_state.getAckStatus();
	if (block->count == 0) {
		map<MemberID, bool>::iterator itr;
		for (itr = list->begin(); itr != list->end(); itr++) {
			m_listMembers->erase(itr->first);
		}
		m_state.setStateType(State::LEADER);
		pthread_cond_broadcast(&m_condState);
	} else {
		block->count--;
		map<MemberID, bool>::iterator itr;
		for (itr = list->begin(); itr != list->end(); itr++) {
			map<MemberID, Member *>::iterator it = m_listMembers->find(itr->first);
			if (it == m_listMembers->end()) {
				continue;
			}
			string contentDigest = crypto_box(block->message.getChatContent(), nonce, it->second->getPublicKey(), priv_key);
			block->message.setContentDigest(contentDigest);
			sendMessage(block->message, itr->first.m_ipAddress, itr->first.m_port);
		}
		m_timer->schedule(MULTICAST_TIMER_ID, 1);
	}
	pthread_mutex_unlock(&m_mutexEvent);
}

void Model::doListenMessage() {
	while (true) {
		Message message = recvMessage();
		switch (message.getMessageType()) {
		case Message::CONNECT:
			handleConnectMsg(message);
			break;
		case Message::CONNECT_OK:
			handleConnectOkMsg(message);
			break;
		case Message::HELLO:
			handleHelloMsg(message);
			break;
		case Message::LOGIN:
			handleLoginMsg(message);
			break;
		case Message::LOGIN_OK:
			handleLoginOkMsg(message);
			break;
		case Message::ELECTION:
			handleElectionMsg(message);
			break;
		case Message::BULLY:
			handleBullyMsg(message);
			break;
		case Message::LEADER:
			handleLeaderMsg(message);
			break;
		case Message::CHAT:
			handleChatMsg(message);
			break;
		case Message::MULTICAST:
			handleMulticastMsg(message);
			break;
		case Message::ACK:
			handleAckMsg(message);
			break;
		}
	}
}

void Model::doSend() {
	while (true) {
		pthread_mutex_lock(&m_mutexEvent);
		while (m_messageQueue.empty()) {
			pthread_cond_wait(&m_condQueue, &m_mutexEvent);
		}
		while (m_state.getStateType() != State::NORMAL && m_state.getStateType() != State::LEADER) {
			pthread_cond_wait(&m_condState, &m_mutexEvent);
		}
		Message msg = m_messageQueue.front();
		m_messageQueue.pop_front();
		if (m_state.getStateType() == State::NORMAL) {
			msg.setMessageType(Message::CHAT);
			ostringstream os;
			os << password;
			string digest = crypto_box(os.str(), nonce, pub_leader, priv_key);
			string contentDigest = crypto_box(msg.getChatContent(), nonce, pub_leader, priv_key);
			msg.setDigest(digest);
			msg.setContentDigest(contentDigest);
			sendMessage(msg, m_leader.m_ipAddress, m_leader.m_port);
			MessageBlock *block = m_state.getMessageStatus();
			block->message = msg;
			block->count = 2;
			m_state.setStateType(State::CHAT);
			m_timer->schedule(CHAT_TIMER_ID, 1);
		} else if (m_state.getStateType() == State::LEADER) {
			if (msg.getMessageType() == Message::CHAT) {
				msg.setMessageType(Message::MULTICAST);
				msg.setSender(m_me->getId());
			}
			map<MemberID, bool> *copy = m_state.getAckStatus();
			copy->clear();
			map<MemberID, Member *>::iterator itr;
			for (itr = m_listMembers->begin(); itr != m_listMembers->end(); itr++) {
				if (itr->first == m_me->getId()) {
					cout << m_me->getName() + ":" + msg.getChatContent() << endl;
					continue;
				}
				copy->insert(make_pair(itr->first, false));
				string contentDigest;
				try{
				contentDigest= crypto_box(msg.getChatContent(), nonce, itr->second->getPublicKey(), priv_key);
				}catch(exception& e) {
					cout << e.what() << endl;
				}
				msg.setContentDigest(contentDigest);
				sendMessage(msg, itr->first.m_ipAddress, itr->first.m_port);
			}
			if (copy->size() == 0) {
				pthread_mutex_unlock(&m_mutexEvent);
				continue;
			}
			MessageBlock *block = m_state.getMessageStatus();
			block->message = msg;
			block->count = 2;
			m_state.setStateType(State::MULTICAST);
			m_timer->schedule(MULTICAST_TIMER_ID, 1);
		}
		pthread_mutex_unlock(&m_mutexEvent);
	}
}

void Model::chat(string content) {
	pthread_mutex_lock(&m_mutexEvent);
	uint32_t sequenceNumber = getNextSequenceNumber();
	Message message;
	if (m_state.getStateType() == State::LEADER || m_state.getStateType() == State::MULTICAST) {
		message = Message(Message::MULTICAST, sequenceNumber, m_me->getId());
		message.setSender(m_me->getId());
	} else {
		message = Message(Message::CHAT, sequenceNumber, m_me->getId());
	}
	message.setChatContent(content);
	m_messageQueue.push_back(message);
	pthread_cond_broadcast(&m_condQueue);
	pthread_mutex_unlock(&m_mutexEvent);
}

void Model::login() {
	//Send message
	Message msg1(Message::LOGIN, getNextSequenceNumber(), m_me->getId());
	msg1.setName(m_me->getName());
	msg1.setPublicKey(pub_key);
	ostringstream os;
	os << password;
	string digest = crypto_box(os.str(), nonce, pub_leader, priv_key);
	msg1.setDigest(digest);
	sendMessage(msg1, m_leader.m_ipAddress, m_leader.m_port);
	MessageBlock *block = m_state.getLoginStatus();
	block->message = msg1;
	block->count = 2;
	block->timestamp = Model::now();

	//Manipulate timers
	m_state.setStateType(State::LOGIN);
	m_timer->schedule(LOGIN_TIMER_ID, 1);
}

int64_t Model::now() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (int64_t) (tv.tv_sec * 1000000) + tv.tv_usec;
}
