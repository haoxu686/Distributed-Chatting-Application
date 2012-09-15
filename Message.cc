#include "Message.h"

OutputBuffer::OutputBuffer(uint32_t size) {
    m_buffer = new char[size];
    m_current = m_buffer;
}

OutputBuffer::~OutputBuffer() {
    delete m_buffer;
}

void OutputBuffer::writeU8(uint8_t value) {
    *m_current = (char) value;
    m_current++;
}

void OutputBuffer::writeU16(uint16_t value) {
	char *p = (char *)&value;
	*m_current = *p;
	m_current++;
	p++;
	*m_current = *p;
	m_current++;
    p++;
}

char * OutputBuffer::toByteArray() {
	return m_buffer;
}

void OutputBuffer::writeU32(uint32_t value) {
    char *p = (char *)&value;
    *m_current = *p;
    m_current++;
    p++;
    *m_current = *p;
    m_current++;
    p++;
    *m_current = *p;
    m_current++;
    p++;
    *m_current = *p;
    m_current++;
    p++;
}

void OutputBuffer::write(const char *value, uint32_t length) {
    const char *p = value;
    for (uint32_t i = 0; i < length; i++) {
        *m_current = *p;
        m_current++;
        p++;
    }
}

InputBuffer::InputBuffer(char *buffer) {
    m_buffer = buffer;
    m_current = m_buffer;
}

InputBuffer::~InputBuffer() {
    delete m_buffer;
}

uint8_t InputBuffer::readU8() {
    uint8_t result = *m_current;
    m_current++;
    return result;
}

uint16_t InputBuffer::readU16() {
	uint16_t result;
	char *p = (char *)&result;
	*p = *m_current;
	m_current++;
	p++;
	*p = *m_current;
	m_current++;
	p++;
	return result;
}

uint32_t InputBuffer::readU32() {
    uint32_t result;
    char *p = (char *)&result;
    *p = *m_current;
    m_current++;
    p++;
    *p = *m_current;
    m_current++;
    p++;
    *p = *m_current;
    m_current++;
    p++;
    *p = *m_current;
    m_current++;
    p++;
    return result;
}

char * InputBuffer::read(uint32_t length) {
    char *result = new char[length];
    char *p = result;
    for (uint32_t i = 0; i < length; i++) {
        *p = *m_current;
        p++;
        m_current++;
    }
    return result;
}

Message::Message() {

}

Message::Message(InputBuffer *buffer) {
    m_type = (MessageType) buffer->readU8();
    m_sequenceNumber = buffer->readU32();
    m_originator.m_ipAddress = buffer->readU32();
    m_originator.m_port = buffer->readU16();
	switch (m_type) {
	case CONNECT:
		break;
	case CONNECT_OK: {
		m_leader.m_ipAddress = buffer->readU32();
		m_leader.m_port = buffer->readU16();
		m_memberList.deserialize(buffer);
		uint32_t length = buffer->readU32();
		char *tmp = buffer->read(length);
		m_publicKey = string(tmp, length);
		delete tmp;
		break;
	}
	case LOGIN: {
		uint32_t length = buffer->readU32();
		char *tmp = buffer->read(length);
		m_name = string(tmp, length);
		delete tmp;
		length = buffer->readU32();
		tmp = buffer->read(length);
		m_publicKey = string(tmp, length);
		delete tmp;
		length = buffer->readU32();
		tmp = buffer->read(length);
		m_digest = string(tmp, length);
		delete tmp;
		break;
	}
	case LOGIN_OK: {
		m_memberList.deserialize(buffer);
		uint32_t length = buffer->readU32();
		char *tmp = buffer->read(length);
		m_publicKey = string(tmp, length);
		delete tmp;
		break;
	}
	case HELLO: {
		uint32_t length = buffer->readU32();
		char *tmp = buffer->read(length);
		m_publicKey = string(tmp, length);
		delete tmp;
		length = buffer->readU32();
		tmp = buffer->read(length);
		m_digest = string(tmp, length);
		delete tmp;
		break;
	}
	case LEADER: {
		m_memberList.deserialize(buffer);
		uint32_t length = buffer->readU32();
		char *tmp = buffer->read(length);
		m_publicKey = string(tmp, length);
		delete tmp;
		break;
	}
	case ELECTION:
		break;
	case BULLY:
		break;
	case CHAT: {
		uint32_t length;
		length = buffer->readU32();
		char *tmp;
		tmp = buffer->read(length);
		m_digest = string(tmp, length);
		delete tmp;
		length = buffer->readU32();
		tmp = buffer->read(length);
		m_contentDigest = string(tmp, length);
		delete tmp;
		break;
	}
	case MULTICAST: {
		m_sender.m_ipAddress = buffer->readU32();
		m_sender.m_port = buffer->readU16();
		uint32_t length;
		length = buffer->readU32();
		char *tmp;
		tmp = buffer->read(length);
		m_contentDigest = string(tmp, length);
		delete tmp;
		break;
	}
	case ACK:
		break;
	}
}

Message::Message(Message::MessageType type, uint32_t sequenceNumber, MemberID originator) {
    m_type = type;
    m_sequenceNumber = sequenceNumber;
    m_originator = originator;
}

void Message::setMessageType(MessageType type) {
	m_type = type;
}

Message::MessageType Message::getMessageType() {
    return m_type;
}

uint32_t Message::getSequenceNumber() {
	return m_sequenceNumber;
}

uint32_t Message::getSerializedSize() {
    uint32_t result = 0;
    result += sizeof(uint8_t);//type
    result += sizeof(uint32_t);//sequenceNumber
    result += sizeof(uint32_t);//sender ip
    result += sizeof(uint16_t);//sender port
    switch (m_type) {
    case CONNECT:
    	break;
    case CONNECT_OK:
    	result += sizeof(uint32_t);
    	result += sizeof(uint16_t);
    	result += m_memberList.getSerializedSize();
    	result += sizeof(uint32_t);
    	result += m_publicKey.length();
    	break;
    case LOGIN:
    	result += sizeof(uint32_t);
    	result += m_name.length();
    	result += sizeof(uint32_t);
    	result += m_publicKey.length();
    	result += sizeof(uint32_t);
    	result += m_digest.length();
    	break;
    case LOGIN_OK:
    	result += m_memberList.getSerializedSize();
    	result += sizeof(uint32_t);
    	result += m_publicKey.length();
    	break;
    case HELLO:
    	result += sizeof(uint32_t);
    	result += m_publicKey.length();
    	result += sizeof(uint32_t);
    	result += m_digest.length();
    	break;
    case LEADER:
    	result += m_memberList.getSerializedSize();
    	result += sizeof(uint32_t);
    	result += m_publicKey.length();
    	break;
    case ELECTION:
    	break;
    case BULLY:
    	break;
    case CHAT:
    	result += sizeof(uint32_t);
    	result += m_digest.length();
    	result += sizeof(uint32_t);
    	result += m_contentDigest.length();
    	break;
    case MULTICAST:
    	result += sizeof(uint32_t);//sender ip
    	result += sizeof(uint16_t);//sender port
    	result += sizeof(uint32_t);
    	result += m_contentDigest.length();
    	break;
    case ACK:
    	break;
    }
    return result;
}

void Message::serialize(OutputBuffer *buffer) {
    buffer->writeU8(m_type);
    buffer->writeU32(m_sequenceNumber);
    buffer->writeU32(m_originator.m_ipAddress);
    buffer->writeU16(m_originator.m_port);

	switch (m_type) {
	case CONNECT:
		break;
	case CONNECT_OK:
		buffer->writeU32(m_leader.m_ipAddress);
		buffer->writeU16(m_leader.m_port);
		m_memberList.serialize(buffer);
		buffer->writeU32(m_publicKey.length());
		buffer->write(m_publicKey.c_str(), m_publicKey.length());
		break;
	case LOGIN:
		buffer->writeU32(m_name.length());
		buffer->write(m_name.c_str(), m_name.length());
		buffer->writeU32(m_publicKey.length());
		buffer->write(m_publicKey.c_str(), m_publicKey.length());
		buffer->writeU32(m_digest.length());
		buffer->write(m_digest.c_str(), m_digest.length());
		break;
	case LOGIN_OK:
		m_memberList.serialize(buffer);
		buffer->writeU32(m_publicKey.length());
		buffer->write(m_publicKey.c_str(), m_publicKey.length());
		break;
	case HELLO:
		buffer->writeU32(m_publicKey.length());
		buffer->write(m_publicKey.c_str(), m_publicKey.length());
		buffer->writeU32(m_digest.length());
		buffer->write(m_digest.c_str(), m_digest.length());
		break;
	case LEADER:
		m_memberList.serialize(buffer);
		buffer->writeU32(m_publicKey.length());
		buffer->write(m_publicKey.c_str(), m_publicKey.length());
		break;
	case ELECTION:
		break;
	case BULLY:
		break;
	case CHAT:
		buffer->writeU32(m_digest.length());
		buffer->write(m_digest.c_str(), m_digest.length());
		buffer->writeU32(m_contentDigest.length());
		buffer->write(m_contentDigest.c_str(), m_contentDigest.length());
		break;
	case MULTICAST:
		buffer->writeU32(m_sender.m_ipAddress);
		buffer->writeU16(m_sender.m_port);
		buffer->writeU32(m_contentDigest.length());
		buffer->write(m_contentDigest.c_str(), m_contentDigest.length());
		break;
	case ACK:
		break;
	}
}

MemberID Message::getOriginator() {
    return m_originator;
}

uint32_t Message::MemberList::getSerializedSize() {
	uint32_t result;
	result = sizeof(uint32_t);//size of map
	map<MemberID, Member *>::iterator it;
	for (it = list->begin(); it != list->end(); it++) {
		result += sizeof(uint32_t);//ip
		result += sizeof(uint16_t);//port
		result += sizeof(uint32_t);//length of name length
		result += it->second->getName().length();//name
		result += sizeof(uint32_t);
		result += it->second->getPublicKey().length();
	}
	return result;
}

void Message::MemberList::serialize(OutputBuffer *buffer) {
	buffer->writeU32(list->size());
	map<MemberID, Member *>::iterator it;
	for (it = list->begin(); it != list->end(); it++) {
		buffer->writeU32(it->second->getId().m_ipAddress);
		buffer->writeU16(it->second->getId().m_port);
		buffer->writeU32(it->second->getName().length());
		buffer->write(it->second->getName().c_str(), it->second->getName().length());
		uint32_t length = it->second->getPublicKey().length();
		buffer->writeU32(length);
		buffer->write(it->second->getPublicKey().c_str(), length);
	}
}

void Message::MemberList::deserialize(InputBuffer *buffer) {
	list = new map<MemberID, Member *>();
	uint32_t size = buffer->readU32();
	for (uint32_t i = 0; i < size; i++) {
		MemberID id;
		id.m_ipAddress = buffer->readU32();
		id.m_port = buffer->readU16();
		uint32_t length = buffer->readU32();
		char *tmp = buffer->read(length);
		string name = string(tmp, length);
		delete tmp;
		Member *member = new Member(id.m_ipAddress, id.m_port, name);
		length = buffer->readU32();
		tmp = buffer->read(length);
		string publicKey = string(tmp, length);
		member->setPublicKey(publicKey);
		delete tmp;
		list->insert(make_pair(id, member));
	}
}

void Message::setName(string name) {
	m_name = name;
}

string Message::getName() {
	return m_name;
}

void Message::setLeader(MemberID id) {
	m_leader = id;
}

MemberID Message::getLeader() {
	return m_leader;
}

void Message::setMemberList(map<MemberID, Member *> *m) {
	m_memberList.list = m;
}

map<MemberID, Member *> * Message::getMemberList() {
	return m_memberList.list;
}

void Message::setChatContent(string content) {
	m_chatContent = content;
}

string Message::getChatContent() {
	return m_chatContent;
}

void Message::setSender(MemberID id) {
	m_sender = id;
}

MemberID Message::getSender() {
	return m_sender;
}

void Message::setDigest(string digest) {
	m_digest = digest;
}

string Message::getDigest() {
	return m_digest;
}

void Message::setContentDigest(string digest) {
	m_contentDigest = digest;
}

string Message::getContentDigest() {
	return m_contentDigest;
}

void Message::setPublicKey(string key) {
	m_publicKey = key;
}

string Message::getPublicKey() {
	return m_publicKey;
}
