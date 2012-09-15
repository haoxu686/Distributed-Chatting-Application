#include "ElectionTimer.h"
#include "Model.h"

ElectionTimer::ElectionTimer(Model *model) : Runnable(model) {

}

void ElectionTimer::run() {
	model->handleElectionTimeout();
}
