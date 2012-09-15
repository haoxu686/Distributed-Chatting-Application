#pragma once

class Model;
class Runnable {
protected:
	Model *model;
public:
	Runnable(Model *);
	virtual ~Runnable();
	virtual void run() = 0;
};

