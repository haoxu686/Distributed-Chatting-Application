#pragma once
#include "Runnable.h"

class MulticastTimer: public Runnable {
public:
	MulticastTimer(Model *);
	void run();
};

