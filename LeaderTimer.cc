#include "LeaderTimer.h"
#include "Model.h"

LeaderTimer::LeaderTimer(Model *model) : Runnable(model) {

}

void LeaderTimer::run() {
	model->handleLeaderTimeout();
}
