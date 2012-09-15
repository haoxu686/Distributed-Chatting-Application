#pragma once
#include "Runnable.h"

class HelloTimer: public Runnable {
public:
	HelloTimer(Model *);
	void run();
};

