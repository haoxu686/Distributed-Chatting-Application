#pragma once
#include <stdint.h>
#include <vector>
#include "Runnable.h"

using namespace std;

class Runnable;
struct Task {
	Runnable *r;
	uint32_t time;
	uint32_t interval;
	bool active;
};

class Timer {
private:
	vector<Task *> m_taskPool;
public:
	Timer();
	~Timer();
	uint32_t add(Runnable *);
	void schedule(uint32_t, uint32_t);
	void suspend(uint32_t);
	void doTimer();
};

