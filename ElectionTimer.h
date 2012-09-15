#pragma once
#include "Runnable.h"

class ElectionTimer: public Runnable {
public:
	ElectionTimer(Model *);
	void run();
};

