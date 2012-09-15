#include "MulticastTimer.h"
#include "Model.h"

MulticastTimer::MulticastTimer(Model *model) : Runnable(model) {

}

void MulticastTimer::run() {
	model->handleMulticastTimeout();
}
