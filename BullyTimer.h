#pragma once
#include "Runnable.h"

class BullyTimer: public Runnable {
public:
	BullyTimer(Model *);
	void run();
};
