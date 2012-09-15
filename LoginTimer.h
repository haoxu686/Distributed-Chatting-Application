#pragma once
#include "Runnable.h"

class LoginTimer: public Runnable {
public:
	LoginTimer(Model *);
	void run();
};

