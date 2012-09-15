#include "ConnectTimer.h"
#include "Model.h"

ConnectTimer::ConnectTimer(Model *model) : Runnable(model) {

}

void ConnectTimer::run() {
	model->handleConnectTimeout();
}
