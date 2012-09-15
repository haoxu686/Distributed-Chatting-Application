#pragma once

#include "Runnable.h"

class ConnectTimer: public Runnable {
public:
	ConnectTimer(Model *);
	void run();
};

