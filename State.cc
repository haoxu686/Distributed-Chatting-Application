#include "State.h"
#include <iostream>
using namespace std;

State::State() {
	m_ackStatus = new map<MemberID, bool>();
	m_messageTracker = new MessageBlock();
	m_connectTracker = new MessageBlock();
	m_loginTracker = new MessageBlock();
}

State::~State() {
	delete m_ackStatus;
	delete m_messageTracker;
	delete m_connectTracker;
	delete m_loginTracker;
}

void State::setStateType(StateType value) {
	m_type = value;
}

State::StateType State::getStateType() {
	return m_type;
}

void State::setSequenceNumber(uint32_t value) {
	m_sequenceNumber = value;
}

uint32_t State::getSequenceNumber() {
	return m_sequenceNumber;
}

map<MemberID, bool> * State::getAckStatus() {
	return m_ackStatus;
}

MessageBlock * State::getConnectStatus() {
	return m_connectTracker;
}

MessageBlock * State::getLoginStatus() {
	return m_loginTracker;
}

MessageBlock * State::getMessageStatus() {
	return m_messageTracker;
}
