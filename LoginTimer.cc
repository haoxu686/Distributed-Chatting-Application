#include "LoginTimer.h"
#include "Model.h"

LoginTimer::LoginTimer(Model *model) : Runnable(model) {

}

void LoginTimer::run() {
	model->handleLoginTimeout();
}
