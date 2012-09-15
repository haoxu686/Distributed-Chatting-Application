#include "Member.h"

Member::Member(uint32_t ipAddress, uint16_t port, string name) {
    m_id.m_ipAddress = ipAddress;
    m_id.m_port = port;
    this->m_name = name;
}

MemberID Member::getId() {
    return m_id;
}

string Member::getName() {
    return m_name;
}

void Member::setLastSequenceNumber(uint32_t number) {
	m_lastSequenceNumber = number;
}

uint32_t Member::getLastSequenceNumber() {
	return m_lastSequenceNumber;
}

int64_t Member::getTimestamp() {
	return m_timestamp;
}

void Member::setTimestamp(int64_t value) {
	m_timestamp = value;
}

void Member::setPassword(uint32_t password) {
	m_password = password;
}

uint32_t Member::getPassword() {
	return m_password;
}

void Member::setPublicKey(string key) {
	m_publicKey = key;
}

string Member::getPublicKey() {
	return m_publicKey;
}

bool MemberID::operator<(const MemberID &id) const {
    if (m_ipAddress < id.m_ipAddress) {
        return true;
    } else if (m_ipAddress > id.m_ipAddress) {
        return false;
    } else {
        return m_port < id.m_port;
    }
}

bool MemberID::operator<=(const MemberID &id) const {
	return (*this) < id || (*this) == id;
}

bool MemberID::operator >(const MemberID &id) const {
	if (m_ipAddress > id.m_ipAddress) {
		return true;
	} else if (m_ipAddress < id.m_ipAddress) {
		return false;
	} else {
		return m_port > id.m_port;
	}
}

bool MemberID::operator >=(const MemberID &id) const {
	return (*this) > id || (*this) == id;
}

bool MemberID::operator ==(const MemberID &id) const {
	return (m_ipAddress == id.m_ipAddress) && (m_port == id.m_port);
}

bool MemberID::operator !=(const MemberID &id)  const {
	if (m_ipAddress != id.m_ipAddress) {
		return true;
	} else {
		return m_port != id.m_port;
	}
}
