#include "Timer.h"

using namespace std;

Timer::Timer() {

}

Timer::~Timer() {
	for (uint32_t i = 0; i < m_taskPool.size(); i++) {
		delete m_taskPool.at(i);
	}
}

uint32_t Timer::add(Runnable *r) {
	Task *t = new Task();
	t->r = r;
	t->active = false;
	m_taskPool.push_back(t);
	return m_taskPool.size()-1;
}

void Timer::schedule(uint32_t id, uint32_t time) {
	if (id >= m_taskPool.size()) {
		return;
	}
	Task *t = m_taskPool.at(id);
	t->interval = time;
	t->time = time;
	t->active = true;
}

void Timer::suspend(uint32_t id) {
	if (id > m_taskPool.size()) {
		return;
	}
	m_taskPool.at(id)->active = false;
}

void Timer::doTimer() {
	for (uint32_t i = 0; i < m_taskPool.size(); i++) {
		Task *t = m_taskPool.at(i);
		if (!t->active) {
			continue;
		}
		if (t->time != 0) {
			t->time--;
			continue;
		}
		t->active = false;
		t->r->run();
	}
}
