#include "BullyTimer.h"
#include "Model.h"

BullyTimer::BullyTimer(Model *model) : Runnable(model) {

}

void BullyTimer::run() {
	model->handleBullyTimeout();
}
