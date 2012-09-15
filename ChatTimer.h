#pragma once
#include "Runnable.h"

class ChatTimer: public Runnable {
public:
	ChatTimer(Model *);
	void run();
};

