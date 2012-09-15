#include "HelloTimer.h"
#include "Model.h"

HelloTimer::HelloTimer(Model *model) : Runnable(model) {

}

void HelloTimer::run() {
	model->handleHelloTimeout();
}
