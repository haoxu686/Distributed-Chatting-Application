#pragma once
#include "Runnable.h"

class LeaderTimer: public Runnable {
public:
	LeaderTimer(Model *);
	void run();
};

